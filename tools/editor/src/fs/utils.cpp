#include "utils.h"

#if defined( _WIN32 )
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif

namespace nc::editor {

void reveal_in_explorer( StringView p_path )
{
    if (p_path.empty())
        return;

#if defined( _WIN32 )
    std::wstring wide( p_path.size(), L'\0' );
    MultiByteToWideChar(
        CP_UTF8, 0, p_path.data(), static_cast<int>( p_path.size() ), wide.data(), static_cast<int>( wide.size() )
    );

    const std::wstring arguments = L"/select,\"" + wide + L"\"";
    ShellExecuteW( nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL );
#else
    ( void ) p_path;
#endif
}

} // namespace nc::editor
