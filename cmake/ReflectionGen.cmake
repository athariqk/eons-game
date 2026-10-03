# ReflectionGen.cmake
#
# nc_add_reflection_gen(<target>
#     HEADERS <abs-header>...
#     [FLAGS_FROM <regex>]   # compile_commands entry regex supplying flags
#                            # (default: ncore/src/)
#     [GEN_DIR <dir>]        # default: ${CMAKE_CURRENT_BINARY_DIR}/generated)
#
# Mirrors the per-header reflection generation block in ncore/CMakeLists.txt
# for other targets (eons-game, tools/editor). Each header is processed
# individually via --single-header so changing one header only re-parses that
# header through Clang. A filtered compilation database is derived from the
# first compile_commands entry whose file path matches FLAGS_FROM, so each
# target parses its headers with its own include paths and defines.

function(nc_add_reflection_gen target)
    cmake_parse_arguments(ARG "" "FLAGS_FROM;GEN_DIR" "HEADERS" ${ARGN})
    if(NOT ARG_HEADERS)
        message(FATAL_ERROR "nc_add_reflection_gen(${target}): HEADERS is required")
    endif()
    if(NOT ARG_FLAGS_FROM)
        set(ARG_FLAGS_FROM "ncore/src/")
    endif()
    if(NOT ARG_GEN_DIR)
        set(ARG_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    endif()

    set(reflection_db "${ARG_GEN_DIR}/reflection_db")
    set(headers_list "${ARG_GEN_DIR}/${target}_headers.txt")

    # Header list for the filtered compile DB
    file(GENERATE OUTPUT "${headers_list}"
         CONTENT "$<JOIN:${ARG_HEADERS},\n>")

    # Filtered compilation database
    add_custom_command(
        OUTPUT "${reflection_db}/compile_commands.json"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${reflection_db}"
        COMMAND ${CMAKE_COMMAND}
                -DCOMPILE_COMMANDS=${CMAKE_BINARY_DIR}/compile_commands.json
                -DHEADERS_FILE=${headers_list}
                -DOUTPUT=${reflection_db}/compile_commands.json
                -DFLAGS_FROM=${ARG_FLAGS_FROM}
                -P ${CMAKE_SOURCE_DIR}/cmake/ReflectionCompileDb.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/cmake/ReflectionCompileDb.cmake
        COMMENT "Generating filtered compilation database for ${target} reflection_gen"
        VERBATIM
    )

    # Per-header generation
    set(gen_headers "")
    foreach(header IN LISTS ARG_HEADERS)
        file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${header}")
        string(REGEX REPLACE "\\.h$" ".gen.h" gen_name "${rel}")
        string(REGEX REPLACE "[/\\\\]" "_" gen_name "${gen_name}")
        set(gen_header "${ARG_GEN_DIR}/${gen_name}")

        add_custom_command(
            OUTPUT "${gen_header}"
            COMMAND reflection_gen
                    --single-header=${header}
                    --output=${gen_header}
                    -p=${reflection_db}
                    --output-dir=${ARG_GEN_DIR}
            DEPENDS "${header}"
                    reflection_gen
                    "${reflection_db}/compile_commands.json"
            COMMENT "Generating reflection for ${rel}"
            VERBATIM
        )
        list(APPEND gen_headers "${gen_header}")
    endforeach()

    # Aggregate implementation unit (just #includes all .gen.h files)
    add_custom_command(
        OUTPUT "${ARG_GEN_DIR}/reflection_impl.cpp"
        COMMAND ${CMAKE_COMMAND}
                "-DOUTPUT_DIR=${ARG_GEN_DIR}"
                "-DGEN_HEADERS=${gen_headers}"
                -P ${CMAKE_SOURCE_DIR}/cmake/WriteReflectionImpl.cmake
        DEPENDS ${gen_headers}
        COMMENT "Generating reflection implementation unit for ${target}"
    )

    target_sources(${target} PRIVATE "${ARG_GEN_DIR}/reflection_impl.cpp")
    target_include_directories(${target} PRIVATE "${ARG_GEN_DIR}")
endfunction()
