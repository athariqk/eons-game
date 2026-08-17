// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
//
// Leak reporter for AddressSanitizer builds (ASAN).
//
// The ASAN runtime on Windows ships without LeakSanitizer, so this module
// installs the sanitizer's malloc/free hooks to track live allocations and
// prints a symbolized leak report at process exit (ncore.dll unload — the
// engine is the first DLL loaded and the last unloaded, so the report runs
// after every other module has torn down).
//
// The report groups leaked blocks by their allocation stack, each frame
// symbolized to function/file:line by the sanitizer runtime.
//
// This translation unit is a no-op outside ASAN builds (NC_ASAN_ENABLED).

#include <ncore/core/memory.h>

#if NC_ASAN_ENABLED

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

#include <sanitizer/allocator_interface.h>
#include <sanitizer/asan_interface.h>
#include <sanitizer/common_interface_defs.h>

#ifdef _WIN32
// clang-format off
#include <Windows.h>
#include <DbgHelp.h> // must be included after Windows.h
#include <Psapi.h>
// clang-format on
#endif

namespace nc::asan {

namespace {

// ---------------------------------------------------------------------------
// Static state. Nothing in this module allocates dynamically — the hooks
// must never call malloc.
// ---------------------------------------------------------------------------

constexpr size_t kSetCapacity        = static_cast<size_t>( 1 ) << 21; // 2M live pointers (16 MB)
constexpr size_t kAggCapacity        = 1 << 14;                        // 16K distinct allocation stacks
constexpr uint32_t kMaxTraceDepth    = 16;
constexpr uint32_t kMaxPrintedStacks = 20;

constexpr uintptr_t kSlotEmpty     = 0;
constexpr uintptr_t kSlotTombstone = 1;

uintptr_t g_live[kSetCapacity] = {};
size_t g_live_count            = 0;
size_t g_dropped               = 0;

struct Aggregate {
    uint64_t stack_hash         = 0;
    uint32_t depth              = 0;
    uint32_t count              = 0;
    size_t bytes                = 0;
    void* trace[kMaxTraceDepth] = {};
};

Aggregate g_aggs[kAggCapacity] = {};
size_t g_agg_count             = 0;
size_t g_agg_overflow          = 0;
size_t g_stackless_blocks      = 0;
size_t g_stackless_bytes       = 0;

std::mutex g_mutex;

void print_line( const char* text )
{
    std::fputs( text, stderr );
    std::fputc( '\n', stderr );
#ifdef _WIN32
    OutputDebugStringA( text );
    OutputDebugStringA( "\n" );
#endif
}

#ifdef _WIN32

bool g_dbghelp_ok = false;

bool init_dbghelp()
{
    SymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES );
    g_dbghelp_ok = SymInitialize( GetCurrentProcess(), nullptr, TRUE ) == TRUE;
    return g_dbghelp_ok;
}

void shutdown_dbghelp()
{
    if (g_dbghelp_ok)
        SymCleanup( GetCurrentProcess() );
}

#endif // _WIN32

// Returns a static buffer with "function file:line" for the frame, or the
// sanitizer symbolizer's best effort if dbghelp is unavailable.
const char* symbolize_frame( void* pc )
{
    static char s_frame[1024];
    s_frame[0] = 0;

#ifdef _WIN32
    if (g_dbghelp_ok && pc != nullptr) {
        // MaxNameLen wide characters + the struct itself.
        char sym_buf[sizeof( SYMBOL_INFOW ) + 1024];
        SYMBOL_INFOW* sym = reinterpret_cast<SYMBOL_INFOW*>( sym_buf );
        sym->SizeOfStruct = sizeof( SYMBOL_INFOW );
        sym->MaxNameLen   = 511;

        char name[512]    = {};
        DWORD64 disp      = 0;
        const BOOL sym_ok = SymFromAddrW( GetCurrentProcess(), reinterpret_cast<DWORD64>( pc ), &disp, sym );
        if (sym_ok) {
            WideCharToMultiByte( CP_UTF8, 0, sym->Name, -1, name, sizeof( name ), nullptr, nullptr );
        }

        IMAGEHLP_LINEW64 line_info = {};
        line_info.SizeOfStruct     = sizeof( line_info );
        DWORD line_disp            = 0;
        const BOOL line_ok =
            SymGetLineFromAddrW64( GetCurrentProcess(), reinterpret_cast<DWORD64>( pc ), &line_disp, &line_info );

        if (line_ok) {
            char file[512] = {};
            WideCharToMultiByte( CP_UTF8, 0, line_info.FileName, -1, file, sizeof( file ), nullptr, nullptr );
            if (name[0]) {
                std::snprintf(
                    s_frame, sizeof( s_frame ), "%s %s:%lu", name, file,
                    static_cast<unsigned long>( line_info.LineNumber )
                );
            } else {
                std::snprintf(
                    s_frame, sizeof( s_frame ), "%s:%lu", file, static_cast<unsigned long>( line_info.LineNumber )
                );
            }
        } else if (name[0]) {
            std::snprintf( s_frame, sizeof( s_frame ), "%s", name );
        }
    }
#endif // _WIN32

    if (s_frame[0] == 0)
        __sanitizer_symbolize_pc( pc, "%f:%l", s_frame, sizeof( s_frame ) );

    return s_frame;
}

#ifdef _WIN32

// Loaded-module address ranges (symbol-independent — works for modules
// without local PDBs).
struct ModuleRange {
    uintptr_t begin        = 0;
    uintptr_t end          = 0;
    wchar_t path[MAX_PATH] = {};
};

ModuleRange g_modules[256]    = {};
int g_module_count            = 0;
uintptr_t g_asan_runtime_base = 0;

void enumerate_modules()
{
    HMODULE mods[256];
    DWORD needed = 0;
    if (!EnumProcessModules( GetCurrentProcess(), mods, sizeof( mods ), &needed ))
        return;

    const int n    = static_cast<int>( needed / sizeof( HMODULE ) );
    g_module_count = n < 256 ? n : 256;
    for (int i = 0; i < g_module_count; ++i) {
        MODULEINFO info = {};
        if (GetModuleInformation( GetCurrentProcess(), mods[i], &info, sizeof( info ) )) {
            g_modules[i].begin = reinterpret_cast<uintptr_t>( info.lpBaseOfDll );
            g_modules[i].end   = g_modules[i].begin + info.SizeOfImage;
            GetModuleFileNameW( mods[i], g_modules[i].path, MAX_PATH );

            const wchar_t* name = g_modules[i].path;
            for (const wchar_t* p = g_modules[i].path; *p; ++p) {
                if (*p == L'\\')
                    name = p + 1;
            }
            // Skip the sanitizer runtime's malloc-wrapping frames when
            // attributing allocations to modules.
            if (name[0] == L'c' && name[1] == L'l' && name[2] == L'a' && name[3] == L'n' && name[4] == L'g' &&
                name[5] == L'_' && name[6] == L'r' && name[7] == L't') {
                g_asan_runtime_base = g_modules[i].begin;
            }
        }
    }
}

uintptr_t find_module_base( uintptr_t pc )
{
    for (int i = 0; i < g_module_count; ++i) {
        if (pc >= g_modules[i].begin && pc < g_modules[i].end)
            return g_modules[i].begin;
    }
    return 0;
}

const wchar_t* module_path( uintptr_t base )
{
    for (int i = 0; i < g_module_count; ++i) {
        if (g_modules[i].begin == base)
            return g_modules[i].path;
    }
    return L"";
}

// Modules whose allocations belong to the project itself (exe + engine DLLs).
// Frames in any other module (CRT, OS, drivers, third-party DLLs) are treated
// as noise — usually process-lifetime caches or modules that unload after
// ncore, and not fixable from project code.
uintptr_t g_project_bases[4] = {};
int g_project_base_count     = 0;

void cache_project_bases()
{
    g_project_base_count = 0;

    HMODULE exe = GetModuleHandleW( nullptr );
    if (exe)
        g_project_bases[g_project_base_count++] = reinterpret_cast<uintptr_t>( exe );

    const wchar_t* names[] = { L"ncore_d.dll", L"ncore.dll", L"ncore_editor_d.dll", L"ncore_editor.dll" };
    for (const wchar_t* name : names) {
        HMODULE mod = GetModuleHandleW( name );
        if (mod)
            g_project_bases[g_project_base_count++] = reinterpret_cast<uintptr_t>( mod );
    }
}

bool is_project_base( uintptr_t base )
{
    for (int i = 0; i < g_project_base_count; ++i) {
        if (g_project_bases[i] == base)
            return true;
    }
    return false;
}

// True if any frame of the stack belongs to a project module.
bool stack_is_project( const Aggregate& a )
{
    for (uint32_t f = 0; f < a.depth; ++f) {
        const uintptr_t pc = reinterpret_cast<uintptr_t>( a.trace[f] );
        if (is_project_base( find_module_base( pc ) ))
            return true;
    }
    return false;
}

// Attribute folded noise to the module of the first resolvable frame.
struct NoiseModule {
    uintptr_t base = 0;
    size_t blocks  = 0;
    size_t bytes   = 0;
    char name[64]  = {};
};

int g_noise_module_count        = 0;
NoiseModule g_noise_modules[64] = {};

void noise_tally( const Aggregate& a )
{
    // Attribute to the deepest resolvable frame (the allocation origin),
    // skipping the sanitizer runtime's malloc-wrapping frames.
    uintptr_t base = 0;
    for (uint32_t f = a.depth; f > 0; --f) {
        const uintptr_t pc             = reinterpret_cast<uintptr_t>( a.trace[f - 1] );
        const uintptr_t base_candidate = find_module_base( pc );
        if (base_candidate == 0 || base_candidate == g_asan_runtime_base)
            continue;
        base = base_candidate;
        break;
    }

    NoiseModule* slot = nullptr;
    for (int i = 0; i < g_noise_module_count; ++i) {
        if (g_noise_modules[i].base == base) {
            slot = &g_noise_modules[i];
            break;
        }
    }
    if (!slot && g_noise_module_count < 64) {
        slot       = &g_noise_modules[g_noise_module_count++];
        slot->base = base;
        if (base != 0) {
            const wchar_t* path = module_path( base );
            const wchar_t* name = path;
            for (const wchar_t* p = path; *p; ++p) {
                if (*p == L'\\')
                    name = p + 1;
            }
            WideCharToMultiByte( CP_UTF8, 0, name, -1, slot->name, sizeof( slot->name ), nullptr, nullptr );
        }
    }
    if (slot) {
        slot->blocks += a.count;
        slot->bytes += a.bytes;
    }
}

#endif // _WIN32

inline size_t mix_ptr( uintptr_t x )
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return static_cast<size_t>( x );
}

inline uint64_t hash_trace( void* const* trace, uint32_t depth )
{
    uint64_t h = 14695981039346656037ULL;
    for (uint32_t i = 0; i < depth; ++i) {
        h ^= static_cast<uint64_t>( reinterpret_cast<uintptr_t>( trace[i] ) );
        h *= 1099511628211ULL;
    }
    return h;
}

void on_alloc( const volatile void* ptr, size_t /*size*/ )
{
    const uintptr_t key = reinterpret_cast<uintptr_t>( ptr );
    if (key == 0)
        return;

    std::lock_guard<std::mutex> lock( g_mutex );
    for (size_t i = 0; i < kSetCapacity; ++i) {
        const size_t slot   = ( mix_ptr( key ) + i ) & ( kSetCapacity - 1 );
        const uintptr_t cur = g_live[slot];
        if (cur == key)
            return;
        if (cur == kSlotEmpty || cur == kSlotTombstone) {
            g_live[slot] = key;
            ++g_live_count;
            return;
        }
    }
    ++g_dropped; // table full
}

void on_free( const volatile void* ptr )
{
    const uintptr_t key = reinterpret_cast<uintptr_t>( ptr );
    if (key == 0)
        return;

    std::lock_guard<std::mutex> lock( g_mutex );
    for (size_t i = 0; i < kSetCapacity; ++i) {
        const size_t slot   = ( mix_ptr( key ) + i ) & ( kSetCapacity - 1 );
        const uintptr_t cur = g_live[slot];
        if (cur == key) {
            g_live[slot] = kSlotTombstone;
            --g_live_count;
            return;
        }
        if (cur == kSlotEmpty)
            return; // allocated before the hooks were installed
    }
}

void write_report()
{
    // Stop tracking so allocations made while printing are not recorded.
    __sanitizer_install_malloc_and_free_hooks( nullptr, nullptr );

    {
        std::lock_guard<std::mutex> lock( g_mutex );

        for (size_t slot = 0; slot < kSetCapacity; ++slot) {
            const uintptr_t key = g_live[slot];
            if (key == kSlotEmpty || key == kSlotTombstone)
                continue;

            void* const ptr = reinterpret_cast<void*>( key );
            void* trace[kMaxTraceDepth];
            int thread_id          = 0;
            const size_t depth_raw = __asan_get_alloc_stack( ptr, trace, kMaxTraceDepth, &thread_id );

            if (depth_raw == 0) {
                ++g_stackless_blocks;
                g_stackless_bytes += __sanitizer_get_allocated_size( reinterpret_cast<const volatile void*>( ptr ) );
                continue;
            }

            const uint32_t depth = static_cast<uint32_t>( depth_raw );
            const uint64_t hash  = hash_trace( trace, depth );

            size_t agg = kAggCapacity;
            for (size_t i = 0; i < kAggCapacity; ++i) {
                const size_t s = static_cast<size_t>( ( hash + i ) % kAggCapacity );
                Aggregate& a   = g_aggs[s];
                if (a.count == 0 || ( a.stack_hash == hash && a.depth == depth )) {
                    agg = s;
                    break;
                }
            }
            if (agg == kAggCapacity) {
                ++g_agg_overflow;
                continue;
            }

            Aggregate& a = g_aggs[agg];
            if (a.count == 0) {
                a.stack_hash = hash;
                a.depth      = depth;
                std::memcpy( a.trace, trace, static_cast<size_t>( depth ) * sizeof( void* ) );
                ++g_agg_count;
            }
            ++a.count;
            a.bytes += __sanitizer_get_allocated_size( reinterpret_cast<const volatile void*>( ptr ) );
        }

        std::memset( g_live, 0, sizeof( g_live ) );
        g_live_count = 0;
        g_dropped    = 0;
    }

    // Snapshot is taken — dbghelp (and its internal caches) may allocate now
    // without polluting the report.
#ifdef _WIN32
    enumerate_modules();
    init_dbghelp();
    cache_project_bases();
#endif

    static uint32_t order[kAggCapacity];
    size_t used = 0;
    for (size_t i = 0; i < kAggCapacity; ++i) {
        if (g_aggs[i].count > 0)
            order[used++] = static_cast<uint32_t>( i );
    }
    std::sort( order, order + used, []( uint32_t x, uint32_t y ) { return g_aggs[x].bytes > g_aggs[y].bytes; } );

    size_t total_blocks = g_stackless_blocks;
    size_t total_bytes  = g_stackless_bytes;
    for (size_t i = 0; i < used; ++i) {
        total_blocks += g_aggs[order[i]].count;
        total_bytes += g_aggs[order[i]].bytes;
    }

    char line[2048];
    std::snprintf(
        line, sizeof( line ), "== NCORE LEAK REPORT == %zu block(s), %zu byte(s) still allocated at process exit",
        total_blocks, total_bytes
    );
    print_line( line );

    const size_t print_n = used < kMaxPrintedStacks ? used : kMaxPrintedStacks;
    size_t printed       = 0;
    size_t rest_stacks   = 0;
    size_t rest_blocks   = 0;
    size_t rest_bytes    = 0;
    size_t noise_blocks  = g_stackless_blocks;
    size_t noise_bytes   = g_stackless_bytes;
#ifdef _WIN32
    g_noise_module_count = 0;
#endif

    for (size_t i = 0; i < print_n; ++i) {
        const Aggregate& a = g_aggs[order[i]];

#ifdef _WIN32
        if (g_dbghelp_ok && !stack_is_project( a )) {
            noise_blocks += a.count;
            noise_bytes += a.bytes;
            noise_tally( a );
            continue;
        }
#endif
        if (printed >= kMaxPrintedStacks) {
            ++rest_stacks;
            rest_blocks += a.count;
            rest_bytes += a.bytes;
            continue;
        }

        std::snprintf( line, sizeof( line ), "%zu byte(s) in %u allocation(s), allocated from:", a.bytes, a.count );
        print_line( line );
        for (uint32_t f = 0; f < a.depth; ++f) {
            std::snprintf( line, sizeof( line ), "    #%u %s", f, symbolize_frame( a.trace[f] ) );
            print_line( line );
        }
        ++printed;
    }

    for (size_t i = print_n; i < used; ++i) {
        const Aggregate& a = g_aggs[order[i]];
#ifdef _WIN32
        if (g_dbghelp_ok && !stack_is_project( a )) {
            noise_blocks += a.count;
            noise_bytes += a.bytes;
            noise_tally( a );
            continue;
        }
#endif
        ++rest_stacks;
        rest_blocks += a.count;
        rest_bytes += a.bytes;
    }

    if (rest_stacks > 0) {
        std::snprintf(
            line, sizeof( line ), "... %zu more project allocation stack(s): %zu block(s), %zu byte(s)", rest_stacks,
            rest_blocks, rest_bytes
        );
        print_line( line );
    }
    if (noise_blocks > 0) {
        std::snprintf(
            line, sizeof( line ),
            "%zu system/third-party block(s), %zu byte(s) (process-lifetime caches and still-loaded modules)",
            noise_blocks, noise_bytes
        );
        print_line( line );
#ifdef _WIN32
        for (int i = 0; i < g_noise_module_count; ++i) {
            const NoiseModule& nm = g_noise_modules[i];
            std::snprintf(
                line, sizeof( line ), "    [%s] %zu block(s), %zu byte(s)", nm.name[0] ? nm.name : "unknown module",
                nm.blocks, nm.bytes
            );
            print_line( line );
        }
#endif
    }
    if (g_agg_overflow > 0 || g_dropped > 0) {
        std::snprintf(
            line, sizeof( line ), "(%zu stack(s) dropped from the report, %zu allocation(s) untracked)", g_agg_overflow,
            g_dropped
        );
        print_line( line );
    }
    std::fflush( stderr );

#ifdef _WIN32
    shutdown_dbghelp();
#endif

    std::memset( g_aggs, 0, sizeof( g_aggs ) );
    g_agg_count        = 0;
    g_agg_overflow     = 0;
    g_stackless_blocks = 0;
    g_stackless_bytes  = 0;
}

// Installed at ncore.dll load (before main and before other modules' static
// initializers). The report is registered with atexit, so it runs at
// ncore.dll unload — after every other module has torn down.
struct LeakTracker {
    LeakTracker()
    {
        __sanitizer_install_malloc_and_free_hooks( &on_alloc, &on_free );
        std::atexit( &write_report );
    }
};

LeakTracker g_tracker;

} // namespace

} // namespace nc::asan

#endif // NC_ASAN_ENABLED
