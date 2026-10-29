# ctest rwe_refbuild_NAME: the reference build of an entry's SOURCE (REF,
# ref/NAME.obj of ctest rwe_refbuild) equals its REFERENCE binary (ORIG)
# byte for byte (docs/rwelibs/README.md 3.1).  77 (skipped) if one of them
# is missing: no CD, or rwe_refbuild was skipped (no Amiberry).
#   cmake -DREF=... -DORIG=... -P refbuild-compare.cmake
foreach(f ORIG REF)
	if(NOT EXISTS ${${f}})
		message(STATUS "no ${${f}}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
file(SIZE ${REF} a)
file(SIZE ${ORIG} b)
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${REF} ${ORIG} RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "${REF} (${a} bytes) differs from ${ORIG} (${b} bytes)")
endif()
message(STATUS "${REF}: ${a} bytes, the same as ${ORIG}")
