set_project("SM-ModSearch")
set_version("0.1.0")
set_languages("cxx17")
add_rules("mode.debug", "mode.release")

if not is_plat("windows") then
    set_defaultplat("windows")
end
set_arch("x64")

add_requires("minhook")

-- MyGUIEngine.dll ships next to ScrapMechanic.exe but has no import .lib.
-- We build one from mygui_engine.def (the handful of mangled exports we
-- actually call) via lib.exe, and link against that.
rule("mygui_engine_implib")
    on_load(function (target)
        local outlib = path.join(target:targetdir(), "MyGUIEngine.lib")
        target:add("linkdirs", target:targetdir())
        target:add("links", "MyGUIEngine")
        target:data_set("mygui_engine_implib", outlib)
    end)

    before_build(function (target)
        import("core.project.depend")

        local defpath = path.join(os.projectdir(), "mygui_engine.def")
        local outlib  = target:data("mygui_engine_implib")
        os.mkdir(target:targetdir())

        depend.on_changed(function ()
            -- lib.exe lives next to cl.exe in the same MSVC host/target bin dir.
            local cl = target:tool("cxx")
            local libexe = path.join(path.directory(cl), "lib.exe")
            assert(os.isfile(libexe), "lib.exe not found next to cl.exe at " .. libexe)

            os.vrunv(libexe, {
                "/def:" .. defpath,
                "/out:" .. outlib,
                "/machine:x64"
            })
            print("generated %s", outlib)
        end, {files = {defpath}})
    end)
rule_end()

target("SM-ModSearch")
    set_kind("shared")
    add_files("src/*.cpp")
    add_includedirs("vendor/mygui-src/MyGUIEngine/include")
    add_packages("minhook")
    add_rules("mygui_engine_implib")
    add_syslinks("user32", "gdi32")
target_end()
