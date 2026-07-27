workspace "Archimedies"
    configurations { "Debug", "Release" }
    platforms { "x64" }

    filter "platforms:x64"
        architecture "x86_64"

    targetdir ("bin/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")
    objdir ("bin-int/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")

project "zlib"
    location "Engine/Vendor/zlib"
    kind "StaticLib"
    language "C"
    staticruntime "off"
    system "Windows"
    warnings "off"
    
    files { "Engine/Vendor/zlib/*.h", "Engine/Vendor/zlib/*.c" }
    
    filter "files:Engine/Vendor/zlib/example.c"
        buildaction "None"
    filter "files:Engine/Vendor/zlib/minigzip.c"
        buildaction "None"
    filter {}

    includedirs { "Engine/Vendor/zlib" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        runtime "Release"
        optimize "On"

project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"
    system "Windows"
    linkoptions { "/ignore:4006" }

    pchheader "archpch.h"
    pchsource "Engine/src/archpch.cpp"

    files 
    { 
        "Engine/src/**.h", 
        "Engine/src/**.cpp",
        "Engine/Vendor/imgui/**.cpp",
        "Engine/Vendor/imgui/backends/**.cpp"
    }

    filter "files:Engine/Vendor/**.cpp"
        enablepch ("Off")
    filter {}

    includedirs 
    { 
        "Engine/src",
        "Engine/Vendor",
        "Engine/Vendor/imgui",
        "Engine/Vendor/imgui/backends",
        "Engine/Vendor/zlib",
        "Engine/Vendor/json/include",
		"Engine/Vendor/gltf" 
	}

    links 
    { 
        "d3d11.lib", "d3d12.lib", "dxgi.lib", "d3dcompiler.lib",
        "zlib",
    }

    defines { "YAML_CPP_STATIC_LIB" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    system "Windows"

    files { "Sandbox/src/**.h", "Sandbox/src/**.cpp" }

    includedirs 
    { 
        "Engine/src",
        "Engine/Vendor",
        "Engine/Vendor/imgui",
        "Engine/Vendor/imgui/backends",
        "Engine/Vendor/zlib",
        "Engine/Vendor/json/include",
		"Engine/Vendor/gltf" 
	}

    links { "Engine" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"