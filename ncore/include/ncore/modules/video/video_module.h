#pragma once

#include <memory>
#include <span>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/module.h>

#include "window_event.h"
#include "window_types.h"

namespace nc {

class Viewport;
class IWindow;
class Image;

/**
 * @brief VideoModule defines an interface for OS window/display management.
 */
class NCORE_API VideoModule : public IModule {
    NCLASS( VideoModule, IModule )

public:
    static const uint8_t DEFAULT_WINDOW_FLAGS = 0x01;

    struct NCORE_API VideoSettings {
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
    VideoModule();
    ~VideoModule() override;

    Error init( ConfFile& cfg_file ) override;
    void finalize() override;

    const VideoSettings& get_settings() const
    {
        return settings;
    }

    uint32_t create_window( uint8_t flags = DEFAULT_WINDOW_FLAGS );
    void set_window_parent( uint32_t window_id, uint32_t parent ) const;

    void set_window_position( uint32_t window_id, Vec2 position ) const;
    void set_window_centered( uint32_t window_id ) const;

    void set_window_visible( uint32_t window_id, bool visible ) const;

    Vec2 get_window_resolution( uint32_t window_id ) const;
    void set_window_resolution( uint32_t window_id, Vec2 resolution );

    void set_window_icon( uint32_t window_id, const Image& image ) const;
    void set_window_title( uint32_t window_id, const std::string& title ) const;

    void set_window_fullscreen( uint32_t window_id, bool fullscreen );

    /**
     * @brief Destroy the window with given ID.
     * @return True if success, false if an error occured.
     */
    bool pop_window( uint32_t window_id );

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

    Viewport* get_viewport() const;
    Viewport* get_viewport( uint32_t window_id ) const;

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
};

} // namespace nc
