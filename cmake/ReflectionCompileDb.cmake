# cmake/ReflectionCompileDb.cmake
#
# Generates a filtered compile_commands.json for reflection_gen.
# Reads the main compilation database, extracts flags from the first entry
# whose file path matches FLAGS_FROM (all entries of one target share
# identical flags), strips PCH/output/sanitizer flags, and emits entries for
# each listed header.
#
# Invoked as:
#   cmake -DCOMPILE_COMMANDS=<path/to/compile_commands.json>
#         -DHEADERS_FILE=<file-listing-absolute-header-paths>
#         -DOUTPUT=<output-dir/compile_commands.json>
#         [-DFLAGS_FROM=<regex, default ncore/src/>]
#         -P cmake/ReflectionCompileDb.cmake

if(NOT EXISTS "${COMPILE_COMMANDS}")
    message(FATAL_ERROR "compile_commands.json not found: ${COMPILE_COMMANDS}")
endif()

if(NOT EXISTS "${HEADERS_FILE}")
    message(FATAL_ERROR "Headers file not found: ${HEADERS_FILE}")
endif()

# --- Read and parse the compilation database ---

file(READ "${COMPILE_COMMANDS}" db_json)
string(JSON db_length LENGTH "${db_json}")

if(db_length EQUAL 0)
    message(FATAL_ERROR "compile_commands.json is empty")
endif()

# --- Find the first entry matching FLAGS_FROM (default: ncore/src/) ---

if(NOT DEFINED FLAGS_FROM OR FLAGS_FROM STREQUAL "")
    set(FLAGS_FROM "ncore/src/")
endif()

set(found FALSE)
math(EXPR last "${db_length} - 1")
foreach(i RANGE 0 ${last})
    string(JSON entry GET "${db_json}" ${i})
    string(JSON file_path GET "${entry}" file)
    if(file_path MATCHES "${FLAGS_FROM}")
        string(JSON directory GET "${entry}" directory)
        string(JSON command GET "${entry}" command)
        set(found TRUE)
        break()
    endif()
endforeach()

if(NOT found)
    message(FATAL_ERROR "No entries matching '${FLAGS_FROM}' found in compile_commands.json")
endif()

# --- Parse the command string into individual arguments ---

separate_arguments(args NATIVE_COMMAND "${command}")

# --- Filter: keep only include paths, defines, standard, target ---

set(filtered_args)
foreach(arg IN LISTS args)
    # Stop at -- separator (source files follow)
    if(arg STREQUAL "--")
        break()
    endif()

    # PCH flags — all have value attached (e.g. /Yu"path"), skip entirely
    if(arg MATCHES "^/Yu" OR arg MATCHES "^/Yc"
       OR arg MATCHES "^/Fp" OR arg MATCHES "^/FI")
        continue()
    endif()

    # Output flags
    if(arg MATCHES "^/Fo" OR arg MATCHES "^/Fd" OR arg STREQUAL "-c")
        continue()
    endif()

    # Cosmetic / tooling flags irrelevant to header parsing
    if(arg STREQUAL "-nologo"
       OR arg MATCHES "^-fsanitize"
       OR arg MATCHES "^-Werror" OR arg STREQUAL "/WX"
       OR arg MATCHES "^-fcolor" OR arg MATCHES "^-fdiagnostics-absolute")
        continue()
    endif()

    list(APPEND filtered_args "${arg}")
endforeach()

# Headers are parsed as the main TU by ClangTool — suppress diagnostics
# that only fire in that mode (pragma once in main file, macros/consts
# defined for consumers, exit-time singletons already handled in source).
list(APPEND filtered_args
     "-Wno-pragma-once-outside-header"
     "-Wno-unused-macros"
     "-Wno-unused-const-variable"
     "-Wno-exit-time-destructors")

# --- Read header list ---

file(STRINGS "${HEADERS_FILE}" headers_list)

# --- Build cleaned command string (escaped for JSON) ---

string(JOIN " " cmd_str ${filtered_args})
string(REPLACE "\\" "\\\\" cmd_str "${cmd_str}")
string(REPLACE "\"" "\\\"" cmd_str "${cmd_str}")

# Escape directory for JSON
string(REPLACE "\\" "\\\\" dir_escaped "${directory}")
string(REPLACE "\"" "\\\"" dir_escaped "${dir_escaped}")

# --- Generate JSON entries for each header ---

set(json_entries "")
foreach(header IN LISTS headers_list)
    if(NOT header OR header STREQUAL "")
        continue()
    endif()
    string(REPLACE "\\" "\\\\" hdr "${header}")
    string(REPLACE "\"" "\\\"" hdr "${hdr}")
    # Append the header path as the source file argument — ClangTool
    # requires a source file in the command or it gets "no input files".
    string(APPEND json_entries
        "  {\"directory\":\"${dir_escaped}\",\"command\":\"${cmd_str} ${hdr}\",\"file\":\"${hdr}\"},\n")
endforeach()

# Remove trailing comma
string(REGEX REPLACE ",\n$" "\n" json_entries "${json_entries}")

# --- Write output ---

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${OUTPUT}" "[\n${json_entries}]\n")

list(LENGTH headers_list count)
message(STATUS "Generated reflection compilation database: ${count} headers")
