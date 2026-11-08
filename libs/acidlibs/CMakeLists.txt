# blitz2:acidlibs - the command libraries, one C file each (NAME.c ->
# NAME.obj), joined in the order of the original file (blitz2.s loads the
# libraries in file order).
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
		if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
			message(FATAL_ERROR "acidlibs: no ${name}.c")
		endif()
		# a variant's library files in variants/ (not next to the default
		# ones: libs_emulator takes every NAME.obj of this directory)
		set(obj ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
		if(prefix)
			set(obj ${CMAKE_CURRENT_BINARY_DIR}/variants/${prefix}${name}.obj)
		endif()
		# the default variant's: checked against the command reference
		set(cmdref CMDREF acidlibs/${name})
		if(prefix OR NOT BLITZ_DEFAULT_VARIANT STREQUAL CMDREF_VARIANT)
			set(cmdref)
		endif()
		blitz_library(${prefix}${name} VARIANT ${variant} OBJ ${obj} ${cmdref} ${name}.c)
		list(APPEND objs ${obj})
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

# asm checks: the routines that stay assembler, compared with the original
# libraries of the CD (BLITZ2/ACIDLIBS, by library number) instruction by
# instruction (refcheck asm, tools/refcheck, with the toolchain's objdump; skipped without
# the CD).  wbstartuplib, intlib and switchlib are C (intlib's only
# assembler is the `Dc $a001` prefixes, checked by declarations_acidlibs),
# so only runerrlib, debuglib, dhandlerlib and memacclib have routines
# here.  debuglib's
# handleflags state machine (and executestate behind it) is left out: it
# is being rewritten in C.
# Arguments NAME:ORIGADDR:CSYMBOL:COUNT; routines without a global symbol of
# their own are given as an offset from the nearest one (CSYMBOL SYM+HEX,
# HEX may be negative), taken from the original's layout.
# Only routines still equal to the original are listed: a fix of an
# original bug that changes a listed routine removes its argument from the
# list in the same commit (and names it in the commit message); there is
# no list of intended differences for instructions.
function(asm_check lib libnum)
	add_test(NAME asmcmp_${lib}
		COMMAND ${REFCHECK} asm ${CMAKE_OBJDUMP}
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
	docall:bd2:debuglib_docall:3)

# dhandlerlib: all code is assembler (the process segment template
# mystart with off_1/off_2, the ltable stubs, init/finit, the VBlank
# server check_c).  Only _dhandlerlib_codeinit/_dhandlerlib_codefinit
# are global:
#   mystart code = codeinit-$94, l0 = codeinit-$4e, check_c = codeinit+$90
#   codefinit includes the trailing MOVEQ #0,d0:RTS (program end)
asm_check(dhandlerlib 1
	mystart:9c:dhandlerlib_codeinit+-94:7
	ltable_stubs:e2:dhandlerlib_codeinit+-4e:19
	codeinit:130:dhandlerlib_codeinit:35
	check_c:1c0:dhandlerlib_codeinit+90:18
	codefinit:20a:dhandlerlib_codefinit:21)

# memacclib: Call (JMP with every register passed through).  The inline
# routines of mathtranslib are compared byte by byte in the emulator tests
# (still in bb2: systest_cmdlibs).
asm_check(memacclib 180
	docall:186:memacclib_docall:2)
