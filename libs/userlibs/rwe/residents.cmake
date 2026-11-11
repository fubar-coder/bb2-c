# The residents RWE's library sources load that exist nowhere: lvo.res,
# custom.res, datatypes.res, AHI.res (docs/rwelibs/README.md 3.1).  They hold only
# constants, so tools/blitzres writes them directly from the NDK of the
# toolchain - the same contents blitz2 2.10's Create Resident makes of
# Blitz sources with these constants (ctest rwe_residents_cd):
#   lvo.res        #_Name=-offset of the public functions of the OS 3.1
#                  libraries (FD files)
#   custom.res     #custom=$dff000, the registers of hardware/custom.i,
#                  #color00..#color31
#   datatypes.res  datatypes.library (with _SetDTAttrs for SetDTAttrsA, as
#                  the sources write it) and the constants of datatypes/*.i;
#                  not in lvo.res: two residents with one name clash
#   AHI.res        (risoundlib2) ahi.device's functions without the _ of
#                  lvo.res, as the source writes them (JSR
#                  AHI_AudioRequestA(a6)), from lvo/ahi.i (no FD file in
#                  the NDK), and the constants of devices/ahi.i (tags,
#                  AHIST_M8S, AHI_DEFAULT_ID ...; no name of lvo.res,
#                  amigalibs.res, bb2objtypes.res).  From the AHI SDK the
#                  toolchain installs (AHI 6; RWE's was of 1997, AHI 4: the
#                  V6 names are extra); built only if it is there.
# Left out: names TED tokenises (a keyword behind a digit: _Date2Amiga,
# bpl1mod, bpl2mod) - no source can use them.
# Result: RWE_RESIDENTS (the files in build/<preset>/libs/userlibs/rwe/
# residents/), empty without the NDK.
set(RWE_RESIDENTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/residents)
set(RWE_RESIDENTS)
set(res_sources)
set(res_pass "")
set(tokens_ted ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
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

# AHI.res: the AHI SDK of the toolchain (include/, not the NDK); without
# it risoundlib2 is not in the reference build
set(ahi_lvo ${ndk}/include/lvo/ahi.i)
set(ahi_inc ${ndk}/include/devices/ahi.i)
set(ahi_args --lvo ${ahi_lvo} --sym TAG_USER=-2147483648 --equ ${ahi_inc})
set(residents lvo custom datatypes)
if(EXISTS ${ahi_lvo} AND EXISTS ${ahi_inc})
	list(APPEND residents AHI)
else()
	message(STATUS "rwe residents: no AHI SDK in ${ndk}/include - AHI.res not built")
endif()

foreach(r IN LISTS residents)
	if(r STREQUAL "lvo")
		set(args ${lvo_args})
		set(deps ${lvo_deps})
	elseif(r STREQUAL "custom")
		set(args ${custom_args})
		set(deps ${ndkinc}/hardware/custom.i)
	elseif(r STREQUAL "AHI")
		set(args ${ahi_args})
		set(deps ${ahi_lvo} ${ahi_inc})
	else()
		set(args ${dt_args})
		set(deps ${fd}/datatypes_lib.fd ${dt_incs})
	endif()
	set(res ${RWE_RESIDENTS_DIR}/${r}.res)
	# blitzres reads the file back: exactly these constants, else it fails
	add_custom_command(OUTPUT ${res}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${RWE_RESIDENTS_DIR}
		COMMAND ${BLITZRES} create -o ${res} ${skip} ${args}
		DEPENDS ${deps} ${BLITZRES} host_tools
		COMMENT "Resident ${r}.res"
		VERBATIM)
	list(APPEND RWE_RESIDENTS ${res})
	# its constants as a Blitz source, tokenised like TED saves it: the
	# source of the resident for blitz2's Create Resident (below)
	set(bas ${RWE_RESIDENTS_DIR}/cd/${r}.bas)
	set(bb2 ${RWE_RESIDENTS_DIR}/cd/${r}.bb2)
	add_custom_command(OUTPUT ${bb2}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${RWE_RESIDENTS_DIR}/cd
		COMMAND ${BLITZRES} source ${res} ${bas}
		COMMAND ${TEDSRC} tok ${tokens_ted} ${bas} ${bb2}
		BYPRODUCTS ${bas}
		DEPENDS ${res} ${BLITZRES} ${TEDSRC} ${tokens_ted} host_tools
		COMMENT "Resident ${r}.res as Blitz source"
		VERBATIM)
	list(APPEND res_sources ${bb2})
	string(APPEND res_pass "file=res/${r}.bb2|${bb2}\n"
		"jobline=resident=${r}|CORPUS:res/${r}.bb2|CORPUS:out/${r}.res\n")
endforeach()
add_custom_target(rwe_residents ALL DEPENDS ${RWE_RESIDENTS} ${res_sources})

# ctest rwe_residents_cd: the residents have the contents of those blitz2
# 2.10 of the CD (ref) and blitz2-c (port) save of their Blitz sources
# with Create Resident (tedstub resident=, one corpusrun pass with run=off:
# work/TAG/vol/out/NAME.res); the job compiles a source that uses one
# constant of every resident, with all of them loaded.  Label emulator,
# skipped (77) without Amiberry or the CD.
set(RWE_RESIDENTS_TIMEOUT 300 CACHE STRING
	"rwe_residents_cd: time limit of one emulator session (seconds)")
set(cd_blitz2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
set(res_work ${RWE_RESIDENTS_DIR}/cdwork)
set(use "a.l=#custom+#_Forbid+#DTM_Dummy")
if("AHI" IN_LIST residents)
	string(APPEND use "+#AHI_TagBase")
endif()
set(probe_bas ${RWE_RESIDENTS_DIR}/cd/residents.bas)
set(probe ${RWE_RESIDENTS_DIR}/cd/residents.bb2)
file(CONFIGURE OUTPUT ${probe_bas} CONTENT
	"; rwe_residents_cd: compiled with the residents loaded\n${use}\nEnd\n" @ONLY)
add_custom_command(OUTPUT ${probe}
	COMMAND ${TEDSRC} tok ${tokens_ted} ${probe_bas} ${probe}
	DEPENDS ${probe_bas} ${TEDSRC} ${tokens_ted} host_tools
	COMMENT "Tokenising the source of rwe_residents_cd"
	VERBATIM)
add_custom_target(rwe_residents_cd_source ALL DEPENDS ${probe})
file(GENERATE OUTPUT ${res_work}.pass CONTENT
"# rwe_residents_cd: generated by libs/userlibs/rwe/residents.cmake
work=${res_work}
report=${res_work}.txt
runtime_errors=off
run=off
timeout=${RWE_RESIDENTS_TIMEOUT}
parallel=2
ted=$<TARGET_FILE:tedstub>
defaultdbug=${cd_blitz2}/DEFAULTDBUG
config=ref|ref|${cd_blitz2}/BLITZ2|${cd_blitz2}/DEFLIBS|${cd_blitz2}/ACIDLIBS
config=port|exe|$<TARGET_FILE:blitz2>|${cd_blitz2}/DEFLIBS|${cd_blitz2}/ACIDLIBS
source=${probe}
${res_pass}")
string(REPLACE ";" "$<SEMICOLON>" names_arg "${residents}")
add_test(NAME rwe_residents_cd COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=${CORPUSRUN}
	-DPASS=${res_work}.pass -DWORK=${res_work} -DBLITZRES=${BLITZRES}
	-DRESIDENTS=${RWE_RESIDENTS_DIR} "-DNAMES=${names_arg}"
	-DCHECK=${PROJECT_SOURCE_DIR}/tools/blitzres/check.cmake
	-P ${CMAKE_CURRENT_SOURCE_DIR}/residents-cd.cmake)
# two sessions, one after the other if there are not enough emulator
# slots free, each maybe waiting for one
math(EXPR limit "${RWE_RESIDENTS_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
# label originals: the CD's compiler (preset originals)
set_tests_properties(rwe_residents_cd PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${limit})
# the check of the comparison: two residents with other contents differ
add_test(NAME rwe_residents_check_broken COMMAND ${CMAKE_COMMAND} -DBLITZRES=${BLITZRES}
	-DRES=${RWE_RESIDENTS_DIR}/custom.res -DEXPECT=${RWE_RESIDENTS_DIR}/lvo.res
	-P ${PROJECT_SOURCE_DIR}/tools/blitzres/check.cmake)
set_tests_properties(rwe_residents_check_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "differ[ \n]+from")  # CMake wraps long messages
