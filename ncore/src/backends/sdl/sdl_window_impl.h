#pragma once

#include <ncore/modules/video/window_module.h>

namespace nc {

struct Vec2;

class SDLWindowImpl : public IWindowModule {
    NCLASS( SDLWindowImpl, IWindowModule )

public:
    Error init() override;
    void finalize() override;

    RID create_window( const std::string& title, Vec2 size, bool fullscreen ) override;
    void destroy_window( RID window_id ) override;
    RID get_main_window_id() const override;

    Vec2 get_resolution( RID window_id ) const override;
    void set_title( RID window_id, const std::string& title ) const override;

    void set_cursor_type( CursorType cursor_type ) override;

    bool show_message_box( MessageBoxType type, const std::string& title, const std::string& message ) const override;

    Viewport* get_viewport() const override;

    void* get_native_handle( RID window_id ) const override;

private:
    std::unordered_map<RID, std::unique_ptr<Viewport>> viewport; // TODO: properly implement later
    std::unordered_map<CursorType, SDL_Cursor*> mouse_cursors;
};

} // namespace nc
