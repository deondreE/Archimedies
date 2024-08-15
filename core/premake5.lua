project "Core"
  kind "ConsoleApp"
  language "C++"
  cppdialect "C++20"
  staticruntime "off"

  targetdir ("${wks.location}/bin/" .. outputdir .. "/${prj.name}")
  objdir ("${wks.location}/bin/" .. outputdir .. "/${prj.name}")
    
  files { 
    "src/**.hpp", 
    "**.cpp" 
  }

  includedirs {
    "src",
    "%{IncludeDir.Glad}",
    "%{IncludeDir.GLFW}"
  }

  links {
    "GLFW",
    "vulkan",
    "GLFW"
  }

  filter "system:windows"
    systemversion "latest"

    defines {
    }

    links {
    }

  filter "configurations:Debug"
    defines "AR_DEBUG"
    runtime "Debug"
    symbols "on" 

    links {}

  filter "configurations:Release"
		defines "HZ_RELEASE"
		runtime "Release"
		optimize "on"

		links {
		}

