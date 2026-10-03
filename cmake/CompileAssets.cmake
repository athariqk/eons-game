# CompileAssets.cmake — offline asset pipeline step (script mode).
#
#   cmake -DASSET_COMPILER=<path to resource_compiler>
#         -DBIN_DIR=<runtime output dir, e.g. bin/Debug>
#         "-DSRC_DIRS=<semicolon list of source asset dirs>"
#         -P CompileAssets.cmake
#
# 1. Stages raw sources into ${BIN_DIR}/assets, skipping every file the
#    pipeline compiles (COMPILE_EXTS) and every file no compiler handles
#    (OMIT_EXTS). Raw originals therefore never reach the runtime tree —
#    only their .bin does. This includes .slang: shaders resolve their
#    #include targets through --search-paths pointing straight at SRC_DIRS.
#      - excluded from compiling: shaders/ncore/** (include-only modules, no
#        entry points) and shaders/triangle.slang (dead, includes a
#        nonexistent header).
# 2. Deletes anything left in ${BIN_DIR}/assets whose extension is in
#    SKIP_STAGE — legacy leftovers, or raw files staged by other build steps.
# 3. Compiles every eligible source to ${BIN_DIR}/assets/<rel>.bin, reading
#    inputs straight from SRC_DIRS (never from the staged tree) and passing
#    --search-paths so Slang finds #include targets among the raw sources.
# 4. Deletes orphaned .bin files whose source no longer exists.
#
# Recompile triggers: output missing, source newer than output, or (for
# .slang) any raw shader source in SRC_DIRS newer than output (conservative
# include-dependency sweep — no per-file dep graph, so any include edit
# recompiles every shader in that run).
#
# SRC_DIRS are applied in order; when two dirs hold the same relative path the
# later one wins for both staging and compilation. Slang resolves #include
# against earlier search paths first, so SRC_DIRS is reversed when building
# --search-paths, keeping "later dir wins" consistent between staging and
# shader compilation.
#
# --search-paths values are joined with '|': it cannot occur in a Windows
# path, whereas CMake treats ';' as a list separator. All entries are absolute
# (derived from SRC_DIRS), so nothing here depends on the working directory.

cmake_minimum_required(VERSION 3.27)

if(NOT DEFINED ASSET_COMPILER OR NOT DEFINED BIN_DIR OR NOT DEFINED SRC_DIRS)
    message(FATAL_ERROR "CompileAssets: ASSET_COMPILER, BIN_DIR and SRC_DIRS are required")
endif()
if(NOT EXISTS "${ASSET_COMPILER}")
    message(FATAL_ERROR "CompileAssets: compiler not found: ${ASSET_COMPILER}")
endif()

set(ASSETS_DIR "${BIN_DIR}/assets")

set(COMPILE_EXTS .slang .png .jpg .jpeg .wav .ttf .otf)
set(OMIT_EXTS .webp) # no compiler registered for these; deliberately dropped

# Never staged: everything the pipeline compiles, plus everything with no
# compiler at all. Only .bin outputs are ever materialised in the tree.
set(SKIP_STAGE ${COMPILE_EXTS})
list(APPEND SKIP_STAGE ${OMIT_EXTS})

# --- #include search roots for Slang ---------------------------------------
# Reversed so that, with earlier paths winning, "later SRC_DIR wins" holds
# here exactly as it does when staging. Each source dir contributes its
# shaders/ subtree plus the two subfolders the engine's material modules are
# resolved from.
set(SHADER_SEARCH_PATHS "")
set(reversed_src_dirs ${SRC_DIRS})
list(REVERSE reversed_src_dirs)
foreach(src_dir IN LISTS reversed_src_dirs)
    foreach(sub shaders shaders/core shaders/materials)
        if(EXISTS "${src_dir}/${sub}")
            if(SHADER_SEARCH_PATHS STREQUAL "")
                set(SHADER_SEARCH_PATHS "${src_dir}/${sub}")
            else()
                set(SHADER_SEARCH_PATHS "${SHADER_SEARCH_PATHS}|${src_dir}/${sub}")
            endif()
        endif()
    endforeach()
endforeach()
if(SHADER_SEARCH_PATHS STREQUAL "")
    message(FATAL_ERROR "CompileAssets: no shaders/ directory found under SRC_DIRS — shader #include resolution would have no roots")
endif()

# --- 1. Stage raw sources (filtered) ------------------------------------------

foreach(src_dir IN LISTS SRC_DIRS)
    if(NOT IS_DIRECTORY "${src_dir}")
        message(FATAL_ERROR "CompileAssets: source asset dir not found: ${src_dir}")
    endif()

    file(GLOB_RECURSE rels LIST_DIRECTORIES false RELATIVE "${src_dir}" "${src_dir}/*")
    foreach(rel IN LISTS rels)
        get_filename_component(ext "${rel}" EXT)
        string(TOLOWER "${ext}" ext)

        list(FIND SKIP_STAGE "${ext}" skip_idx)
        if(NOT skip_idx EQUAL -1)
            continue() # raw original of a compiled asset — never staged
        endif()

        set(dst "${ASSETS_DIR}/${rel}")
        get_filename_component(dst_dir "${dst}" DIRECTORY)
        file(MAKE_DIRECTORY "${dst_dir}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${src_dir}/${rel}" "${dst}"
            RESULT_VARIABLE copy_rc)
        if(NOT copy_rc EQUAL 0)
            message(FATAL_ERROR "CompileAssets: failed to copy '${src_dir}/${rel}'")
        endif()
    endforeach()
endforeach()

# --- 2. Drop raw originals ----------------------------------------------------

file(GLOB_RECURSE staged LIST_DIRECTORIES false RELATIVE "${ASSETS_DIR}" "${ASSETS_DIR}/*")
foreach(rel IN LISTS staged)
    get_filename_component(ext "${rel}" EXT)
    string(TOLOWER "${ext}" ext)

    list(FIND SKIP_STAGE "${ext}" skip_idx)
    if(skip_idx EQUAL -1)
        continue()
    endif()

    message(STATUS "CompileAssets: dropping raw original ${rel}")
    file(REMOVE "${ASSETS_DIR}/${rel}")
endforeach()

# --- 3. Compile from the source tree ------------------------------------------

# Conservative include-dependency sweep: every raw shader in every source dir.
# Nothing .slang-shaped remains in ${ASSETS_DIR} to glob, so this reads SRC_DIRS.
set(SHADER_SOURCES "")
foreach(src_dir IN LISTS SRC_DIRS)
    file(GLOB_RECURSE dir_shaders LIST_DIRECTORIES false "${src_dir}/shaders/*.slang")
    list(APPEND SHADER_SOURCES ${dir_shaders})
endforeach()

set(HAD_ERRORS 0)
set(COMPILED 0)

foreach(src_dir IN LISTS SRC_DIRS)
    file(GLOB_RECURSE src_rels LIST_DIRECTORIES false RELATIVE "${src_dir}" "${src_dir}/*")
    foreach(rel IN LISTS src_rels)
        get_filename_component(ext "${rel}" EXT)
        string(TOLOWER "${ext}" ext)

        list(FIND COMPILE_EXTS "${ext}" ext_idx)
        if(ext_idx EQUAL -1)
            continue() # not a compilable format
        endif()

        set(input "${src_dir}/${rel}")
        set(output "${ASSETS_DIR}/${rel}.bin")

        set(need_compile FALSE)
        if(NOT EXISTS "${output}")
            set(need_compile TRUE)
        else()
            file(TIMESTAMP "${input}" input_ts)
            file(TIMESTAMP "${output}" output_ts)
            if(input_ts STRGREATER output_ts)
                set(need_compile TRUE)
            elseif(ext STREQUAL ".slang")
                foreach(shader_src IN LISTS SHADER_SOURCES)
                    file(TIMESTAMP "${shader_src}" shader_ts)
                    if(shader_ts STRGREATER output_ts)
                        set(need_compile TRUE)
                        break()
                    endif()
                endforeach()
            endif()
        endif()

        if(NOT need_compile)
            continue()
        endif()

        execute_process(
            COMMAND "${ASSET_COMPILER}" --input "${input}" --output "${output}"
                    --search-paths "${SHADER_SEARCH_PATHS}"
            WORKING_DIRECTORY "${BIN_DIR}"
            RESULT_VARIABLE compile_rc
            OUTPUT_VARIABLE compile_out
            ERROR_VARIABLE compile_err)
        if(NOT compile_rc EQUAL 0)
            message("CompileAssets: FAILED ${rel}\n${compile_out}${compile_err}")
            math(EXPR HAD_ERRORS "${HAD_ERRORS} + 1")
        else()
            message(STATUS "CompileAssets: ${rel} -> ${rel}.bin")
            math(EXPR COMPILED "${COMPILED} + 1")
        endif()
    endforeach()
endforeach()

# --- 4. Orphaned .bin cleanup -------------------------------------------------

file(GLOB_RECURSE bins LIST_DIRECTORIES false RELATIVE "${ASSETS_DIR}" "${ASSETS_DIR}/*.bin")
foreach(rel IN LISTS bins)
    string(REGEX REPLACE "\\.bin$" "" src_rel "${rel}")

    set(found FALSE)
    foreach(src_dir IN LISTS SRC_DIRS)
        if(EXISTS "${src_dir}/${src_rel}")
            set(found TRUE)
            break()
        endif()
    endforeach()

    if(NOT found)
        message(STATUS "CompileAssets: removing orphan ${rel}")
        file(REMOVE "${ASSETS_DIR}/${rel}")
    endif()
endforeach()

if(HAD_ERRORS GREATER 0)
    message(FATAL_ERROR "CompileAssets: ${HAD_ERRORS} asset(s) failed to compile")
endif()
