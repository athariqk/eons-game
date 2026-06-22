#pragma once

#include <ncore/modules/video/render_service.h>

#include <unordered_map>

#include <SDL3/SDL_render.h>

namespace ncore {

class SDLRenderImpl : public IRenderService {
    NCLASS(SDLRenderImpl, IRenderService)

public:
    SDLRenderImpl(uint32_t window_id) : window_id(window_id) {}
    ~SDLRenderImpl() override {}

    Error init() override;
    void finalize() override {}

    void new_frame() override;
    void present_frame() override;

    void set_draw_color(const Color &color) override;
    void draw_line(const Vec4 &rect) override;
    void draw_rect(const Vec4 &rect) override;
    void fill_rect(const Vec4 &rect) override;
    void draw_point(float x, float y) override;

    void draw_texture(const Image *tex, const Vec4 &destRect, const Vec4 &srcRect, float angle,
                      const Color &color) override;

    void *get_native_handle() const override { return renderer; }

private:
    uint32_t window_id;
    SDL_Renderer *renderer = nullptr;
    std::unordered_map<const Image *, SDL_Texture *> texture_cache;
    SDL_Texture *get_or_upload_img(const Image *image);
};

} // namespace ncore
