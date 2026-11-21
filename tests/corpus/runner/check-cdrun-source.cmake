# ctest cdrun_source: the source= lines of corpusrun cd with broken input, on the
# host (no emulator: the errors come before any session).  A "CD" with one
# tokenised source and dummy compiler files:
#   a source= that is not on the CD   -> exit 2, named, OUT removed
#   source= without OUT               -> exit 2 (pass file error)
#   current=, test=, intended= (the comparison with the prev build) in a wrong combination
#                                     -> exit 2
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

# the comparison with the previous commit (current=, test=, intended=) with broken pass
# files: each is an error (exit 2) before any session
function(expect_error name text regex)
	pass(${name} "${text}")
	execute_process(COMMAND ${CORPUSRUN} cd ${WORK}/${name}.pass RESULT_VARIABLE rc
		OUTPUT_VARIABLE out ERROR_VARIABLE out)
	message("${out}")
	if(NOT rc EQUAL 2 OR NOT out MATCHES "${regex}")
		message(FATAL_ERROR "${name}: exit ${rc}, expected 2 and \"${regex}\"")
	endif()
endfunction()
expect_error(cur_notest "current=${WORK}/cur.tsv\nreport=${WORK}/r.txt\n"
	"current= and test= belong together")
expect_error(test_nocur "test=corpus_cd_prev\n" "current= and test= belong together")
expect_error(intended_nocur "intended=corpus_cd_prev:A/B.BB\n" "intended= needs them")
expect_error(cur_noreport "current=${WORK}/cur.tsv\ntest=corpus_cd_prev\n" "report with reference or current")
expect_error(cur_and_ref "current=${WORK}/cur.tsv\ntest=corpus_cd_prev\nreport=${WORK}/r.txt\nreference=${WORK}/ref.tsv\n"
	"reference= and current= exclude each other")
expect_error(intended_unknown "current=${WORK}/cur.tsv\ntest=corpus_cd_prev\nreport=${WORK}/r.txt\nintended=nosuch:A\n"
	"unknown test")
expect_error(intended_library "current=${WORK}/cur.tsv\ntest=corpus_cd_prev\nreport=${WORK}/r.txt\nintended=libs/x.obj\n"
	"is not for the CD")
