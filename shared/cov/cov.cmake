# Function coverage of the m68k programs (cov.h): the compiler options and
# the recording module, for the coverage configuration (preset amiga-cov).
# A program uses all three: its sources instrumented, the sources that run
# before a4 is loaded excluded, the module linked.
set(COV_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# cov_instrument(TARGET): a hook call at the first entry of every function
# (-once: a guard byte per function in .bss, later calls only test it);
# -g for the line tables cmake/coverage-map.cmake reads from the objects
# (it leaves the code as it is; the release link strips the program)
function(cov_instrument target)
	target_compile_options(${target} PRIVATE -finstrument-functions-once -g)
endfunction()

# cov_exclude(SOURCE...): sources without the hook call, e.g. entry stubs
# that load a4 (the hook addresses its globals relative to a4).  Source
# options come after the target's on the command line.
function(cov_exclude)
	set_source_files_properties(${ARGN} DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		PROPERTIES COMPILE_OPTIONS -fno-instrument-functions)
endfunction()

# cov_link(TARGET NAME): the module, writing TEST:cov/NAME.cov at the end
# (cov.c is compiled with the target's options and excluded itself)
function(cov_link target name)
	target_sources(${target} PRIVATE ${COV_SOURCE_DIR}/cov.c)
	target_compile_definitions(${target} PRIVATE COV_PROGRAM="${name}")
	cov_exclude(${COV_SOURCE_DIR}/cov.c)
endfunction()
