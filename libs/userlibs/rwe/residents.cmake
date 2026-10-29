# The residents RWE's library sources load that exist nowhere: lvo.res,
# custom.res, datatypes.res (docs/rwelibs/README.md 3.1).  They hold only
# constants, so tools/blitzres writes them directly from the NDK of the
# toolchain - the same contents blitz2 2.10's Create Resident made of
# Blitz sources with these constants (ctest rwe_resident_NAME):
#   lvo.res        #_Name=-offset of the public functions of the OS 3.1
#                  libraries (FD files)
#   custom.res     #custom=$dff000, the registers of hardware/custom.i,
#                  #color00..#color31
#   datatypes.res  datatypes.library (with _SetDTAttrs for SetDTAttrsA, as
#                  the sources write it) and the constants of datatypes/*.i;
#                  not in lvo.res: two residents with one name clash
# Left out: names TED tokenises (a keyword behind a digit: _Date2Amiga,
# bpl1mod, bpl2mod) - no source can use them.
# Result: RWE_RESIDENTS (the files in build/<preset>/libs/userlibs/rwe/
# residents/), empty without the NDK.
set(RWE_ANALYSIS_DIR ${PROJECT_SOURCE_DIR}/.cache/rwelibs-analysis CACHE PATH
	"the raw material of docs/rwelibs (not in git), for the comparisons with it")
set(RWE_RESIDENTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/residents)
set(RWE_RESIDENTS)
set(ndk ${AMIGA_TOOLCHAIN_PREFIX}/m68k-amigaos)
set(fd ${ndk}/ndk/lib/fd)
set(ndkinc ${ndk}/ndk-include)
if(NOT EXISTS ${fd}/exec_lib.fd OR NOT EXISTS ${ndkinc}/hardware/custom.i)
	message(STATUS "rwe residents: no NDK in ${ndk} - not built")
	return()
endif()
set(skip --skip _Date2Amiga --skip bpl1mod --skip bpl2mod)

# lvo.res: the libraries in this order; a name in two FD files (with one
# offset) counts once
set(lvo_args)
set(lvo_deps)
foreach(l exec dos graphics intuition layers icon wb gadtools utility diskfont asl
		commodities iffparse keymap locale expansion console input timer mathffp
		mathtrans mathieeedoubbas mathieeedoubtrans mathieeesingbas mathieeesingtrans
		rexxsyslib amigaguide lowlevel nonvolatile realtime translator potgo cia misc
		disk battclock battmem bullet colorwheel ramdrive cardres)
	list(APPEND lvo_args --fd ${fd}/${l}_lib.fd)
	list(APPEND lvo_deps ${fd}/${l}_lib.fd)
endforeach()

set(custom_args --const custom=$dff000 --equ ${ndkinc}/hardware/custom.i)
foreach(i RANGE 31)
	math(EXPR reg "0x180 + 2 * ${i}" OUTPUT_FORMAT DECIMAL)
	string(LENGTH "${i}" n)
	if(n EQUAL 1)
		set(i 0${i})
	endif()
	list(APPEND custom_args --const color${i}=${reg})
endforeach()

# the datatypes includes: TAG_USER of utility/tagitem.i, DTM_Dummy (the
# methods of datatypesclass.i start at $600) defined first
set(dt_incs)
foreach(i datatypesclass datatypes pictureclass soundclass textclass animationclass)
	list(APPEND dt_incs ${ndkinc}/datatypes/${i}.i)
endforeach()
set(dt_args --sym TAG_USER=-2147483648 --fd-varargs ${fd}/datatypes_lib.fd
	--const DTM_Dummy=1536)
foreach(i IN LISTS dt_incs)
	list(APPEND dt_args --equ ${i})
endforeach()

foreach(r lvo custom datatypes)
	if(r STREQUAL "lvo")
		set(args ${lvo_args})
		set(deps ${lvo_deps})
	elseif(r STREQUAL "custom")
		set(args ${custom_args})
		set(deps ${ndkinc}/hardware/custom.i)
	else()
		set(args ${dt_args})
		set(deps ${fd}/datatypes_lib.fd ${dt_incs})
	endif()
	set(res ${RWE_RESIDENTS_DIR}/${r}.res)
	# blitzres reads the file back: exactly these constants, else it fails
	add_custom_command(OUTPUT ${res}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${RWE_RESIDENTS_DIR}
		COMMAND ${BLITZRES} -o ${res} ${skip} ${args}
		DEPENDS ${deps} ${BLITZRES} host_tools
		COMMENT "Resident ${r}.res from the NDK"
		VERBATIM)
	list(APPEND RWE_RESIDENTS ${res})
	# the contents equal the resident blitz2 2.10 made in the analysis
	# (.cache/rwelibs-analysis/residents/res, not in git: skipped without)
	add_test(NAME rwe_resident_${r} COMMAND ${CMAKE_COMMAND} -DBLITZRES=${BLITZRES}
		-DRES=${res} -DEXPECT=${RWE_ANALYSIS_DIR}/residents/res/${r}.res
		-P ${PROJECT_SOURCE_DIR}/tools/blitzres/check.cmake)
	set_tests_properties(rwe_resident_${r} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
add_custom_target(rwe_residents ALL DEPENDS ${RWE_RESIDENTS})
