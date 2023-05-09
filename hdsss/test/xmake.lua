add_requires("gtest 1.12.1")


target("HDSSStest")
    set_kind("binary")
    add_deps("HDSSSlib")
    set_languages("c11", "cxx17")
    add_packages("gtest")

    add_files("*.cpp")
