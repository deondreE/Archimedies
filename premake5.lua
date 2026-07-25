workspace "Archimedies"
    configurations { "Debug", "Release" }
    platforms { "x64" } -- Using standard 'x64' naming

    filter "platforms:x64"
        architecture "x86_64"

    -- Global output paths
    targetdir ("bin/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")
    objdir ("bin-int/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")

project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    system "Windows"
	
	linkoptions { "/ignore:4006" }

    -- PCH Configuration
    pchheader "archpch.h"
    pchsource "Engine/src/archpch.cpp"

    files 
    { 
        "Engine/src/**.h", 
        "Engine/src/**.cpp" 
    }

    includedirs 
    { 
        "Engine/src",
        "Engine/Vendor"
    }

    links { "d3d11.lib", "dxgi.lib", "d3dcompiler.lib" }

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
    cppdialect "C++20"
    system "Windows"

    files 
    { 
        "Sandbox/src/**.h", 
        "Sandbox/src/**.cpp" 
    }

    includedirs 
    { 
        "Engine/src" 
    }

    links 
    { 
        "Engine" 
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"