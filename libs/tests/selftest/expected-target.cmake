# The orchestration of the targets libs-capture and libs-update
# (expected.cmake) with a fake emulator run (fake-emulator.cmake), no
# emulator: a deterministic recording writes the expected data with the
# original's set; one whose two runs differ (the second has LIBS_PAD) is
# refused as "test not deterministic" and leaves the expected data as it
# was; an update with FIX makes the pair [orig]/[fixed #N] from the port's
# run (LIBSET=obj); a skipped run (77) fails naming what it needs; every
# entry of USERLIBS reaches the runs; the runs' raw block files are
# deleted after a written record and kept after a refused one; another
# machine: MACHINE and every UAE entry reach the runs, an update with NEW
# writes a program without expected data as [fixed #N] cases, and with
# broken input a MACHINE not in MACHINES, NEW without a filter, NEW that
# is no issue number and NEW of a record are refused, nothing written.
#   cmake -DRESULTS=results -DSCRIPT=expected.cmake -DFAKE=fake-emulator.cmake
#         -DWORK=DIR -P expected-target.cmake
foreach(v RESULTS SCRIPT FAKE WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "expected-target.cmake: -D${v}=... missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
set(exp ${WORK}/expected)
set(head "# capture prog 1\n")
file(WRITE ${WORK}/det "${head}r|a||1\t$00000001\nr|set||1\t\"@LIBSET@\"\n")
file(WRITE ${WORK}/nondet "${head}r|a||1\t$00000001\nr|pad||1\t\"@PAD@\"\n")
file(WRITE ${WORK}/port "${head}r|a||1\t$00000005\nr|set||1\t\"@LIBSET@\"\n")
file(WRITE ${WORK}/skip "")

# MODE PROGRAM pass|fail RE [ARGS...]: the target passes or fails, and its
# output matches the regular expression RE
function(run mode program want re)
	execute_process(COMMAND ${CMAKE_COMMAND} -DMODE=${mode} -DEMULATOR=${FAKE} -DRESULTS=${RESULTS}
		-DEXPECTED=${exp} -DWORK=${WORK}/work -DAMIGA_RUN=none -DPROGRAM=${WORK}/${program}
		-DLIBS=none ${ARGN} -P ${SCRIPT} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	message(STATUS "${mode} ${program}: ${out}")
	if(want STREQUAL "pass" AND NOT rc EQUAL 0)
		message(FATAL_ERROR "expected-target: ${mode} ${program} failed")
	elseif(want STREQUAL "fail" AND rc EQUAL 0)
		message(FATAL_ERROR "expected-target: ${mode} ${program} did not fail")
	elseif(NOT out MATCHES "${re}")
		message(FATAL_ERROR "expected-target: ${mode} ${program}: no \"${re}\" in the output")
	endif()
endfunction()
function(want_file text)
	file(READ ${exp}/prog.txt got)
	if(NOT got STREQUAL "${text}")
		message(FATAL_ERROR "expected-target: ${exp}/prog.txt is\n${got}\nnot\n${text}")
	endif()
endfunction()

# the raw block files of the runs: deleted after a written record, kept
# when nothing is written
function(want_bins want)
	file(GLOB bins ${WORK}/work/*/test/captures/*.bin)
	if(want AND NOT bins)
		message(FATAL_ERROR "expected-target: the runs' NAME.bin were not kept")
	elseif(NOT want AND bins)
		message(FATAL_ERROR "expected-target: the runs' NAME.bin were not deleted: ${bins}")
	endif()
endfunction()

run(capture det pass "libs-capture: [^\n]*expected up to date")
set(recorded "${head}r|a||1\t$00000001\nr|set||1\t\"orig\"\n")
want_file("${recorded}")
want_bins(OFF)
run(capture nondet fail
	"test not deterministic: prog\nprog: r\\|pad\\|\\|1: run 1 \"0\", run 2 \"4104\"")
want_file("${recorded}")
want_bins(ON)
# the port's run has LIBSET=obj: its other value of r|set||1 is reported, not taken
run(update port pass "prog: r\\|set\\|\\|1: the port gives \"obj\", expected \"orig\""
	"-DFIX=r|a||1#7")
want_file("${head}r|a||1 [orig]\t$00000001\nr|a||1 [fixed #7]\t$00000005\nr|set||1\t\"orig\"\n")
run(capture skip fail "libs-capture: the run was skipped: it needs Amiberry")
# the filter: the expected data of a program the runs do not have stays,
# without the filter it is refused
set(other "# capture other 1\nr|c||1\t$00000000\n")
file(WRITE ${exp}/other.txt "${other}")
run(capture det fail "other: expected data, but no output of the runs")
run(capture det pass "libs-capture: [^\n]*expected up to date" "-DFILTER=[prog]")
file(READ ${exp}/other.txt got)
if(NOT got STREQUAL other)
	message(FATAL_ERROR "expected-target: the filtered capture changed other.txt")
endif()
# every USERLIBS entry reaches the runs (not only the first)
file(WRITE ${WORK}/userlibs "${head}r|userlibs||1\t\"@USERLIBS@\"\n")
file(REMOVE ${exp}/prog.txt)
run(capture userlibs pass "libs-capture: [^\n]*expected up to date" "-DFILTER=[prog]"
	"-DUSERLIBS=a.obj|a.orig\\;b.obj|b.orig\\;c.obj|c.orig")
want_file("${head}r|userlibs||1\t\"3\"\n")
# another machine: its own expected data, a new program from the port.
# wrapped(VAR RE): RE in VAR, each blank also matching the line breaks
# of a CMake error message
function(wrapped var re)
	string(REPLACE " " "[ \n]+" re "${re}")
	set(${var} "${re}" PARENT_SCOPE)
endfunction()
set(mexp ${WORK}/expected-m1)
# run_m1: run on the machine m1 (of m0 and m1) with two UAE changes
function(run_m1 mode program want re)
	run(${mode} ${program} ${want} "${re}" -DEXPECTED=${mexp} -DMACHINE=m1 "-DMACHINES=m0\\;m1"
		"-DUAE=a=1\\;b=2" ${ARGN})
endfunction()
file(WRITE ${WORK}/machine "${head}r|machine||1\t\"@MACHINE@\"\nr|uae||1\t\"@UAE@\"\n"
	"r|set||1\t\"@LIBSET@\"\n")
string(CONCAT newfile "${head}r|machine||1 [fixed #9]\t\"m1\"\nr|set||1 [fixed #9]\t\"obj\"\n"
	"r|uae||1 [fixed #9]\t\"a=1,b=2\"\n")
wrapped(re "libs-update: MACHINE=m2 \\(LIBS_MACHINE\\): no machine of the libs tests \\(m0, m1;")
run(update machine fail "${re}"
	-DEXPECTED=${mexp} -DMACHINE=m2 "-DMACHINES=m0\\;m1" -DFILTER=[prog] -DNEW=9)
wrapped(re "libs-update: NEW=9 \\(LIBS_UPDATE_NEW\\) needs a filter")
run_m1(update machine fail "${re}" -DNEW=9)
wrapped(re "libs-update: NEW=9x \\(LIBS_UPDATE_NEW\\): not an issue number")
run_m1(update machine fail "${re}" -DFILTER=[prog] -DNEW=9x)
run_m1(capture machine fail "expected.cmake: NEW is for MODE=update" -DFILTER=[prog] -DNEW=9)
if(EXISTS ${mexp}/prog.txt)
	message(FATAL_ERROR "expected-target: a refused run wrote ${mexp}/prog.txt")
endif()
run_m1(update machine fail "prog: no expected data" -DFILTER=[prog])
run_m1(update machine pass "prog: new, 3 cases \\[fixed #9\\], of the port only" -DFILTER=[prog] -DNEW=9)
file(READ ${mexp}/prog.txt got)
if(NOT got STREQUAL newfile)
	message(FATAL_ERROR "expected-target: ${mexp}/prog.txt is\n${got}\nnot\n${newfile}")
endif()
message(STATUS "expected-target: all runs as expected")
