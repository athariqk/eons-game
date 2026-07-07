#pragma once

#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/module.h>

#include "window_types.h"

namespace nc {

class Viewport;

/**
 * @brief IWindowModule defines an interface for OS window/display management.
 */
class NCORE_API IWindowModule : public IModule {
    NCLASS( IWindowModule, IModule )

public:
    virtual RID create_window( const std::string& title, Vec2 size, bool fullscreen ) = 0;
    virtual void destroy_window( RID window_id )                                      = 0;
    virtual RID get_main_window_id() const                                            = 0;

    virtual Vec2 get_resolution( RID window_id ) const                      = 0;
    virtual void set_title( RID window_id, const std::string& title ) const = 0;

    /**
     * @brief Sets the mouse cursor visual type globally.
     */
    virtual void set_cursor_type( CursorType cursor_type ) = 0;

    /**
     * @brief Shows a message box on the main window.
     */
    virtual bool
    show_message_box( MessageBoxType type, const std::string& title, const std::string& message ) const = 0;

    // HACK: properly implement later
    virtual Viewport* get_viewport() const = 0;

    virtual void* get_native_handle( RID window_id ) const = 0;

protected:
    RID main_window_id;
};

} // namespace nc
