# The bug-orig tests with a Blitz program (tests/README.md, "bug-orig tests"): a program that
# triggers a bug of the original, compiled and run in Amiberry by the
# original blitz2 1.95 with the CD's libraries (ref) and by the port (port:
# blitz2-c with the CD's libraries, clib: blitz2-c with the C libraries).
# The expectation is the correct behaviour: the test fails as long as the
# port reproduces the bug, and it is broken if the original does not show
# the bug (corpus.h expect=, bugorig.h).  The reference stays 1.95 (the
# build of blitz2.s), not 2.10 like the corpus: 2.10 fixed #2 and has all
# other bugs unchanged (docs/v2.10), so the test of #2 stays valid.
#
# One test = two files in an area directory (compiler/, libs/ ...):
#   AREA/ID.bas      the program (text, tokenised here like TED would save
#                    it); its header may set
#                      ; @pass KEY=VALUE    a line of the pass file
#                                           (tests/corpus/runner/corpus.h),
#                                           @VAR@ replaced (see below)
#                      ; @configs TAG...    the configurations judged
#                                           (port clib, default both;
#                                           regression/: clib)
#                      ; @machine NAME      the emulated machine (pass
#                                           file machine=, corpus.h:
#                                           a1200 default, a500 = 68000
#                                           with Kickstart 1.3,
#                                           a500-68010, a600 = 68000
#                                           with Kickstart 3.1); on Kickstart
#                                           1.x the compiler and the
#                                           program run there too (no
#                                           gate, no @INJECT@/@WBRUN@:
#                                           they need V36; tests/README.md,
#                                           "bug-orig tests")
#                      ; @rwe NAME...       RWE userlibs (libs/userlibs/
#                                           rwe/NAME.c) in blitz2:userlibs/
#                                           rwe/: ref (blitz2 2.10 of the
#                                           CD) gets the reference
#                                           (RWE_LIBS: the CD's binary or
#                                           the reference build, then the
#                                           test needs the fixture
#                                           rwe_refbuild), port and clib
#                                           the C build; clib with the
#                                           acidlibs/deflibs of the
#                                           library's variant, the source
#                                           tokenised with the tokens of
#                                           blitz2-c, that variant and
#                                           the libraries (blitztokens)
#                      ; @fixed #NNN        the bug is fixed in the port
#                                           (issue NNN): a fixed test,
#                                           see below
#   AREA/ID.expect   the correct behaviour (sections, bugorig.h)
#   AREA/ID.orig     the recorded observation of the original (an open bug;
#                    not for a fixed test or a port regression test).  The
#                    normal run judges against it and starts no session of
#                    the original; ctest bug_orig_record_ID (label
#                    originals, preset originals; or BUGORIG_RECORD=1)
#                    writes it with the original (the reference build of
#                    blitz2.s, or the CD's 2.10 for @rwe) and needs the CD
# ID is the bug ID of the issue; ctest bug_orig_ID (- as _), labels
# bug-orig and emulator, skipped (77) without Amiberry.  Without the CD
# the configuration port (the CD's libraries) is not judged and the run
# goes without blitz2:defaultdbug (not with runtime_errors=on: skipped);
# the volume BB2: is mounted (the CD) only for a test whose source names it
# or whose layout needs it (@pass mount=BB2|@CD@), skipped without the CD.
# A fixed test (; @fixed #NNN, added by the commit that fixes the bug;
# corpus.h fixed=) has no reference session and uses nothing of the CD
# (no BB2: volume, no blitz2:defaultdbug, unless its @pass lines name them,
# e.g. mount=BB2|@CD@): only clib is judged and must be FIXED, label
# emulator only (it runs in the presets emulator and all, not in
# bug-orig), declared here directly, not in the order of
# cmake/bug-orig-order.cmake (it passes, ctest learns its cost).
# The area regression/ holds port regression tests instead (a bug of the
# port, not of the original; corpus.h regression=on): expect tests like
# those of tests/blitz2, with no reference session and nothing of the CD
# (as a fixed test, unless the @pass lines name it), judged against the
# expectation only, in clib unless @configs names others; ctest
# regression_ID, label emulator only (it must pass in emulator and all).
# The files are found by a glob: adding a test touches no list.
#   @VAR@ in @pass: USERLIB (amiga/userlib.c: a library for
#   blitz2:userlibs), INJECT (amiga/inject.c: faults, counts and input;
#   for the compiler with
#   wrap=@INJECT@|..., for the programs with runwrap=@INJECT@|...),
#   WBRUN (amiga/wbrun.c: started like from the Workbench, wrap=@WBRUN@ or
#   runwrap=@WBRUN@),
#   CD (the CD), CD_BLITZ2 (the CD's BLITZ2 directory), TESTDATA
#   (the generated test data, see below; use
#   file=DEST|@TESTDATA@/NAME, the program reads CORPUS:DEST)
#
# Test data (sounds, pictures, modules) is generated, not checked in:
#   AREA/NAME.spec   a description for tools/mktestdata (bytes, IFF
#                    chunks, labels; tools/lib/mktestdata/mktestdata.h),
#                    built to testdata/NAME in the build directory
#   AREA/NAME.src    a further Blitz source as text (e.g. the source of a
#                    resident, tedstub resident=), tokenised like ID.bas
#                    to testdata/NAME
# NAME is unique over all areas and begins with the ID of the test that
# uses it (e.g. libs/ID.8svx.spec).
add_subdirectory(amiga)

set(BUG_ORIG_TIMEOUT 90 CACHE STRING "bug-orig: time limit of one emulator session (seconds)")
set(CD ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1)
set(CD_BLITZ2 ${CD}/BLITZ2)
set(REF_BLITZ2 ${CMAKE_BINARY_DIR}/programs/blitz2/ref/blitz2)
set(USERLIB ${CMAKE_CURRENT_BINARY_DIR}/amiga/bugorig_userlib.obj)
set(INJECT $<TARGET_FILE:inject>)
set(WBRUN $<TARGET_FILE:wbrun>)
set(TESTDATA ${CMAKE_CURRENT_BINARY_DIR}/testdata)
# the configurations of a test run at the same time as the corpus passes
# do (CORPUS_PARALLEL, tests/corpus/CMakeLists.txt)
set(parallel ${CORPUS_PARALLEL})

# the token list: TED's plus the test library's
set(ted_tokens ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set(lib_tokens ${CMAKE_CURRENT_SOURCE_DIR}/amiga/userlib.tokens)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ted_tokens} ${lib_tokens})
file(READ ${ted_tokens} t1)
file(READ ${lib_tokens} t2)
set(tokens ${CMAKE_CURRENT_BINARY_DIR}/tokens.txt)
file(CONFIGURE OUTPUT ${tokens} CONTENT "${t1}${t2}" @ONLY)

# a fixed test keeps no recorded original: checked at configure time
include(${CMAKE_CURRENT_SOURCE_DIR}/check-fixed-orig.cmake)
bug_orig_fixed_orig_check(${CMAKE_CURRENT_SOURCE_DIR})

# bug_orig_test(BAS NAME [WANT RC]): tokenise, pass file, ctest NAME.
# WANT: the test passes if corpusrun returns RC (the self test).
set(bug_orig_sources)
set(bug_orig_deps)
function(bug_orig_test bas name)
	cmake_parse_arguments(PARSE_ARGV 2 arg "REGRESSION" "WANT" "")
	get_filename_component(id ${bas} NAME_WE)
	get_filename_component(dir ${bas} DIRECTORY)
	set(expect ${dir}/${id}.expect)
	if(NOT EXISTS ${expect})
		message(FATAL_ERROR "bug-orig: ${bas} has no ${id}.expect")
	endif()
	set(configs port clib)
	set(rwe "")
	set(extra "")
	set(fixed "")
	set(configs_given OFF)
	set(session_limit ${BUG_ORIG_TIMEOUT})
	set(orig_by "the original blitz2 1.95 (the build of blitz2.s) with the CD's libraries")
	# the header lines begin with ';' - the list separator: replaced first.
	# They make the pass file: a changed header configures again.
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${bas})
	file(READ ${bas} content)
	set(bas_text "${content}")
	string(REPLACE ";" "<semicolon>" content "${content}")
	string(REGEX MATCHALL "(^|\n)<semicolon> @[^\n]*" header "${content}")
	foreach(h IN LISTS header)
		string(STRIP "${h}" h)
		if(h MATCHES "^<semicolon> @fixed #([1-9][0-9]*)$")
			if(fixed)
				message(FATAL_ERROR "bug-orig: ${bas}: @fixed twice")
			endif()
			set(fixed ${CMAKE_MATCH_1})
		elseif(h MATCHES "^<semicolon> @configs (.+)$")
			separate_arguments(configs UNIX_COMMAND "${CMAKE_MATCH_1}")
			set(configs_given ON)
		elseif(h MATCHES "^<semicolon> @machine ([a-z0-9-]+)$")
			string(APPEND extra "machine=${CMAKE_MATCH_1}\n")
		elseif(h MATCHES "^<semicolon> @rwe (.+)$")
			separate_arguments(rwe UNIX_COMMAND "${CMAKE_MATCH_1}")
		elseif(h MATCHES "^<semicolon> @pass ([a-z_]+=.*)$")
			string(REPLACE "<semicolon>" ";" line "${CMAKE_MATCH_1}")
			string(CONFIGURE "${line}" line @ONLY)
			string(APPEND extra "${line}\n")
			if(line MATCHES "^timeout=([0-9]+)$")
				set(session_limit ${CMAKE_MATCH_1})
			endif()
		else()
			message(FATAL_ERROR "bug-orig: ${bas}: bad header line: ${h}")
		endif()
	endforeach()
	# a fixed test: no reference, judged in clib only (blitz2-c with the C
	# libraries, as the port ships; port needs the CD's libraries)
	if(fixed)
		if(arg_REGRESSION OR DEFINED arg_WANT)
			message(FATAL_ERROR "bug-orig: ${bas}: @fixed in a port regression or self test")
		endif()
		set(configs clib)
	endif()
	# a port regression test: no reference either, clib unless @configs
	# names others
	set(noref ${fixed})
	if(arg_REGRESSION)
		set(noref ON)
		if(NOT configs_given)
			set(configs clib)
		endif()
	endif()

	# RWE userlibs: per configuration the reference or the C build, the
	# variant's libraries for clib, a token list with the libraries
	set(clib_dir ${BLITZ2_DIR})
	set(ref_blitz2 ${REF_BLITZ2})
	set(fixtures "")
	set(tok ${tokens})
	set(tokdeps "")
	if(rwe)
		# the reference compiler: 2.10 (1.95 never loads a library of
		# blitz2:userlibs/DIR, #2)
		set(ref_blitz2 ${CD_BLITZ2}/BLITZ2)
		set(orig_by "the CD's blitz2 2.10 with the CD's libraries and the reference RWE library")
		set(variant "")
		set(objs "")
		foreach(n IN LISTS rwe)
			get_property(info GLOBAL PROPERTY RWE_LIB_${n})
			if(NOT info)
				message(FATAL_ERROR "bug-orig: ${bas}: @rwe ${n}: no RWE userlib of that name "
					"(libs/userlibs/rwe/${n}.c, RWE_LIBS)")
			endif()
			string(REPLACE "|" ";" info "${info}")
			list(GET info 0 obj)
			list(GET info 1 reffile)
			list(GET info 2 fixture)
			list(GET info 3 v)
			list(GET info 4 vout)
			if(variant AND NOT v STREQUAL variant)
				message(FATAL_ERROR "bug-orig: ${bas}: @rwe: ${n} is built for the variant ${v}, "
					"the others for ${variant}")
			endif()
			set(variant ${v})
			set(clib_dir ${vout})
			# the reference library only for ref (a fixed test and a port
			# regression test have none)
			if(NOT noref)
				string(APPEND extra "file=blitz2/userlibs/rwe/${n}.obj|${reffile}|ref\n")
				list(APPEND fixtures ${fixture})
			endif()
			foreach(c IN LISTS configs)
				string(APPEND extra "file=blitz2/userlibs/rwe/${n}.obj|${obj}|${c}\n")
			endforeach()
			list(APPEND objs ${obj})
			list(APPEND tokdeps ${v}-${n}_obj)
		endforeach()
		list(APPEND tokdeps ${variant}-acidlibs ${variant}-deflibs)
		set(tok ${CMAKE_CURRENT_BINARY_DIR}/tokens/${id}.txt)
		add_custom_command(OUTPUT ${tok}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/tokens
			COMMAND ${BLITZTOKENS} -o ${tok} $<TARGET_FILE:blitz2> ${clib_dir}/acidlibs
				${clib_dir}/deflibs ${objs}
			DEPENDS ${objs} ${clib_dir}/acidlibs ${clib_dir}/deflibs blitz2 ${BLITZTOKENS}
				host_tools
			COMMENT "bug-orig ${id}: token list with the RWE userlibs"
			VERBATIM)
		set(bug_orig_deps ${bug_orig_deps} ${tokdeps} PARENT_SCOPE)
	endif()
	set(src ${CMAKE_CURRENT_BINARY_DIR}/src/${id}.bb2)
	add_custom_command(OUTPUT ${src}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/src
		COMMAND ${TEDSRC} tok ${tok} ${bas} ${src}
		DEPENDS ${bas} ${tok} ${TEDSRC} host_tools
		COMMENT "Tokenising bug-orig ${id}.bas"
		VERBATIM)
	set(bug_orig_sources ${bug_orig_sources} ${src} PARENT_SCOPE)

	set(work ${CMAKE_CURRENT_BINARY_DIR}/${id})
	set(text "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${bas}\n")
	string(APPEND text "work=${work}\nreport=${work}.txt\nruntime_errors=off\n"
		"timeout=${BUG_ORIG_TIMEOUT}\nparallel=${parallel}\n"
		"ted=$<TARGET_FILE:tedstub>\nlauncher=$<TARGET_FILE:reqtrap>\n"
		"rcput=$<TARGET_FILE:rcput>\n")
	if(fixed)
		# no reference and nothing of the CD (a test that needs it says so:
		# @pass defaultdbug=@CD_BLITZ2@/DEFAULTDBUG, @pass mount=BB2|@CD@)
		string(APPEND text "fixed=${fixed}\n")
	elseif(arg_REGRESSION)
		# the same for a port regression test
		string(APPEND text "regression=on\n")
	else()
		# an open bug: judged against the recorded original (ID.orig, next
		# to the test).  The first configuration is the original, which only
		# the recording run (BUGORIG_RECORD=1, ctest bug_orig_record_ID,
		# label originals) starts; the normal run drops it and what is only
		# for it, and judges the others without the CD files it lacks
		# (corpus.h orig=)
		string(APPEND text "orig=${dir}/${id}.orig\norig_by=${orig_by}\n"
			"defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n"
			"config=ref|ref|${ref_blitz2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
	endif()
	foreach(c IN LISTS configs)
		if(c STREQUAL "port")
			string(APPEND text
				"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
		elseif(c STREQUAL "clib")
			string(APPEND text
				"config=clib|output|$<TARGET_FILE:blitz2>|${clib_dir}/deflibs|${clib_dir}/acidlibs\n")
		else()
			message(FATAL_ERROR "bug-orig: ${bas}: unknown configuration ${c} (port, clib)")
		endif()
	endforeach()
	# the volume BB2: (the CD) only for a test whose source names it
	if(NOT noref AND bas_text MATCHES "[Bb][Bb]2:")
		string(APPEND text "mount=BB2|${CD}\n")
	endif()
	string(APPEND text "source=${src}\nexpect=${expect}\n${extra}")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")

	# a session per judged configuration, one after the other if there are
	# not enough emulator slots free, each maybe waiting for one
	list(LENGTH configs nsessions)
	math(EXPR limit
		"${BUG_ORIG_TIMEOUT} * ${nsessions} + 60 + ${nsessions} * ${EMULATION_QUEUE_WAIT}")
	# the order: ctest learns the cost only from passing tests, and these
	# fail on purpose; the session limit times the sessions puts the tests
	# that can take long first (cmake/bug-orig-order.cmake)
	math(EXPR cost "${session_limit} * ${nsessions}")
	list(REMOVE_DUPLICATES fixtures)
	list(REMOVE_ITEM fixtures "")
	# the recording of the original (an open bug): one session of the
	# reference configuration, writes ID.orig; label originals only (in no
	# preset but originals), needs the CD and, for RWE libraries, the
	# reference build
	if(NOT noref)
		string(REGEX REPLACE "^bug_orig_" "bug_orig_record_" rec_name "${name}")
		math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
		add_test(NAME ${rec_name} COMMAND ${CORPUSRUN} pass ${work}.pass)
		set_tests_properties(${rec_name} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
			TIMEOUT ${rec_limit} ENVIRONMENT BUGORIG_RECORD=1)
		if(fixtures)
			set_tests_properties(${rec_name} PROPERTIES FIXTURES_REQUIRED "${fixtures}")
		endif()
		set(fixtures "")
	endif()
	if(NOT DEFINED arg_WANT AND NOT arg_REGRESSION AND NOT fixed)
		bug_orig_add_test(${name} COST ${cost} COMMAND ${CORPUSRUN} pass ${work}.pass
			LABELS bug-orig emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
		return()
	endif()
	# a fixed test (it passes: ctest learns its cost), a port regression or
	# a self test: label emulator only
	if(DEFINED arg_WANT)
		add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${CORPUSRUN} -DSUB=pass
			-DPASS=${work}.pass -DWANT=${arg_WANT} -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
	else()
		add_test(NAME ${name} COMMAND ${CORPUSRUN} pass ${work}.pass)
	endif()
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT ${limit} COST ${cost})
	if(fixtures)
		set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED "${fixtures}")
	endif()
endfunction()

# the test data: AREA/NAME.spec -> testdata/NAME
set(testdata_files)
file(GLOB specs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.spec)
foreach(spec IN LISTS specs)
	get_filename_component(name ${spec} NAME)
	string(REGEX REPLACE "\\.spec$" "" name "${name}")
	set(out ${TESTDATA}/${name})
	if(out IN_LIST testdata_files)
		message(FATAL_ERROR "bug-orig: test data ${name} described twice (${spec})")
	endif()
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTDATA}
		COMMAND ${MKTESTDATA} ${spec} ${out}
		DEPENDS ${spec} ${MKTESTDATA} host_tools
		COMMENT "Generating bug-orig test data ${name}"
		VERBATIM)
	list(APPEND testdata_files ${out})
endforeach()

# further sources: AREA/NAME.src (text like ID.bas) -> testdata/NAME,
# tokenised like the tests (e.g. the source of a resident, tedstub
# resident=)
file(GLOB srcs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.src)
foreach(s IN LISTS srcs)
	get_filename_component(name ${s} NAME)
	string(REGEX REPLACE "\\.src$" "" name "${name}")
	set(out ${TESTDATA}/${name})
	if(out IN_LIST testdata_files)
		message(FATAL_ERROR "bug-orig: test data ${name} described twice (${s})")
	endif()
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTDATA}
		COMMAND ${TEDSRC} tok ${tokens} ${s} ${out}
		DEPENDS ${s} ${tokens} ${TEDSRC} host_tools
		COMMENT "Tokenising bug-orig source ${name}"
		VERBATIM)
	list(APPEND testdata_files ${out})
endforeach()

file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/*.bas)
foreach(bas IN LISTS tests)
	get_filename_component(dir ${bas} DIRECTORY)
	get_filename_component(area ${dir} NAME)
	if(area STREQUAL "selftest")
		continue()
	endif()
	get_filename_component(id ${bas} NAME_WE)
	if(area STREQUAL "regression")
		# a port regression test: ctest regression_ID, label emulator only
		string(REPLACE "-" "_" name "regression_${id}")
		bug_orig_test(${bas} ${name} REGRESSION)
	else()
		string(REPLACE "-" "_" name "bug_orig_${id}")
		bug_orig_test(${bas} ${name})
	endif()
endforeach()

# the self test: an expectation equal to what the original does must be
# reported as broken (exit status 3), not as a reproduced bug
bug_orig_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest/bug-orig-selftest-broken.bas
	bug_orig_selftest_broken WANT 3)
# the recorded original (ID.orig) with broken input, before any session (no
# emulator, no files of the pass needed): a file that is missing or not an
# observation is an error (2), one that meets the expectation is a broken
# test (3)
set(orig_selftest ${CMAKE_CURRENT_SOURCE_DIR}/selftest)
function(bug_orig_orig_test name want orig)
	set(pass ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass)
	file(WRITE ${pass} "# ${name}: generated by tests/bug-orig/CMakeLists.txt\n"
		"work=${CMAKE_CURRENT_BINARY_DIR}/${name}\nreport=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt\n"
		"ted=/nonexistent/ted\nlauncher=/nonexistent/launcher\ndefaultdbug=/nonexistent/dbug\n"
		"config=ref|ref|/nonexistent/a|/nonexistent/b|/nonexistent/c\n"
		"config=clib|output|/nonexistent/a|/nonexistent/b|/nonexistent/c\n"
		"source=/nonexistent/x.bb2\nexpect=${orig_selftest}/bug-orig-selftest-broken.expect\n"
		"orig=${orig}\n")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${CORPUSRUN} -DSUB=pass
		-DPASS=${pass} -DWANT=${want} -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
endfunction()
bug_orig_orig_test(bug_orig_orig_missing 2 ${orig_selftest}/nonexistent.orig)
bug_orig_orig_test(bug_orig_orig_garbage 2 ${orig_selftest}/garbage.orig)
bug_orig_orig_test(bug_orig_orig_broken 3 ${orig_selftest}/bug-orig-selftest-broken.orig)
# the self test of a port regression test (the expect tests of
# tests/blitz2 too): an expectation the port does not meet must fail
# (exit status 1, REGRESSED)
bug_orig_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest/regression-selftest-wrong.bas
	regression_selftest_wrong REGRESSION WANT 1)

# inject's fill= option (inject-fill.cmake), and the check itself: a wrong
# expect file must fail
function(inject_fill name expect)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN}
		-DRESULTS=${RESULTS} -DINJECT=${INJECT} -DMEMFILL=$<TARGET_FILE:memfill>
		-DEXPECT=${expect} -DWORK=${work}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/inject-fill.cmake)
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT 240)
endfunction()
inject_fill(inject_fill ${CMAKE_CURRENT_SOURCE_DIR}/inject-fill.expect)
inject_fill(inject_fill_broken ${CMAKE_CURRENT_SOURCE_DIR}/inject-fill.broken.expect)
set_tests_properties(inject_fill_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"FAIL:[ \n]+results[ \n]+match:[ \n]+[^ \n]+[ \n]+expect[ \n]+line[ \n]+10:[ \n]+\"plain:[ \n]+a5[^\"]*\",[ \n]+output[ \n]+line[ \n]+10:[ \n]+\"plain:[ \n]+00")

# inject's datestamp= option (inject-datestamp.cmake), and the check itself:
# a wrong expect file must fail
function(inject_datestamp name expect)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN}
		-DRESULTS=${RESULTS} -DINJECT=${INJECT} -DDSDAYS=$<TARGET_FILE:dsdays>
		-DEXPECT=${expect} -DWORK=${work}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/inject-datestamp.cmake)
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT 240)
endfunction()
inject_datestamp(inject_datestamp ${CMAKE_CURRENT_SOURCE_DIR}/inject-datestamp.expect)
inject_datestamp(inject_datestamp_broken
	${CMAKE_CURRENT_SOURCE_DIR}/inject-datestamp.broken.expect)
set_tests_properties(inject_datestamp_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"FAIL:[ \n]+results[ \n]+match:[ \n]+[^ \n]+[ \n]+expect[ \n]+line[ \n]+24:[ \n]+\"days=17828[ \n]+minute=0[ \n]+tick=0\",[ \n]+output[ \n]+line[ \n]+24:[ \n]+\"days=17829")

add_custom_target(bug_orig_sources ALL DEPENDS ${bug_orig_sources} ${testdata_files})
# for the equivalence of blitz2-ab3 (tests/ab3)
set_property(GLOBAL PROPERTY BUG_ORIG_SOURCES ${bug_orig_sources})
set_property(GLOBAL PROPERTY BUG_ORIG_USERLIB ${USERLIB})
if(bug_orig_deps)
	list(REMOVE_DUPLICATES bug_orig_deps)
	add_dependencies(bug_orig_sources ${bug_orig_deps})
endif()

# The bug-orig tests of TED: ted/ID.drv, a TED A/B script (tests/ted-ab,
# runner/tedab.h; @file, @command and the @expect kinds data, stdout, log,
# screen are made for these tests) whose expectations are the correct
# behaviour.  tedab in its bug-orig mode plays it against the original TED
# (R5: TED 1.1 under blitz2 2.10, the reference) and TED-C (C5), each run in
# an emulator session of its own, and gives the verdict like corpusrun
# (exit status 0 FIXED, 1 REPRODUCED, 3 BROKEN, 4 DIFFERS); report
# ID/report.txt, the runs in ID/sNNN/vol/runs/ID/XX.  ctest bug_orig_ID,
# labels bug-orig and emulator, skipped (77) without Amiberry or the CD.
# The scripts without the emulator: ctest bug_orig_ted_lint (host).
set(ted_scripts ${CMAKE_CURRENT_SOURCE_DIR}/ted)
# the two runs of a test in parallel, unless CORPUS_PARALLEL says 1
if(CORPUS_PARALLEL GREATER 1)
	set(ted_parallel 2)
else()
	set(ted_parallel 1)
endif()
function(bug_orig_ted_test scripts id name)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "WANT" "")
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${id})
	set(text "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${scripts}/${id}.drv\n")
	string(APPEND text "work=${work}\nreport=${work}.txt\nscripts=${scripts}\nfilter=${id}\n"
		"gates=${PROJECT_SOURCE_DIR}/tests/ted-ab/scripts\n"
		"src=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\ncombos=R5,C5\nbugorig=1\n"
		"corpus_src=${CMAKE_BINARY_DIR}/tests/corpus/src/tests\n"
		"corpus_out=${CMAKE_BINARY_DIR}/tests/corpus/corpus/ref/vol/out\n"
		"expect_dir=${CMAKE_BINARY_DIR}/tests/ted-ab/expect\n"
		"cd=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1\n"
		"oracle=${PROJECT_SOURCE_DIR}/tests/data/ted-oracle.sha256\n"
		"fonts=${PROJECT_SOURCE_DIR}/archive/sources/ted-1.1\n"
		"teddrive=$<TARGET_FILE:teddrive>\nteddrivetest=$<TARGET_FILE:teddrivetest>\n"
		"tedprobe=$<TARGET_FILE:tedprobe>\nreqtrap=$<TARGET_FILE:reqtrap>\n"
		"inject=${INJECT}\nwbrun=${WBRUN}\n"
		"ted_ref=${CMAKE_BINARY_DIR}/programs/ted/ref/ted\nted_c=$<TARGET_FILE:ted>\n"
		"blitz2_ref=${CD_BLITZ2}/BLITZ2\nblitz2_c=$<TARGET_FILE:blitz2>\n"
		"timeout=${BUG_ORIG_TIMEOUT}\nparallel=${ted_parallel}\n"
		"orig=${scripts}/${id}.orig\n"
		"orig_by=TED 1.1 (the original, ted.s) under the CD's blitz2 2.10 (R5)\n")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	# the recording of the original (BUGORIG_RECORD=1): only the reference
	# run (R5), writes ID.orig; label originals only
	string(REGEX REPLACE "^bug_orig_" "bug_orig_record_" rec_name "${name}")
	math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
	add_test(NAME ${rec_name} COMMAND ${TEDAB} run ${work}.pass)
	set_tests_properties(${rec_name} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
		TIMEOUT ${rec_limit} ENVIRONMENT BUGORIG_RECORD=1)
	# one session (C5; the reference R5 only for the recording), maybe
	# repeated once (gates failed), maybe waiting for a slot
	math(EXPR limit "${BUG_ORIG_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
	# the order (see bug_orig_test): the limit of the session
	set(cost ${BUG_ORIG_TIMEOUT})
	if(DEFINED arg_WANT)
		add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
			-DPASS=${work}.pass -DWANT=${arg_WANT} -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
		set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
			TIMEOUT ${limit} COST ${cost})
	else()
		bug_orig_add_test(${name} COST ${cost} COMMAND ${TEDAB} run ${work}.pass
			LABELS bug-orig emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
	endif()
endfunction()

file(GLOB drvs CONFIGURE_DEPENDS ${ted_scripts}/*.drv)
foreach(drv IN LISTS drvs)
	get_filename_component(id ${drv} NAME_WE)
	string(REPLACE "-" "_" name "bug_orig_${id}")
	bug_orig_ted_test(${ted_scripts} ${id} ${name})
endforeach()
if(drvs)
	# the scripts (syntax, directives) without the emulator
	set(lint ${CMAKE_CURRENT_BINARY_DIR}/ted_lint.pass)
	string(CONCAT text
		"work=${CMAKE_CURRENT_BINARY_DIR}/ted_lint\nreport=${CMAKE_CURRENT_BINARY_DIR}/ted_lint.txt\n"
		"scripts=${ted_scripts}\nsrc=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
		"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
		"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
		"blitz2_ref=-\nblitz2_c=-\n")
	file(GENERATE OUTPUT ${lint} CONTENT "${text}")
	add_test(NAME bug_orig_ted_lint COMMAND ${TEDAB} lint ${lint})
endif()

# the self test of the TED variant: an expectation equal to what the
# original TED does must be reported as broken (exit status 3)
bug_orig_ted_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-bug-orig-selftest-broken
	bug_orig_ted_selftest_broken WANT 3)

# the recorded original of a TED test with broken input, before any session
# (no emulator): missing or not an observation is an error (2)
function(bug_orig_ted_orig_test name orig)
	set(pass ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass)
	string(CONCAT text
		"work=${CMAKE_CURRENT_BINARY_DIR}/${name}\nreport=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt\n"
		"scripts=${CMAKE_CURRENT_SOURCE_DIR}/selftest\nsrc=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
		"combos=R5,C5\nbugorig=1\norig=${orig}\n"
		"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
		"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
		"blitz2_ref=-\nblitz2_c=-\n")
	file(WRITE ${pass} "${text}")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
		-DPASS=${pass} -DWANT=2 -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
endfunction()
bug_orig_ted_orig_test(bug_orig_ted_orig_missing ${CMAKE_CURRENT_SOURCE_DIR}/selftest/nonexistent.orig)
bug_orig_ted_orig_test(bug_orig_ted_orig_garbage ${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig)

# all bug-orig tests (also those of libs/tests), the longest first
# (cmake/bug-orig-order.cmake)
bug_orig_declare_tests()
# the order as ctest has it, and the check itself: a test with a larger
# COST after a smaller one must fail
add_test(NAME bug_orig_order COMMAND ${CMAKE_COMMAND} -DBUILD=${CMAKE_BINARY_DIR} -DLABEL=bug-orig
	-P ${PROJECT_SOURCE_DIR}/cmake/check-test-order.cmake)
set(broken ${CMAKE_CURRENT_BINARY_DIR}/order-broken.json)
file(WRITE ${broken} "{\"tests\":[\n"
	"{\"name\":\"x_other\",\"properties\":[{\"name\":\"COST\",\"value\":900},"
	"{\"name\":\"LABELS\",\"value\":[\"emulator\"]}]},\n"
	"{\"name\":\"x_long\",\"properties\":[{\"name\":\"COST\",\"value\":270},"
	"{\"name\":\"LABELS\",\"value\":[\"bug-orig\",\"emulator\"]}]},\n"
	"{\"name\":\"x_short\",\"properties\":[{\"name\":\"COST\",\"value\":60},"
	"{\"name\":\"LABELS\",\"value\":[\"emulator\",\"bug-orig\"]}]},\n"
	"{\"name\":\"x_late\",\"properties\":[{\"name\":\"COST\",\"value\":180},"
	"{\"name\":\"LABELS\",\"value\":[\"bug-orig\"]}]}]}\n")
add_test(NAME bug_orig_order_broken COMMAND ${CMAKE_COMMAND} -DJSON=${broken} -DLABEL=bug-orig
	-P ${PROJECT_SOURCE_DIR}/cmake/check-test-order.cmake)
set_tests_properties(bug_orig_order_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"x_late \\(COST 180\\) is declared after[ \n]+x_short \\(COST 60\\)")

# a fixed test keeps no recorded original (check-fixed-orig.cmake): the
# configure above stops on a stale ID.orig; the check with a fake tree (a
# fixed .bas and a fixed TED .drv with their .orig) must name both
add_test(NAME bug_orig_fixed_orig_broken COMMAND ${CMAKE_COMMAND}
	-DROOT=${CMAKE_CURRENT_SOURCE_DIR}/selftest/fixed-orig
	-P ${CMAKE_CURRENT_SOURCE_DIR}/check-fixed-orig.cmake)
set_tests_properties(bug_orig_fixed_orig_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"compiler/fixed-kept\\.orig \\([^)]*fixed-kept\\.bas is fixed\\)[ \n]+[^\n]*ted/ted-fixed-kept\\.orig"
	FAIL_REGULAR_EXPRESSION "open-kept|fixed-clean")
