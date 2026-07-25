target_sources(
  ncore
  PRIVATE "application.cpp"
          "core/memory.cpp"
          "core/object.cpp"
          "core/structures.cpp"
          "core/types.cpp"
          "backends/box2d/box2d_physics_impl.h"
          "backends/box2d/box2d_physics_impl.cpp"
          "backends/diligent/diligent_allocator.h"
          "backends/diligent/rhi_diligent.h"
          "backends/diligent/rhi_diligent.cpp"
          "backends/diligent/diligent_type_helpers.h"
          "backends/diligent/diligent_type_helpers.cpp"
          "backends/flecs/flecs_helpers.h"
		  "backends/imgui/imgui_utils.h"
          "backends/sdl/sdl_type_helpers.h"
          "backends/sdl/sdl_render_device.h"
          "backends/sdl/sdl_audio_loader.h"
          "backends/sdl/sdl_audio_loader.cpp"
          "backends/sdl/sdl_image_loader.h"
          "backends/sdl/sdl_image_loader.cpp"
          "modules/audio/audio_module.cpp"
          "modules/events/event_bus.cpp"
          "modules/input/input_module.cpp"
          "modules/io/importers/material_template_importer.h"
          "modules/io/importers/material_template_importer.cpp"
          "modules/io/importers/slang_importer.h"
          "modules/io/importers/slang_importer.cpp"
          "modules/io/resource_manager.cpp"
          "modules/io/shader_compiler.h"
          "modules/io/shader_compiler.cpp"
          "modules/video/renderer/geometry.cpp"
          "modules/video/renderer/renderer_storage.cpp"
          "modules/video/renderer/world_renderer.cpp"
          "modules/video/render_module.cpp"
          "modules/video/window_module.cpp"
          "resources/audio_clip.cpp"
          "resources/resource.cpp"
          "resources/image.cpp"
          "resources/material.cpp"
          "resources/mesh.cpp"
          "resources/shader.cpp"
          "runtime/audio/ecs_audio.h"
          "runtime/debug/ecs_debug.h"
          "runtime/debug/ecs_debug.cpp"
          "runtime/input/ecs_inputs.h"
          "runtime/input/ecs_inputs.cpp"
          "runtime/video/ecs_gui_feature.h"
          "runtime/video/ecs_gui_feature.cpp"
          "runtime/video/ecs_window_feature.h"
          "runtime/video/ecs_window_feature.cpp"
          "runtime/video/ecs_render_feature.h"
          "runtime/video/ecs_render_feature.cpp"
          "runtime/physics/ecs_physics.h"
          "runtime/ecs_base_features.cpp"
          "runtime/ecs_world.cpp"
          "runtime/ecs_system.cpp"
          "runtime/ecs_query.cpp"
          "runtime/ecs_entity_builder.cpp"
          "runtime/register_types.h"
          "runtime/register_types.cpp"
          "runtime/job_scheduler.h"
          "scene/scene.cpp"
          "scene/node.cpp"
          "utils/logger/log_api.cpp"
          "utils/logger/log_level.h"
          "utils/logger/log_channel.h"
          "utils/logger/log_channel.cpp"
          "utils/logger/logger.h"
          "utils/logger/logger.cpp"
          "utils/logger/sink.h"
          "utils/logger/sink.cpp"
          "utils/logger/assert_handler.h"
          "utils/logger/assert_handler.cpp"
          "utils/config.cpp"
          "utils/four_cc.cpp")

target_include_directories(ncore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# target_compile_definitions(ncore PRIVATE
# NC_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders" )
#
# set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders) set(SHADER_OUT
# ${CMAKE_CURRENT_BINARY_DIR}/generated/shaders)
#
# find_program( SLANGC NAMES slangc PATHS "$ENV{VULKAN_SDK}/Bin" NO_DEFAULT_PATH
# REQUIRED)
#
# set(BUILTIN_SHADERS ${SHADER_DIR}/sprite.vert.slang
# ${SHADER_DIR}/sprite.frag.slang ${SHADER_DIR}/sprite_gamma.frag.slang)
#
# foreach(SOURCE ${BUILTIN_SHADERS}) get_filename_component(FILE_NAME ${SOURCE}
# NAME_WLE) string(REGEX REPLACE "\\." "_" SAFE_NAME "${FILE_NAME}")
# string(TOUPPER "${SAFE_NAME}" VAR_NAME) set(C_OUT
# ${SHADER_OUT}/${FILE_NAME}.h)
#
# add_custom_command( OUTPUT ${C_OUT} COMMAND ${SLANGC} ${SOURCE} -target spirv
# -matrix-layout-column-major -source-embed-style u32 -source-embed-name
# ${VAR_NAME}_SPIRV -o ${C_OUT} DEPENDS ${SOURCE} ${SLANGC} COMMENT "Compiling
# built-in shader: ${FILE_NAME}") list(APPEND C_HEADERS ${C_OUT}) endforeach()
#
# set(COMBINE_OUT ${SHADER_OUT}/combined.h) string(REPLACE ";" " " C_HEADERS_STR
# "${C_HEADERS}") add_custom_command( OUTPUT ${COMBINE_OUT} COMMAND
# ${CMAKE_COMMAND} -DC_HEADERS="${C_HEADERS_STR}" -DOUTPUT="${COMBINE_OUT}" -P
# ${CMAKE_CURRENT_SOURCE_DIR}/assets/cmake/CombineShaders.cmake DEPENDS
# ${C_HEADERS} COMMENT "Combining shader headers")
#
# add_custom_target(CompileShaders ALL DEPENDS ${COMBINE_OUT})
#
# target_include_directories(ncore PRIVATE
# ${CMAKE_CURRENT_BINARY_DIR}/generated) add_dependencies(ncore CompileShaders)
