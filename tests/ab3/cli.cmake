# The command line tests of blitz2-ab3 in Amiberry (tests/ab3/CMakeLists.txt):
#   cmake -DAMIGA_RUN=... -DOUTMATCH=... -DREQTRAP=... -DCONFIG=exe|ref
#         -DCASES=FILE -DBLITZ2_DIR=DIR -DAB3=DIR -DSOURCES=FILE;...
#         -DUSERLIB=OBJ -DWORK=DIR [-DTIMEOUT=SEC] [-DSAME=A=B,...]
#         [-DHUNKCHECK=TOOL -DHUNKS=NAME=LAYOUT|...] -P cli.cmake
# One emulator session compiles every case of CASES and runs the program
# through reqtrap (tests/corpus/amiga: a requester is printed and answered
# at once, nothing waits for a click):
#   exe  blitz2-ab3 -s SOURCE -e RAM:NAME ARGS (BLITZ2_DIR: blitz2-ab3,
#        ab3cli and BB2's libraries as blitz2:)
#   ref  AmiBlitz3 v3.14.0's own compiler, Blitz3:Amiblitz3 -s SOURCE
#        -e RAM:NAME (CLI mode)
# TEST:src holds SOURCES and AmiBlitz3's test programs (AB3/Sourcecodes/
# Amiblitz3/Tests, and int_tests_nostop.ab3: int_tests.ab3 without its Stop
# line).  Blitz3: is one of three trees, built from AB3: Blitz3
# (BlitzLibs/ with baseLibs, utilLibs, OsLibs, csLibs, Residents/all.res
# and an empty UserLibs/; an empty Libs/; for ref the compiler
# Amiblitz3), Blitz3-nolib (without csLibs), Blitz3-userlib (USERLIB in
# UserLibs/).
# CASES: one line per case, NAME|TREE|SOURCE|ARGS|EXPECT[|REFEXPECT[|norun]]
# (norun: for ref, the executable is kept but not run).  The output of a
# case (TEST:out/NAME.txt) is
#   [compile]   the compiler's console output, rc=RC
#   [run]       the program's output and rc=RC, "not run" or "no executable"
# and is compared with EXPECT by outmatch: exe the whole file, ref only
# [run] (the console output of AmiBlitz3 is its own), against REFEXPECT if
# given (a difference of AmiBlitz3 recorded in docs/ab3/variant.md).
# The executable of a case is kept as TEST:out/NAME.exe; SAME lists pairs
# of cases whose executables must be byte-identical (A=B), HUNKS the hunk
# layout of the executable of a case (hunkcheck --hunks LAYOUT).
# Exit status 0 if every case matches; 77 (skipped) without the emulator
# or the fetched AmiBlitz3 files.
cmake_minimum_required(VERSION 3.29)

foreach(v AMIGA_RUN OUTMATCH REQTRAP CONFIG CASES BLITZ2_DIR AB3 WORK)
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
if(CONFIG STREQUAL "ref")
	list(APPEND need ${AB3}/Amiblitz3)
endif()
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
	# Libs/: AmiBlitz3 adds Blitz3:Libs to LIBS: without checking the lock
	# (docs/ab3/variant.md); without the directory the entries pile up and
	# ramlib crashes after some 36 compiles
	file(MAKE_DIRECTORY ${libs}/Residents ${libs}/UserLibs ${test}/${tree}/Libs)
	foreach(f IN LISTS biglibs)
		if(NOT (tree STREQUAL "Blitz3-nolib" AND f STREQUAL "csLibs"))
			file(COPY_FILE ${AB3}/BlitzLibs/${f} ${libs}/${f})
		endif()
	endforeach()
	file(COPY_FILE ${AB3}/BlitzLibs/Residents/all.res ${libs}/Residents/all.res)
	if(CONFIG STREQUAL "ref")
		file(COPY_FILE ${AB3}/Amiblitz3 ${test}/${tree}/Amiblitz3)
	endif()
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

# int_tests without its Stop line (an empty line instead, the line numbers
# stay): AmiBlitz3 ignores Stop without its debugger (docs/ab3/variant.md)
file(READ ${tests_dir}/int_tests.ab3 text)
string(REGEX REPLACE "\nStop([\r\n])" "\n\\1" text "${text}")
file(WRITE ${test}/src/int_tests_nostop.ab3 "${text}")

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
	list(LENGTH c n)
	set(run "  TEST:reqtrap >>${out} RAM:${name}\n  Echo \"rc=$RC\" >>${out}\n")
	if(CONFIG STREQUAL "ref" AND n GREATER 6)
		list(GET c 6 mode)
		if(mode STREQUAL "norun")
			set(run "  Echo \"not run\" >>${out}\n")
		endif()
	endif()
	if(CONFIG STREQUAL "ref")
		set(cmd "Blitz3:Amiblitz3 >>${out} -s ${source} -e RAM:${name}")
	else()
		set(cmd "blitz2:blitz2-ab3 >>${out} -s ${source} -e RAM:${name} ${args}")
	endif()
	string(APPEND script "Assign Blitz3: TEST:${tree}\n"
		"Echo \"[compile]\" >${out}\n${cmd}\nEcho \"rc=$RC\" >>${out}\n"
		"Echo \"[run]\" >>${out}\n"
		"If EXISTS RAM:${name}\n${run}"
		"  Copy RAM:${name} TEST:out/${name}.exe QUIET\n"
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
	list(LENGTH c n)
	set(section)
	if(CONFIG STREQUAL "ref")
		set(section --section run)
		if(n GREATER 5)
			list(GET c 5 expect)
		endif()
	endif()
	set(out ${test}/out/${name}.txt)
	if(NOT EXISTS ${out})
		message(STATUS "FAIL ${name}: no output")
		math(EXPR failed "${failed} + 1")
		continue()
	endif()
	execute_process(COMMAND ${OUTMATCH} ${section} ${expect} ${out}
		RESULT_VARIABLE r ERROR_VARIABLE msg)
	string(STRIP "${msg}" msg)
	if(r)
		message(STATUS "FAIL ${name}: ${msg}")
		math(EXPR failed "${failed} + 1")
	else()
		message(STATUS "ok   ${name}")
	endif()
endforeach()
# the executables that must be the same
string(REPLACE "," ";" same "${SAME}")
foreach(p IN LISTS same)
	string(REPLACE "=" ";" p "${p}")
	list(GET p 0 a)
	list(GET p 1 b)
	set(fa ${test}/out/${a}.exe)
	set(fb ${test}/out/${b}.exe)
	if(NOT EXISTS ${fa} OR NOT EXISTS ${fb})
		message(STATUS "FAIL ${a}=${b}: no executable")
		math(EXPR failed "${failed} + 1")
		continue()
	endif()
	file(SHA256 ${fa} ha)
	file(SHA256 ${fb} hb)
	if(ha STREQUAL hb)
		message(STATUS "ok   ${a}=${b}: the same executable")
	else()
		message(STATUS "FAIL ${a}=${b}: the executables differ")
		math(EXPR failed "${failed} + 1")
	endif()
endforeach()
# the hunk layouts
string(REPLACE "|" ";" hunks "${HUNKS}")
foreach(h IN LISTS hunks)
	string(FIND "${h}" "=" eq)
	string(SUBSTRING "${h}" 0 ${eq} n)
	math(EXPR eq "${eq} + 1")
	string(SUBSTRING "${h}" ${eq} -1 layout)
	execute_process(COMMAND ${HUNKCHECK} --hunks ${layout} ${test}/out/${n}.exe
		RESULT_VARIABLE r ERROR_VARIABLE msg)
	string(STRIP "${msg}" msg)
	if(r)
		message(STATUS "FAIL ${n}: hunks: ${msg}")
		math(EXPR failed "${failed} + 1")
	else()
		message(STATUS "ok   ${n}: hunks ${layout}")
	endif()
endforeach()
if(failed)
	message(FATAL_ERROR "cli.cmake: ${failed} case(s) failed (${CONFIG}, outputs in ${test}/out)")
endif()
