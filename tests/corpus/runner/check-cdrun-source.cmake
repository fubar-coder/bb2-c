# ctest cdrun_source: the source= lines of corpusrun cd with broken input, on the
# host (no emulator: the errors come before any session).  A "CD" with one
# tokenised source and dummy compiler files:
#   a source= that is not on the CD   -> exit 2, named, OUT removed
#   source= without OUT               -> exit 2 (pass file error)
#   cmake -DCORPUSRUN=... -DWORK=... -P check-cdrun-source.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/cd/sub ${WORK}/out)
foreach(f blitz2 deflibs acidlibs defaultdbug ted tokens)
	file(WRITE ${WORK}/${f} "")
endforeach()
# (a text source: the selection only looks at the paths)
file(WRITE ${WORK}/cd/sub/a.bb "text\n")
file(WRITE ${WORK}/out/stale.obj "old")

function(pass name text)
	file(WRITE ${WORK}/${name}.pass "work=${WORK}/w\noutput=${WORK}/${name}.tsv\ncd=${WORK}/cd\n"
		"tokens=${WORK}/tokens\nblitz2=${WORK}/blitz2\ndeflibs=${WORK}/deflibs\n"
		"acidlibs=${WORK}/acidlibs\ndefaultdbug=${WORK}/defaultdbug\nted=${WORK}/ted\n${text}")
endfunction()

pass(missing "source=sub/a.bb|${WORK}/out/a.obj\nsource=sub/nosuch.bb|${WORK}/out/stale.obj\n")
execute_process(COMMAND ${CORPUSRUN} cd ${WORK}/missing.pass RESULT_VARIABLE rc OUTPUT_VARIABLE out
	ERROR_VARIABLE out)
message("${out}")
if(NOT rc EQUAL 2 OR NOT out MATCHES "source sub/nosuch.bb is not below")
	message(FATAL_ERROR "a missing source: exit ${rc}, expected 2 and the message")
endif()
if(EXISTS ${WORK}/out/stale.obj)
	message(FATAL_ERROR "the OUT of an earlier run is still there")
endif()

pass(noout "source=sub/a.bb\n")
execute_process(COMMAND ${CORPUSRUN} cd ${WORK}/noout.pass RESULT_VARIABLE rc OUTPUT_VARIABLE out
	ERROR_VARIABLE out)
message("${out}")
if(NOT rc EQUAL 2 OR NOT out MATCHES "source=REL\\|OUT")
	message(FATAL_ERROR "source= without OUT: exit ${rc}, expected 2 and the message")
endif()
