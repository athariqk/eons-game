#pragma once

#include <string>
#include <vector>

#include "ast_visitor.h" // for ReflectionResult

// Generates the content of a single reflection header for one source file.
// The header contains the static-initializer registration code and is intended
// to be #included from exactly one translation unit (the generated impl file).
std::string generate_reflection_header(
    const ReflectionResult& result, const std::string& source_path, const std::string& guard_macro
);

// Generates the content of the single implementation .cpp that #includes every
// generated reflection header.
std::string generate_reflection_impl( const std::vector<std::string>& relative_header_paths );
