#pragma once

#include <DeviceContext.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <vector>

#include <ncore/kernel/structures.h>

namespace nc {

struct BatchDrawCmd {
    std::vector<uint8_t> vertices;
    std::vector<uint16_t> indices;
    void* native_texture = nullptr;
    Vec4 clip_rect;
    bool is_textured = false;
};

class BatchRenderer2D {
    template<typename T>
    using DiligentRef = Diligent::RefCntAutoPtr<T>;

public:
    BatchRenderer2D(
        DiligentRef<Diligent::IRenderDevice> device, DiligentRef<Diligent::IDeviceContext> ctx,
        DiligentRef<Diligent::IPipelineState> pso, DiligentRef<Diligent::IShaderResourceBinding> srb,
        Diligent::IShaderResourceVariable* tex_var
    );

    void push_quad( void* native_texture, Vec4 dest, Vec4 src, Color tint );
    void push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count,
        void* native_texture, Vec4 clip_rect
    );
    void flush( DiligentRef<Diligent::IBuffer> constants, Vec2 surf_size );

private:
    void ensure_buffers_( size_t needed_vb, size_t needed_ib );

    DiligentRef<Diligent::IRenderDevice> render_device;
    DiligentRef<Diligent::IDeviceContext> device_ctx;
    DiligentRef<Diligent::IPipelineState> m_pso;
    DiligentRef<Diligent::IShaderResourceBinding> m_srb;
    Diligent::IShaderResourceVariable* m_texture_var;

    DiligentRef<Diligent::IBuffer> m_vb;
    DiligentRef<Diligent::IBuffer> m_ib;
    Diligent::Uint32 m_vb_capacity = 0;
    Diligent::Uint32 m_ib_capacity = 0;

    static constexpr size_t VERTEX_STRIDE = sizeof( float ) * 4 + sizeof( uint32_t );
    std::vector<BatchDrawCmd> m_cmds;
};

} // namespace nc
