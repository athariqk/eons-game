#pragma once

#include <ncore/modules/service.h>

namespace ncore {

/**
 * @brief Immediate-mode GUI service interface.
 */
class IImGuiService : public IService {
    NCLASS( IImGuiService, IService )

public:
    struct IMGuiLayer {
        std::string name;
        int order;
        std::function<void()> callback;
    };

public:
    virtual ~IImGuiService() = default;

    void update();
    void add_layer( const std::string& layer_name, int order, std::function<void()> callback );

    virtual void begin_frame()  = 0;
    virtual void render_frame() = 0;

protected:
    std::vector<IMGuiLayer> layers;
};

} // namespace ncore
