#pragma once

#include <span>

#include <ncore/kernel/collection.h>
#include <ncore/modules/input/input_event.h>
#include <ncore/modules/module.h>

namespace nc {

/**
 * @brief InputModule owns mouse, keyboard, and text input events.
 */
class NCORE_API InputModule : public IModule {
    NCLASS( InputModule, IModule )

public:
    InputModule() = default;

    Error init( ConfFile& cfg_file ) override;
    void finalize() override;

    /**
     * @brief Updates the internal event queue.
     */
    void pump_events();

    /**
     * @brief Returns the input events collected by the last pump_events().
     * Valid until the next pump_events() call.
     */
    std::span<const InputEvent> input_events() const
    {
        return std::span<const InputEvent>( event_queue.data(), event_queue.size() );
    }

private:
    Vector<InputEvent> event_queue;
};

} // namespace nc
