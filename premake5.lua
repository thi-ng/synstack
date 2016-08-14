workspace "synstack"
configurations { "debug", "release" }
platforms { "sse", "no_sse" }
language "C"
includedirs { "ext", "src" }
targetdir "bin/%{cfg.buildcfg}"
flags { "Symbols", "C++11" }
linkoptions "-lm"

filter "platforms:sse"
defines { "CT_FEATURE_SSE" }
buildoptions { "-msse", "-msse2", "-msse3", "-msse4.1" }

filter "configurations:debug"
defines { "DEBUG", "CT_FEATURE_CHECKS", "CT_FEATURE_CHECK_MEM" }

filter "configurations:release"
defines { "NDEBUG", "CT_FEATURE_LOG" }
optimize "Size"

----- test
--[[
project "test"
kind "ConsoleApp"
files { "ext/**.c", "test/**.c" }
defines { "CT_FEATURE_ANSI" }
links "lib"
dependson "lib"
flags { "FatalWarnings", "LinkTimeOptimization" }

----- test w/ addr sanitizer

project "test_asan"
kind "ConsoleApp"
files { "ext/**.c", "test/**.c" }
defines { "CT_FEATURE_ANSI", "CT_FEATURE_CHECKS", "CT_FEATURE_CHECK_MEM" }
links "lib"
dependson "lib"
flags { "FatalWarnings" }
buildoptions { "-fsanitize=address", "-fno-omit-frame-pointer" }
linkoptions { "-fsanitize=address" }
--]]

----- lib

project "lib"
files { "src/**.h", "src/**.c" }
kind "StaticLib"
targetname "synstack"

----- drone -----

project "synth_drone"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_drone.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- formant_seq -----

project "synth_formantseq"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_formantseq.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- keys -----

project "synth_keys"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_keys.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- ksensemble -----

project "synth_ksensemble"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_ksensemble.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- pan -----

project "synth_pan"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_pan.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- pwm -----

project "synth_pwm"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_pwm.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- render -----

project "synth_render"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_render.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }

----- spiral -----

project "synth_spiral"
kind "ConsoleApp"
files { "examples/demo_common.c", "examples/synth_spiral.c" }
includedirs { "ext", "src", "examples" }
links "lib"
dependson "lib"
flags { "LinkTimeOptimization" }
linkoptions { "-lncurses", "-lportaudio" }
