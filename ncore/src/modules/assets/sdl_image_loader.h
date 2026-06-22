#pragma once

#include <modules/assets/asset_loader.h>
#include <ncore/modules/video/image.h>

namespace ncore {

struct SDLImageLoader : public IAssetLoader<Image> {
public:
    std::unique_ptr<Image> load(const std::string_view path) override;
};

} // namespace ncore
