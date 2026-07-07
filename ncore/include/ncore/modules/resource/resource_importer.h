// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>

#include <ncore/kernel/object.h>
#include <ncore/kernel/reference.h>
#include <ncore/modules/resource/resource.h>
#include <ncore/utils/log.h>

namespace nc {

class IResource;

class IResourceImporter : public NcObject {
    NCLASS( IResourceImporter, NcObject )

public:
    virtual bool is_handling_extension( const std::string& ext ) = 0;

    Ref<IResource> operator()( const std::string_view path )
    {
        auto resource = import( path );
        if (resource) {
            NC_LOG_TRACE_C( log::IO, "Imported resource '{}' from path: '{}'", resource->get_class_name(), path );
        }
        return resource;
    }

    virtual Ref<IResource> import( const std::string_view path ) = 0;
};

} // namespace nc
