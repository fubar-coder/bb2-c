# ctest rwe_lesfont_cd: the font of the LES debug library
# (rilesdebuglib-font.bin, cut from AmiBlitz3's RILESDEBUGLib.obj at 4566,
# archive/README.md) equals the 2048 bytes at OFFSET of BIN - the CD's
# BUM RILESDEBUGLIB.OBJ at 3460.  77 (skipped) without BIN (no CD).
#   cmake -DFONT=rilesdebuglib-font.bin -DBIN=... -DOFFSET=3460 -P lesfont-check.cmake
foreach(v FONT BIN OFFSET)
	if("${${v}}" STREQUAL "")
		message(FATAL_ERROR "lesfont-check.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT EXISTS ${BIN})
	message(STATUS "no ${BIN}: skipped")
	cmake_language(EXIT 77)
endif()
file(SIZE ${FONT} size)
if(NOT size EQUAL 2048)
	message(FATAL_ERROR "${FONT}: ${size} bytes, not 2048")
endif()
file(READ ${FONT} font HEX)
file(READ ${BIN} bin OFFSET ${OFFSET} LIMIT 2048 HEX)
if(NOT font STREQUAL bin)
	message(FATAL_ERROR "${FONT} differs from the 2048 bytes at ${OFFSET} of ${BIN}")
endif()
message(STATUS "${FONT}: the same as the bytes at ${OFFSET} of ${BIN}")
