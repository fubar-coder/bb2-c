# tedstub: blitz2:ted for batch compiles (edstub frame, no C runtime)
add_executable(tedstub $<TARGET_OBJECTS:edstub> tedstub.c)
target_include_directories(tedstub PRIVATE ${EDSTUB_DIR})
target_compile_options(tedstub PRIVATE ${EDSTUB_COMPILE_OPTIONS})
target_link_options(tedstub PRIVATE ${EDSTUB_LINK_OPTIONS})
set_target_properties(tedstub PROPERTIES SUFFIX "")
# hunk layout: +0 moveq #20,d0 (shell start fails), +4 movem.l (outside)
add_dependencies(tedstub host_tools)
add_custom_command(TARGET tedstub POST_BUILD
	COMMAND ${HUNKCHECK} start $<TARGET_FILE:tedstub> 0:7014 4:48e7
	VERBATIM)

# reqtrap COMMAND: AutoRequest/EasyRequestArgs answered at once (texts to
# stdout); reqclose: the same plus a process that answers modal requesters
# as ENV:REQCLOSE says (reqclose.h); reqclosetest: proves reqclose (run it
# through reqclose); rcput FILE: rc=N of the previous command appended (the
# session scripts on Kickstart 1.x).  reqtrap and rcput also run on
# Kickstart 1.x: linked with libnix13 (libnix20, the default, does not
# start there)
add_executable(reqtrap reqtrap.c)
add_executable(reqclose reqtrap.c)
target_compile_definitions(reqclose PRIVATE REQCLOSE)
add_executable(reqclosetest reqclosetest.c)
add_executable(rcput rcput.c)
# hangstub: never ends (the fake prev compiler of compiler_selftest_lost_*_prev)
add_executable(hangstub hangstub.c)
# patchblocktest: proves patchblock.h (the patches reqtrap and teddrive
# use), run by the tests patchblock and patchblock_broken
add_executable(patchblocktest patchblocktest.c)
# patchflagstest: a PB_THRU patch keeps the condition codes of its stub
add_executable(patchflagstest patchflagstest.c)
# reqtraptest: proves the handlers of reqtrap (no DOS call in the context of
# the task that raises the requester, ctxwrite.h); reqtrapdirect: reqtrap with
# the old handlers (the caller writes), the broken input of the test
add_executable(reqtraptest reqtraptest.c)
add_executable(reqtrapdirect reqtrap.c)
target_compile_definitions(reqtrapdirect PRIVATE CW_DIRECT)
foreach(t reqtrap reqclose reqclosetest rcput hangstub patchblocktest patchflagstest reqtraptest reqtrapdirect)
	target_compile_options(${t} PRIVATE -Wall -Wno-pointer-sign)
	set_target_properties(${t} PROPERTIES SUFFIX "")
endforeach()
set_source_files_properties(reqtrap.c patchblocktest.c patchflagstest.c PROPERTIES
	OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/patchblock.h;${CMAKE_CURRENT_SOURCE_DIR}/ctxwrite.h")
foreach(t reqtrap rcput reqtrapdirect patchflagstest)
	# libnix13 in front of the default libnix20: what it has comes from it
	# (-mcrt=nix13 would select it alone, but cannot go with -noixemul of
	# the toolchain's flags)
	target_link_libraries(${t} PRIVATE nix13)
endforeach()
