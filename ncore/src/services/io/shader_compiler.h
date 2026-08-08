#pragma once

#include <slang.h>

#include <ncore/core/object.h>
#include <ncore/resources/shader.h>

#include <slang-com-ptr.h>

namespace nc {

struct ShaderCompileDesc {
    std::string_view filepath;
    bool recreate_session = false;
};

struct ShaderCompileResult {
    std::string_view shader_name;
    DynamicArray<ShaderDesc> programs;
    bool ok = false;
    std::string diagnostics;
};

// struct CustomSlangFileSystem : public ISlangFileSystem {
//     SlangResult loadFile( const char* path, ISlangBlob** outBlob ) override;
// };

class NCAPI ShaderCompiler : public NcObject {
    NCLASS( ShaderCompiler, NcObject )

public:
    ShaderCompiler();
    ~ShaderCompiler() override;

    ShaderCompiler( const ShaderCompiler& )            = delete;
    ShaderCompiler& operator=( const ShaderCompiler& ) = delete;

    /**
     * @brief Resolve shader programs and reflection metadata.
     */
    ShaderCompileResult compile( const ShaderCompileDesc& desc );

private:
    bool ensure_session( bool recreate_session = false );

    Slang::ComPtr<slang::IGlobalSession> global_session;
    Slang::ComPtr<slang::ISession> compile_session;
    bool session_created        = false;
    const char* search_paths[1] = { "assets/shaders" };
};

} // namespace nc
