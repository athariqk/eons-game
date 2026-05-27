# ImGui
add_library(ImGui STATIC
        imgui/imgui.h
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_internal.h
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
        imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(ImGui PUBLIC imgui)
target_compile_features(ImGui PRIVATE cxx_std_17)
set_target_properties(ImGui PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

# Box2D
add_subdirectory(box2d)

# spdlog
add_subdirectory(spdlog)
