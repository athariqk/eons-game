# FreeType (for ImGui font rasterization)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(freetype SYSTEM)

# ImGui
add_library(
  ImGui STATIC
  imgui/imgui.h
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_internal.h
  imgui/imstb_rectpack.h
  imgui/imstb_textedit.h
  imgui/imstb_truetype.h
  imgui/misc/cpp/imgui_stdlib.cpp
  imgui/misc/freetype/imgui_freetype.cpp
  $<$<CONFIG:Debug>:imgui/imgui_demo.cpp>)
target_include_directories(ImGui SYSTEM PUBLIC imgui)
target_compile_features(ImGui PRIVATE cxx_std_17)
set_target_properties(ImGui PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ImGui PUBLIC freetype)
target_compile_definitions(ImGui PUBLIC IMGUI_ENABLE_FREETYPE)

set(IMGUIZMO_BUILD_EXAMPLE
    OFF
    CACHE BOOL "" FORCE)

add_subdirectory(imguizmo SYSTEM)

target_link_libraries(imguizmo PUBLIC ImGui)
target_link_libraries(ncore_editor PRIVATE ImGui imguizmo freetype)
