project "shinyProfiler"
	kind "StaticLib"
	language "C"

	includedirs
	{
		"./include"
	}

	files
	{
		"**.h",
		"**.c"
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		
usage "shinyProfiler"
	includedirs { 
		"./include"
	}
	links "shinyProfiler"
