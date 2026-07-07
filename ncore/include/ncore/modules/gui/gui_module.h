#pragma once

#include <ncore/modules/module.h>

namespace nc {

class Event;

/**
 * @brief IImGuiModule provides base functionality for immediate-mode GUI
 * implementations.
 */
class IImGuiModule : public IModule {
    NCLASS( IImGuiModule, IModule )

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

    virtual bool process_event( Event* event ) = 0;

protected:
    std::vector<IMGuiLayer> layers;
};

} // namespace nc
