# The orchestration of the targets libs-capture and libs-update
# (expected.cmake) with a fake emulator run (fake-emulator.cmake), no
# emulator: a deterministic recording writes the expected data with the
# original's set; one whose two runs differ (the second has LIBS_PAD) is
# refused as "test not deterministic" and leaves the expected data as it
# was; an update with FIX makes the pair [orig]/[fixed #N] from the port's
# run (LIBSET=obj); a skipped run (77) fails naming what it needs.
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

run(capture det pass "libs-capture: [^\n]*expected up to date")
set(recorded "${head}r|a||1\t$00000001\nr|set||1\t\"orig\"\n")
want_file("${recorded}")
run(capture nondet fail
	"test not deterministic: prog\nprog: r\\|pad\\|\\|1: run 1 \"0\", run 2 \"4104\"")
want_file("${recorded}")
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
message(STATUS "expected-target: all runs as expected")
