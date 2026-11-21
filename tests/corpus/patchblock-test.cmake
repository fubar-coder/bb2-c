# patchblocktest in Amiberry (tests/corpus/amiga/patchblocktest.c): the output
# and the return code of one run, judged against WANT_RC and REGEX.
#   cmake -DAMIGA_RUN=... -DPROGRAM=PATH [-DARGS=nowait] -DWANT_RC=N "-DREGEX=..."
#         -DWORK=DIR -P patchblock-test.cmake
# 77 (skipped) without the emulator.
set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY ${PROGRAM} DESTINATION ${test})
get_filename_component(name ${PROGRAM} NAME)
file(WRITE ${WORK}/run.script
	"TEST:${name} ${ARGS} >TEST:out.txt\nEcho \"rc=$RC\" >>TEST:out.txt\n")
execute_process(COMMAND ${AMIGA_RUN} --timeout 60 --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(FATAL_ERROR "patchblock-test: the emulator run failed (${WORK})")
endif()
set(out "")
if(EXISTS ${test}/out.txt)
	file(READ ${test}/out.txt out)
endif()
message("${out}")
if(NOT out MATCHES "rc=${WANT_RC}[\r\n]*$")
	message(FATAL_ERROR "patchblock-test: the return code is not ${WANT_RC}")
endif()
if(NOT out MATCHES "${REGEX}")
	message(FATAL_ERROR "patchblock-test: the output does not match ${REGEX}")
endif()
message("patchblock-test: as expected")
