# The reference build of the RWE sources (docs/rwelibs/README.md 3.1):
# the SOURCE of every RWE_LIBS entry compiled by the CD's blitz2 2.10 with
# the CD's libraries, as RWE built its libraries ("the executable is the
# library file").  Included by CMakeLists.txt before its loop over
# RWE_LIBS (after residents.cmake).
#
# Staging (build step, target rwe_refstage, part of ALL when the CD and
# rwelibsrc are there): build/<preset>/libs/userlibs/rwe/refstage/, see
# refbuild-stage.cmake - the CD's EXTRAS/BLITZLIBS/ with a LIBNUMS.BB that
# has the numbers of the variant's libnums.h appended
# (refbuild-libnums.cmake), the residents of residents.cmake, sbra.res,
# and the sources with their INCDIR lines neutralised (tools/bblines: the
# line becomes ";" - original bugs #341/#342: blitz2 2.10 puts the INCDIR
# in front of every include name, also of the absolute one in LIBMACS.BB,
# and keeps it for the next compile; INCDIR generates no code).
#
# The compile (ctest rwe_refbuild, label emulator, fixture rwe_refbuild):
# cdrun (tests/corpus/runner) with the staging directory as its "CD" and
# a source= line per source; the result of entry NAME is
#   build/<preset>/libs/userlibs/rwe/ref/NAME.obj
# (removed first; skipped with 77 without Amiberry or the CD, fails if a
# source does not compile).  The table: refbuild.tsv next to ref/.
#
# Its uses:
#   rwe_refbuild_NAME  the reference build of SOURCE equals REFERENCE byte
#                      for byte - for the entries whose REFERENCE is the
#                      binary of the source (stage A, 3.1), not for those
#                      in RWE_REFBUILD_NEWER (3.2: a newer binary is the
#                      reference; they only have to compile)
#   REFERENCE refbuild an entry whose reference is the reference build of
#                      its SOURCE (3.2 "Source", 3.3): rwe_reference()
#                      gives ref/NAME.obj and the fixture, for
#                      rwe_declarations_NAME and the pairs of libs_emulator
#   rwe_refbuild_selftest  a dry refbuild entry (RWE_REFBUILD_SELFTEST),
#                      not in RWE_LIBS: its declarations test through
#                      rwe_reference(), against the C build
set(RWE_REF_DIR ${CMAKE_CURRENT_BINARY_DIR}/ref)
set(RWE_REFBUILD_NEWER riamosfunclib risoundlib ristonecrackerlib rifnslib)
set(RWE_REFBUILD_SELFTEST "riencryptlib|Encryptlib/Encrypt-Library.bb|refbuild")

# rwe_reference(NAME REFERENCE OUT_FILE OUT_FIXTURE): the reference file
# of an entry - archive/REFERENCE, or for "refbuild" ref/NAME.obj and the
# fixture its tests need (else OUT_FIXTURE is empty)
function(rwe_reference name reference out_file out_fixture)
	if(reference STREQUAL "refbuild")
		set(${out_file} ${RWE_REF_DIR}/${name}.obj PARENT_SCOPE)
		set(${out_fixture} rwe_refbuild PARENT_SCOPE)
	else()
		set(${out_file} ${PROJECT_SOURCE_DIR}/archive/${reference} PARENT_SCOPE)
		set(${out_fixture} "" PARENT_SCOPE)
	endif()
endfunction()

set(cd ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1)
set(cdlibs ${cd}/EXTRAS/BLITZLIBS)
set(stage ${CMAKE_CURRENT_BINARY_DIR}/refstage)
set(gen ${CMAKE_CURRENT_BINARY_DIR}/refgen)
set(tokens ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set(have_originals FALSE)
if(EXISTS ${cdlibs}/LIBNUMS.BB AND EXISTS ${rwesrc} AND RWE_RESIDENTS)
	set(have_originals TRUE)
endif()
# the token word of INCDIR (a compiler token, the same in every version)
file(STRINGS ${tokens} incdir REGEX "^[0-9A-F]+ INCDIR$")
string(REGEX REPLACE " .*" "" incdir "${incdir}")
if(NOT incdir MATCHES "^[0-9A-F]+$")
	message(FATAL_ERROR "${tokens}: no INCDIR")
endif()

# the entries: RWE_LIBS and the dry one; a source once per name
set(ref_entries ${RWE_LIBS} ${RWE_REFBUILD_SELFTEST})
set(ref_names)
set(ref_sources)        # REL|FILE[|XTRA] for the staging
set(ref_staged)
set(ref_pass "")        # source= lines of the pass
foreach(l IN LISTS ref_entries)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(GET l 1 source)
	if(name IN_LIST ref_names)
		continue()
	endif()
	list(APPEND ref_names ${name})
	if(NOT have_originals OR NOT EXISTS ${rwesrc}/${source})
		continue()
	endif()
	string(APPEND ref_pass "source=RWE/${source}|${RWE_REF_DIR}/${name}.obj\n")
	if("RWE/${source}" IN_LIST ref_staged)
		continue()
	endif()
	list(APPEND ref_staged RWE/${source})
	# the INCDIR lines neutralised on the bytes (a text round trip through
	# TED would tokenise other lines with this build's token list); a BB22
	# source in TED's format first (blitz2 does not read BB22)
	rwe_ted_source(${source} tedsrc)
	set(tok ${gen}/src/${source})
	get_filename_component(tokdir ${tok} DIRECTORY)
	add_custom_command(OUTPUT ${tok}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${tokdir}
		COMMAND ${BBLINES} --comment ${incdir} ${tedsrc} ${tok}
		DEPENDS ${tedsrc} ${BBLINES} host_tools
		COMMENT "Reference build: rwelibsrc/${source} without INCDIR"
		VERBATIM)
	# the .xtra next to the source (the name may differ in case:
	# soundlib/RISoundLib.bb.xtra)
	get_filename_component(srcdir ${rwesrc}/${source} DIRECTORY)
	get_filename_component(srcname ${source} NAME)
	string(TOLOWER "${srcname}.xtra" want)
	file(GLOB here LIST_DIRECTORIES false ${srcdir}/*)
	set(xtra "")
	foreach(f IN LISTS here)
		get_filename_component(fn ${f} NAME)
		string(TOLOWER "${fn}" fn)
		if(fn STREQUAL want)
			set(xtra "|${f}")
		endif()
	endforeach()
	list(APPEND ref_sources "${source}|${tok}${xtra}")
endforeach()

if(have_originals)
	set(libnums ${gen}/LIBNUMS.BB)
	set(libnums_h ${variant_dir}/libnums.h)
	add_custom_command(OUTPUT ${libnums}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${gen}
		COMMAND ${CMAKE_COMMAND} -DIN=${cdlibs}/LIBNUMS.BB -DLIBNUMS_H=${libnums_h}
			-DTEDDETOK=${TEDDETOK} -DTEDTOK=${TEDTOK} -DTOKENS=${tokens} -DOUT=${libnums}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/refbuild-libnums.cmake
		BYPRODUCTS ${libnums}.txt ${libnums}.add ${libnums}.add.txt
		DEPENDS ${cdlibs}/LIBNUMS.BB ${libnums_h} ${tokens}
			${CMAKE_CURRENT_SOURCE_DIR}/refbuild-libnums.cmake ${TEDDETOK} ${TEDTOK} host_tools
		COMMENT "Reference build: LIBNUMS.BB with the numbers of ${RWE_VARIANT}"
		VERBATIM)
	set(stage_in ${libnums} ${RWE_RESIDENTS})
	set(stage_out ${stage}/BLITZLIBS/LIBNUMS.BB)
	foreach(s IN LISTS ref_sources)
		string(REPLACE "|" ";" s "${s}")
		list(GET s 0 rel)
		list(GET s 1 f)
		list(APPEND stage_in ${f})
		list(APPEND stage_out ${stage}/RWE/${rel})
	endforeach()
	file(GLOB cdfiles LIST_DIRECTORIES false ${cdlibs}/*)
	set(sbra ${cd}/BUM10A/BLITZLIBS/ASMMACS.RES)
	set(optidec ${rwesrc}/optidec.bin)
	set(fnsfont ${cd}/BSS2/REDBLITZLIBS/EXAMPLES/FNS/FONTS/PERSONAL.FNS)
	string(REPLACE ";" "$<SEMICOLON>" res_arg "${RWE_RESIDENTS}")
	string(REPLACE ";" "$<SEMICOLON>" src_arg "${ref_sources}")
	add_custom_command(OUTPUT ${stage_out}
		COMMAND ${CMAKE_COMMAND} -DSTAGE=${stage} -DBLITZLIBS=${cdlibs} -DLIBNUMS=${libnums}
			-DSBRA=${sbra} "-DRESIDENTS=${res_arg}" -DOPTIDEC=${optidec} -DFNSFONT=${fnsfont}
			"-DSOURCES=${src_arg}" -P ${CMAKE_CURRENT_SOURCE_DIR}/refbuild-stage.cmake
		DEPENDS ${stage_in} ${cdfiles} ${sbra} ${optidec} ${fnsfont}
			${CMAKE_CURRENT_SOURCE_DIR}/refbuild-stage.cmake
		COMMENT "Reference build: staging the RWE sources"
		VERBATIM)
	add_custom_target(rwe_refstage ALL DEPENDS ${stage_out})
	# the check of the LIBNUMS.BB step with a broken libnums.h: a number
	# the CD's file has with another value
	set(broken ${CMAKE_CURRENT_BINARY_DIR}/refbuild-broken)
	file(WRITE ${broken}/libnums.h "#define LIB_ripacklib 99\n")
	add_test(NAME rwe_refbuild_libnums_broken COMMAND ${CMAKE_COMMAND}
		-DIN=${cdlibs}/LIBNUMS.BB -DLIBNUMS_H=${broken}/libnums.h -DTEDDETOK=${TEDDETOK}
		-DTEDTOK=${TEDTOK} -DTOKENS=${tokens} -DOUT=${broken}/LIBNUMS.BB
		-P ${CMAKE_CURRENT_SOURCE_DIR}/refbuild-libnums.cmake)
	set_tests_properties(rwe_refbuild_libnums_broken PROPERTIES
		PASS_REGULAR_EXPRESSION "#ripacklib: 73 in")
else()
	add_custom_target(rwe_refstage
		COMMAND ${CMAKE_COMMAND} -E echo "rwe_refstage: no CD or rwelibsrc (cmake -P cmake/fetch-originals.cmake): skipped"
		VERBATIM)
endif()

# the compile: the CD's blitz2 2.10 with the CD's libraries in Amiberry
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	set(parallel_default 3)
else()
	set(parallel_default 1)
endif()
set(RWE_REFBUILD_PARALLEL ${parallel_default} CACHE STRING
	"rwe_refbuild: emulator sessions at the same time (as CORPUS_PARALLEL)")
set(RWE_REFBUILD_TIMEOUT 300 CACHE STRING
	"rwe_refbuild: time limit of one emulator session (seconds)")
set(pass ${CMAKE_CURRENT_BINARY_DIR}/refbuild.pass)
file(GENERATE OUTPUT ${pass} CONTENT
"# the reference build of the RWE sources: generated by libs/userlibs/rwe/refbuild.cmake
work=${CMAKE_CURRENT_BINARY_DIR}/refwork
output=${CMAKE_CURRENT_BINARY_DIR}/refbuild.tsv
cd=${stage}
tokens=${tokens}
blitz2=${cd}/BLITZ2/BLITZ2
deflibs=${cd}/BLITZ2/DEFLIBS
acidlibs=${cd}/BLITZ2/ACIDLIBS
defaultdbug=${cd}/BLITZ2/DEFAULTDBUG
ted=$<TARGET_FILE:tedstub>
timeout=${RWE_REFBUILD_TIMEOUT}
parallel=${RWE_REFBUILD_PARALLEL}
assign=blitzlibs|BLITZLIBS
assign=libincs|BLITZLIBS
assign=blitzincs|BLITZLIBS
assign=FNS|FNS
${ref_pass}")
add_test(NAME rwe_refbuild COMMAND ${CDRUN} ${pass})
math(EXPR limit "${RWE_REFBUILD_TIMEOUT} * 3 + 120")
set_tests_properties(rwe_refbuild PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} FIXTURES_SETUP rwe_refbuild)

# stage A: the reference build is the reference binary, byte for byte
foreach(l IN LISTS RWE_LIBS)
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(GET l 2 reference)
	if(reference STREQUAL "refbuild" OR name IN_LIST RWE_REFBUILD_NEWER)
		continue()
	endif()
	add_test(NAME rwe_refbuild_${name} COMMAND ${CMAKE_COMMAND}
		-DREF=${RWE_REF_DIR}/${name}.obj -DORIG=${archive}/${reference}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/refbuild-compare.cmake)
	set_tests_properties(rwe_refbuild_${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		FIXTURES_REQUIRED rwe_refbuild)
endforeach()

# the dry refbuild entry: its reference through rwe_reference() (the
# path and the fixture an entry with REFERENCE refbuild gets), against
# the C build of the library
string(REPLACE "|" ";" l "${RWE_REFBUILD_SELFTEST}")
list(GET l 0 name)
list(GET l 2 reference)
rwe_reference(${name} ${reference} file fixture)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.c)
	add_test(NAME rwe_refbuild_selftest COMMAND ${BLITZLIBCOMPARE} ${file}
		${out}/userlibs/rwe/${name}.obj)
	set_tests_properties(rwe_refbuild_selftest PROPERTIES LABELS emulator
		SKIP_RETURN_CODE 77 FIXTURES_REQUIRED ${fixture})
endif()

# the comparison with broken input: two different files
add_test(NAME rwe_refbuild_compare_broken COMMAND ${CMAKE_COMMAND}
	-DREF=${CMAKE_CURRENT_SOURCE_DIR}/refbuild-compare.cmake
	-DORIG=${CMAKE_CURRENT_SOURCE_DIR}/refbuild-stage.cmake
	-P ${CMAKE_CURRENT_SOURCE_DIR}/refbuild-compare.cmake)
set_tests_properties(rwe_refbuild_compare_broken PROPERTIES PASS_REGULAR_EXPRESSION "differs from")
