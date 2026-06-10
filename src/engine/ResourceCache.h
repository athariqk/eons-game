#pragma once

#include <unordered_map>

#include <Definitions.h>

namespace ncore {

template<typename TResource, typename TLoader>
class ResourceCache {
public:
    ResourceCache() {}
    ~ResourceCache() {}

    TResource operator[](const ResourceID &resId) { return cache_store[resId]; }

private:
    std::unordered_map<ResourceID, TResource> cache_store;
};

} // namespace ncore
