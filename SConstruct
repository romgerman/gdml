#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=[
    "src/",
    "tree-sitter/lib/src",
    "tree-sitter/lib/include",
    "grammar/src",
    "grammar/bindings/c"
])
sources = Glob("src/*.cpp") \
	+ Glob("src/render/*.cpp") \
    + Glob("src/nodes/*.cpp") \
    + Glob("src/built-in/*.cpp") \
	+ Glob("src/built-in/functions/*.cpp") \
	+ Glob("src/built-in/decorators/*.cpp") \
    + [
    	"grammar/src/parser.c"
	]

tree_sitter_library = env.StaticLibrary("demo/addons/gdml/bin/tree-sitter", ["tree-sitter/lib/src/lib.c"])
env.Append(LIBS=[tree_sitter_library])
env.Append(CFLAGS=['/std:c11'])
env.Append(CFLAGS=['/TC'])

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/addons/gdml/bin/libgdml.{}.{}.framework/libgdml.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/addons/gdml/bin/libgdml.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/addons/gdml/bin/libgdml.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/addons/gdml/bin/libgdml{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
