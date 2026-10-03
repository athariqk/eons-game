// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <functional>

#include <ncore/core/errors.h>
#include <ncore/core/object.h>
#include <ncore/core/reference.h>
#include <ncore/resources/resource.h>

namespace nc {

class IResource;

struct AssetImportContext {
    std::function<RID( const String& filepath )> load;
    std::function<Ref<IResource>( RID handle )> get;
    bool skip_cache;
    /// Directories a compiler may resolve `#include` targets against, in priority
    /// order. Empty means the compiler's own default. The shader compiler uses it
    /// to reach raw .slang sources directly, so they need never be staged into the
    /// runtime tree.
    DynamicArray<String> search_paths;
};

class IAssetCompiler : public Object {
    NCLASS( IAssetCompiler, Object )

public:
    virtual StringView get_format_name() const              = 0;
    virtual bool is_handling_extension( const String& ext ) = 0;

    Error operator()( StringView p_input, StringView p_output, AssetImportContext p_ctx )
    {
        return compile( p_input, p_output, p_ctx );
    }

    virtual Error compile( StringView p_input, StringView p_output, AssetImportContext p_ctx ) = 0;
};

} // namespace nc
