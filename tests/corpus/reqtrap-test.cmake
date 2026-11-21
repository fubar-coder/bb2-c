# reqtraptest (tests/corpus/amiga/reqtraptest.c) in Amiberry, run through a
# launcher (reqtrap, reqclose, inject): the output and the return code of one
# run, judged against WANT_RC and REGEX.
#   cmake -DAMIGA_RUN=... -DLAUNCHER=PATH -DPROGRAM=PATH [-DPROGRAM_ARGS=ARGS]
#         -DWANT_RC=N "-DREGEX=..." -DWORK=DIR -P reqtrap-test.cmake
# The command is "LAUNCHER PROGRAM ARGS" with both copied to TEST:.  For a
# longer command: -DFILES=PATH|PATH... (copied) -DCMDLINE=... in which <NAME>
# stands for TEST:NAME of a copied file.  -DWANT_TIMEOUT=1: the run is
# meant not to end (a hang is what a broken twin shows): amiga_run ends it
# after TIMEOUT seconds (default 60), the output of that run is judged
# against REGEX alone.
# 77 (skipped) without the emulator.
set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
if(NOT DEFINED FILES)
	set(FILES "${LAUNCHER}|${PROGRAM}")
	get_filename_component(l ${LAUNCHER} NAME)
	get_filename_component(p ${PROGRAM} NAME)
	set(CMDLINE "<${l}> <${p}> ${PROGRAM_ARGS}")
endif()
string(REPLACE "|" ";" files "${FILES}")
foreach(f ${files})
	file(COPY ${f} DESTINATION ${test})
	get_filename_component(n ${f} NAME)
	string(REPLACE "<${n}>" "TEST:${n}" CMDLINE "${CMDLINE}")
endforeach()
file(WRITE ${WORK}/run.script
	"${CMDLINE} >TEST:out.txt\nEcho \"rc=$RC\" >>TEST:out.txt\n")
if(NOT DEFINED TIMEOUT)
	set(TIMEOUT 60)
endif()
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(WANT_TIMEOUT)
	if(NOT rc EQUAL 1)
		message(FATAL_ERROR "reqtrap-test: the run was to end by the time limit, exit status ${rc} (${WORK})")
	endif()
elseif(rc)
	message(FATAL_ERROR "reqtrap-test: the emulator run failed (${WORK})")
endif()
set(out "")
if(EXISTS ${test}/out.txt)
	file(READ ${test}/out.txt out)
endif()
message("${out}")
if(NOT WANT_TIMEOUT AND NOT out MATCHES "rc=${WANT_RC}[\r\n]*$")
	message(FATAL_ERROR "reqtrap-test: the return code is not ${WANT_RC}")
endif()
if(NOT out MATCHES "${REGEX}")
	message(FATAL_ERROR "reqtrap-test: the output does not match ${REGEX}")
endif()
message("reqtrap-test: as expected")
