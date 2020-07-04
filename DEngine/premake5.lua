-- DECore
project "DECore"
	location "Build"
	kind "StaticLib"
	defines {"_CRT_SECURE_NO_WARNINGS"}
	includedirs { "Source/", "Source/DECore/" }
	buildoptions { "/sdl" } -- code generation for uninitialized variable

	files 
	{ 
		"Source/DECore/**.h", 
		"Source/DECore/**.cpp",
		"Source/DECore/**.natvis",
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		targetdir "Bin/Debug"
		objdir "Intermediate/Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		targetdir "Bin/Release"
		objdir "Intermediate/Release"
		
-- DERendering
project "DERendering"
	location "Build"
	kind "StaticLib"
	defines {"_CRT_SECURE_NO_WARNINGS"}
	includedirs { "Source/", "Source/DERendering/" }
	buildoptions { "/sdl" } -- code generation for uninitialized variable
	links { "DECore", "D3d12", "DXGI" }
	
	files 
	{ 
		"Source/DERendering/**.h", 
		"Source/DERendering/**.cpp",
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		targetdir "Bin/Debug"
		objdir "Intermediate/Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		targetdir "Bin/Release"
		objdir "Intermediate/Release"

-- DEGame
project "DEGame"
	location "Build"
	kind "StaticLib"
	defines {"_CRT_SECURE_NO_WARNINGS"}
	includedirs {  "Source/", "Source/DEGame/" }
	buildoptions { "/sdl" } -- code generation for uninitialized variable
	links { "DECore", "DERendering" }

	files 
	{ 
		"Source/DEGame/**.h", 
		"Source/DEGame/**.cpp",
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		targetdir "Bin/Debug"
		objdir "Intermediate/Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		targetdir "Bin/Release"
		objdir "Intermediate/Release"