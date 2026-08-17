// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <functional>
#include <string>

#include <ncore/core/object.h>
#include <ncore/core/reference.h>
#include <ncore/resources/resource.h>

namespace nc {

class IResource;

class IResourceImporter : public NcObject {
    NCLASS( IResourceImporter, NcObject )

public:
    struct Context {
        std::function<RID( const String& filepath )> load;
        std::function<Ref<IResource>( RID handle )> get;
        bool skip_cache;
    };

    virtual bool is_handling_extension( const String& ext ) = 0;

    Ref<IResource> operator()( const String& path, Context ctx )
    {
        auto resource = import( path, ctx );
        return resource;
    }

    virtual Ref<IResource> import( const String& path, Context ctx ) = 0;
};

} // namespace nc
