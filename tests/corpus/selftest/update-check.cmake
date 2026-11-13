# update-check.cmake - corpusrun pass --update-expect clib on a pass of the
# one program t01_int whose expected output is wrong (the test
# corpus_update_fix, label emulator): without --fix the update refuses and
# names the program, the file is unchanged; with --fix corpus:t01_int#1
# the file becomes the pair t01_int.orig.out (the old content) and
# t01_int.fixed-1.out (the port's output); a second update without --fix
# leaves the pair as it is.
#   cmake -DRUNNER=corpusrun -DPASS=PASSFILE -DDIR=EXPECT_DIR -DWRONG=FILE -DRIGHT=FILE
#         -P update-check.cmake
foreach(v RUNNER PASS DIR WRONG RIGHT)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "update-check.cmake: -D${v}=... missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${DIR})
file(MAKE_DIRECTORY ${DIR})
file(COPY_FILE ${WRONG} ${DIR}/t01_int.out)
file(READ ${WRONG} wrong)
file(READ ${RIGHT} right)

# update(WANT REGEX ARGS...)
function(update want re)
	execute_process(COMMAND ${RUNNER} pass --update-expect clib ${ARGN} ${PASS}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	endif()
	message(STATUS "update ${ARGN}: ${rc}\n${out}${err}")
	if(NOT rc EQUAL want OR NOT "${out}${err}" MATCHES "${re}")
		message(FATAL_ERROR "the update ${ARGN} returned ${rc} (expected ${want}) or its output "
			"does not match: ${re}")
	endif()
endfunction()
function(holds file want)
	if(want STREQUAL "-")
		if(EXISTS ${DIR}/${file})
			message(FATAL_ERROR "${file} exists")
		endif()
		return()
	endif()
	file(READ ${DIR}/${file} got)
	if(NOT got STREQUAL want)
		message(FATAL_ERROR "${file} holds\n${got}\nnot\n${want}")
	endif()
endfunction()

update(1 "corpus:t01_int: t01_int.out changed, line 1: expected \"667 1801 -699678 -3\", got \"667 1801 -699678 -2\": name the fix with FIX=corpus:t01_int#N")
holds(t01_int.out "${wrong}")
holds(t01_int.orig.out -)
update(0 "wrote [^\n]*t01_int.orig.out\nwrote [^\n]*t01_int.fixed-1.out\nremoved [^\n]*/t01_int.out\n"
	--fix "corpus:t01_int#1")
holds(t01_int.out -)
holds(t01_int.orig.out "${wrong}")
holds(t01_int.fixed-1.out "${right}")
update(0 "the expected data written")
holds(t01_int.fixed-1.out "${right}")
message(STATUS "update-check: all updates as expected")
