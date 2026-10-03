#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace clang::tooling {
class FrontendActionFactory;
}

struct ReflectedField {
    std::string name;
    std::string type_name;          // full type string for display
    std::string element_type_name;  // element type after stripping ptr/array
    uint64_t type_id_hash;          // FNV-1a of element_type_name
    size_t field_size;              // sizeof field type
    size_t field_offset;            // byte offset in parent
    uint32_t pointer_count;
    uint32_t array_length;
    bool is_cstring;
};

struct ReflectedRecord {
    std::string name;
    std::string qualified_name;
    std::string parent_name;
    bool is_class;
    bool is_ecs_component;  // inherits nc::detail::EcsComponentBase<T>
    bool has_nstruct_v;     // has get_class_info() from NSTRUCT_V
    size_t record_size;
    size_t record_alignment;
    std::vector<ReflectedField> fields;
};

struct ReflectedDepType {
    std::string type_key;
    std::string registration_code;
};

struct ReflectionResult {
    std::vector<ReflectedRecord> records;
    std::vector<ReflectedDepType> dep_types;
};

struct ReflectionStats {
    int records_visited = 0;
    int records_with_refl_fields     = 0;
    int fields_with_refl_attr        = 0;
};

// FNV-1a hash (matches ncore's detail::fnv1a)
uint64_t compute_type_id_hash( const std::string& name );

std::unique_ptr<clang::tooling::FrontendActionFactory>
create_reflection_action_factory( ReflectionResult& out, ReflectionStats& stats );
