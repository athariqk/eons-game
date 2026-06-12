#pragma once

#include <SDL3/SDL_render.h>

#include <modules/graphics/IGraphicsContext.h>
#include <unordered_map>

namespace ncore {

class SDLGraphicsContext : public IGraphicsContext {
public:
    SDLGraphicsContext(SDL_Renderer *p_renderer) : renderer(p_renderer) {}

    void clear() override;
    void present() override;
    void set_draw_color(const Color &color) override;

    void draw_line(const Vec4 &rect) override;
    void draw_rect(const Vec4 &rect) override;
    void fill_rect(const Vec4 &rect) override;
    void draw_point(float x, float y) override;

    void draw_texture(const Image *tex, const Vec4 &destRect, const Vec4 &srcRect, float angle,
                      const Color &color) override;

    void *get_native_handle() const override { return renderer; }

private:
    SDL_Renderer *renderer;
    std::unordered_map<const Image *, SDL_Texture *> texture_cache;
    SDL_Texture *get_or_upload_img(const Image *image);
};

} // namespace ncore
