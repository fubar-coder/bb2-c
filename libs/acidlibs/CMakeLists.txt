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

# the libraries and acidlibs of a variant (VARIANT: its directory, with
# its libnums.h): target names PREFIX + name ("" for the default variant)
function(acidlibs_build prefix variant output)
	set(objs)
	foreach(name ${ACIDLIBS_ORDER})
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
			# a variant's library files in variants/ (not next to the default
			# ones: libs_emulator takes every NAME.obj of this directory)
			set(obj ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
			if(prefix)
				set(obj ${CMAKE_CURRENT_BINARY_DIR}/variants/${prefix}${name}.obj)
			endif()
			blitz_library(${prefix}${name} VARIANT ${variant} OBJ ${obj} ${name}.c)
			list(APPEND objs ${obj})
		elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/${name}.obj)
			list(APPEND objs ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/${name}.obj)
		else()
			message(FATAL_ERROR "acidlibs: neither ${name}.c nor prebuilt/${name}.obj")
		endif()
	endforeach()

	# freqlib: the file requester core shared with TED, compiled here with
	# the library options; filereq_env.h (this directory) is its environment
	target_sources(${prefix}freqlib PRIVATE ${FILEREQ_DIR}/filereq_core.c)
	target_include_directories(${prefix}freqlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${FILEREQ_DIR})

	biglib_join(${prefix}acidlibs ${output} ${objs})
endfunction()

acidlibs_build("" ${BLITZ_DEFAULT_VARIANT_DIR} ${BLITZ2_DIR}/acidlibs)
foreach(build IN LISTS BLITZ_VARIANT_BUILDS)
	string(REPLACE "|" ";" build "${build}")
	list(GET build 0 dir)
	list(GET build 1 out)
	get_filename_component(v ${dir} NAME)
	acidlibs_build(${v}- ${dir} ${out}/acidlibs)
endforeach()

# the prebuilt libraries are not built here: checked by ctest
file(GLOB prebuilt CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/*.obj)
if(prebuilt)
	add_test(NAME prebuilt_acidlibs COMMAND ${BLITZLIBCHECK} ${prebuilt})
endif()

# asm checks: the routines that stay assembler, compared with the original
# libraries of the CD (BLITZ2/ACIDLIBS, by library number) instruction by
# instruction (tools/asmcmp with the toolchain's objdump; skipped without
# the CD).  wbstartuplib, intlib and switchlib are C (intlib's only
# assembler is the `Dc $a001` prefixes, checked by declarations_acidlibs),
# so only runerrlib, debuglib and memacclib have routines here.  debuglib's
# handleflags state machine (and executestate behind it) is left out: it
# is being rewritten in C.
# Arguments NAME:ORIGADDR:CSYMBOL:COUNT; routines without a global symbol of
# their own are given as an offset from the nearest one (CSYMBOL SYM+HEX,
# HEX may be negative), taken from the original's layout.
function(asm_check lib libnum)
	add_test(NAME asmcmp_${lib}
		COMMAND ${ASMCMP} ${CMAKE_OBJDUMP}
			${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS ${libnum}
			${CMAKE_CURRENT_BINARY_DIR}/${lib}.linked ${ARGN})
	set_tests_properties(asmcmp_${lib} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()

# runerrlib: checks on a7 / the caller's stack / the N flag, the trap
# handler, the error chain (overerr..currenterr, arrerr-$28 = overerr)
asm_check(runerrlib 65500
	finalscheck:604:finalscheck:3
	stackchk:6d6:stackchk:5
	retchk:6ea:retchk:5
	nextchkw:6fe:nextchkw:3
	nextchkl:70a:nextchkl:3
	casechkw:716:casechkw:3
	casechkl:722:casechkl:3
	bmodechk:6ca:bmodechk:2
	amodechk:6d0:amodechk:2
	trapper:748:trapper:13
	errchain:59c:arrerr+-28:26)

# debuglib: trap handlers, default handlers, evaluate/execute (up to the
# TRAP #15 parameter block), dirdone/touser2, output jump table, docall.
#   mytrap1: only the handler itself (up to its RTE), not touser /
#            handleflags / executestate
#   evaluate = astring+$11E (astring 272, "Print " 8, dirmode 2, oldouth 4)
#   dirdone  = doutx-$5A
asm_check(debuglib 64060
	handlers_:4fc:statehandler_:7
	mytrap4:50e:mytrap4:23
	mytrap5:55a:mytrap5:16
	mytrap2:5cc:mytrap2:15
	mytrap3:602:mytrap3:2
	mytrap1:60a:mytrap1:16
	mytrap0:7ec:mytrap0:30
	evaluate:98c:astring+11e:29
	dirdone:a10:doutx+-5a:21
	douttab:a6c:douthandler+-8:2
	docall:bd2:docall:3)

# memacclib: Call (JMP with every register passed through).  The inline
# routines of mathtranslib are compared byte by byte in the emulator tests
# (still in bb2: systest_cmdlibs).
asm_check(memacclib 180
	docall:186:docall:2)
