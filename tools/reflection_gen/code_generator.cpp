// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "code_generator.h"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace {

// A qualified_name is a display/registry-key string (e.g. "ns::Foo"). When
// emitting it as an actual C++ type reference in generated code, force
// global-scope resolution so it can't be shadowed by anything visible inside
// the generated file's `namespace nc { ... }` wrapper.
std::string qualify_for_cpp( const std::string& qualified_name )
{
    return qualified_name.empty() ? qualified_name : "::" + qualified_name;
}

// Turns a qualified name into a valid, collision-resistant C++ identifier
// fragment, for use in generated static-init variable names. Two classes
// named "Foo" in different namespaces must not produce the same identifier.
// Collapses consecutive underscores to avoid reserved identifiers.
std::string sanitize_identifier( const std::string& qualified_name )
{
    std::string id = qualified_name;
    std::replace( id.begin(), id.end(), ':', '_' );
    std::replace( id.begin(), id.end(), '<', '_' );
    std::replace( id.begin(), id.end(), '>', '_' );
    std::replace( id.begin(), id.end(), ' ', '_' );
    std::replace( id.begin(), id.end(), ',', '_' );
    std::replace( id.begin(), id.end(), '*', '_' );
    // Collapse consecutive underscores
    std::string result;
    bool last_was_underscore = false;
    for (char c : id) {
        if (c == '_') {
            if (!last_was_underscore) {
                result += c;
                last_was_underscore = true;
            }
        } else {
            result += c;
            last_was_underscore = false;
        }
    }
    return result;
}

std::string generate_dep_type_registrations( const std::vector<ReflectedDepType>& dep_types )
{
    if (dep_types.empty())
        return {};

    std::ostringstream oss;
    for (const auto& dep : dep_types) {
        oss << dep.registration_code;
    }
    return oss.str();
}

std::string property_flags_expr( const ReflectedField& f )
{
    std::string flags = "::nc::types::PropertyFlags::SERIALIZABLE | ::nc::types::PropertyFlags::EDITABLE";
    if (f.hidden)
        flags += " | ::nc::types::PropertyFlags::HIDDEN";
    if (f.read_only)
        flags += " | ::nc::types::PropertyFlags::READ_ONLY";
    return flags;
}

std::string generate_field_array( const ReflectedRecord& rec )
{
    if (rec.fields.empty())
        return {};

    std::ostringstream oss;
    oss << "        static const ::nc::types::FieldInfo fields[] = {\n";
    for (const auto& f : rec.fields) {
        oss << "            { \"" << f.name << "\",\n"
            << "              ::nc::types::TypeId{" << f.type_id_hash << "ULL},\n"
            << "              " << f.field_size << ",\n"
            << "              " << f.field_offset << ",\n"
            << "              " << property_flags_expr( f ) << ",\n"
            << "              ::nc::types::Qualifier{";
        if (f.array_length > 0)
            oss << ".array_length = " << f.array_length;
        else if (f.pointer_count > 0) {
            oss << ".pointer_count = " << f.pointer_count;
            if (f.is_cstring)
                oss << ", .is_cstring = true";
        }
        oss << "} },\n";
    }
    oss << "        };\n";
    return oss.str();
}

std::string generate_record_code( const ReflectedRecord& rec )
{
    std::ostringstream oss;

    // Use the fully qualified name for every actual C++ reference. Use the
    // unqualified qualified_name (no leading "::") for the registry's
    // lookup-by-name string.
    std::string cpp_name = qualify_for_cpp( rec.qualified_name );

    // Compute TypeId hash from the qualified name
    uint64_t type_hash = compute_type_id_hash( rec.qualified_name );

    // NC_COMPONENT types inherit EcsComponentBase<T>, whose registration
    // lives in a static member of a class template — only instantiated on
    // odr-use, which mere inheritance never does. Force it here by calling
    // CLASS_INFO() (static function wrapping a function-local
    // static, idempotent, thread-safe). Needs the complete type, so
    // generate_reflection_header #includes the source header for ECS records.
    // NSTRUCT_V's nc_register_##T sits in the non-template class body and
    // runs when the source header is included — attach fields by TypeId only
    // (provide_fields is order-independent: attaches now or stashes pending
    // until register_type).
    if (rec.is_ecs_component || rec.has_nstruct_v) {
        if (rec.is_ecs_component) {
            oss << "        (void)::nc::EcsComponentBase<" << cpp_name << ">::CLASS_INFO();\n";
        }
        if (!rec.fields.empty()) {
            oss << "    {\n"
                << generate_field_array( rec ) << "        ::nc::TypeDB::provide_fields(\n"
                << "            ::nc::types::TypeId{" << type_hash << "ULL}, fields, fields + " << rec.fields.size()
                << " );\n"
                << "    }\n";
        }
        return oss.str();
    }

    // Pure REFLECT type (no ECS_COMPONENT / NSTRUCT_V): standalone plain
    // RecordInfo — the only registration path.
    oss << "    {\n"
        << "        static ::nc::types::RecordInfo CLASS_INFO(\n"
        << "            \"" << rec.qualified_name << "\",\n"
        << "            ::nc::types::TypeId{" << type_hash << "ULL},\n"
        << "            " << rec.record_size << ", " << rec.record_alignment << "\n"
        << "        );\n"
        << "        ::nc::TypeDB::register_type( &CLASS_INFO );\n";

    // Set parent_id for NCLASS types
    if (rec.is_class && !rec.parent_name.empty()) {
        uint64_t parent_hash = compute_type_id_hash( rec.parent_name );
        oss << "        CLASS_INFO.parent_id = ::nc::types::TypeId{" << parent_hash << "ULL};\n";
    }

    if (!rec.fields.empty()) {
        oss << generate_field_array( rec );
        oss << "        CLASS_INFO.fields_begin = fields;\n"
            << "        CLASS_INFO.fields_end = fields + " << rec.fields.size()
            << ";\n"
            // Also hand the fields to the registry: NSTRUCT_V/NCLASS/
            // ECS_COMPONENT ObjectClass entries may already have won the
            // type_cache (static-init order vs this TU is unspecified), and
            // the winner would otherwise keep an empty field list. provide_fields
            // attaches to whoever is cached, or defers until they register.
            << "        ::nc::TypeDB::provide_fields(\n"
            << "            ::nc::types::TypeId{" << type_hash << "ULL}, fields, fields + " << rec.fields.size()
            << " );\n";
    }

    oss << "    }\n";
    return oss.str();
}

} // anonymous namespace

// Convert a source file path to a relative include path.
// E.g., "D:/dev/eons/ncore/include/ncore/runtime/components/material.h"
//       -> "<ncore/runtime/components/material.h>"
// E.g., "D:/dev/eons/ncore/src/runtime/scene_plugins.h"
//       -> "<runtime/scene_plugins.h>"  (ncore/src is on the include path)
std::string source_to_include_path( const std::string& source_path )
{
    namespace fs         = std::filesystem;
    fs::path p           = source_path;
    std::string path_str = p.generic_string();

    // Find the "include/" prefix and extract the relative path after it
    std::string include_marker = "include/";
    auto pos                   = path_str.find( include_marker );
    if (pos != std::string::npos) {
        return "<" + path_str.substr( pos + include_marker.length() ) + ">";
    }

    // Private header: path relative to ncore/src/ (which ncore adds as an
    // include directory for its own sources, including reflection_impl.cpp).
    std::string src_marker = "src/";
    pos                    = path_str.rfind( src_marker );
    if (pos != std::string::npos) {
        return "<" + path_str.substr( pos + src_marker.length() ) + ">";
    }

    // Fallback: just use the filename
    return "<" + p.filename().string() + ">";
}

std::string generate_reflection_header(
    const ReflectionResult& result, const std::string& source_path, const std::string& guard_macro
)
{
    std::ostringstream oss;

    // ECS records force odr-use of EcsComponentBase<T>::CLASS_INFO(),
    // which requires the complete type — pull in the source header so the type
    // is declared in this TU. Dependent-type registrations also spell concrete
    // C++ types (e.g. std::vector<nc::ShaderDesc, ...> for VectorClass), which
    // need the source header's declarations. NSTRUCT_V / pure REFLECT records
    // stay TypeId-only.
    bool needs_source_header = !result.dep_types.empty();
    for (const auto& rec : result.records) {
        if (rec.is_ecs_component) {
            needs_source_header = true;
            break;
        }
    }

    oss << "// <auto-generated>\n"
        << "// DO NOT EDIT!\n"
        << "// Source: " << source_path << "\n\n"
        << "#pragma once\n\n"
        << "#ifndef " << guard_macro << "\n"
        << "#define " << guard_macro << "\n\n"
        << "#include <ncore/types/typedb.h>\n\n"
        << "#include <ncore/types/classinfo.h>\n\n"
        << "#include <cstddef>\n\n";

    if (needs_source_header)
        oss << "#include " << source_to_include_path( source_path ) << "\n\n";

    oss << "namespace nc {\n\n";

    // Dependent type registrations.
    // The static variable name must be unique per generated header because
    // every header is eventually #included into the same translation unit.
    std::string dep_code = generate_dep_type_registrations( result.dep_types );
    if (!dep_code.empty()) {
        std::string dep_var = "nc_register_dep_types_" + sanitize_identifier( guard_macro );
        oss << "// --- Dependent type registrations ---\n"
            << "namespace {\n"
            << "inline const bool " << dep_var << " = []() -> bool {\n"
            << dep_code << "    return true;\n"
            << "}();\n"
            << "} // anonymous namespace\n\n";
    }

    // Record registrations
    for (const auto& rec : result.records) {
        std::string var_id = sanitize_identifier( rec.qualified_name );
        oss << "// --- " << rec.qualified_name << " ---\n"
            << "namespace {\n"
            << "inline const bool nc_register_" << var_id << " = []() -> bool {\n"
            << generate_record_code( rec ) << "    return true;\n"
            << "}();\n"
            << "} // anonymous namespace\n\n";
    }

    oss << "} // namespace nc\n\n"
        << "#endif // " << guard_macro << "\n";

    return oss.str();
}

std::string generate_reflection_impl( const std::vector<std::string>& relative_header_paths )
{
    std::ostringstream oss;

    oss << "// <auto-generated>\n"
        << "// DO NOT EDIT!\n\n";

    for (const auto& rel : relative_header_paths) {
        // Use quotes so the include is relative to the location of this .cpp
        // (or to the include path that contains the generated/ tree).
        oss << "#include \"" << rel << "\"\n";
    }

    oss << "\n";
    return oss.str();
}
