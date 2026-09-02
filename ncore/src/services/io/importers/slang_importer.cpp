#include "slang_importer.h"

#include <sstream>

#include <ncore/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

Ref<IResource> SlangImporter::import( const String& path, Context ctx )
{
    auto slang = compiler.compile( ShaderCompileDesc{ .filepath = path, .recreate_session = ctx.skip_cache } );
    if (!slang.diagnostics.empty()) {
        NC_LOG_DEBUG_C( log::IO, "{}", slang.diagnostics );
    }
    if (!slang.ok) {
        NC_LOG_ERROR_C( log::IO, "Shader compilation FAILED" );
        return nullptr;
    }

    auto shaders = Ref<Shader>::create( slang.programs );
    return shaders;
}

} // namespace nc
