# ted - the C translation of TED 1.1, the Blitz Basic 2 editor (ted.s).
# blitz2 loads it as blitz2:ted and calls hunk offset 4 (`outside`).

include(sources.cmake)

# archive/sources/ted-1.1: the original sources of TED 1.1 (ted.s and its
# includes, unchanged) - for the reference build below and for the
# resources: the binary includes of ted.s (incbin) as C arrays,
# src/core/res.c embeds the original files at build time
set(TED_ORIG_DIR ${PROJECT_SOURCE_DIR}/archive/sources/ted-1.1)
set(RES_DIR ${TED_ORIG_DIR})
set(RES_FILES sleep.bin pointer.bin src.info chars.b chars8.b)
list(TRANSFORM RES_FILES PREPEND ${RES_DIR}/)
set_source_files_properties(src/core/res.c PROPERTIES
	COMPILE_OPTIONS --embed-dir=${RES_DIR}
	OBJECT_DEPENDS "${RES_FILES}")

# ------------------------------------------------------- reference build
# The original ted.s assembled with vasm as Devpac did (-nocase as in
# GenAm.opts): the reference of the port (not the CD binary, which is a
# different build), in build/<preset>/programs/ted/ref/:
#   ted         the original TED
#   ted.lst     vasm listing (addresses + opcodes)
#   ted.map     TSV address -> file:line, bytes, source
#   ted.labels  TSV address, label, file:line
# The same step checks that listing and binary agree (refcheck listing-verify).
if(NOT VASM)
	find_program(VASM vasmm68k_mot REQUIRED)
endif()
set(REF_DIR ${CMAKE_CURRENT_BINARY_DIR}/ref)
set(REF_INPUTS ted.s myoffs wbheader aboutreq specialreq
	sleep.bin pointer.bin src.info rawed.obj filereq chars.b chars8.b)
list(TRANSFORM REF_INPUTS PREPEND ${TED_ORIG_DIR}/)
add_custom_command(
	OUTPUT ${REF_DIR}/ted ${REF_DIR}/ted.lst ${REF_DIR}/ted.map ${REF_DIR}/ted.labels
	COMMAND ${CMAKE_COMMAND} -E make_directory ${REF_DIR}
	COMMAND ${VASM} -quiet -devpac -nocase -m68000 -Fhunkexe
		-nowarn=62 -nowarn=40 -nowarn=41 -nowarn=51
		-o ${REF_DIR}/ted -L ${REF_DIR}/ted.lst -Lall ted.s > /dev/null
	COMMAND ${REFCHECK} listing-map ${REF_DIR}/ted.lst > ${REF_DIR}/ted.map
	COMMAND ${REFCHECK} listing-labels ${REF_DIR}/ted.lst > ${REF_DIR}/ted.labels
	COMMAND ${REFCHECK} listing-verify ${REF_DIR}/ted.lst ${REF_DIR}/ted
	DEPENDS ${REF_INPUTS} ${REFCHECK} host_tools
	WORKING_DIRECTORY ${TED_ORIG_DIR}
	COMMENT "Assembling the original ted.s (reference)"
	VERBATIM)
# not ALL: the comparisons with the originals use it (ted_ab_originals,
# the recording of the TED bug-orig tests, the TED oracle data; ctest
# originals_refbuild builds it for the preset originals) and the target
# ted-checkdata that writes the reference data of checkdata below
add_custom_target(ted_ref DEPENDS ${REF_DIR}/ted)

set(core_sources)
foreach(m IN LISTS TED_CORE_MODULES)
	list(APPEND core_sources src/core/${m}.c)
endforeach()
add_library(tedcore STATIC ${core_sources} src/core/res.c)
target_include_directories(tedcore PUBLIC src/core)
target_compile_options(tedcore PRIVATE -Wall -Wno-pointer-sign -Wno-parentheses)

set(amiga_sources)
foreach(m IN LISTS TED_AMIGA_MODULES)
	list(APPEND amiga_sources src/${m}.c)
endforeach()
file(GLOB glue_sources CONFIGURE_DEPENDS src/glue*.c)
list(REMOVE_ITEM glue_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_start.c)

# glue_start.c must be the first object: blitz2 calls hunk offset 4
# (`outside`); no libnix startup code in front of it.  The file requester
# core shared with freqlib is compiled here with TED's options;
# src/filereq_env.h is its environment.
add_library(tedobj OBJECT src/glue_start.c ${amiga_sources} ${glue_sources}
	${FILEREQ_DIR}/filereq_core.c)
target_include_directories(tedobj PRIVATE src src/core ${FILEREQ_DIR})
target_compile_options(tedobj PRIVATE
	-fomit-frame-pointer -fno-strict-aliasing
	-Wall -Wno-pointer-sign -Wno-parentheses)
option(TED_BASEREL "address globals relative to a4 (-fbaserel)" ON)
if(TED_BASEREL)
	target_compile_options(tedcore PRIVATE -fbaserel)
	target_compile_options(tedobj PRIVATE -fbaserel)
endif()

# ted: the editor (no symbols in release builds); ted.sym: the same link
# with symbols, for refcheck data below and for debugging (strip cannot handle
# the baserel data hunk, so it is linked twice)
foreach(exe ted ted_sym)
	add_executable(${exe} $<TARGET_OBJECTS:tedobj>)
	target_link_options(${exe} PRIVATE -nostartfiles)
	# whole archive: every core module is linked
	target_link_libraries(${exe} PRIVATE -Wl,--whole-archive tedcore -Wl,--no-whole-archive)
	if(TED_BASEREL)
		# .bss stored in the data hunk: no startup code clears it, and
		# Kickstart 1.x's LoadSeg does not (#963, cmake/baserel-nostart.ld)
		target_link_options(${exe} PRIVATE -fbaserel -T ${BASEREL_NOSTART_LD})
		set_property(TARGET ${exe} APPEND PROPERTY LINK_DEPENDS ${BASEREL_NOSTART_LD})
	endif()
endforeach()
target_link_options(ted PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>
	-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/ted.map)
set_target_properties(ted PROPERTIES OUTPUT_NAME ted SUFFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})
set_target_properties(ted_sym PROPERTIES OUTPUT_NAME ted.sym SUFFIX "")

# checks after every link; if one fails, ted does not count as built:
#   - hunk layout: blitz2 calls offset 4 of the first hunk (`outside`), DOS
#     a standalone TED at offset 0: bsr.w edstart ($6100), outside_stub
#     starts with movem.l ...,-(sp) ($48E7)
#   - every CODE/DATA hunk stored whole (hunkcheck stored): the .bss is
#     in the data hunk, Kickstart 1.x's LoadSeg would not clear an
#     unstored tail (#963)
#   - no libgcc/libnix arithmetic helpers (they need UtilityBase, which TED
#     never opens) and nothing else from the libraries but string/setjmp code
#   - every code label of ted.s exactly once as "ted.s:NNN label" in src/
#     (refcheck labels; a changed comment rebuilds the object, so ted relinks)
add_dependencies(ted host_tools)
add_custom_command(TARGET ted POST_BUILD
	COMMAND ${HUNKCHECK} start $<TARGET_FILE:ted> 0:6100 4:48e7
	COMMAND ${HUNKCHECK} stored $<TARGET_FILE:ted>
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/checklink.cmake
		${CMAKE_CURRENT_BINARY_DIR}/ted.map
	COMMAND ${REFCHECK} labels -q ${TED_ORIG_DIR}/ted.s ${CMAKE_CURRENT_SOURCE_DIR}/src
	VERBATIM)
add_subdirectory(tests/oracle)

# the data of the C build against the reference build of ted.s, after every
# link of ted.sym (it has the symbols): checkdata.tsv lists C symbol, start
# label, end label or size - fonts, pointers, option block, texts, screen,
# window, gadgets, menus, requesters (534 checks; relocated longs of the
# reference are skipped).  ted is linked only after this check passed.
# The reference bytes are committed generated data,
# tests/data/ted-checkdata.txt (refcheck data-ref), so the build assembles
# no ted.s: the target ted-checkdata writes them after a change of
# checkdata.tsv, ctest ted_checkdata_reference (preset originals) checks
# that they are those of a fresh reference build.
# checkdata-intended.txt, if there: the checks that differ on purpose
# (fixed original bugs, "CSYM<TAB>#NNN<TAB>NOTE"); each of them must
# differ, a stale entry fails (configure again after adding the file).
set(CHECKDATA_REF ${PROJECT_SOURCE_DIR}/tests/data/ted-checkdata.txt)
add_dependencies(ted_sym host_tools)
set(checkdata_intended)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/checkdata-intended.txt)
	set(checkdata_intended ${CMAKE_CURRENT_SOURCE_DIR}/checkdata-intended.txt)
endif()
set_property(TARGET ted_sym APPEND PROPERTY
	LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/checkdata.tsv ${CHECKDATA_REF} ${checkdata_intended})
add_custom_command(TARGET ted_sym POST_BUILD
	COMMAND ${REFCHECK} data -q ${CMAKE_CURRENT_SOURCE_DIR}/checkdata.tsv
		${CHECKDATA_REF} $<TARGET_FILE:ted_sym> ${checkdata_intended}
	VERBATIM)
# writing tests/data/ted-checkdata.txt (not ALL: after a change of
# checkdata.tsv, from the reference build)
add_custom_target(ted-checkdata
	COMMAND ${REFCHECK} data-ref ${CMAKE_CURRENT_SOURCE_DIR}/checkdata.tsv
		${REF_DIR}/ted.lst ${REF_DIR}/ted -o ${CHECKDATA_REF}
	DEPENDS ted_ref host_tools
	COMMENT "Reference data of checkdata (tests/data/ted-checkdata.txt)"
	VERBATIM)
# ctest ted_checkdata_reference (label originals): the reference data from
# a fresh reference build into the build directory, equal to the committed
# ones (checkdata-reference.cmake); with broken input: a changed byte
# fails, the same file passes (no reference build)
set(checkdata_check ${CMAKE_CURRENT_SOURCE_DIR}/checkdata-reference.cmake)
add_test(NAME ted_checkdata_reference COMMAND ${CMAKE_COMMAND} -DREFCHECK=${REFCHECK}
	-DTABLE=${CMAKE_CURRENT_SOURCE_DIR}/checkdata.tsv -DLISTING=${REF_DIR}/ted.lst
	-DREFERENCE=${REF_DIR}/ted -DOUT=${CMAKE_CURRENT_BINARY_DIR}/ted-checkdata.txt
	-DDATA=${CHECKDATA_REF} -P ${checkdata_check})
set_tests_properties(ted_checkdata_reference PROPERTIES LABELS originals
	FIXTURES_REQUIRED originals_refbuild)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CHECKDATA_REF})
file(READ ${CHECKDATA_REF} checkdata_ref_text)
string(REGEX MATCH "\nsleep\t[^\t]*\t(.)" m "${checkdata_ref_text}")
if(CMAKE_MATCH_1 STREQUAL "0")
	set(m 1)
else()
	set(m 0)
endif()
string(REGEX REPLACE "\n(sleep\t[^\t]*\t).([^\n]*)" "\n\\1${m}\\2" checkdata_ref_text
	"${checkdata_ref_text}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ted-checkdata-wrong.txt "${checkdata_ref_text}")
foreach(name right wrong)
	set(f ${CHECKDATA_REF})
	if(name STREQUAL "wrong")
		set(f ${CMAKE_CURRENT_BINARY_DIR}/ted-checkdata-wrong.txt)
	endif()
	add_test(NAME ted_checkdata_reference_check_${name} COMMAND ${CMAKE_COMMAND} -DNO_RUN=ON
		-DOUT=${f} -DDATA=${CHECKDATA_REF} -P ${checkdata_check})
endforeach()
set_tests_properties(ted_checkdata_reference_check_right PROPERTIES
	PASS_REGULAR_EXPRESSION "are those of the reference build")
set_tests_properties(ted_checkdata_reference_check_wrong PROPERTIES WILL_FAIL ON)
# the intended list on the real build (tables and lists of the build
# directory): a check that differs, listed (passes) and not listed (must
# fail); a listed check that is equal (stale, must fail)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/checkdata_differs.tsv
	"_sleep_\tpointer\tpointerf\tbroken: another label\n_pointer\tpointer\tpointerf\tequal\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/checkdata_intended.txt
	"_sleep_\t#1\tthe broken entry\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/checkdata_stale.txt
	"_sleep_\t#1\tthe broken entry\n_pointer\t#2\tequal: stale\n")
set(checkdata_args ${CMAKE_CURRENT_BINARY_DIR}/checkdata_differs.tsv
	${CHECKDATA_REF} $<TARGET_FILE:ted_sym>)
add_test(NAME ted_checkdata_intended COMMAND ${REFCHECK} data ${checkdata_args}
	${CMAKE_CURRENT_BINARY_DIR}/checkdata_intended.txt)
add_test(NAME ted_checkdata_unlisted COMMAND ${REFCHECK} data ${checkdata_args})
add_test(NAME ted_checkdata_stale COMMAND ${REFCHECK} data ${checkdata_args}
	${CMAKE_CURRENT_BINARY_DIR}/checkdata_stale.txt)
set_tests_properties(ted_checkdata_intended PROPERTIES
	PASS_REGULAR_EXPRESSION "intended _sleep_ .*checkdata: 2 checks, 0 failed, 1 intended")
set_tests_properties(ted_checkdata_unlisted PROPERTIES
	PASS_REGULAR_EXPRESSION "FAIL    _sleep_ .*checkdata: 2 checks, 1 failed")
set_tests_properties(ted_checkdata_stale PROPERTIES
	PASS_REGULAR_EXPRESSION "STALE   _pointer .*checkdata: 2 checks, 1 failed, 1 intended")
add_dependencies(ted ted_sym)

