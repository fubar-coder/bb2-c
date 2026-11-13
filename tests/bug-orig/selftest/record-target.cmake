# record-target.cmake - the orchestration of the target bug-orig-record
# (record.cmake) with fake runs (FAKE: no runner, no emulator), on files in
# WORK: test a records the same twice (its ID.orig is written), test b
# differs in a section (refused: "test not deterministic", its ID.orig
# unchanged, the target fails), test c is left out by the filter (its
# ID.orig untouched), test d's second run fails (the target fails, nothing
# written); a second run limited to a finds it unchanged and passes.
#   cmake -DRESULTS=results -DSCRIPT=record.cmake -DWORK=DIR -P record-target.cmake
foreach(v RESULTS SCRIPT WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "record-target.cmake: -D${v}=... missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/fake ${WORK}/tree)
set(note "# the original\n")
set(obs "${note}[session]\ndone\n[run]\nptr=$1000\nrc=0\n")
file(WRITE ${WORK}/fake/a.run1 "${obs}")
file(WRITE ${WORK}/fake/a.run2 "${obs}")
file(WRITE ${WORK}/fake/b.run1 "${obs}")
file(WRITE ${WORK}/fake/b.run2 "${note}[session]\ndone\n[run]\nptr=$2000\nrc=0\n")
file(WRITE ${WORK}/fake/c.run1 "${obs}")
file(WRITE ${WORK}/fake/c.run2 "${obs}")
file(WRITE ${WORK}/fake/d.run1 "${obs}")
set(old "${note}[session]\ntimeout\n")
foreach(t b c d)
	file(WRITE ${WORK}/tree/${t}.orig "${old}")
endforeach()
set(list "")
foreach(t a b c d)
	string(APPEND list "${t}|blitz|-|-|${WORK}/${t}.rec1|${WORK}/${t}.rec2|${WORK}/tree/${t}.orig\n")
endforeach()
file(WRITE ${WORK}/list "${list}")

# run(FILTER WANT_OK REGEX)
function(run filter ok re)
	execute_process(COMMAND ${CMAKE_COMMAND} -DLIST=${WORK}/list -DRESULTS=${RESULTS}
		-DWORK=${WORK}/work -DFAKE=${WORK}/fake -DFILTER=${filter} -DPARALLEL=2 -P ${SCRIPT}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	message(STATUS "FILTER=${filter}: ${rc}\n${out}")
	if((ok AND NOT rc EQUAL 0) OR (NOT ok AND rc EQUAL 0) OR NOT out MATCHES "${re}")
		message(FATAL_ERROR "bug-orig-record FILTER=${filter}: exit status ${rc}, no \"${re}\"")
	endif()
endfunction()
function(holds t want)
	file(READ ${WORK}/tree/${t}.orig got)
	if(NOT got STREQUAL want)
		message(FATAL_ERROR "${t}.orig holds\n${got}\nnot\n${want}")
	endif()
endfunction()

run("a,b,d" FALSE "a: recorded [^\n]*a\\.orig\n.*test not deterministic: b\n.*d: run 2 failed.*refused: b. failed: d")
holds(a "${obs}")
holds(b "${old}")
holds(c "${old}")
holds(d "${old}")
run("a" TRUE "1 tests, 2 at a time\n.*a: [^\n]*a\\.orig unchanged")
run("x*" FALSE "no test matches the filter")
message(STATUS "record-target: all runs as expected")
