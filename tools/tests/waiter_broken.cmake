# The check of SNAP:waitjob in amiga_emulation.c (static_assert on its
# hunk header and size) with broken waiters: waitjob.s assembled the
# wrong way must not compile, the right way must.
#   cmake -DCC=... -DVASM=... -DSRC=<tools/lib/amiga_emulation> -DWORK=<dir> -P waiter_broken.cmake
foreach(v CC VASM SRC WORK)
	if(NOT ${v})
		message(FATAL_ERROR "waiter_broken.cmake: ${v} missing")
	endif()
endforeach()

# NAME EXPECT FLAGS...: assembles waitjob.s with FLAGS into WORK/NAME/waitjob,
# compiles amiga_emulation.c against it; EXPECT: "ok" or the message
function(case name expect)
	set(dir ${WORK}/${name})
	file(REMOVE_RECURSE ${dir})
	file(MAKE_DIRECTORY ${dir})
	execute_process(COMMAND ${VASM} -quiet -m68000 ${ARGN} -o ${dir}/waitjob ${SRC}/waitjob.s
		RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "${name}: vasm failed")
	endif()
	execute_process(COMMAND ${CC} -std=gnu23 -fsyntax-only --embed-dir=${dir} -I${SRC}
			${SRC}/amiga_emulation.c
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	if(expect STREQUAL "ok")
		if(rc)
			message(FATAL_ERROR "${name}: the waiter is refused:\n${out}")
		endif()
	elseif(NOT rc)
		message(FATAL_ERROR "${name}: the broken waiter compiles")
	elseif(NOT out MATCHES "${expect}")
		message(FATAL_ERROR "${name}: not refused with \"${expect}\":\n${out}")
	endif()
	message(STATUS "${name}: ${expect}")
endfunction()

case(good ok -Fhunkexe -nosym)
# an object file (HUNK_UNIT), not an executable
case(object "waitjob: no HUNK_HEADER" -Fhunk -nosym)
# with a symbol hunk: more than the code hunk and HUNK_END
case(symbols "waitjob: not one code hunk" -Fhunkexe)
# a raw binary: no hunks at all
case(raw "waitjob: no HUNK_HEADER" -Fbin)
