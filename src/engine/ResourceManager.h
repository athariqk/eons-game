#pragma once

#include "ResourceCache.h"
#include "TextureLoader.h"

namespace ncore {

class TextureResource;

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

private:
    ResourceCache<TextureResource, TextureLoader> textures_cache;
};

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager() {}

} // namespace ncore
