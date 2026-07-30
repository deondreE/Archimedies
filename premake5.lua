workspace "Archimedies"
    configurations { "Debug", "Release" }
    platforms { "x64" }

    filter "platforms:x64"
        architecture "x86_64"

    targetdir ("bin/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")
    objdir ("bin-int/%{cfg.buildcfg}-%{cfg.platform}/%{prj.name}")

-- Global paths
local dotnet_path = "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/10.0.10/runtimes/win-x64/native"
local VK_SDK = "C:/VulkanSDK/1.4.350.0"

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
		
    filter "kind:not StaticLib"
        defines { 'SOLUTION_DIR=LR"(' .. _MAIN_SCRIPT_DIR .. ')"' }

project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"
    system "Windows"
    multiprocessorcompile "On"
    linkoptions { "/ignore:4006" }
	
	buildoptions { "/Zc:preprocessor" }

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
    filter "files:Engine/src/Asset.cpp"
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
        "Engine/Vendor/gltf",
        "Engine/Vendor/cora/Engine.Native",
        (VK_SDK .. "/Include")
    }

    libdirs { (VK_SDK .. "/Lib") }
    links 
    { 
        "d3d11.lib", 
        "d3d12.lib", 
        "dxgi.lib", 
        "d3dcompiler.lib",
        "zlib", 
        "xaudio2.lib", 
        "xapobase.lib", 
        "xinput.lib",
        "vulkan-1.lib",
		"volk.lib",
		-- "slang.lib", -- Eval
		"Engine.Native"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"
    filter "system:windows"
        systemversion "latest" 
        defines { "WIN32_LEAN_AND_MEAN", "_WIN32_WINNT=0x0A00" }

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    system "Windows"

    files { 
        "Sandbox/src/**.h", 
        "Sandbox/src/**.cpp",
        "Engine/src/**.rc" 
    }

    includedirs 
    { 
        "Engine/src",
        "Engine/Vendor",
        "Engine/Vendor/imgui",
        "Engine/Vendor/zlib",
		"Engine/Vendor/json/include",
        "Engine/Vendor/cora/Engine.Native",
        dotnet_path,
		(VK_SDK .. "/Include")
    }
	
    libdirs { dotnet_path }
    links { "Engine", "nethost", "Engine.Native" }

    -- Required to run .NET hosting
    postbuildcommands {
        "{COPY} \"" .. dotnet_path .. "/nethost.dll\" \"%{cfg.targetdir}\""
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"

externalproject "Engine.Managed"
    location "Engine/Vendor/cora/Engine.Managed"
    kind "SharedLib"
    language "C#"
   
project "Engine.Native"
    location "Engine/Vendor/cora/Engine.Native"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    system "Windows"
	
    files { 
        "Engine/Vendor/cora/Engine.Native/**.h", 
        "Engine/Vendor/cora/Engine.Native/**.cpp"
    }
	
    includedirs { 
        "Engine/Vendor/cora/Engine.Native",
        dotnet_path 
    }

    libdirs { dotnet_path }
    links { "nethost" }
	
    dependson { "Engine.Managed" }
	
    postbuildcommands {
        "{COPY} \"$(TargetPath)\" \"%{wks.location}/bin/%{cfg.buildcfg}-%{cfg.platform}/Sandbox/\"",
        "{COPY} \"" .. dotnet_path .. "/nethost.dll\" \"%{cfg.targetdir}\""
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"