# results bug-orig-accept on files in WORK (the step of the target
# bug-orig-record that writes ID.orig): equal runs are written, a second
# accept leaves the file unchanged; runs that differ in a section are
# refused naming the test and the section, a wildcard of the committed file
# that a run does not match is refused, a malformed run is an error - and
# in these cases the committed file stays as it was.
#   cmake -DRESULTS=results -DWORK=DIR -P bug_orig_accept.cmake
foreach(v RESULTS WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "bug_orig_accept.cmake: -D${v}=... missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
set(note "# the original\n")
file(WRITE ${WORK}/run1 "${note}[session]\ndone\n[run]\nptr=$1000\nrc=0\n")
file(WRITE ${WORK}/run2 "${note}[session]\ndone\n[run]\nptr=$2000\nrc=0\n")
file(WRITE ${WORK}/bad "junk\n[run]\n")
set(wild "${note}[session]\ndone\n[run]\n# varies: the address of an allocation\nptr=$*\nrc=0\n")
file(WRITE ${WORK}/none "${note}[session]\ndone\n[run]\nnone\nrc=0\n")

# accept(RUN1 RUN2 OLD WANT_RC REGEX EXPECT): OLD the committed file
# (- none), EXPECT what it holds afterwards
function(accept run1 run2 old want re expect)
	set(out ${WORK}/t-id.orig)
	file(REMOVE ${out})
	if(NOT old STREQUAL "-")
		file(WRITE ${out} "${old}")
	endif()
	execute_process(COMMAND ${RESULTS} bug-orig-accept ${WORK}/${run1} ${WORK}/${run2} ${out}
		RESULT_VARIABLE rc OUTPUT_VARIABLE o ERROR_VARIABLE o)
	message(STATUS "${run1} ${run2}: ${rc}\n${o}")
	if(NOT rc EQUAL want OR NOT o MATCHES "${re}")
		message(FATAL_ERROR "bug-orig-accept ${run1} ${run2}: exit status ${rc} (wanted ${want}), "
			"no \"${re}\"")
	endif()
	file(READ ${out} got)
	if(NOT got STREQUAL expect)
		message(FATAL_ERROR "bug-orig-accept ${run1} ${run2}: the file holds\n${got}\nnot\n${expect}")
	endif()
endfunction()

file(READ ${WORK}/run1 r1)
accept(run1 run1 - 0 "t-id: recorded " "${r1}")
accept(run1 run1 "${r1}" 0 "t-id: [^\n]*t-id\\.orig unchanged" "${r1}")
accept(run1 run2 "${r1}" 1 "test not deterministic: t-id\n(.*\n)*t-id: \\[run\\] line 1: run 1 \"ptr=\\$1000\", run 2 \"ptr=\\$2000\"" "${r1}")
accept(run1 run2 "${wild}" 0 "t-id: [^\n]*t-id\\.orig unchanged" "${wild}")
accept(run1 none "${wild}" 1 "t-id: \\[run\\]: the wildcard lines of the recorded original do not match run 2" "${wild}")
accept(bad run1 "${r1}" 2 "bad:1: text before the first \\[section\\]" "${r1}")
message(STATUS "bug_orig_accept: all runs as expected")
