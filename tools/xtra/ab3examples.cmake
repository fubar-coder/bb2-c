# ctest xtra_ab3_examples: AmiBlitz3 sources from GitHub (commit b1a9dee150ef,
# archive/amiblitz3/github-b1a9dee, cmake/fetch-originals.cmake) through
# xtra - exit 77 (skipped) without them.
#   * all five: the header written again byte for byte (also the three in
#     older layouts: no LabelCase, CreateDbgFile 65535), written as a new
#     header and read again: the same fields
#   * the two in the layout of the current write_AB3Header: also the new
#     header is byte for byte the file's (check --exact)
#   * a header with residents ",all.res", CLI argument, maxima: the dump
#   cmake -DXTRA=... -DREPO=... -P ab3examples.cmake
set(dir ${REPO}/archive/amiblitz3/github-b1a9dee)
set(exact
	Sourcecodes/Amiblitz3/Tests/compilersignals.ab3
	Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/LotanMemLib.ab3)
set(others
	Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/ElmoreDosLib.ab3
	Docs/Blitzlibs/examples/RIAppLib3.ab3
	Sourcecodes/Amiblitz3/Compiler/CLIModeSupport.ab3)
foreach(f IN LISTS exact others)
	if(NOT EXISTS ${dir}/${f})
		message(STATUS "no ${dir}/${f} (cmake -P cmake/fetch-originals.cmake): skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()

function(xtra)
	execute_process(COMMAND ${XTRA} ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(rc)
		message(FATAL_ERROR "xtra ${ARGN}: exit ${rc}\n${out}${err}")
	endif()
	set(out "${out}" PARENT_SCOPE)
endfunction()

list(TRANSFORM exact PREPEND ${dir}/)
list(TRANSFORM others PREPEND ${dir}/)
xtra(check --ab3 ${exact} ${others})
if(NOT out MATCHES "^5 files ok")
	message(FATAL_ERROR "check --ab3: ${out}")
endif()
xtra(check --ab3 --exact ${exact})
xtra(dump --ab3 ${dir}/Sourcecodes/Amiblitz3/Tests/compilersignals.ab3)
foreach(want "\nresident 1 +\"all.res\"\n" "\ncli argument +\"test this args\"\n"
		"\ncursor +line 12, column 17 " "\n  \"Screen\": 5\n" "\n  NumberOfBuilds 15\n")
	if(NOT out MATCHES "${want}")
		message(FATAL_ERROR "compilersignals.ab3: no ${want} in\n${out}")
	endif()
endforeach()
message(STATUS "5 AmiBlitz3 headers read and written again byte for byte")
