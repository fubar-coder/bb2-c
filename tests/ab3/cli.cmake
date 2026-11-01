# The command line tests of blitz2-ab3 in Amiberry (tests/ab3/CMakeLists.txt):
#   cmake -DAMIGA_RUN=... -DOUTMATCH=... -DREQTRAP=... -DCASES=FILE
#         -DBLITZ2_DIR=DIR -DAB3=DIR -DSOURCES=FILE;... -DUSERLIB=OBJ
#         -DWORK=DIR [-DTIMEOUT=SEC] -P cli.cmake
# One emulator session compiles every case of CASES with
# blitz2-ab3 -s SOURCE -e RAM:NAME ARGS (BLITZ2_DIR: blitz2-ab3, ab3cli and
# BB2's libraries as blitz2:) and runs the program through reqtrap
# (tests/corpus/amiga: a requester is printed and answered at once,
# nothing waits for a click).
# TEST:src holds SOURCES and AmiBlitz3's test programs (AB3/Sourcecodes/
# Amiblitz3/Tests; memoryleak_test.ab3 without its OPTIMIZE line, a G2
# directive).  Blitz3: is one of three trees, built from AB3: Blitz3
# (BlitzLibs/ with baseLibs, utilLibs, OsLibs, csLibs, Residents/all.res
# and an empty UserLibs/), Blitz3-nolib (without csLibs), Blitz3-userlib
# (USERLIB in UserLibs/).
# CASES: one line per case, NAME|TREE|SOURCE|ARGS|EXPECT.  The output of a
# case (TEST:out/NAME.txt) is
#   [compile]   the compiler's console output, rc=RC
#   [run]       the program's output and rc=RC, or "no executable"
# and is compared with EXPECT by outmatch.  Exit status 0 if every case
# matches; 77 (skipped) without the emulator or the fetched AmiBlitz3
# files.
cmake_minimum_required(VERSION 3.29)

foreach(v AMIGA_RUN OUTMATCH REQTRAP CASES BLITZ2_DIR AB3 WORK)
	if(NOT ${v})
		message(FATAL_ERROR "cli.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 600)
endif()
set(biglibs baseLibs utilLibs OsLibs csLibs)
set(tests_dir ${AB3}/Sourcecodes/Amiblitz3/Tests)
foreach(f IN LISTS biglibs)
	set(need ${need} ${AB3}/BlitzLibs/${f})
endforeach()
list(APPEND need ${AB3}/BlitzLibs/Residents/all.res ${tests_dir}/atoi.ab3)
foreach(f IN LISTS need)
	if(NOT EXISTS ${f})
		message(STATUS "cli.cmake: no ${f} - skipped (fetch-originals)")
		cmake_language(EXIT 77)
	endif()
endforeach()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test}/out ${test}/src ${test}/blitz2)

# blitz2: and the Blitz3: trees
foreach(f blitz2-ab3 ab3cli acidlibs deflibs)
	file(COPY_FILE ${BLITZ2_DIR}/${f} ${test}/blitz2/${f})
endforeach()
file(COPY_FILE ${REQTRAP} ${test}/reqtrap)
foreach(tree Blitz3 Blitz3-nolib Blitz3-userlib)
	set(libs ${test}/${tree}/BlitzLibs)
	file(MAKE_DIRECTORY ${libs}/Residents ${libs}/UserLibs)
	foreach(f IN LISTS biglibs)
		if(NOT (tree STREQUAL "Blitz3-nolib" AND f STREQUAL "csLibs"))
			file(COPY_FILE ${AB3}/BlitzLibs/${f} ${libs}/${f})
		endif()
	endforeach()
	file(COPY_FILE ${AB3}/BlitzLibs/Residents/all.res ${libs}/Residents/all.res)
endforeach()
if(USERLIB)
	file(COPY_FILE ${USERLIB} ${test}/Blitz3-userlib/BlitzLibs/UserLibs/ab3userlib.obj)
endif()

# the sources
file(GLOB ab3_tests ${tests_dir}/*.ab3)
foreach(f IN LISTS ab3_tests SOURCES)
	get_filename_component(n ${f} NAME)
	file(COPY_FILE ${f} ${test}/src/${n})
endforeach()
file(READ ${tests_dir}/memoryleak_test.ab3 text)
string(REGEX REPLACE "\nOPTIMIZE [^\n]*\n" "\n" text "${text}")
file(WRITE ${test}/src/memoryleak_test.ab3 "${text}")

# the session
file(STRINGS ${CASES} cases)
set(script "FailAt 21\nAssign blitz2: TEST:blitz2\nCd TEST:src\n")
foreach(c IN LISTS cases)
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 name)
	list(GET c 1 tree)
	list(GET c 2 source)
	list(GET c 3 args)
	set(out TEST:out/${name}.txt)
	set(cmd "blitz2:blitz2-ab3 >>${out} -s ${source} -e RAM:${name} ${args}")
	string(APPEND script "Assign Blitz3: TEST:${tree}\n"
		"Echo \"[compile]\" >${out}\n${cmd}\nEcho \"rc=$RC\" >>${out}\n"
		"Echo \"[run]\" >>${out}\n"
		"If EXISTS RAM:${name}\n  TEST:reqtrap >>${out} RAM:${name}\n  Echo \"rc=$RC\" >>${out}\n"
		"  Delete >NIL: RAM:${name} QUIET\nElse\n  Echo \"no executable\" >>${out}\nEndIf\n")
endforeach()
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "cli.cmake: the emulator session did not end (${WORK})")
endif()

# the comparison
set(failed 0)
foreach(c IN LISTS cases)
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 name)
	list(GET c 4 expect)
	set(out ${test}/out/${name}.txt)
	if(NOT EXISTS ${out})
		message(STATUS "FAIL ${name}: no output")
		math(EXPR failed "${failed} + 1")
		continue()
	endif()
	execute_process(COMMAND ${OUTMATCH} ${expect} ${out} RESULT_VARIABLE r ERROR_VARIABLE msg)
	string(STRIP "${msg}" msg)
	if(r)
		message(STATUS "FAIL ${name}: ${msg}")
		math(EXPR failed "${failed} + 1")
	else()
		message(STATUS "ok   ${name}")
	endif()
endforeach()
if(failed)
	message(FATAL_ERROR "cli.cmake: ${failed} case(s) failed (outputs in ${test}/out)")
endif()
