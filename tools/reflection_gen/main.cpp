#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ast_visitor.h"
#include "code_generator.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

namespace fs = std::filesystem;

static llvm::cl::OptionCategory ReflectionGenCategory( "reflection-gen options" );

static llvm::cl::opt<std::string> OutputDir(
    "output-dir", llvm::cl::desc( "Directory where generated reflection files are written" ),
    llvm::cl::init( "generated" ), llvm::cl::cat( ReflectionGenCategory )
);

static llvm::cl::opt<std::string> ImplFileName(
    "impl-file",
    llvm::cl::desc(
        "Name of the single implementation .cpp that includes all "
        "generated headers (written into --output-dir)"
    ),
    llvm::cl::init( "reflection_impl.cpp" ), llvm::cl::cat( ReflectionGenCategory )
);

static llvm::cl::opt<std::string> SingleHeader(
    "single-header", llvm::cl::desc( "Process only this one header; skip reflection_impl.cpp generation" ),
    llvm::cl::init( "" ), llvm::cl::cat( ReflectionGenCategory )
);

static llvm::cl::opt<std::string> SingleOutput(
    "output", llvm::cl::desc( "Exact output file path (used with --single-header)" ), llvm::cl::init( "" ),
    llvm::cl::cat( ReflectionGenCategory )
);

// NOTE: Do NOT register a cl::opt named "p" — CommonOptionsParser already owns it.
// In single-header mode we extract -p ourselves from argv.

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string compute_output_path( const std::string& input, const std::string& output_dir )
{
    fs::path input_path = fs::absolute( input );
    fs::path base       = fs::current_path();

    std::error_code ec;
    fs::path rel = fs::relative( input_path, base, ec );

    fs::path sub_dir;
    if (!ec && !rel.empty() && rel.begin()->string() != "..")
        sub_dir = rel.parent_path();

    std::string filename = input_path.stem().string() + ".gen.h";
    return ( fs::path( output_dir ) / sub_dir / filename ).lexically_normal().string();
}

std::string make_guard_macro( const std::string& out_path )
{
    std::string guard;
    guard.reserve( out_path.size() + 20 );
    guard = "NC_REFLECTION_GEN_";

    bool last_was_underscore = false;
    for (unsigned char c : out_path) {
        if (std::isalnum( c )) {
            guard += static_cast<char>( std::toupper( c ) );
            last_was_underscore = false;
        } else if (!last_was_underscore) {
            guard += '_';
            last_was_underscore = true;
        }
    }
    return guard;
}

std::string relative_to_output_dir( const std::string& out_path, const std::string& output_dir )
{
    std::error_code ec;
    fs::path rel = fs::relative( fs::path( out_path ), fs::path( output_dir ), ec );
    if (ec || rel.empty())
        return fs::path( out_path ).filename().string();
    return rel.generic_string(); // forward slashes for #include
}

bool has_arg_prefix( int argc, const char** argv, llvm::StringRef prefix )
{
    for (int i = 1; i < argc; ++i) {
        llvm::StringRef arg( argv[i] );
        if (arg.starts_with( prefix ) || arg == prefix.drop_back()) // e.g. "--single-header=" or "--single-header"
            return true;
        // exact match for forms without '='
        if (arg == prefix.rtrim( '=' ))
            return true;
    }
    return false;
}

// Extract value of -p / -p=... without registering a second cl::opt named "p".
std::string extract_compile_db_path( int argc, const char** argv )
{
    for (int i = 1; i < argc; ++i) {
        llvm::StringRef arg( argv[i] );
        if (arg.starts_with( "-p=" ))
            return arg.drop_front( 3 ).str();
        if (arg == "-p" && i + 1 < argc)
            return argv[++i];
    }
    return {};
}

std::unique_ptr<clang::tooling::CompilationDatabase>
load_compilation_database( const std::string& path, std::string& err_msg )
{
    if (path.empty())
        return nullptr;

    // Accept either a directory containing compile_commands.json or the file itself.
    fs::path p( path );
    if (fs::is_regular_file( p ) && p.filename() == "compile_commands.json")
        p = p.parent_path();

    return clang::tooling::CompilationDatabase::autoDetectFromDirectory( p.string(), err_msg );
}

bool write_file( const std::string& path, const std::string& content )
{
    std::error_code ec;
    fs::create_directories( fs::path( path ).parent_path(), ec );
    if (ec) {
        llvm::errs() << "Error: cannot create directory for " << path << ": " << ec.message() << "\n";
        return false;
    }

    std::ofstream file( path, std::ios::binary );
    if (!file) {
        llvm::errs() << "Error: cannot open " << path << "\n";
        return false;
    }
    file << content;
    if (!file) {
        llvm::errs() << "Error: failed writing " << path << "\n";
        return false;
    }
    return true;
}

// Always produce a .gen.h so CMake's OUTPUT is satisfied, even with zero records.
std::string generate_empty_reflection_header( const std::string& source, const std::string& guard )
{
    return "// Auto-generated — no reflectable records in " + source +
           "\n"
           "#ifndef " +
           guard +
           "\n"
           "#define " +
           guard +
           "\n"
           "#endif // " +
           guard + "\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main( int argc, const char** argv )
{
    const bool single_header_mode = has_arg_prefix( argc, argv, "--single-header=" );

    std::unique_ptr<clang::tooling::CompilationDatabase> compilations;
    std::unique_ptr<clang::tooling::CommonOptionsParser> options_parser;
    std::vector<std::string> sources;

    if (single_header_mode) {
        // Extract -p BEFORE filtering/parsing, from the untouched argv.
        std::string db_path = extract_compile_db_path( argc, argv );

        // Build a filtered argv with -p / -p=... removed, since no cl::opt
        // named "p" exists in this branch (CommonOptionsParser isn't built).
        std::vector<const char*> filtered_argv;
        filtered_argv.reserve( static_cast<size_t>( argc ) );
        for (int i = 0; i < argc; ++i) {
            llvm::StringRef arg( argv[i] );
            if (arg == "-p") {
                ++i; // also skip the separate value token, e.g. `-p "path"`
                continue;
            }
            if (arg.starts_with( "-p=" ))
                continue;
            filtered_argv.push_back( argv[i] );
        }
        int filtered_argc = static_cast<int>( filtered_argv.size() );

        llvm::cl::ParseCommandLineOptions(
            filtered_argc, filtered_argv.data(), "reflection-gen: per-header reflection code generator\n"
        );

        if (SingleHeader.empty()) {
            llvm::errs() << "Error: --single-header requires a header path\n";
            return 1;
        }
        sources.push_back( SingleHeader.getValue() );

        std::string err_msg;
        compilations = load_compilation_database( db_path, err_msg );
        if (!compilations) {
            if (!db_path.empty()) {
                llvm::errs() << "Warning: could not load compilation database from '" << db_path << "': " << err_msg
                             << "\nProceeding with empty compilation database.\n";
            }
            compilations =
                std::make_unique<clang::tooling::FixedCompilationDatabase>( ".", std::vector<std::string>{} );
        }
    } else {
        auto expected = clang::tooling::CommonOptionsParser::create( argc, argv, ReflectionGenCategory );
        if (!expected) {
            llvm::errs() << expected.takeError();
            return 1;
        }
        options_parser = std::make_unique<clang::tooling::CommonOptionsParser>( std::move( *expected ) );
        sources        = options_parser->getSourcePathList();
        // compilations live inside options_parser
    }

    const clang::tooling::CompilationDatabase& db = options_parser ? options_parser->getCompilations() : *compilations;

    std::vector<std::string> relative_includes;
    int exit_code = 0;

    for (const auto& source : sources) {
        const std::string out_path =
            !SingleOutput.empty() ? SingleOutput.getValue() : compute_output_path( source, OutputDir );

        ReflectionResult result;
        ReflectionStats stats;

        clang::tooling::ClangTool tool( db, { source } );
        auto factory = create_reflection_action_factory( result, stats );
        if (tool.run( factory.get() ) != 0) {
            llvm::errs() << "Warning: parse errors in " << source << "\n";
            // Still emit a stub so the OUTPUT file exists for CMake.
        }

        std::string content;
        if (result.records.empty()) {
            llvm::outs() << "No reflectable records: " << source << "\n";
            content = generate_empty_reflection_header( source, make_guard_macro( out_path ) );
        } else {
            content = generate_reflection_header( result, source, make_guard_macro( out_path ) );
        }

        if (!write_file( out_path, content )) {
            exit_code = 1;
            continue;
        }

        llvm::outs() << "Generated: " << out_path << "\n";
        relative_includes.push_back( relative_to_output_dir( out_path, OutputDir ) );
    }

    // Batch mode only: rewrite the aggregate implementation unit.
    if (!single_header_mode) {
        const fs::path impl_path = ( fs::path( OutputDir.getValue() ) / ImplFileName.getValue() ).lexically_normal();

        const std::string impl_content = generate_reflection_impl( relative_includes );
        if (!write_file( impl_path.string(), impl_content ))
            return 1;

        llvm::outs() << "Generated implementation unit: " << impl_path.string() << "\n";
    }

    return exit_code;
}
