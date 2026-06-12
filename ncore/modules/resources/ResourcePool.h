#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Resource.h"
#include "ResourceHandle.h"

namespace ncore {

template<std::derived_from<Resource> T>
struct ResourceSlot {
    std::optional<T> data;
    uint32_t generation = 0;
    std::string filepath;
};

template<std::derived_from<Resource> T>
class ResourcePool {
public:
    ~ResourcePool() { unload_all(); }

    using LoaderFn = std::function<T(const std::string &path)>;

    ResourceHandle get_or_load(const std::string &path) {
        auto it = path_map.find(path);
        if (it != path_map.end()) {
            auto &slot = slots[it->second.id];
            if (slot.generation == it->second.generation)
                return it->second;
        }
        return load(path);
    }

    void set_loader(LoaderFn loader) { loader_func = std::move(loader); }

    T *get(ResourceHandle handle) {
        if (!handle.is_valid() || handle.id >= slots.size())
            return nullptr;
        auto &slot = slots[handle.id];
        if (slot.generation != handle.generation || !slot.data.has_value())
            return nullptr;
        return &slot.data.value();
    }

    void destroy(ResourceHandle handle) {
        if (!handle.is_valid() || handle.id >= slots.size())
            return;
        auto &slot = slots[handle.id];
        if (slot.generation != handle.generation)
            return;

        auto it = path_map.find(slot.filepath);
        if (it != path_map.end() && it->second.id == handle.id)
            path_map.erase(it);

        slot.data.reset();
        slot.generation++;
        slot.filepath.clear();
        free_ids.push_back(handle.id);
    }

    void unload_all() {
        slots.clear();
        path_map.clear();
        free_ids.clear();
    }

    void on_gui_render();

    auto &all_slots() { return slots; }
    size_t count() const { return slots.size() - free_ids.size(); }

private:
    ResourceHandle load(const std::string &path) {
        NC_ASSERT_RETVAL(loader_func, ResourceHandle(), "Failed to load the requested resource. It has no loader");

        uint32_t idx;
        if (!free_ids.empty()) {
            idx = free_ids.back();
            free_ids.pop_back();
        } else {
            idx = static_cast<uint32_t>(slots.size());
            slots.emplace_back();
        }

        auto &slot = slots[idx];
        slot.data = loader_func(path);
        slot.generation++;
        slot.filepath = path;

        ResourceHandle handle{idx, slot.generation};
        path_map[path] = handle;
        return handle;
    }

    std::vector<ResourceSlot<T>> slots;
    std::vector<ResourceID> free_ids;
    std::unordered_map<std::string, ResourceHandle> path_map;
    LoaderFn loader_func;
};

} // namespace ncore
