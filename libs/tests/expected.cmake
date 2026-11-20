# The targets that write the expected data of the library tests,
# libs/tests/expected (design D5 of the openspec change libs-tests-capture):
#   cmake -DMODE=capture|update -DEMULATOR=emulator.cmake -DRESULTS=results
#         -DEXPECTED=DIR -DWORK=DIR [-DFILTER=snitch-filter] [-DFIX=KEY#N;...]
#         -DAMIGA_RUN=... -DPROGRAM=libs_tests -DLIBS=DIR [-DBLITZLIB=...
#         -DORIGINALS=... -DUSERLIBS=...] [-DTIMEOUT=SEC] [-DKEEP_BIN=ON]
#         [-DMACHINE=NAME [-DUAE=KEY=VALUE;...] -DMACHINES=NAME;...] [-DNEW=N]
#         -P expected.cmake
# capture (target libs-capture, the record from the original; needs the CD,
# the reference build of the RWE sources and Amiberry): libs_tests runs
# twice with LIBSET=orig, the second time with LIBS_PAD (another memory
# layout), and results libs-accept writes EXPECTED from the two outputs if
# they are identical (else "test not deterministic" and nothing written).
# update (target libs-update, from the port; no CD): one run with
# LIBSET=obj, then results libs-update rewrites the [fixed #N] cases, and
# every FIX entry KEY#N makes the unqualified case KEY the pair
# KEY [orig] / KEY [fixed #N]; NEW (an issue N, with FILTER) writes the
# expected data of a program of the filter that has none, every case of
# the port's run KEY [fixed #N] (results libs-update --new: a machine on
# which the original does not run). With FILTER only the programs that the
# filter runs and that write cases (--only), the other files stay as they
# are. MACHINE and UAE: the runs' machine and its configuration changes
# (emulator.cmake); MACHINE must be one of MACHINES (the machines of the
# libs tests, libs/tests/CMakeLists.txt), EXPECTED is its expected data
# (expected-MACHINE). The runs stay in WORK (run1, run2 or run, as emulator.cmake leaves
# them: the cases in test/captures); their raw block files (NAME.bin) are
# deleted once the expected data is written, unless KEEP_BIN or the
# environment variable LIBS_KEEP_BIN is set, and kept when it is not
# written. EMULATOR is the script of the runs; the test of this script
# gives a fake one.
cmake_minimum_required(VERSION 4.3)

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
	# the C build only (no CD)
	set(set_args -DLIBSET=obj -DNO_ORIGINALS=ON)
else()
	message(FATAL_ERROR "expected.cmake: MODE=${MODE}: capture or update")
endif()
if(FIX AND NOT MODE STREQUAL "update")
	message(FATAL_ERROR "expected.cmake: FIX is for MODE=update")
endif()
if(NEW)
	if(NOT MODE STREQUAL "update")
		message(FATAL_ERROR "expected.cmake: NEW is for MODE=update")
	elseif(NOT FILTER)
		message(FATAL_ERROR "${target}: NEW=${NEW} (LIBS_UPDATE_NEW) needs a filter"
			" (LIBS_CAPTURE_FILTER): it writes the programs the filter runs")
	elseif(NOT NEW MATCHES "^[1-9][0-9]*$")
		message(FATAL_ERROR "${target}: NEW=${NEW} (LIBS_UPDATE_NEW): not an issue number")
	endif()
endif()
if(DEFINED MACHINE AND NOT MACHINE IN_LIST MACHINES)
	list(JOIN MACHINES ", " m)
	message(FATAL_ERROR "${target}: MACHINE=${MACHINE} (LIBS_MACHINE): no machine of the libs"
		" tests (${m}; empty: the a1200)")
endif()

set(common -DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=${PROGRAM} -DLIBS=${LIBS})
foreach(v BLITZLIB ORIGINALS TIMEOUT FILTER)
	if(${v})
		list(APPEND common "-D${v}=${${v}}")
	endif()
endforeach()
# one argument each: its ; escaped, or the run gets the first entry only
foreach(v USERLIBS UAE)
	if(${v})
		string(REPLACE ";" "\\;" l "${${v}}")
		list(APPEND common "-D${v}=${l}")
	endif()
endforeach()
if(DEFINED MACHINE)
	list(APPEND common -DMACHINE=${MACHINE})
endif()

# the failure message of a finished run (rc: its exit status)
function(check_run name rc)
	if(rc EQUAL 77)
		message(FATAL_ERROR "${target}: the run was skipped: it needs Amiberry"
			" and, with the original, the CD (cmake -P cmake/fetch-originals.cmake) and the"
			" reference build of the RWE sources (ctest --preset originals -R rwe_refbuild)")
	elseif(rc)
		message(FATAL_ERROR "${target}: the run failed (${WORK}/${name}); nothing written")
	endif()
endfunction()

# one run of libs_tests: its cases in WORK/NAME/test/captures
function(run name pad)
	execute_process(COMMAND ${CMAKE_COMMAND} ${common} ${set_args} -DLIBS_PAD=${pad}
		-DWORK=${WORK}/${name} -P ${EMULATOR} RESULT_VARIABLE rc)
	check_run(${name} ${rc})
endfunction()

file(REMOVE_RECURSE ${WORK})
if(MODE STREQUAL "capture")
	# run 2: a padding that is no multiple of a page or a slot.  The two runs
	# are independent (own work directories, own sessions): at the same
	# time, as the commands of one execute_process: the standard output of the
	# first (a run's progress lines, nothing the result depends on) goes into
	# the pipe, and cmake -E cat in the middle reads it, so that the first run is
	# never cut off by a broken pipe when the second ends first; the output of
	# the second is OUT; the runs' messages come on the standard error
	execute_process(
		COMMAND ${CMAKE_COMMAND} ${common} ${set_args} -DLIBS_PAD=0
			-DWORK=${WORK}/run1 -P ${EMULATOR}
		COMMAND ${CMAKE_COMMAND} -E cat
		COMMAND ${CMAKE_COMMAND} ${common} ${set_args} -DLIBS_PAD=4104
			-DWORK=${WORK}/run2 -P ${EMULATOR}
		RESULTS_VARIABLE rcs ERROR_VARIABLE err OUTPUT_VARIABLE out)
	if(err)
		message("${err}")
	endif()
	if(out)
		message("${out}")
	endif()
	list(GET rcs 0 rc1)
	list(GET rcs 2 rc2)
	check_run(run1 ${rc1})
	check_run(run2 ${rc2})
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
	if(NEW)
		list(APPEND fixes --new ${NEW})
	endif()
	execute_process(COMMAND ${RESULTS} libs-update ${dir} ${EXPECTED} ${only} ${fixes}
		RESULT_VARIABLE rc)
endif()
if(rc)
	message(FATAL_ERROR "${target}: ${EXPECTED} not written (the report above; the runs in ${WORK})")
endif()
# written: the raw block files of the runs (NAME.bin) are not needed any more
if(NOT KEEP_BIN AND NOT DEFINED ENV{LIBS_KEEP_BIN})
	file(GLOB bins ${WORK}/*/test/captures/*.bin)
	if(bins)
		file(REMOVE ${bins})
	endif()
endif()
message(STATUS "${target}: ${EXPECTED} up to date (the report above); review it with git diff")
