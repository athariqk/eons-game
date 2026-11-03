#pragma once

#include <ncore/services/video/render_service.h>

namespace nc {

/**
 * @brief Whether a previously prepared handle is still alive on the current device.
 */
inline bool is_prepared_alive( RenderService* p_renderer, RID p_rid, uint32_t p_service_revision )
{
    return p_rid.is_valid() && p_renderer && p_renderer->get_device_revision() == p_service_revision;
}

} // namespace nc
