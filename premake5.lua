include "Dependencies.lua"

workspace "Archimedies"
	architecture "x86_64"
	startproject "Core"

	configurations {
		"Debug",
		"Release"
	}

	flags {
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
  include "core/vendor/glad"
  include "core/vendor/GLFW"
group ""

group "Core"
  include "Core"
group ""