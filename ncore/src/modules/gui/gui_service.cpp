#include <ncore/modules/gui/gui_service.h>

namespace ncore {

void IImGuiService::update()
{
    begin_frame();
    for (const auto& layer : layers) {
        layer.callback();
    }
    render_frame();
}

void IImGuiService::add_layer( const std::string& layer_name, int order, std::function<void()> callback )
{
    layers.push_back( { layer_name, order, callback } );

    // Sort layers by order
    std::sort( layers.begin(), layers.end(), []( const IMGuiLayer& a, const IMGuiLayer& b ) {
        return a.order < b.order;
    } );
}

}; // namespace ncore
