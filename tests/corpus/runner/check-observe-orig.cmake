# ctest corpusrun_observe_orig: corpusrun pass --observe on a bug-orig pass
# (orig=, expect=) is refused before any session, on the host (no emulator).
# It used to end with no configuration at all (the selection dropped the
# reference, which was the observed one) and crash.  A pass file with dummy
# files:
#   --observe clib / ref   -> exit 2, the message, no report (OUT.txt, OUT.work)
#   a pass file without a reference -> exit 2
#   cmake -DCORPUSRUN=... -DWORK=... -P check-observe-orig.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
foreach(f ref clib ted launcher dbug t.bb2)
	file(WRITE ${WORK}/${f} "")
endforeach()
# a recorded original and its expected behaviour (tests/bug-orig/compiler)
file(WRITE ${WORK}/exp "[compile]\nok\n[run]\nlocal 7\nrc=0\n")
file(WRITE ${WORK}/orig "# the original\n[session]\nhalt\n[compile]\nok\n[run]\nlocal wrong\n[blitz2]\nrc=0\n[log]\nBATCH\tend\n")
file(WRITE ${WORK}/bo.pass "work=${WORK}/w\nreport=${WORK}/r.txt\nted=${WORK}/ted\n"
	"launcher=${WORK}/launcher\ndefaultdbug=${WORK}/dbug\n"
	"config=ref|ref|${WORK}/ref|${WORK}/ref|${WORK}/ref\n"
	"config=clib|output|${WORK}/clib|${WORK}/clib|${WORK}/clib\n"
	"source=${WORK}/t.bb2\nexpect=${WORK}/exp\norig=${WORK}/orig\norig_by=blitz2 1.95\n")

foreach(tag clib ref)
	execute_process(COMMAND ${CORPUSRUN} pass --observe ${tag} --to ${WORK}/o ${WORK}/bo.pass
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	message("${out}")
	if(NOT rc EQUAL 2 OR NOT out MATCHES "--observe is not for a bug-orig or expect pass")
		message(FATAL_ERROR "--observe ${tag} on a bug-orig pass: exit ${rc}, expected 2 and the message")
	endif()
	if(EXISTS ${WORK}/o.txt OR EXISTS ${WORK}/o.work OR EXISTS ${WORK}/o)
		message(FATAL_ERROR "--observe ${tag} on a bug-orig pass left a report or work directory")
	endif()
endforeach()

# a refused pass file (no reference configuration): exit 2 and nothing else
# (under HOST_SANITIZE a leaked pass file text made it a LeakSanitizer exit)
file(WRITE ${WORK}/bad.pass "work=${WORK}/w\nreport=${WORK}/r.txt\nted=${WORK}/ted\n"
	"launcher=${WORK}/launcher\ndefaultdbug=${WORK}/dbug\n"
	"config=clib|output|${WORK}/clib|${WORK}/clib|${WORK}/clib\nsource=${WORK}/t.bb2\n")
execute_process(COMMAND ${CORPUSRUN} pass ${WORK}/bad.pass
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
message("${out}")
if(NOT rc EQUAL 2 OR NOT out MATCHES "the first configuration is the reference")
	message(FATAL_ERROR "a pass without reference: exit ${rc}, expected 2 and the message")
endif()
