#pragma once

#include <memory>
#include <unordered_map>

#include <modules/assets/asset_pool.h>
#include <ncore/kernel/types.h>
#include <ncore/modules/service.h>

namespace ncore {

class AssetManager : public IService {
    NCLASS(AssetManager, IService)

public:
    AssetManager() = default;
    Error init() override { return Error::OK; }
    void finalize() override {}

    template<typename T>
    void register_loader(typename AssetPool<T>::LoaderFn loader) {
        get_pool<T>().set_loader(std::move(loader));
    }

    template<typename T>
    RID load(const std::string_view &path) {
        return get_pool<T>().load(path);
    }

    template<typename T>
    T *get(RID ref) {
        auto result = get_pool<T>().get(ref);
        NC_ASSERT(result != nullptr, "Failed to get asset resource: invalid handle or type mismatch");
        return result;
    }

    void unload_all();

    template<typename T>
    size_t count() const {
        auto idx = rfl::Registry::get_type_id<T>();
        auto it = pools.find(idx);
        if (it == pools.end())
            return 0;

        auto *pool = static_cast<AssetPool<T> *>(it->second.get());
        return pool ? pool->count() : 0;
    }

private:
    template<typename T>
    AssetPool<T> &get_pool() {
        auto idx = rfl::Registry::get_type_id<T>();
        auto it = pools.find(idx);
        if (it != pools.end())
            return *static_cast<AssetPool<T> *>(it->second.get());

        auto pool = std::make_shared<AssetPool<T>>();
        pools[idx] = pool;
        return *pool;
    }

    std::unordered_map<rfl::TypeId, std::shared_ptr<void>> pools;
};

} // namespace ncore
