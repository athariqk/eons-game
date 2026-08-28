#pragma once

#include <memory>
#include <span>

#include <ncore/core/collection.h>
#include <ncore/core/rect.h>
#include <ncore/core/reference.h>
#include <ncore/core/vector.h>
#include <ncore/services/service.h>

#include "window/window_event.h"
#include "window/window_types.h"

namespace nc {

class IWindow;
class Image;

/**
 * @brief WindowService defines an interface for OS window/display management.
 */
class NCAPI WindowService : public IService {
    NCLASS( WindowService, IService )

public:
    enum WindowFlag : uint8_t {
        RESIZABLE  = 1 << 0,
        FULLSCREEN = 1 << 1,
        MAXIMIZED  = 1 << 2,
    };

    static const WindowFlag DEFAULT_WINDOW_FLAGS = WindowFlag::RESIZABLE;

    struct NCAPI WindowSettings {
        int SizeWidth        = 800;
        int SizeHeight       = 800;
        WindowMode Mode      = WindowMode::WINDOWED;
        float PixelsPerMeter = 32.0f;
        NSTRUCTV(
            WindowSettings, NC_F( WindowSettings, SizeWidth ), NC_F( WindowSettings, SizeHeight ),
            NC_F( WindowSettings, Mode ), NC_F( WindowSettings, PixelsPerMeter )
        )
    };

public:
    WindowService();
    ~WindowService() override;

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    const WindowSettings& get_settings() const
    {
        return settings;
    }

    void set_default_icon( const Ref<Image>& image );

    uint32_t window_create( WindowFlag flags = DEFAULT_WINDOW_FLAGS );
    void window_set_parent( uint32_t window_id, uint32_t parent ) const;
    /**
     * @brief Request that the window's position be set.
     *
     * If the window is in an exclusive fullscreen or maximized state, this
     * request has no effect.
     */
    void window_set_position( uint32_t window_id, Vec2i position ) const;
    /**
     * @brief Center the window's position.
     *
     * If the window is in an exclusive fullscreen or maximized state, this
     * request has no effect.
     */
    void window_set_centered( uint32_t window_id ) const;
    void window_set_visible( uint32_t window_id, bool visible ) const;
    Vec2i window_get_resolution( uint32_t window_id ) const;
    /**
     * @brief Request that the size of a window's client area be set.
     *
     * If the window is in a fullscreen or maximized state, this request has no
     * effect.
     */
    void window_set_resolution( uint32_t window_id, Vec2i resolution );
    void window_set_icon( uint32_t window_id, const Image& image ) const;
    void window_set_title( uint32_t window_id, StringView title ) const;
    void window_set_fullscreen( uint32_t window_id, bool fullscreen );

    /**
     * @brief Confines the mouse cursor to a rectangle within the window.
     * The cursor cannot leave the rect while it is set, but continues to
     * behave like a regular (non-grabbed) cursor inside it.
     * @param rect Bounds in window coordinates; the rect does not scale with
     * the window, so it must be updated after window resizes.
     */
    void window_set_mouse_confinement( uint32_t window_id, const Rect2f& rect ) const;

    /**
     * @brief Removes the mouse confinement rect set for the window.
     */
    void window_clear_mouse_confinement( uint32_t window_id ) const;

    /**
     * @brief Grabs the mouse cursor to the window, unlike confinement which
     * restricts it to a rect: the cursor is locked inside the whole window,
     * keeps receiving input even when the pointer leaves the window, and
     * becomes visible again only once ungrab (and focus) returns.
     * @see window_set_mouse_confinement
     */
    void window_set_mouse_grab( uint32_t window_id, bool grabbed ) const;

    /**
     * @brief Warps the mouse cursor to a position inside the window.
     * Generates a mouse motion event. No-op on some platforms (e.g. Remote
     * Desktop).
     */
    void window_set_mouse_position( uint32_t window_id, Vec2f position ) const;

    bool window_get_mouse_locked( uint32_t window_id ) const;
    void window_set_mouse_locked( uint32_t window_id, bool enabled ) const;

    /**
     * @brief Destroy the window with given ID.
     * @return True if success, false if an error occured.
     */
    bool window_pop( uint32_t window_id );

    /**
     * @return The first window ID, or MAX_UINT32 if none exist.
     */
    uint32_t get_main_window_id() const;

    /**
     * @brief Sets the mouse cursor visual type globally.
     */
    void set_cursor_type( CursorType cursor_type );

    /**
     * @brief Sets the mouse cursor visibility globally.
     */
    void set_cursor_visible( bool visible );

    /**
     * @return True if the mouse cursor is currently visible.
     */
    bool get_cursor_visible() const;

    /**
     * @brief Shows a message box on the main window.
     */
    virtual bool show_message_box( MessageBoxType type, const String& title, const String& message ) const;

    /**
     * @return The window's native OS handle.
     */
    void* get_native_handle( uint32_t window_id ) const;

    /**
     * @brief Updates the internal event queue.
     */
    void pump_events();

    /**
     * @brief Returns window events collected by the last pump_events().
     * Valid until the next pump_events() call.
     */
    Span<const WindowEvent> window_events() const
    {
        return Span<const WindowEvent>( event_queue.data(), event_queue.size() );
    }

private:
    WindowSettings settings;
    struct Impl;
    Ptr<Impl> pImpl;
    DynamicArray<WindowEvent> event_queue;
    Ref<Image> default_app_icon;
};

} // namespace nc
