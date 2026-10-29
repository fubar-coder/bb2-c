# ctest bb22_rwelibsrc: the 9 BB22 files of rwelibsrc (docs/rwelibs/README.md
# 4.1) through bb22 into TED's format (OUTDIR/NAME, '/' of the path as
# '_'), and every result read by teddetok --check with the token list of
# the CD's compiler, acidlibs, deflibs and the RI libraries in LIBS (all
# tokens known, no token byte without its second byte).  Exit 77
# (skipped) without the Aminet archive or the CD.
#   cmake -DBB22=... -DTEDDETOK=... -DREPO=... -DOUTDIR=... -P rwelibsrc.cmake
set(dir ${REPO}/archive/aminet/amibl2src/rwelibsrc)
set(cd ${REPO}/archive/cd/ultimate-blitz-basic-2.1)
set(files
	ANIMLib/Anim-Library3.bb ANIMLib/Anim-Library3.bb.bak
	AppLib/applib3.bb AppLib/applib3.bb.bak
	Registry/Registry3.bb
	rexxlib/NEWAREXXLIB.bb
	shapeslib/RIShapesLib2.bb
	soundlib/risoundlib2.bb soundlib/risoundlib2.bb.bak)
# the libraries whose tokens the sources have besides acidlibs/deflibs
set(bss2 ${cd}/BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS)
set(libs ${bss2}/RIPACKLIB.OBJ)   # Pack (73): ANIMLib, RIShapesLib2
set(tokens --blitz2 ${cd}/BLITZ2/BLITZ2 --lib ${cd}/BLITZ2/ACIDLIBS --lib ${cd}/BLITZ2/DEFLIBS)
foreach(l IN LISTS libs)
	list(APPEND tokens --lib ${l})
endforeach()
foreach(f ${cd}/BLITZ2/BLITZ2 ${cd}/BLITZ2/ACIDLIBS ${cd}/BLITZ2/DEFLIBS ${libs})
	if(NOT EXISTS ${f})
		message(STATUS "no ${f} (cmake -P cmake/fetch-originals.cmake): skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
file(MAKE_DIRECTORY ${OUTDIR})
set(outs)
foreach(f IN LISTS files)
	if(NOT EXISTS ${dir}/${f})
		message(STATUS "no ${dir}/${f} (cmake -P cmake/fetch-originals.cmake): skipped")
		cmake_language(EXIT 77)
	endif()
	string(REPLACE "/" "_" o ${f})
	set(o ${OUTDIR}/${o})
	file(REMOVE ${o})
	execute_process(COMMAND ${BB22} ${dir}/${f} ${o} RESULT_VARIABLE rc)
	if(rc OR NOT EXISTS ${o})
		message(FATAL_ERROR "bb22 ${f}: exit ${rc}")
	endif()
	list(APPEND outs ${o})
endforeach()
execute_process(COMMAND ${TEDDETOK} ${tokens} --check ${outs} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "teddetok: exit ${rc}")
endif()
list(LENGTH files n)
message(STATUS "${n} BB22 files of ${dir}: converted, all tokens known")
