#pragma once

#include <IGraphicsContext.h>
#include <SDL3/SDL_render.h>

namespace ncore {

class SDLGraphicsContext : public IGraphicsContext {
public:
    SDLGraphicsContext(SDL_Renderer *p_renderer) : renderer(p_renderer) {}

    void clear() override;
    void present() override;
    void set_draw_color(const Color &color) override;

	void draw_line(const Rect &rect) override;
    void draw_rect(const Rect &rect) override;
    void fill_rect(const Rect &rect) override;
    void draw_point(float x, float y) override;

    void *get_native_handle() const override { return renderer; }

private:
    SDL_Renderer *renderer;
};

} // namespace ncore
