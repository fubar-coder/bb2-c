# The libraries of the sources Red When Excited released in September 1999
# (archive/aminet/amibl2src/rwelibsrc, docs/rwelibs/README.md), as
# userlibs of the variant rwe-1999: NAME.c -> userlibs/rwe/NAME.obj.
#
# RWE_LIBS: NAME|SOURCE|REFERENCE[|USES] per library - SOURCE in rwelibsrc
# (the Blitz source the C file translates), REFERENCE the binary below
# archive/ (fetch-originals: the CD, AmiBlitz3's SVN) whose state the
# port has: the one the reference build of SOURCE with blitz2 2.10 equals
# byte for byte (docs/rwelibs/README.md 3.1), or the newer binary chosen
# in 3.2 (then SOURCE is the closest source).  USES: binaries below
# archive/ whose tokens SOURCE has (ALibJsr calls into other libraries or
# its own), for reading it.  A library is built as soon as NAME.c exists.
# The C build is real C (assembler only where needed), checked like the
# acidlibs: the declarations equal REFERENCE's (ctest
# rwe_declarations_NAME, skipped without the originals), and the
# behaviour is compared with REFERENCE in Amiberry (libs_emulator:
# libs/tests/rwe/NAME.c, the property RWE_EMULATOR_LIBS lists obj|orig).
set(RWE_VARIANT rwe-1999)
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
	"rigfxlib|GFXLib/GFX-Library2.bb|${bss2}/RIGFXLIB.OBJ|${bss2}/RIAMOSFUNCLIB.OBJ"
	"ridoslib|DOSLib/ridoslib2.bb|${bss2}/RIDOSLIB.OBJ"
	"ripacklib|packlib/Pack-Library8.bb|${bum}/RIPACKLIB.OBJ|${bss2}/RIAMOSFUNCLIB.OBJ"
	"ricommoditieslib|Commoditieslib/CommoditiesLib2.bb|${bss2}/RICOMMODITIESLIB.OBJ"
	"riapplib|AppLib/applib2.bb|${bss2}/RIAPPLIB.OBJ"
	"ritooltypeslib|tooltypeslib/ToolTypes4.bb|${bss2}/RITOOLTYPESLIB.OBJ"
	"rizonejoylib|zonejoylib/ZoneJoy-Library4.bb|${bss2}/RIZONEJOYLIB.OBJ"
	"rifxlib|FXLib/FX-Library13.bb|${bss2}/RIFXLIB.OBJ"
	# stage B (docs/rwelibs/README.md 3.2): the AmiBlitz3 SVN binary =
	# BSS2 plus fixes of the AmiBlitz3 team; func-library7 is the closest
	# source
	"riamosfunclib|AMOSFunclib/func-library7.bb|${ab3}/RIAmosFuncLib.obj|${ab3}/RIAmosFuncLib.obj"
	# stage B, the BSS2 binary newer than the source (3.2): the port has
	# the binary's state
	"risoundlib|soundlib/risoundlib.bb|${bss2}/RISOUNDLIB.OBJ"
	"ristonecrackerlib|stclib.bb|${bss2}/RISTONECRACKERLIB.OBJ"
	"rifnslib|FNSLib/fns-library11.bb|${bss2}/RIFNSLIB.OBJ|${bss2}/RIAMOSFUNCLIB.OBJ")

set(variant_dir ${PROJECT_SOURCE_DIR}/libs/variants/${RWE_VARIANT})
set(out ${BLITZ_VARIANTS_DIR}/${RWE_VARIANT})
set(archive ${PROJECT_SOURCE_DIR}/archive)
set(rwesrc ${PROJECT_SOURCE_DIR}/archive/aminet/amibl2src/rwelibsrc)

# the residents the sources load that exist nowhere, from the NDK; the
# reference build of the sources with blitz2 2.10
include(${CMAKE_CURRENT_SOURCE_DIR}/residents.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/refbuild.cmake)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
set(names)
foreach(l IN LISTS RWE_LIBS)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(APPEND names ${name})
endforeach()
foreach(src IN LISTS sources)
	get_filename_component(name ${src} NAME_WE)
	if(NOT name IN_LIST names)
		message(FATAL_ERROR "libs/userlibs/rwe/${name}.c is not in RWE_LIBS")
	endif()
endforeach()

set(objs)
set(readable)
foreach(l IN LISTS RWE_LIBS)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(GET l 1 source)
	list(GET l 2 reference)
	set(uses)
	set(usefiles)
	list(LENGTH l n)
	if(n GREATER 3)
		list(GET l 3 u)
		set(uses --lib ${archive}/${u})
		set(usefiles ${archive}/${u})
	endif()
	# the Blitz source as TED shows it (like bblibsrc): build/<preset>/rwelibsrc/
	# SOURCE, tokens of the compiler and acidlibs/deflibs built here and of
	# USES (skipped if those are missing)
	set(have_uses TRUE)
	foreach(u IN LISTS usefiles)
		if(NOT EXISTS ${u})
			set(have_uses FALSE)
		endif()
	endforeach()
	if(EXISTS ${rwesrc}/${source} AND have_uses)
		set(txt ${CMAKE_BINARY_DIR}/rwelibsrc/${source})
		get_filename_component(txtdir ${txt} DIRECTORY)
		add_custom_command(OUTPUT ${txt}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${txtdir}
			COMMAND ${TEDDETOK} --blitz2 $<TARGET_FILE:blitz2> --lib ${BLITZ2_DIR}/acidlibs
				--lib ${BLITZ2_DIR}/deflibs ${uses} ${rwesrc}/${source} ${txt}
			DEPENDS ${rwesrc}/${source} ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs ${usefiles}
				blitz2 ${TEDDETOK} host_tools
			COMMENT "Detokenising rwelibsrc/${source}"
			VERBATIM)
		list(APPEND readable ${txt})
	endif()
	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
		continue()
	endif()
	set(obj ${out}/userlibs/rwe/${name}.obj)
	blitz_library(${RWE_VARIANT}-${name} VARIANT ${variant_dir} OBJ ${obj} ${name}.c
		CHECK ${out}/acidlibs ${out}/deflibs)
	list(APPEND objs ${obj})
	add_test(NAME rwe_declarations_${name} COMMAND ${BLITZLIBCOMPARE} ${archive}/${reference} ${obj})
	set_tests_properties(rwe_declarations_${name} PROPERTIES SKIP_RETURN_CODE 77)
	set_property(GLOBAL APPEND PROPERTY RWE_EMULATOR_LIBS "${obj}|${archive}/${reference}")
endforeach()
if(readable)
	add_custom_target(rwelibsrc ALL DEPENDS ${readable})
endif()
# the whole set: no number twice in acidlibs, deflibs and all RWE userlibs
if(objs)
	add_test(NAME rwe_numbers COMMAND ${BIGLIB} check ${out}/acidlibs ${out}/deflibs ${objs})
endif()
