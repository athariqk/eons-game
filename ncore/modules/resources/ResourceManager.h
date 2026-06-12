#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "ResourcePool.h"

namespace ncore {

class ResourceManager {
public:
    template<typename T>
    void register_loader(typename ResourcePool<T>::LoaderFn loader) {
        get_pool<T>().set_loader(std::move(loader));
    }

    template<typename T>
    ResourceHandle get(const std::string &path) {
        return get_pool<T>().get_or_load(path);
    }

    template<typename T>
    T *access(ResourceHandle handle) {
        return get_pool<T>().get(handle);
    }

    template<typename T>
    void unload(ResourceHandle handle) {
        get_pool<T>().destroy(handle);
    }

    void unload_all();
    void on_gui_render();

private:
    template<typename T>
    ResourcePool<T> &get_pool() {
        auto idx = std::type_index(typeid(T));
        auto it = pools.find(idx);
        if (it != pools.end())
            return *static_cast<ResourcePool<T> *>(it->second.get());

        auto pool = std::make_shared<ResourcePool<T>>();
        auto *ptr = pool.get();
        pools[idx] = std::move(pool);
        return *ptr;
    }

    std::unordered_map<std::type_index, std::shared_ptr<void>> pools;
};

} // namespace ncore
