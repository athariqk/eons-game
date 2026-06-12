#pragma once

#include <modules/resources/Resource.h>

namespace ncore {

// Can only load from disk lol
template<std::derived_from<Resource> T>
struct IResourceLoader {
    T operator()(const std::string &path) {
        T res = load_from_disk(path);
        NC_LOG_TRACE_C(ncore::log::IO, "Loaded resouce of size {} bytes from path: '{}'", res.get_size_in_bytes(),
                       path);
        return res;
    }

    virtual T load_from_disk(std::string location) = 0;
};

} // namespace ncore
