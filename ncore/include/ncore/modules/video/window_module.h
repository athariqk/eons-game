#pragma once

#include <memory>
#include <span>

#include <ncore/core/collection.h>
#include <ncore/core/reference.h>
#include <ncore/core/structures.h>
#include <ncore/modules/module.h>

#include "window/window_event.h"
#include "window/window_types.h"

namespace nc {

class IWindow;
class Image;

/**
 * @brief WindowModule defines an interface for OS window/display management.
 */
class NCAPI WindowModule : public IModule {
    NCLASS( WindowModule, IModule )

public:
    static const uint8_t DEFAULT_WINDOW_FLAGS = 0x01;

    struct NCAPI VideoSettings {
        int SizeWidth        = 800;
        int SizeHeight       = 800;
        bool Fullscreen      = false;
        float PixelsPerMeter = 32.0f;
        NSTRUCT(
            VideoSettings, NC_F( VideoSettings, SizeWidth ) NC_F( VideoSettings, SizeHeight )
                               NC_F( VideoSettings, Fullscreen ) NC_F( VideoSettings, PixelsPerMeter )
        )
    };

public:
    WindowModule();
    ~WindowModule() override;

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    const VideoSettings& get_settings() const
    {
        return settings;
    }

    void set_default_icon( const Ref<Image>& image );

    uint32_t window_create( uint8_t flags = DEFAULT_WINDOW_FLAGS );
    void window_set_parent( uint32_t window_id, uint32_t parent ) const;
    void window_set_position( uint32_t window_id, Vec2 position ) const;
    void window_set_centered( uint32_t window_id ) const;
    void window_set_visible( uint32_t window_id, bool visible ) const;
    Vec2 window_get_resolution( uint32_t window_id ) const;
    void window_set_resolution( uint32_t window_id, Vec2 resolution );
    void window_set_icon( uint32_t window_id, const Image& image ) const;
    void window_set_title( uint32_t window_id, std::string_view title ) const;
    void window_set_fullscreen( uint32_t window_id, bool fullscreen );

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
     * @brief Shows a message box on the main window.
     */
    virtual bool show_message_box( MessageBoxType type, const std::string& title, const std::string& message ) const;

    /**
     * @return The window's native OS handle.
     */
    void* get_native_whnd( uint32_t window_id ) const;

    /**
     * @brief Updates the internal event queue.
     */
    void pump_events();

    /**
     * @brief Returns window events collected by the last pump_events().
     * Valid until the next pump_events() call.
     */
    std::span<const WindowEvent> window_events() const
    {
        return std::span<const WindowEvent>( event_queue.data(), event_queue.size() );
    }

private:
    VideoSettings settings;
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    Vector<WindowEvent> event_queue;
    Ref<Image> default_app_icon;
};

} // namespace nc
