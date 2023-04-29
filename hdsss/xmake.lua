add_rules("mode.debug", "mode.release")

includes("spv2hpp")

-- glslc toolchain
-- reference: https://github.com/xmake-io/xmake/blob/63e4d08d20a935acf47ac86beb587e9261b78506/xmake/rules/utils/glsl2spv/xmake.lua
rule("glsl2hpp")
    set_extensions(".vert", ".tesc", ".tese", ".geom", ".comp", ".frag", ".comp", ".mesh",
     ".task", ".rgen", ".rint", ".rahit", ".rchit", ".rmiss", ".rcall", ".glsl")
    before_buildcmd_file(function (target, batchcmds, sourcefile_glsl, opt)
        import("lib.detect.find_tool")
        import("lib.detect.find_program")

        -- get glsl
        local glslc
        glslc = find_tool("glslc")
        assert(glslc, "glslc not found!")

        -- glsl to spv
        local outputdir = target:extraconf("rules", "glsl2hpp", "outputdir")
        assert(outputdir, "outputdir not set!")
        local spvfilepath = path.join(outputdir, path.filename(sourcefile_glsl) .. ".spv")
        batchcmds:show_progress(opt.progress, "${color.build.object}generating.glsl2hpp %s", sourcefile_glsl)
        batchcmds:mkdir(outputdir)
        batchcmds:vrunv(glslc.program, {"-fauto-map-locations", "-fauto-bind-uniforms", "-fauto-combined-image-sampler",
         "--target-env=opengl", "-o", path(spvfilepath), path(sourcefile_glsl)})

        -- do bin2c
        local outputfile = spvfilepath:gsub(".spv$", "") .. ".hpp"
        -- get header file
        local headerdir = outputdir
        local headerfile = path.join(headerdir, path.filename(outputfile))
        target:add("includedirs", headerdir)

        -- using black magic here, TODO: fix it
        -- spv2hpp = find_program("spv2hpp", {pathes = {target:targetdir()}})
        -- assert(spv2hpp, "spv2hpp not found!")
    
        -- add commands
        local argv = {"r", "spv2hpp", path.absolute(spvfilepath), path.absolute(headerfile)}
        batchcmds:vrunv(os.programfile(), argv, {envs = {XMAKE_SKIP_HISTORY = "y"}})

        os.rm(path.absolute(spvfilepath))
        -- add deps
        batchcmds:add_depfiles(sourcefile_glsl)
        batchcmds:set_depmtime(os.mtime(outputfile))
        batchcmds:set_depcache(target:dependfile(outputfile))
    end)
rule_end()

target("HDSSS")
    set_kind("binary")

    add_deps("loo", "spv2hpp")

    add_includedirs("include")
    set_languages("c11", "cxx20")
    set_rules("glsl2hpp", {outputdir = "hdsss/include/shaders"})
    add_files("shaders/*.*", "src/*.cpp")
    

    add_defines("_CRT_SECURE_NO_WARNINGS")
    set_policy("build.warning", true)
    set_warnings("all", "extra")


    -- solve msvc unfriendly to unicode and utf8
    add_defines( "UNICODE", "_UNICODE")
    add_cxflags("/execution-charset:utf-8", "/source-charset:utf-8", {tools = {"clang_cl", "cl"}})
