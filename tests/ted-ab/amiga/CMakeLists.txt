# The m68k tools of the TED A/B tests (from bb2 ted-c/tests/amiga), built
# with libnix like reqtrap / reqclose:
#   tedprobe      gate 0: Workbench mode, bitplanes in chip memory, geometry
#   teddrive      plays a script against TED (synthetic IntuiMessages)
#   teddrivetest  its self-test (gate D), run through teddrive
# teddrive includes reqclose.h of tests/corpus/amiga unchanged (one source).
set(REQCLOSE_DIR ${PROJECT_SOURCE_DIR}/tests/corpus/amiga)
foreach(t tedprobe teddrive teddrivetest)
	add_executable(${t} ${t}.c)
	target_compile_options(${t} PRIVATE -Wall -Wno-pointer-sign)
	set_target_properties(${t} PROPERTIES SUFFIX "")
endforeach()
target_include_directories(teddrive PRIVATE ${REQCLOSE_DIR})
# most of reqclose.h (the closer's own loop and answer logic) is not used
target_compile_options(teddrive PRIVATE -Wno-unused-function)
set_source_files_properties(teddrive.c PROPERTIES OBJECT_DEPENDS ${REQCLOSE_DIR}/reqclose.h)
