#pragma once

#include <ncore/kernel/memory.h>
#include <ncore/kernel/resource.h>

namespace ncore {

template<typename T>
class AssetPool {
public:
    ~AssetPool()
    {
        unload_all();
    }

    using LoaderFn = std::function<std::unique_ptr<T>( const std::string_view path )>;
    void set_loader( LoaderFn loader )
    {
        loader_func = std::move( loader );
    }

    /**
     * @brief Loads the given asset into memory.
     * This caches the asset after the first call.
     */
    RID load( const std::string_view path )
    {
        auto it = path_map.find( path );
        if (it != path_map.end()) {
            if (is_valid( it->second )) {
                return it->second;
            }
        }

        NC_ASSERT( loader_func, "Requested asset has no loader" );

        size_t index = slots.alloc();
        Slot* slot   = slots.get( index );

        slot->data     = loader_func( path );
        slot->filepath = std::string( path );
        slot->generation++;

        RID handle               = encode_rid( static_cast<uint32_t>( index ), slot->generation );
        path_map[slot->filepath] = handle;
        return handle;
    }

    T* get( RID handle )
    {
        if (!handle.is_valid())
            return nullptr;

        auto [index, generation] = decode_rid( handle );

        Slot* slot = slots.get( index );
        if (!slot || slot->generation != generation || !slot->data) {
            return nullptr;
        }
        return slot->data.get();
    }

    void unload( RID handle )
    {
        if (!handle.is_valid())
            return;

        auto [index, generation] = decode_rid( handle );

        Slot* slot = slots.get( index );
        if (!slot || slot->generation != generation)
            return;

        path_map.erase( slot->filepath );

        slot->filepath.clear();
        slot->data.reset();

        slots.dealloc( index );
    }

    void unload_all()
    {
        slots.clear();
        path_map.clear();
    }

    size_t count() const
    {
        return slots.size();
    }

private:
    static RID encode_rid( uint32_t index, uint32_t generation )
    {
        uint64_t val = ( static_cast<uint64_t>( generation ) << 32 ) | index;
        return RID{ val };
    }

    static std::pair<uint32_t, uint32_t> decode_rid( RID handle )
    {
        uint32_t index      = static_cast<uint32_t>( handle.value & 0xFFFFFFFFu );
        uint32_t generation = static_cast<uint32_t>( handle.value >> 32 );
        return { index, generation };
    }

    bool is_valid( RID handle ) const
    {
        if (!handle.is_valid())
            return false;
        auto [index, generation] = decode_rid( handle );
        const Slot* slot         = slots.get( index );
        return slot && slot->generation == generation && slot->data != nullptr;
    }

private:
    struct Slot {
        std::unique_ptr<T> data;
        uint32_t generation = 0;
        std::string filepath;
    };

    NoobPool<Slot> slots;

    struct string_hash {
        using is_transparent = void;
        size_t operator()( std::string_view sv ) const
        {
            return std::hash<std::string_view>{}( sv );
        }
    };
    std::unordered_map<std::string, RID, string_hash, std::equal_to<>> path_map;

    LoaderFn loader_func;
};

} // namespace ncore
