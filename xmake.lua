-- include subprojects
includes("lib/commonlibsse-ng")

-- set project constants
set_project("MantellaHeardLines")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

add_requires("nlohmann_json v3.12.0")
add_requires("doctest")

-- define targets
target("MantellaHeardLines")
    add_rules("commonlibsse-ng.plugin", {
        name = "MantellaHeardLines",
        author = "Mantella",
        description = "Catches the voiced lines NPCs hear, for Mantella"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
    add_packages("nlohmann_json")

-- The unit tests: the plain logic in src/logic, without CommonLib or the game. Run by GitHub Actions before the plugin is built
target("MantellaHeardLinesTests")
    set_kind("binary")
    set_default(false)
    add_files("src/logic/**.cpp", "tests/**.cpp")
    add_includedirs("src")
    add_packages("nlohmann_json", "doctest")
