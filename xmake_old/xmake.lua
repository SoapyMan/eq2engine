set_xmakever("2.1.0")
set_project("Equilibrium 2")

-- default configurations
add_rules("mode.debug", "mode.release")

set_languages("c++14", "c98")
set_arch("x64") -- TODO: x86 builds too
set_targetdir("bin/$(arch)")

-- some packages
add_requires("zlib", "libjpeg", "libogg", "libvorbis", "libsdl")

-----------------------------------------------------

-- default configuration capabilities
Groups = {
    core = "Framework",
    
    engine1 = "Equilibrium 1 port",
    component1 = "Equilibrium 1 port/Components",

    engine2 = "Equilibrium 2",
    tools = "Tools",

    game = "Game",
}

-- folders for framework, libraries and tools
Folders = {
    public =  "$(projectdir)/public/",
    matsystem1 = "$(projectdir)/materialsystem1/",
    shared_engine = "$(projectdir)/shared_engine/",
    shared_game = "$(projectdir)/shared_game/",
    dependency = "$(projectdir)/src_dependency/",
    game = "$(projectdir)/game/",
}

function add_sources_headers(...)
    local arg = {...}
    for i,v in ipairs(arg) do 
        add_files(v..".cpp")
        add_headerfiles(v..".h")
    end
end

function add_eq_deps() -- for internal use only
    add_deps("coreLib", "frameworkLib")
end

function add_eqcore_deps()
    add_eq_deps()
    add_deps("e2Core")
end

-----------------------------------------------------

if is_plat("windows") or is_plat("linux") or is_plat("android") then
    add_defines("PLAT_SDL=1")
end

function add_OpenAL()
    if is_plat("windows") then
        add_includedirs(Folders.dependency.."openal-soft/include")
        if is_arch("x64") then
            add_linkdirs(Folders.dependency.."openal-soft/libs/Win64")
        else
            add_linkdirs(Folders.dependency.."openal-soft/libs/Win32")
        end
        add_links("OpenAL32")
    else
        --print("FIXME: setup other platforms!")
    end
end

if is_plat("windows") then
    add_defines(
        "_UNICODE",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_SECURE_NO_DEPRECATE")

elseif is_plat("linux") then 

    add_syslinks("pthread")
    add_cxflags(
        "-fexceptions", 
        "-fpic")
end


-- default runtime configuration
if is_mode("debug") or is_mode("releasedbg") then

    add_defines("EQ_DEBUG")
    set_symbols("debug", "edit")

elseif is_mode("release") then

    add_ldflags("/LTCG", "/OPT:REF")
    add_cxflags("/Ot", "/GL", "/Ob2", "/Oi", "/GS-")
    add_defines("NDEBUG")

    set_optimize("fastest")
    set_symbols("debug")
end

-- extra runtime configuration (if needed)
function setup_runtime_config(crtDebug)
    if is_mode("debug") then
        set_runtimes("MD")
        --[[set_runtimes("MDd")

        -- allow CRT debugging features on windows
        if is_plat("windows") and crtDebug then
            add_defines("CRT_DEBUG_ENABLED")
        end]]
    else 
        set_runtimes("MD")
		set_optimize("fastest")
    end
end

-- add dependency packages that not in Xmake repo
includes("src_dependency/xmake.lua")

function add_wxwidgets()
    if is_plat("windows") then
        add_includedirs(Folders.dependency.."wxWidgets/include")
        add_includedirs(Folders.dependency.."wxWidgets/include/msvc")

        if is_arch("x64") then
            add_linkdirs(Folders.dependency.."wxWidgets/lib/vc_x64_lib")
        else
            add_linkdirs(Folders.dependency.."wxWidgets/lib/vc_lib")
        end
    else
        --print("FIXME: setup other platforms!")
    end
end

----------------------------------------------
-- Core library

target("coreLib")
    set_group(Groups.core)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.public.. "/core/*.cpp")
    add_headerfiles(Folders.public.. "/core/**.h")
    add_includedirs(Folders.public, { public = true })

----------------------------------------------
-- Framework library

target("frameworkLib")
    set_group(Groups.core)
    set_kind("static")
    setup_runtime_config()
    add_files(
        Folders.public.. "/utils/*.cpp",
        Folders.public.. "/math/*.cpp",
        Folders.public.. "/imaging/*.cpp")
    add_headerfiles(
        Folders.public.. "/utils/*.h",
        Folders.public.. "/math/*.h", Folders.public.. "/math/*.inl",
        Folders.public.. "/imaging/*.h")
    add_packages("zlib", "libjpeg")
    add_includedirs(Folders.public, { public = true })
    add_syslinks("User32")

----------------------------------------------
-- e2Core

target("e2Core")
    set_group(Groups.core)
    set_kind("shared")
    setup_runtime_config()
    add_files(
        "core/**.cpp",
        "core/minizip/*.c")
    add_headerfiles(
        "core/**.h",
        Folders.public.. "/core/**.h")
    add_defines(
        "CORE_INTERFACE_EXPORT",
        "COREDLL_EXPORT")

    if is_plat("android") then 
        add_files("core/android_libc/*.c") 
        add_headerfiles("core/android_libc/*.h")
    end

    add_packages("zlib")
    add_eq_deps()

    if is_os("windows") then
        add_syslinks("User32", "DbgHelp", "Advapi32")
    end

----------------------------------------------
-- Equilibruim 2

target("sysLib")
    set_group(Groups.engine2)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "sys/**.cpp")
    add_headerfiles(Folders.shared_engine.. "sys/**.h")
    add_files(Folders.shared_engine.. "input/**.cpp")
    add_headerfiles(Folders.shared_engine.. "input/**.h")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()
    add_packages("libsdl")

target("equiLib")
    set_group(Groups.engine2)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "equi/**.cpp")
    add_headerfiles(Folders.shared_engine.. "equi/**.h")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()

target("networkLib")
    set_group(Groups.engine2)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "network/**.cpp")
    add_headerfiles(Folders.shared_engine.. "network/**.h")
    add_includedirs(Folders.shared_engine, { public = true })
    add_packages("zlib")
    add_syslinks("wsock32")
    add_eq_deps()

target("soundSystemLib")
    set_group(Groups.engine2)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "audio/**.cpp")
    add_headerfiles(Folders.shared_engine.. "audio/**.h")
    add_headerfiles(Folders.public.. "audio/**.h")
    add_includedirs(Folders.public, { public = true })
    add_eq_deps()
    add_packages("libvorbis", "libogg")
    add_OpenAL()

target("physicsLib")
    set_group(Groups.engine2)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "physics/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_deps("bullet2")
    add_eq_deps()

includes("xmake-eq1.lua")
includes("game/DriverSyndicate/xmake.lua")

-- only build tools for big machines
if is_plat("windows", "linux") then
    includes("utils/xmake-utils.lua")
end

