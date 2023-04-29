
add_rules("mode.debug", "mode.release")

target("spv2hpp")
    set_kind("binary")
    add_files("spv2hpp.c")
    set_languages("c11")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    set_policy('build.across_targets_in_parallel', false)

    after_build(function (target)
        -- os.cp("spv2hpp.exe", path.join("$(environment)", "bin"))
        -- import("package.tools.xmake").install(package, {plat = os.host(), arch = os.arch()})
    end)
