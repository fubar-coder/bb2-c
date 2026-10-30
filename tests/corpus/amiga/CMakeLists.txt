# tedstub: blitz2:ted for batch compiles (edstub frame, no C runtime)
add_executable(tedstub $<TARGET_OBJECTS:edstub> tedstub.c)
target_include_directories(tedstub PRIVATE ${EDSTUB_DIR})
target_compile_options(tedstub PRIVATE ${EDSTUB_COMPILE_OPTIONS})
target_link_options(tedstub PRIVATE ${EDSTUB_LINK_OPTIONS})
set_target_properties(tedstub PROPERTIES SUFFIX "")
# hunk layout: +0 moveq #20,d0 (shell start fails), +4 movem.l (outside)
add_dependencies(tedstub host_tools)
add_custom_command(TARGET tedstub POST_BUILD
	COMMAND ${HUNKCHECK} $<TARGET_FILE:tedstub> 0:7014 4:48e7
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
foreach(t reqtrap reqclose reqclosetest rcput)
	target_compile_options(${t} PRIVATE -Wall -Wno-pointer-sign)
	set_target_properties(${t} PROPERTIES SUFFIX "")
endforeach()
foreach(t reqtrap rcput)
	# libnix13 in front of the default libnix20: what it has comes from it
	# (-mcrt=nix13 would select it alone, but cannot go with -noixemul of
	# the toolchain's flags)
	target_link_libraries(${t} PRIVATE nix13)
endforeach()
