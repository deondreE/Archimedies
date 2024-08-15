include "Dependencies.lua"

workspace "Archimedies"
	architecture "x86_64"
	startproject "Core"

  configurations {
		"Debug",
		"Release"
  }

  solution_items {
		".editorconfig"
	}

	flags {
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
  include "vendor/premake"
  include "core/vendor/glad"
  incldue "core/vendor/GLFW"
group ""

group "Core"
  include "Core"
group ""