# The libraries of the sources Red When Excited released in September 1999
# (archive/aminet/amibl2src/rwelibsrc, docs/rwelibs/README.md), as
# userlibs of the variant rwe-1999: NAME.c -> userlibs/rwe/NAME.obj (of
# the variant; of its own variant for RWE_OWN_VARIANTS).
#
# RWE_LIBS: NAME|SOURCE|REFERENCE per library - SOURCE in rwelibsrc (the
# Blitz source the C file translates), REFERENCE the binary below archive/
# (fetch-originals: the CD, AmiBlitz3's SVN) whose state the port has: the
# one the reference build of SOURCE with blitz2 2.10 equals byte for byte
# (docs/rwelibs/README.md 3.1), or the newer binary chosen in 3.2 (then
# SOURCE is the closest source); or "refbuild": the reference build of
# SOURCE itself (refbuild.cmake: ref/NAME.obj, made by ctest rwe_refbuild
# in Amiberry).  Every library is built from NAME.c.
#
# Target rwelibsrc: every SOURCE as TED shows it (like bblibsrc), in
# build/<preset>/rwelibsrc/SOURCE, with the tokens of the files of the
# library's variant - blitz2, the variant's acidlibs and deflibs, all RWE
# userlibs of RWE_VARIANT built here and those of its own variant (the
# tokens of ALibJsr calls into other RWE libraries or its own).  So it
# needs only the Aminet archive, not the CD.  It depends on the userlibs;
# their build does not depend on it.
# The C build is real C (assembler only where needed), checked like the
# acidlibs: the declarations equal REFERENCE's (ctest
# rwe_declarations_NAME, skipped without the originals), and the
# behaviour is compared with REFERENCE in Amiberry (libs_emulator:
# libs/tests/rwe/NAME.c, the property RWE_EMULATOR_LIBS lists obj|orig).
set(RWE_VARIANT rwe-1999)
# RWE_OWN_VARIANTS: NAME|VARIANT[|REPLACES] for a library whose number
# another library of RWE_VARIANT has: built with the numbers of VARIANT
# (which includes RWE_VARIANT's libnums.h and frees the number), checked
# against VARIANT's acidlibs and deflibs (ctest rwe_numbers_VARIANT).
# REPLACES: an RWE library of RWE_VARIANT with the same number that
# VARIANT's set has instead of NAME's (VARIANT's numbers can stay
# RWE_VARIANT's)
set(RWE_OWN_VARIANTS "rievallib|rwe-1999-eval" "risoundlib2|rwe-1999-ahi|risoundlib")
set(bss2 cd/ultimate-blitz-basic-2.1/BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS)
set(ol cd/ultimate-blitz-basic-2.1/EXTRAS/BLITZLIBS/OTHERLIBS)
set(bum cd/ultimate-blitz-basic-2.1/BUM10A/BLITZLIBS/RILIBS)
set(ab3 amiblitz3/svn-r279/BlitzLibs/UserLibs/userlibs)
set(RWE_LIBS
	"riencryptlib|Encryptlib/Encrypt-Library.bb|${bss2}/RIENCRYPTLIB.OBJ"
	"rinonvolatilelib|nonvolatile.bb|${bss2}/RINONVOLATILELIB.OBJ"
	"rireqlib|reqlib/ReqLib2.bb|${ol}/RIREQLIB.OBJ"
	"ridebuglib|Debuglib/Debuglib4.bb|${ol}/RIDEBUGLIB.OBJ"
	"ricopperfxlib|Copperlib/CopperFX3.bb|${bss2}/RICOPPERFXLIB.OBJ"
	"ritrackdisklib|trackdisklib/trackdisk2.bb|${bss2}/RITRACKDISKLIB.OBJ"
	"rifastseriallib|FASTSerial/fastseriallib3.bb|${bss2}/RIFASTSERIALLIB.OBJ"
	"rigfxlib|GFXLib/GFX-Library2.bb|${bss2}/RIGFXLIB.OBJ"
	"ridoslib|DOSLib/ridoslib2.bb|${bss2}/RIDOSLIB.OBJ"
	"ripacklib|packlib/Pack-Library8.bb|${bum}/RIPACKLIB.OBJ"
	"ricommoditieslib|Commoditieslib/CommoditiesLib2.bb|${bss2}/RICOMMODITIESLIB.OBJ"
	"riapplib|AppLib/applib2.bb|${bss2}/RIAPPLIB.OBJ"
	"ritooltypeslib|tooltypeslib/ToolTypes4.bb|${bss2}/RITOOLTYPESLIB.OBJ"
	"rizonejoylib|zonejoylib/ZoneJoy-Library4.bb|${bss2}/RIZONEJOYLIB.OBJ"
	"rifxlib|FXLib/FX-Library13.bb|${bss2}/RIFXLIB.OBJ"
	# stage B (docs/rwelibs/README.md 3.2): the AmiBlitz3 SVN binary =
	# BSS2 plus fixes of the AmiBlitz3 team; func-library7 is the closest
	# source
	"riamosfunclib|AMOSFunclib/func-library7.bb|${ab3}/RIAmosFuncLib.obj"
	# stage B, the BSS2 binary newer than the source (3.2): the port has
	# the binary's state
	"risoundlib|soundlib/risoundlib.bb|${bss2}/RISOUNDLIB.OBJ"
	"ristonecrackerlib|stclib.bb|${bss2}/RISTONECRACKERLIB.OBJ"
	"rifnslib|FNSLib/fns-library11.bb|${bss2}/RIFNSLIB.OBJ"
	# the source does not compile (its RIDiskAnimInit is unfinished, see
	# RWE_REFBUILD_SKIP); the BSS2 binary is the source without it
	"rianimlib|ANIMLib/Anim-Library3.bb|${bss2}/RIANIMLIB.OBJ"
	# stages B/C, the source newer than the binaries (3.2 "Source"): the
	# reference build of the source
	"rilvcallhooklib|lvcallhook/LVCallHook3.bb|refbuild"
	"rimisclib|misclib/misclib2.bb|refbuild"
	"ristcdecrunchlib|stcdecrunchlib.bb|refbuild"
	"ricompactdisclib|CDlib/cdSCSI.bb|refbuild"
	"rigtmenulib|GTMenuLib/GTMenuLib3.bb|refbuild"
	"rilinkedlistlib|Listlib/listlib4.bb|refbuild"
	"neilsreqtoolslib|RTLib.bb|refbuild"
	"ridatatypeslib|datatypeslib6.bb|refbuild"
	"ciatrklib|ptlib/CIATrackerLib1.bb|refbuild"
	"ritrackerlib|ptlib/ptlib6.bb|refbuild"
	"riarexxlib|rexxlib/NEWAREXXLIB.bb|refbuild"
	"rishapeslib|shapeslib/RIShapesLib2.bb|refbuild"
	"risortlib|sortlib/RISortlib2.bb|refbuild"
	"ridisasmlib|Disasmlib/Disasm-library.bb2|refbuild"
	# the font of its IncBin: rilesdebuglib-font.bin (from AmiBlitz3)
	"rilesdebuglib|LESDebuglib/LESDEBUGlib.bb|refbuild"
	# the BSS2 binary newer than the source (3.2), number 121 = the
	# acidlibs' polylib: variant rwe-1999-eval (RWE_OWN_VARIANTS)
	"rievallib|EvalLib4.bb|${bss2}/RIEVALLIB.OBJ"
	# stage D (3.3): no binary anywhere; the reference build of the
	# newest source (docs/rwelibs/README.md 2, column Reference)
	"riregistrylib|Registry/Registry3.bb|refbuild"
	"rimapshapeslib|MapEd/MapShape2.bb|refbuild"
	"riarrayslib|ArraysLib.bb|refbuild"
	"rimapeditorlib|MapEd/MapEd_Bit2.bb|refbuild"
	"riwbspritelib|wbspritelib/WBSprite-library2.bb|refbuild"
	"riintuilib|riintuilib2.bb|refbuild"
	"popupwinlib|popuplib/popupwinlib.bb|refbuild"
	"bb_blitlib|blitzbomberslib/BB_BlitLib4.bb|refbuild"
	"rishapezlib|shapezlib.bb|refbuild"
	"riseriallib|seriallib/Serial-Library.bb|refbuild"
	"richunkylib|chunkylib.bb|refbuild"
	"ridisplaylib|mydisplaylib.bb|refbuild"
	"blitzbomberslibtwo|blitzbomberslib/blitzbombersTwo00.bb|refbuild"
	"ribbmaplib|blitzbomberslib/bbmaplib2.bb|refbuild"
	"rithreadlib|threadlib.bb|refbuild"
	"risnowlib|Snow/SnowLib3.bb|refbuild"
	# risoundlib's successor (number 45, AHI; resident AHI.res): no
	# binary; variant rwe-1999-ahi, where it replaces risoundlib
	# (RWE_OWN_VARIANTS)
	"risoundlib2|soundlib/risoundlib2.bb|refbuild")

set(variant_dir ${PROJECT_SOURCE_DIR}/libs/variants/${RWE_VARIANT})
set(out ${BLITZ_VARIANTS_DIR}/${RWE_VARIANT})
set(archive ${PROJECT_SOURCE_DIR}/archive)
set(rwesrc ${PROJECT_SOURCE_DIR}/archive/aminet/amibl2src/rwelibsrc)

# rwe_ted_source(SOURCE OUT_VAR): rwelibsrc/SOURCE in TED's format - the
# file itself, or for a source in the BB22 format (docs/rwelibs/README.md
# 4.1; neither blitz2 nor TED reads it) its conversion by tools/bb22,
# ted/SOURCE here (one command per source, for the readable source and
# the reference build alike)
function(rwe_ted_source source out)
	set(f ${rwesrc}/${source})
	file(READ ${f} magic LIMIT 4 HEX)
	if(NOT magic STREQUAL "42423232")
		set(${out} ${f} PARENT_SCOPE)
		return()
	endif()
	set(ted ${CMAKE_CURRENT_BINARY_DIR}/ted/${source})
	get_property(done DIRECTORY PROPERTY RWE_TED_SOURCES)
	if(NOT source IN_LIST done)
		set_property(DIRECTORY APPEND PROPERTY RWE_TED_SOURCES ${source})
		get_filename_component(dir ${ted} DIRECTORY)
		add_custom_command(OUTPUT ${ted}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
			COMMAND ${BB22} ${f} ${ted}
			DEPENDS ${f} ${BB22} host_tools
			COMMENT "rwelibsrc/${source}: BB22 to TED's format"
			VERBATIM)
	endif()
	set(${out} ${ted} PARENT_SCOPE)
endfunction()

# the residents the sources load that exist nowhere, from the NDK; the
# reference build of the sources with blitz2 2.10 (REFERENCE "refbuild":
# rwe_reference() there)
include(${CMAKE_CURRENT_SOURCE_DIR}/residents.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/refbuild.cmake)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
set(names)
foreach(l IN LISTS RWE_LIBS)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(APPEND names ${name})
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
		message(FATAL_ERROR "RWE_LIBS: ${name} has no libs/userlibs/rwe/${name}.c")
	endif()
endforeach()
foreach(src IN LISTS sources)
	get_filename_component(name ${src} NAME_WE)
	if(NOT name IN_LIST names)
		message(FATAL_ERROR "libs/userlibs/rwe/${name}.c is not in RWE_LIBS")
	endif()
endforeach()

set(objs)
set(own_variants)
set(readable_entries)    # SOURCE|VARIANT for rwelibsrc
foreach(l IN LISTS RWE_LIBS)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(GET l 1 source)
	list(GET l 2 reference)
	# the variant: RWE_VARIANT or the library's own (RWE_OWN_VARIANTS)
	set(v ${RWE_VARIANT})
	foreach(o IN LISTS RWE_OWN_VARIANTS)
		string(REPLACE "|" ";" o "${o}")
		list(GET o 0 oname)
		if(oname STREQUAL name)
			list(GET o 1 v)
		endif()
	endforeach()
	set(vout ${BLITZ_VARIANTS_DIR}/${v})
	set(obj ${vout}/userlibs/rwe/${name}.obj)
	blitz_library(${v}-${name} VARIANT ${PROJECT_SOURCE_DIR}/libs/variants/${v} OBJ ${obj}
		CMDREF userlibs/${name} ${name}.c CHECK ${vout}/acidlibs ${vout}/deflibs)
	if(v STREQUAL RWE_VARIANT)
		list(APPEND objs ${obj})
	else()
		list(APPEND own_${v} ${obj})
		list(APPEND own_variants ${v})
	endif()
	if(EXISTS ${rwesrc}/${source})
		list(APPEND readable_entries "${source}|${v}")
	endif()
	rwe_reference(${name} ${reference} reffile fixture)
	# for other directories (tests/bug-orig: ; @rwe NAME): the C build, the
	# reference, its fixture (empty: none) and the variant's directory
	set_property(GLOBAL PROPERTY RWE_LIB_${name} "${obj}|${reffile}|${fixture}|${v}|${vout}")
	add_test(NAME rwe_declarations_${name} COMMAND ${BLITZLIBCOMPARE} ${reffile} ${obj})
	set_tests_properties(rwe_declarations_${name} PROPERTIES SKIP_RETURN_CODE 77)
	set_property(GLOBAL APPEND PROPERTY RWE_EMULATOR_LIBS "${obj}|${reffile}")
	if(fixture)
		# the reference build is a test in Amiberry (refbuild.cmake)
		set_tests_properties(rwe_declarations_${name} PROPERTIES LABELS emulator
			FIXTURES_REQUIRED ${fixture})
		set_property(GLOBAL APPEND PROPERTY RWE_EMULATOR_FIXTURES ${fixture})
	endif()
endforeach()
# the RWE userlibs of an own variant: RWE_VARIANT's without those it
# replaces (REPLACES of RWE_OWN_VARIANTS, in replaced_VARIANT)
list(REMOVE_DUPLICATES own_variants)
foreach(v IN LISTS own_variants)
	set(objs_${v} ${objs})
	set(replaced_${v})
endforeach()
foreach(o IN LISTS RWE_OWN_VARIANTS)
	string(REPLACE "|" ";" o "${o}")
	list(LENGTH o n)
	if(n LESS 3)
		continue()
	endif()
	list(GET o 1 v)
	list(GET o 2 r)
	set(robj ${out}/userlibs/rwe/${r}.obj)
	if(NOT robj IN_LIST objs)
		message(FATAL_ERROR "RWE_OWN_VARIANTS: ${r} is no library of ${RWE_VARIANT}")
	endif()
	list(REMOVE_ITEM objs_${v} ${robj})
	list(APPEND replaced_${v} ${robj})
endforeach()

# the readable sources (target rwelibsrc, see above): the libraries of
# the source's variant, those of RWE_VARIANT for an own variant as well
# (its numbers include RWE_VARIANT's) but those it replaces
set(readable)
foreach(e IN LISTS readable_entries)
	string(REPLACE "|" ";" e "${e}")
	list(GET e 0 source)
	list(GET e 1 v)
	set(vout ${BLITZ_VARIANTS_DIR}/${v})
	if(v STREQUAL RWE_VARIANT)
		set(libs ${vout}/acidlibs ${vout}/deflibs ${objs})
	else()
		set(libs ${vout}/acidlibs ${vout}/deflibs ${objs_${v}} ${own_${v}})
	endif()
	set(args)
	foreach(lib IN LISTS libs)
		list(APPEND args --lib ${lib})
	endforeach()
	set(txt ${CMAKE_BINARY_DIR}/rwelibsrc/${source})
	get_filename_component(txtdir ${txt} DIRECTORY)
	rwe_ted_source(${source} tedsrc)
	add_custom_command(OUTPUT ${txt}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${txtdir}
		COMMAND ${TEDDETOK} --blitz2 $<TARGET_FILE:blitz2> ${args} ${tedsrc} ${txt}
		DEPENDS ${tedsrc} ${libs} blitz2 ${TEDDETOK} host_tools
		COMMENT "Detokenising rwelibsrc/${source}"
		VERBATIM)
	list(APPEND readable ${txt})
endforeach()
if(readable)
	add_custom_target(rwelibsrc ALL DEPENDS ${readable})
endif()
# the whole set: no number twice in acidlibs, deflibs and all RWE userlibs
if(objs)
	add_test(NAME rwe_numbers COMMAND ${BIGLIB} check ${out}/acidlibs ${out}/deflibs ${objs})
endif()
# a variant of its own: its acidlibs, deflibs, the RWE userlibs (but the
# replaced ones) and its libraries (its numbers include RWE_VARIANT's);
# and the reason for it, its libraries clash with RWE_VARIANT's
# acidlibs/deflibs or the libraries they replace (must fail)
foreach(v IN LISTS own_variants)
	set(vout ${BLITZ_VARIANTS_DIR}/${v})
	add_test(NAME rwe_numbers_${v} COMMAND ${BIGLIB} check ${vout}/acidlibs ${vout}/deflibs
		${objs_${v}} ${own_${v}})
	add_test(NAME rwe_numbers_${v}_clash COMMAND ${BIGLIB} check ${out}/acidlibs
		${out}/deflibs ${replaced_${v}} ${own_${v}})
	set_tests_properties(rwe_numbers_${v}_clash PROPERTIES WILL_FAIL TRUE)
endforeach()
