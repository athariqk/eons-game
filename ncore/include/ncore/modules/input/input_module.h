#pragma once

#include <span>

#include <ncore/core/collection.h>
#include <ncore/modules/module.h>

#include "input_event.h"

namespace nc {

/**
 * @brief InputModule consumes OS-level mouse, keyboard, and text input events
 * and mapping them to produce high-level input events called "Action" which is
 * inspired in terms by Godot.
 */
class NCAPI InputModule : public IModule {
    NCLASS( InputModule, IModule )

    static constexpr size_t KEY_COUNT          = static_cast<size_t>( Key::COUNT );
    static constexpr size_t MOUSE_BUTTON_COUNT = static_cast<size_t>( ButtonIndex::COUNT );
    inline static const int MAX_BINDS          = 8;

    struct ButtonState {
        bool held      = false;
        bool pressed   = false;
        bool released  = false;
        float strength = 0.0f;
    };

    struct ActionBinding {
        String name;
        InputEvent sources[MAX_BINDS];
        int source_count = 0;
        ButtonState state;
    };

public:
    inline static const char* FORWARD_ACTION_NAME  = "G_Forward";
    inline static const char* BACKWARD_ACTION_NAME = "G_Backward";
    inline static const char* LEFT_ACTION_NAME     = "G_Left";
    inline static const char* RIGHT_ACTION_NAME    = "G_Right";
    inline static const char* UP_ACTION_NAME       = "G_Up";
    inline static const char* DOWN_ACTION_NAME     = "G_Down";

    inline static Array<const char*, 6> DEFAULT_ACTION_NAMES = { FORWARD_ACTION_NAME, BACKWARD_ACTION_NAME,
                                                                 LEFT_ACTION_NAME,    RIGHT_ACTION_NAME,
                                                                 UP_ACTION_NAME,      DOWN_ACTION_NAME };

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    void action_register( const String& name );
    /**
     * @brief Binds an input event shape to an action.
     * @param event The input event shape to compare during mapping.
     */
    void action_bind_event( const char* name, const InputEvent& event );
    bool action_exists( const char* name );

    bool action_is_held( const char* name );
    bool action_is_pressed( const char* name );
    bool action_is_released( const char* name );
    /**
     * @brief Return the strength difference in [0,1] between two actions.
     */
    float action_get_axis( const char* neg_action, const char* pos_action );
    /**
     * @brief This basically perform two-part action_get_axis().
     * @return The normalized values.
     */
    Vec2 action_get_vector(
        const char* neg_action_x, const char* pos_action_x, const char* neg_action_y, const char* pos_action_y
    );

    bool is_anything_held() const;
    bool is_anything_pressed() const;

    Vec2 get_mouse_position() const;
    Vec2 get_mouse_delta() const;
    Vec2 get_mouse_wheel() const;

    bool is_mouse_button_pressed( ButtonIndex button ) const;

    /**
     * @brief Appends a snapshot of all registered action names.
     */
    void action_list( DynamicArray<StringView>& out );

    /**
     * @brief Inserts a new input event to the queue. It will be
     * processed in the next pump_events(). Excessive events will be
     * ignored.
     */
    void add_input_event( const InputEvent& event );

    /**
     * @brief Fills the event queue with OS-generated input events and updates
     * the internal action state tracking from the queued events.
     */
    void update();

    /**
     * @brief Returns the queued OS events collected by the last pump_events().
     * Valid until the next pump_events() call.
     */
    std::span<const InputEvent> get_events() const
    {
        return std::span<const InputEvent>( event_queue.data(), event_queue.get_head() );
    }

private:
    void pump_events_();
    ActionBinding* get_action_( const char* name );

    BumpAllocator<InputEvent> event_queue{ 64 };
    ResourcePool<ActionBinding> actions{ 512 };
    HashMap<String, RID> action_by_name;

    Array<ButtonState, KEY_COUNT> key_states;
    Array<ButtonState, MOUSE_BUTTON_COUNT> mb_states; // mouse button states.
    Vec2 mouse_wheel    = Vec2();                     // accumulated scroll deltas.
    Vec2 mouse_pos      = Vec2();                     // last absolute position (window coords).
    Vec2 mouse_delta    = Vec2();                     // accumulated relative motion.
    bool is_any_held    = false;
    bool is_any_pressed = false;
};

} // namespace nc
