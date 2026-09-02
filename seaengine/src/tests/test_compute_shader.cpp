#include "test_compute_shader.h"

#include <ncore/application.h>
#include <ncore/resources/shader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/services/video/rhi_types.h>

namespace sea {

using namespace nc;

void hello_world_compute_shader( nc::Scene& scene )
{
    auto resources = scene.get_app_ctx()->Services.resolve<ResourceService>();
    auto renderer  = scene.get_app_ctx()->Services.resolve<RenderService>();

    auto hello_world_shd = resources->load<Shader>( "shaders/compute/hello_world.slang" );

    float x_floats[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    float y_floats[16] = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 };
    float z_floats[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    auto buffer0 = renderer->buffer_create(
        rhi::BufferDesc{
            .debug_name   = "buffer0",
            .bind_mask    = rhi::ResourceBindFlags::SHADER_RESOURCE,
            .size         = sizeof( x_floats ),
            .initial_data = x_floats,
            .usage        = rhi::ResourceUsage::IMMUTABLE,
            .access_mask  = rhi::ResourceAccessFlags::NONE,
            .mode         = rhi::BufferMode::RAW
        }
    );
    auto buffer1 = renderer->buffer_create(
        rhi::BufferDesc{
            .debug_name   = "buffer1",
            .bind_mask    = rhi::ResourceBindFlags::SHADER_RESOURCE,
            .size         = sizeof( y_floats ),
            .initial_data = y_floats,
            .usage        = rhi::ResourceUsage::IMMUTABLE,
            .access_mask  = rhi::ResourceAccessFlags::NONE,
            .mode         = rhi::BufferMode::RAW
        }
    );
    auto result = renderer->buffer_create(
        rhi::BufferDesc{
            .debug_name   = "result",
            .bind_mask    = rhi::ResourceBindFlags::UNORDERED_ACCESS,
            .size         = sizeof( z_floats ),
            .initial_data = z_floats,
            .usage        = rhi::ResourceUsage::DEFAULT,
            .access_mask  = rhi::ResourceAccessFlags::NONE,
            .mode         = rhi::BufferMode::RAW
        }
    );
    auto staging = renderer->buffer_create(
        rhi::BufferDesc{
            .debug_name  = "staging",
            .bind_mask   = rhi::ResourceBindFlags::NONE,
            .size        = sizeof( z_floats ),
            .usage       = rhi::ResourceUsage::STAGING,
            .access_mask = rhi::ResourceAccessFlags::READ,
            .mode        = rhi::BufferMode::RAW
        }
    );

    renderer->get_graphics_api()->set_queue( GpuQueue::COMPUTE );
    renderer->get_graphics_api()->set_context_state( false );
    rhi::ResourceMappingEntry shader_resource_entries[3] = {
        { "buffer0", rhi::ResourceType::BUFFER_SRV, buffer0 },
        { "buffer1", rhi::ResourceType::BUFFER_SRV, buffer1 },
        { "result", rhi::ResourceType::BUFFER_UAV, result },
    };
    auto set         = renderer->resource_set_create( *hello_world_shd, 0, shader_resource_entries );
    auto compute_pso = renderer->compute_pipeline_create( *hello_world_shd, { &set, 1 } );

    renderer->compute_pipeline_bind( compute_pso );
    renderer->resource_set_bind( set );
    renderer->compute_dispatch( 1, 1, 1 );
    renderer->get_graphics_api()->set_queue( GpuQueue::TRANSFER );
    renderer->buffer_blit( result, staging );
    renderer->get_graphics_api()->sync_queue( GpuQueue::TRANSFER );
    renderer->buffer_data_read( staging, { reinterpret_cast<std::byte*>( z_floats ), sizeof( z_floats ) } );
    NC_LOG_INFO(
        "Result: [{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}]", z_floats[0], z_floats[1], z_floats[2], z_floats[3],
        z_floats[4], z_floats[5], z_floats[6], z_floats[7], z_floats[8], z_floats[9], z_floats[10], z_floats[11],
        z_floats[12], z_floats[13], z_floats[14], z_floats[15]
    );
}

} // namespace sea
