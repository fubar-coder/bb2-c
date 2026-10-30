# ctest xtra_originals: the real files through xtra (shared/xtra) - exit 77
# (skipped) without the CD or the Aminet archive (cmake/fetch-originals.cmake).
#   * all 575 .xtra of the CD (docs/formats.md 1.2): read without error and
#     written again byte for byte (xtra check)
#   * all .xtra of rwelibsrc (Aminet amibl2src) the same
#   * the 9 BB22 sources of rwelibsrc (docs/rwelibs/README.md 4.1): their
#     header read, merged with the .xtra next to them (a .bak with the one
#     of its source), the cursor as in the README's table
#   cmake -DXTRA=... -DREPO=... -P originals.cmake
set(cd ${REPO}/archive/cd/ultimate-blitz-basic-2.1)
set(rwe ${REPO}/archive/aminet/amibl2src/rwelibsrc)
foreach(d ${cd} ${rwe})
	if(NOT IS_DIRECTORY ${d})
		message(STATUS "no ${d} (cmake -P cmake/fetch-originals.cmake): skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()

function(xtra)
	execute_process(COMMAND ${XTRA} ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(rc)
		message(FATAL_ERROR "xtra: exit ${rc}\n${out}${err}")
	endif()
	set(out "${out}" PARENT_SCOPE)
endfunction()

# the CD: ISO 9660 names, upper case
file(GLOB_RECURSE files ${cd}/*.XTRA)
list(LENGTH files n)
if(NOT n EQUAL 575)
	message(FATAL_ERROR "CD: ${n} .XTRA files, 575 expected")
endif()
xtra(check ${files})
string(STRIP "${out}" out)
message(STATUS "CD: ${out}")

file(GLOB_RECURSE files ${rwe}/*.xtra)
list(LENGTH files n)
if(n LESS 100)
	message(FATAL_ERROR "rwelibsrc: only ${n} .xtra files")
endif()
xtra(check ${files})
string(STRIP "${out}" out)
message(STATUS "rwelibsrc: ${out}")

# BB22 source | line column top (README 4.1, 0-based)
foreach(f
		"ANIMLib/Anim-Library3.bb|97 19 96" "ANIMLib/Anim-Library3.bb.bak|94 9 49"
		"AppLib/applib3.bb|466 13 441" "AppLib/applib3.bb.bak|449 20 441"
		"Registry/Registry3.bb|625 0 571" "rexxlib/NEWAREXXLIB.bb|612 0 606"
		"shapeslib/RIShapesLib2.bb|177 8 170"
		"soundlib/risoundlib2.bb|347 13 315" "soundlib/risoundlib2.bb.bak|347 13 315")
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 src)
	list(GET f 1 want)
	string(REPLACE " " ";" want "${want}")
	list(GET want 0 line)
	list(GET want 1 column)
	list(GET want 2 top)
	string(REGEX REPLACE "\\.bak$" "" base ${src})
	xtra(check --bb22 ${rwe}/${src})
	xtra(dump ${rwe}/${base}.xtra --bb22 ${rwe}/${src})
	if(NOT out MATCHES "\ncursor +line ${line}, column ${column} " OR NOT out MATCHES "\ntop line +${top} ")
		message(FATAL_ERROR "${src}: not line ${line}, column ${column}, top ${top}:\n${out}")
	endif()
endforeach()
message(STATUS "rwelibsrc: 9 BB22 sources with their .xtra, cursor as documented")
