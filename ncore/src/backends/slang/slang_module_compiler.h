#pragma once

#include <slang.h>

#include <ncore/services/io/asset_compiler.h>

#include <slang-com-ptr.h>

namespace nc {

struct ShaderCompileDesc {
    String module_path;
    bool recreate_session = false;
    /**
     * @brief Directories Slang searches for `#include` targets, in priority order.
     *
     * Empty falls back to the legacy cwd-relative trio ("assets/shaders*"). The
     * offline asset pipeline passes absolute roots so raw .slang sources never
     * have to be staged into the runtime tree.
     */
    DynamicArray<String> search_paths;
};

struct MaterialShaderDesc {
    // Spatial vs Canvas, inferred from which interface the material struct implements
    // (ISpatialMaterial -> Spatial, ICanvasMaterial -> Canvas).
    MaterialShaderType material_kind = MaterialShaderType::Spatial;
    gfx::CullMode cull_mode                     = gfx::CullMode::NONE;
    gfx::FillMode fill_mode                     = gfx::FillMode::SOLID;
    bool depth_test                             = false;
    bool depth_write                            = false;
    gfx::BlendPreset blend                      = gfx::BlendPreset::ALPHA_BLEND;
    gfx::MultisampleStateDesc multisample_state = { 1, 0 };
    bool is_material                            = false;
    // Concrete material struct, used to specialize the generic entry points
    // declared by the base material module (ncore.spatial / ncore.canvas).
    slang::TypeReflection* material_type = nullptr;
};

class SlangModuleCompiler : public IAssetCompiler {
    NCLASS( SlangModuleCompiler, IAssetCompiler )

public:
    SlangModuleCompiler();
    ~SlangModuleCompiler() override;

    SlangModuleCompiler( const SlangModuleCompiler& )            = delete;
    SlangModuleCompiler& operator=( const SlangModuleCompiler& ) = delete;

    virtual StringView get_format_name() const override;
    bool is_handling_extension( const String& ext ) override;

    Error compile( StringView p_input, StringView p_output, AssetImportContext p_ctx ) override;

private:
    bool ensure_session_( bool recreate_session = false );

    /// Swap the `#include` search roots. No-op when unchanged; otherwise invalidates
    /// the cached compile session so the next ensure_session_() rebuilds it.
    void set_search_paths_( const DynamicArray<String>& p_paths );

    void reflect_fields_(
        slang::TypeLayoutReflection* type_layout, uint32_t base_offset, const String& name_prefix,
        uint32_t binding_space, gfx::ShaderStage stage, DynamicArray<ShaderParamField>& out_fields,
        DynamicArray<ShaderParamDesc>& out_params
    );
    void reflect_module_params_( slang::ProgramLayout* p_module_layout, ShaderDesc& p_module );

    void reflect_material_attributes_( slang::IModule* p_module, MaterialShaderDesc& out_desc );

    /**
     * @brief Reflects all entry - point parameters : varying inputs( vertex attributes ),
     * uniform/push-constant data, and resources/samplers.
     */
    void reflect_entry_point_params_(
        slang::EntryPointLayout* ep_layout, gfx::ShaderStage stage, DynamicArray<ShaderParamDesc>& out_params,
        gfx::VertexLayout& out_vert_layout
    );

private:
    Slang::ComPtr<slang::IGlobalSession> global_session;
    Slang::ComPtr<slang::ISession> compile_session;
    bool session_created = false;
    /// Owns the search-path strings for the whole session. search_path_ptrs_ points
    /// into it, so this must never be resized once the ptrs have been built.
    DynamicArray<String> search_path_storage_;
    /// Non-owning views handed to slang::SessionDesc::searchPaths, which keeps the
    /// raw pointers for as long as the session lives.
    DynamicArray<const char*> search_path_ptrs_;
};

} // namespace nc
