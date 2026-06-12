#pragma once

#include <modules/resources/IResourceLoader.h>

namespace ncore {

struct SDLImageLoader : public IResourceLoader<Image> {
public:
    Image load_from_disk(std::string location) override;
};

} // namespace ncore
