# api-doxygen-test.cmake - runs Doxygen as the Pages workflow does (podman,
# the pinned image, Doxyfile, warnings are errors) but out of tree, so that
# a comment error breaks here before it breaks the publication:
#
#   cmake -DSOURCE=<repo> -DCASE=real|broken -DWORK=<dir>
#         -P api-doxygen-test.cmake
#
# The source tree is mounted read-only; the Doxyfile goes in on stdin with
# OUTPUT_DIRECTORY overridden (a later assignment wins). CASE real: the
# whole input must give exit code 0 and no output. CASE broken: the input
# is site/tests/api-doxygen/bad-param.c, and the run must fail naming the
# bad @param. Exits 77 (skipped) without podman or the image.
set(IMAGE ghcr.io/doxygen/doxygen:Release_1_16_1)
foreach(v SOURCE CASE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "api-doxygen-test: ${v} is needed")
	endif()
endforeach()
find_program(PODMAN podman)
if(NOT PODMAN)
	message(STATUS "skipped: no podman")
	cmake_language(EXIT 77)
endif()
execute_process(COMMAND ${PODMAN} image exists ${IMAGE} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
	message(STATUS "skipped: no image ${IMAGE} (podman pull ${IMAGE})")
	cmake_language(EXIT 77)
endif()
file(MAKE_DIRECTORY ${WORK}/out)
file(READ ${SOURCE}/Doxyfile doxyfile)
string(APPEND doxyfile "\nOUTPUT_DIRECTORY = /out\n")
if(CASE STREQUAL "broken")
	string(APPEND doxyfile "INPUT = site/tests/api-doxygen/bad-param.c\n")
endif()
file(WRITE ${WORK}/Doxyfile "${doxyfile}")
execute_process(COMMAND ${PODMAN} run --rm -i
		-v ${SOURCE}:/src:ro,Z -v ${WORK}/out:/out:Z -w /src ${IMAGE} -
	INPUT_FILE ${WORK}/Doxyfile
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(CASE STREQUAL "real")
	if(NOT rc EQUAL 0 OR out MATCHES "(warning|error)")
		message(FATAL_ERROR "Doxygen does not run cleanly (${rc}):\n${out}")
	endif()
elseif(rc EQUAL 0)
	message(FATAL_ERROR "the broken comment passed:\n${out}")
elseif(NOT out MATCHES "argument 'x' of command @param is not found")
	message(FATAL_ERROR "the broken comment failed without the expected message:\n${out}")
else()
	message(STATUS "broken: failed as expected")
endif()
