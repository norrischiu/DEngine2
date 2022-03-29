-- DExporter
project "DExporter"
	location "Build"
	configurations { "Debug", "Release" }
	kind "ConsoleApp"
	platforms { "x64" }
	systemversion "10.0.19041.0"

	defines {"_CRT_SECURE_NO_WARNINGS"}
	includedirs { "../External/Assimp/include/" }
	links { "../External/Assimp/lib/Release/assimp-vc143-mt.lib" }

	files 
	{ 
		"**.h", 
		"**.cpp",
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		targetdir "../Bin/Debug"
		objdir "Intermediate/Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		targetdir "../Bin/Release"
		objdir "Intermediate/Release"