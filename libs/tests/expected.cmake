# The targets that write the expected data of the library tests,
# libs/tests/expected (design D5 of the openspec change libs-tests-capture):
#   cmake -DMODE=capture|update -DEMULATOR=emulator.cmake -DRESULTS=results
#         -DEXPECTED=DIR -DWORK=DIR [-DFILTER=snitch-filter] [-DFIX=KEY#N;...]
#         -DAMIGA_RUN=... -DPROGRAM=libs_tests -DLIBS=DIR [-DBLITZLIB=...
#         -DORIGINALS=... -DUSERLIBS=...] [-DTAPES=DIR] [-DTIMEOUT=SEC]
#         -P expected.cmake
# capture (target libs-capture, the record from the original; needs the CD,
# the reference build of the RWE sources and Amiberry): libs_tests runs
# twice with LIBSET=orig, the second time with LIBS_PAD (another memory
# layout), and results libs-accept writes EXPECTED from the two outputs if
# they are identical (else "test not deterministic" and nothing written).
# update (target libs-update, from the port; no CD): one run with
# LIBSET=obj, then results libs-update rewrites the [fixed #N] cases, and
# every FIX entry KEY#N makes the unqualified case KEY the pair
# KEY [orig] / KEY [fixed #N]. With FILTER only the programs that the
# filter runs and that write cases (--only), the other files stay as they
# are. The runs stay in WORK (run1, run2 or run, as emulator.cmake leaves
# them: the cases in test/captures). EMULATOR is the script of the runs;
# the test of this script gives a fake one.
cmake_minimum_required(VERSION 4.2.3)

foreach(v MODE EMULATOR RESULTS EXPECTED WORK AMIGA_RUN PROGRAM LIBS)
	if(NOT ${v})
		message(FATAL_ERROR "expected.cmake: -D${v}=... missing")
	endif()
endforeach()
if(MODE STREQUAL "capture")
	set(target libs-capture)
	set(runs 1 2)
	set(set_args -DLIBSET=orig)
elseif(MODE STREQUAL "update")
	set(target libs-update)
	set(runs "")
	# the programs not converted yet compare with their tapes (no CD)
	set(set_args -DLIBSET=obj -DTAPE=replay -DTAPES=${TAPES})
else()
	message(FATAL_ERROR "expected.cmake: MODE=${MODE}: capture or update")
endif()
if(FIX AND NOT MODE STREQUAL "update")
	message(FATAL_ERROR "expected.cmake: FIX is for MODE=update")
endif()

set(common -DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=${PROGRAM} -DLIBS=${LIBS})
foreach(v BLITZLIB ORIGINALS TIMEOUT FILTER)
	if(${v})
		list(APPEND common "-D${v}=${${v}}")
	endif()
endforeach()
if(USERLIBS)
	# one argument: its ; escaped, or the run gets the first entry only
	string(REPLACE ";" "\\;" userlibs "${USERLIBS}")
	list(APPEND common "-DUSERLIBS=${userlibs}")
endif()

# one run of libs_tests: its cases in WORK/NAME/test/captures
function(run name pad)
	execute_process(COMMAND ${CMAKE_COMMAND} ${common} ${set_args} -DLIBS_PAD=${pad}
		-DWORK=${WORK}/${name} -P ${EMULATOR} RESULT_VARIABLE rc)
	if(rc EQUAL 77)
		message(FATAL_ERROR "${target}: the run was skipped: it needs Amiberry"
			" and, with the original, the CD (cmake -P cmake/fetch-originals.cmake) and the"
			" reference build of the RWE sources (ctest --preset originals -R rwe_refbuild)")
	elseif(rc)
		message(FATAL_ERROR "${target}: the run failed (${WORK}/${name}); nothing written")
	endif()
endfunction()

file(REMOVE_RECURSE ${WORK})
if(MODE STREQUAL "capture")
	# run 2: a padding that is no multiple of a page or a slot
	run(run1 0)
	run(run2 4104)
	set(dir ${WORK}/run1/test/captures)
else()
	run(run 0)
	set(dir ${WORK}/run/test/captures)
endif()

set(only "")
if(FILTER)
	file(GLOB written RELATIVE ${dir} ${dir}/*.txt)
	if(NOT written)
		message(FATAL_ERROR "${target}: no program of the filter \"${FILTER}\" wrote cases;"
			" nothing written")
	endif()
	foreach(w IN LISTS written)
		string(REGEX REPLACE "\\.txt$" "" w "${w}")
		list(APPEND only --only ${w})
	endforeach()
endif()

file(MAKE_DIRECTORY ${EXPECTED})
if(MODE STREQUAL "capture")
	execute_process(COMMAND ${RESULTS} libs-accept ${WORK}/run1/test/captures
		${WORK}/run2/test/captures ${EXPECTED} ${only} RESULT_VARIABLE rc)
else()
	set(fixes "")
	foreach(f IN LISTS FIX)
		list(APPEND fixes --fix "${f}")
	endforeach()
	execute_process(COMMAND ${RESULTS} libs-update ${dir} ${EXPECTED} ${only} ${fixes}
		RESULT_VARIABLE rc)
endif()
if(rc)
	message(FATAL_ERROR "${target}: ${EXPECTED} not written (the report above; the runs in ${WORK})")
endif()
message(STATUS "${target}: ${EXPECTED} up to date (the report above); review it with git diff")
