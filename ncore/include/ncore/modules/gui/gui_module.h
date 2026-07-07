#pragma once

#include <ncore/modules/module.h>

namespace nc {

class BaseEvent;

/**
 * @brief IGuiModule provides base functionality for GUI display/layout.
 *
 * Choosing immediate-mode or retained-mode is an implementation detail.
 */
class IGuiModule : public IModule {
    NCLASS( IGuiModule, IModule )

public:
    struct IMGuiLayer {
        std::string name;
        int order;
        std::function<void()> callback;
    };

public:
    void update();
    void add_layer( const std::string& layer_name, int order, std::function<void()> callback );

    virtual void begin_frame()  = 0;
    virtual void render_frame() = 0;

    virtual bool process_event( BaseEvent* event ) = 0;

protected:
    std::vector<IMGuiLayer> layers;
};

} // namespace nc
