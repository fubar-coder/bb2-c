# A stand-in for emulator.cmake in the test of expected.cmake
# (expected-target.cmake): no emulator; the "program" PROGRAM is a text file,
# written as the run's cases to WORK/test/captures/prog.txt with @PAD@
# replaced by LIBS_PAD (0 if not set), @LIBSET@ by LIBSET and @USERLIBS@
# by the number of entries of USERLIBS. A PROGRAM named skip ends the run
# with 77 (skipped), one named fail with 1.
cmake_minimum_required(VERSION 4.2.3)

get_filename_component(n ${PROGRAM} NAME)
if(n STREQUAL "skip")
	cmake_language(EXIT 77)
elseif(n STREQUAL "fail")
	message(FATAL_ERROR "fake-emulator: the run failed")
endif()
if(NOT LIBS_PAD)
	set(LIBS_PAD 0)
endif()
file(READ ${PROGRAM} text)
string(REPLACE "@PAD@" "${LIBS_PAD}" text "${text}")
string(REPLACE "@LIBSET@" "${LIBSET}" text "${text}")
list(LENGTH USERLIBS userlibs)
string(REPLACE "@USERLIBS@" "${userlibs}" text "${text}")
file(REMOVE_RECURSE ${WORK})
file(WRITE ${WORK}/test/captures/prog.txt "${text}")
