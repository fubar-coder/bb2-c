# blitz2:acidlibs - the command libraries, one C file each (NAME.c ->
# NAME.obj), joined in the order of the original file (blitz2.s loads the
# libraries in file order).  A library without C source comes from
# prebuilt/NAME.obj: dhandlerlib (lib 1) is the original, taken from the CD's
# acidlibs, until it is ported.
set(ACIDLIBS_ORDER
	animlib arexxlib audiolib banklib asllib gtlib bblitlib bitmaplib
	blitlib blitzcoplib blitzkeyslib blitzoslib brexxlib cliargslib collslib
	crunchlib displaylib datelib dhandlerlib editlib environlib fadelib
	fileiolib fontlib freqlib gadgetslib gameiolib iffiolib iffmakelib
	ilbmifflib inputoutputlib intuifontlib linklistlib mathtranslib medlib
	palettelib scrolllib memacclib menuslib mouselib polylib printlib
	qblitlib rawkeylib screenslib seriallib shapeslib shapetrixlib 2dlib
	sortlib speaklib spriteslib stencillib stringfunclib trackerlib utillib
	vallib windowslib arrayslib chipbaselib clrlib datalib debuglib
	errtraplib exitslib ffplib floatquicklib intlib ldivlib lmullib
	maxlenlib maxslib memlib modlib rundebuglib runerrlib staticslib
	strcomplib strings1lib strings2lib switchlib varslib wbstartuplib)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
foreach(src ${sources})
	get_filename_component(name ${src} NAME_WE)
	if(NOT name IN_LIST ACIDLIBS_ORDER)
		message(FATAL_ERROR "libs/acidlibs/${name}.c is not in ACIDLIBS_ORDER")
	endif()
endforeach()

set(objs)
foreach(name ${ACIDLIBS_ORDER})
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
		blitz_library(${name} ${name}.c)
		list(APPEND objs ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
	elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/${name}.obj)
		list(APPEND objs ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/${name}.obj)
	else()
		message(FATAL_ERROR "acidlibs: neither ${name}.c nor prebuilt/${name}.obj")
	endif()
endforeach()

# freqlib: the file requester core shared with TED, compiled here with the
# library options; filereq_env.h (this directory) is its environment
target_sources(freqlib PRIVATE ${FILEREQ_DIR}/filereq_core.c)
target_include_directories(freqlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${FILEREQ_DIR})

biglib_join(acidlibs ${BLITZ2_DIR}/acidlibs ${objs})

# the prebuilt libraries are not built here: checked by ctest
file(GLOB prebuilt CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/*.obj)
if(prebuilt)
	add_test(NAME prebuilt_acidlibs COMMAND ${BLITZLIBCHECK} ${prebuilt})
endif()
