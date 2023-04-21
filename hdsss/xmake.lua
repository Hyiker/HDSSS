add_rules("mode.debug", "mode.release")


target("HDSSS")
    set_kind("binary")
    add_includedirs("include")
    add_files("src/*.cpp")
    set_languages("c11", "cxx20")
    
    add_deps("loo")

    add_defines("_CRT_SECURE_NO_WARNINGS")
    set_policy("build.warning", true)
    set_warnings("all", "extra")


    -- solve msvc unfriendly to unicode and utf8
    add_defines( "UNICODE", "_UNICODE")
    add_cxflags("/execution-charset:utf-8", "/source-charset:utf-8", {tools = {"clang_cl", "cl"}})

    -- spirv shader compilation
    on_config("windows", "linux", function (target) 
        local shader_src = path.join("$(projectdir)", "hdsss", "shaders")
        local shader_target = path.join("$(projectdir)", "hdsss", "include", "shaders")
        local shader_inc = path.join("$(projectdir)", "loo", "shader")
        local glslc = "glslc.exe"
        local glslc_args = "-fauto-map-locations -fauto-bind-uniforms -fauto-combined-image-sampler --target-env=opengl"

        cprintf("${bright green}[INFO] ${clear}compiling shaders under %s\n", shader_src)

        if not os.isdir(shader_src) then
            os.mkdir(shader_src)
        end

        if not os.isdir(shader_target) then
            os.mkdir(shader_target)
        end
        local function hex2cpp(hexfile, target_cppfile, varname)
            local foo = io.open(hexfile, "rb")
            local hex_arr = ""
            -- repeat
                local bytes = foo:read("*all")
                for c in bytes:gmatch '.' do
                    hex_arr = hex_arr .. string.format("0x%02X", c:byte()) .. ", "
                end
            -- until not bytes
            foo:close()
            result_content = string.format([[// This file is generated by the SPIR-V compiler
#ifndef SHADERCONST_%s_HPP
#define SHADERCONST_%s_HPP
#include <vector>
static const std::vector<unsigned char> %s = {%s};
#endif /* SHADERCONST_%s_HPP */]], varname, varname, varname, string.sub(hex_arr, 1, string.len(hex_arr) - 2), varname)
            local foo = io.open(target_cppfile, "w+")
            foo:write(result_content)
            foo:close()
        end
        -- TODO: support recursive dir
        for _, src in ipairs(os.files(path.join(shader_src, "**"))) do
            local fn = path.filename(src)
            if path.extension(fn) == ".glsl" then
                -- skip include files
                goto continue
            end
            local spv_binary_fn = string.format("%s.spv", fn)
            local spv_binary = path.join(shader_target, spv_binary_fn)
            local cmd = string.format(string.format("glslc -I %s %s -o %s %s", shader_inc, src,
             spv_binary, glslc_args))
            
            cprintf("${yellow}$ ${clear}%s\n", cmd)
            
            os.run(cmd)
            
            cpp_target = path.join(shader_target, string.format("%s.hpp", fn))
            hex2cpp(spv_binary, cpp_target, string.upper(fn:gsub("%.", "_")))
            os.rm(spv_binary)
            cprintf("${bright green}[INFO] ${clear}%s generated\n", cpp_target)
            ::continue::
        end

    end)
