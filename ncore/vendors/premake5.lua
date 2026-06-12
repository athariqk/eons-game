project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "on"
    
    targetdir ("glad/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("glad/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "glad/include/glad/glad.h",
        "glad/include/KHR/khrplatform.h",
        "glad/src/glad.c"
    }

    includedirs
    {
        "glad/include"
    }
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

project "ImGui"
	kind "StaticLib"
	language "C++"

	targetdir ("imgui/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("imgui/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"imgui/*.h",
		"imgui/*.cpp",
		"imgui/misc/cpp/imgui_stdlib.cpp",
		"imgui/misc/cpp/imgui_stdlib.h"
	}

	includedirs
	{
		"imgui"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

project "Box2D"
	kind "StaticLib"
	language "C++"

	targetdir ("box2d/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("box2d/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"box2d/include/box2d/**.h",
		"box2d/src/**.h",
		"box2d/src/**.cpp"
	}
	
	includedirs
	{
		"box2d/include",
		"box2d/src"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"