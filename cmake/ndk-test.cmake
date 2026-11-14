# The compile tests of the libraries' shadow headers (cmake/ndk-calls.cmake).
# Each compiles SRC with the library options to assembler text on stdout
# (no file) and judges it; a failure is a FATAL_ERROR.
#
#   cmake -DMODE=varargs -DCC=... -DFLAGS="..." -DSHADOW=DIR -DWANT=NAME
#         -DSRC=cmake/ndk/varargs.c -P ndk-test.cmake
#     with -I SHADOW (none: SHADOW empty, the broken input) the object
#     holds the varargs wrapper __NAME_va and no other one, without a
#     warning, and the wrapper's code equals the one the NDK's header gives
#     without the shadow
#   cmake -DMODE=poison ... -DWANT=NAME -DSRC=cmake/ndk/poisoned.c
#     compiling fails with "attempt to use poisoned 'NAME'" (with SHADOW
#     empty, the broken input, it compiles: that fails the test)
separate_arguments(flags UNIX_COMMAND "${FLAGS}")

function(compile out_asm out_err out_rc)
	set(inc)
	if(ARGN)
		set(inc -I${ARGN})
	endif()
	execute_process(COMMAND ${CC} ${flags} ${inc} -S -o - ${SRC}
		OUTPUT_VARIABLE asm ERROR_VARIABLE err RESULT_VARIABLE rc)
	set(${out_asm} "${asm}" PARENT_SCOPE)
	set(${out_err} "${err}" PARENT_SCOPE)
	set(${out_rc} "${rc}" PARENT_SCOPE)
endfunction()

# the text of the function NAME: its label up to its rts
function(body asm name out)
	string(FIND "${asm}" "\n${name}:" i)
	if(i LESS 0)
		set(${out} "" PARENT_SCOPE)
		return()
	endif()
	string(SUBSTRING "${asm}" ${i} -1 rest)
	string(REGEX MATCH "\n${name}:.*\n\trts\n" b "${rest}")
	string(FIND "${b}" "\trts\n" j)
	math(EXPR j "${j} + 5")
	string(SUBSTRING "${b}" 0 ${j} b)
	set(${out} "${b}" PARENT_SCOPE)
endfunction()

if(MODE STREQUAL "varargs")
	compile(asm err rc ${SHADOW})
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "${SRC} does not compile: ${err}")
	endif()
	if(NOT err STREQUAL "")
		message(FATAL_ERROR "${SRC}: the compiler warns: ${err}")
	endif()
	string(REGEX MATCHALL "\n___[A-Za-z0-9_]+_va:" labels "${asm}")
	list(REMOVE_ITEM labels "\n___${WANT}_va:")
	if(labels)
		list(JOIN labels "" labels)
		string(REPLACE "\n" " " labels "${labels}")
		message(FATAL_ERROR "${SRC}: unused varargs wrappers emitted:${labels}")
	endif()
	body("${asm}" ___${WANT}_va got)
	if(got STREQUAL "")
		message(FATAL_ERROR "${SRC}: no wrapper ___${WANT}_va")
	endif()
	compile(ref err rc)
	body("${ref}" ___${WANT}_va want)
	if(NOT got STREQUAL want)
		message(FATAL_ERROR "${SRC}: ___${WANT}_va differs from the NDK's:\n${got}\n--- NDK:\n${want}")
	endif()
	message(STATUS "${SRC}: only ___${WANT}_va, the NDK's code")
elseif(MODE STREQUAL "poison")
	compile(asm err rc ${SHADOW})
	if(rc EQUAL 0)
		message(FATAL_ERROR "${SRC} compiles: the call of ${WANT} is not refused")
	endif()
	if(NOT err MATCHES "attempt to use poisoned .${WANT}.")
		message(FATAL_ERROR "${SRC} fails, but not for ${WANT}: ${err}")
	endif()
	message(STATUS "${SRC}: ${WANT} refused")
else()
	message(FATAL_ERROR "ndk-test.cmake: MODE varargs or poison")
endif()
