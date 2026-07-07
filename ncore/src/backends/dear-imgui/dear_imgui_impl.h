#pragma once

#include <ncore/modules/gui/gui_module.h>
#include <ncore/modules/video/window_types.h>

class ImGuiContext;

namespace nc {

class GraphicsModule;
class IWindowModule;

class DearImGuiImpl : public IGuiModule {
public:
    DearImGuiImpl( GraphicsModule* gfx, IWindowModule* windows );
    ~DearImGuiImpl() override;

    virtual Error init() override;
    virtual void finalize() override;

    virtual void begin_frame() override;
    virtual void render_frame() override;

    virtual bool process_event( Event* event ) override;

protected:
    GraphicsModule* gfx;
    IWindowModule* windows;
    ImGuiContext* imgui_ctx = nullptr;
    std::unordered_map<ImGuiMouseCursor, CursorType> cursor_map;
};

} // namespace nc
