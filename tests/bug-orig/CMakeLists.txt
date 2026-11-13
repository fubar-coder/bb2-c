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
#                                           the libraries (bbsrc tokens)
#                      ; @fixed #NNN        the bug is fixed in the port
#                                           (issue NNN): a fixed test,
#                                           see below
#   AREA/ID.expect   the correct behaviour (sections, bugorig.h)
#   AREA/ID.orig     the recorded observation of the original (an open bug;
#                    not for a fixed test or a port regression test).  The
#                    normal run judges against it and starts no session of
#                    the original; ctest bug_orig_record_ID (label
#                    originals, preset originals, pass key record=check)
#                    compares the original (the reference build of
#                    blitz2.s, or the CD's 2.10 for @rwe) with it, needs the
#                    CD and writes nothing
# ID is the bug ID of the issue; ctest bug_orig_ID (- as _), labels
# bug-orig and emulator, skipped (77) without Amiberry.  Without the CD
# the configuration port (the CD's libraries) is not judged and the run
# goes without blitz2:defaultdbug (also with runtime_errors=on: the
# programs are executables, which never contain the module);
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
	cmake_parse_arguments(PARSE_ARGV 2 arg "REGRESSION" "WANT;CHANGED_ORIG" "")
	get_filename_component(id ${bas} NAME_WE)
	get_filename_component(dir ${bas} DIRECTORY)
	set(expect ${dir}/${id}.expect)
	if(NOT EXISTS ${expect})
		message(FATAL_ERROR "bug-orig: ${bas} has no ${id}.expect")
	endif()
	set(configs port clib)
	set(rwe "")
	set(extra "")
	set(ref_extra "")
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
			# regression test have none; the recording runs of a fixed test
			# get it, ref_extra)
			string(APPEND ref_extra "file=blitz2/userlibs/rwe/${n}.obj|${reffile}|ref\n")
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
			COMMAND ${BBSRC} tokens -o ${tok} $<TARGET_FILE:blitz2> ${clib_dir}/acidlibs
				${clib_dir}/deflibs ${objs}
			DEPENDS ${objs} ${clib_dir}/acidlibs ${clib_dir}/deflibs blitz2 ${BBSRC}
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
	# the pass file in parts: the keys of every pass (@WORK@ its work
	# directory), the reference, the judged configurations, the rest
	set(head "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${bas}\n")
	string(APPEND head "work=@WORK@\nreport=@WORK@.txt\nruntime_errors=off\n"
		"timeout=${BUG_ORIG_TIMEOUT}\nparallel=${parallel}\n"
		"ted=$<TARGET_FILE:tedstub>\nlauncher=$<TARGET_FILE:reqtrap>\n"
		"rcput=$<TARGET_FILE:rcput>\n")
	# the original: the reference configuration, judged against the recorded
	# original (ID.orig, next to the test).  An open bug's normal run drops
	# it and what is only for it, and judges the others without the CD files
	# it lacks (corpus.h orig=); only the compare test (record=check, ctest
	# bug_orig_record_ID, label originals) and the recording runs of the
	# target bug-orig-record (record=write) start it
	string(CONCAT ref_part "orig=${dir}/${id}.orig\norig_by=${orig_by}\n"
		"defaultdbug=${CD_BLITZ2}/DEFAULTDBUG\n"
		"config=ref|ref|${ref_blitz2}|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
	set(cfg_part "")
	foreach(c IN LISTS configs)
		if(c STREQUAL "port")
			string(APPEND cfg_part
				"config=port|exe|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n")
		elseif(c STREQUAL "clib")
			string(APPEND cfg_part
				"config=clib|output|$<TARGET_FILE:blitz2>|${clib_dir}/deflibs|${clib_dir}/acidlibs\n")
		else()
			message(FATAL_ERROR "bug-orig: ${bas}: unknown configuration ${c} (port, clib)")
		endif()
	endforeach()
	# the volume BB2: (the CD) only for a test whose source names it (a
	# fixed test, which uses nothing of the CD, only in the recording runs)
	set(mount_part "")
	if(bas_text MATCHES "[Bb][Bb]2:")
		set(mount_part "mount=BB2|${CD}\n")
	endif()
	set(tail "source=${src}\nexpect=${expect}\n${extra}")
	if(fixed)
		# no reference and nothing of the CD (a test that needs it says so:
		# @pass defaultdbug=@CD_BLITZ2@/DEFAULTDBUG, @pass mount=BB2|@CD@)
		set(text "${head}fixed=${fixed}\n${cfg_part}${tail}")
	elseif(arg_REGRESSION)
		# the same for a port regression test
		set(text "${head}regression=on\n${cfg_part}${tail}")
	else()
		set(text "${head}${ref_part}${cfg_part}${mount_part}${tail}")
	endif()
	# the passes with the reference (the compare test, the recording runs)
	set(rtext "${head}${ref_part}${cfg_part}${mount_part}${tail}")
	if(noref)
		string(APPEND rtext "${ref_extra}")
	endif()
	string(REPLACE "@WORK@" "${work}" ptext "${text}")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${ptext}")
	# the recording runs of the target bug-orig-record (record=write, the
	# observation into the build directory; the second with another memory
	# layout, a larger Stack), for every test of the original's bugs, open or
	# fixed (not for a port regression test or a self test)
	if(NOT arg_REGRESSION AND NOT DEFINED arg_WANT)
		foreach(k 1 2)
			string(REPLACE "@WORK@" "${work}-rec${k}" ptext "${rtext}")
			string(APPEND ptext "record=write\nrecord_to=${work}-rec${k}.orig\n")
			if(k EQUAL 2)
				string(APPEND ptext "layout=stack\n")
			endif()
			file(GENERATE OUTPUT ${work}-rec${k}.pass CONTENT "${ptext}")
		endforeach()
		set_property(GLOBAL APPEND PROPERTY BUG_ORIG_RECORD_LIST
			"${id}|blitz|${work}-rec1.pass|${work}-rec2.pass|${work}-rec1.orig|${work}-rec2.orig|${dir}/${id}.orig")
	endif()

	# a session per judged configuration, one after the other if there are
	# not enough emulator slots free, each maybe waiting for one
	list(LENGTH configs nsessions)
	math(EXPR limit
		"${session_limit} * ${nsessions} + 60 + ${nsessions} * ${EMULATION_QUEUE_WAIT}")
	# the order: ctest learns the cost only from passing tests, and these
	# fail on purpose; the session limit times the sessions puts the tests
	# that can take long first (cmake/bug-orig-order.cmake)
	math(EXPR cost "${session_limit} * ${nsessions}")
	list(REMOVE_DUPLICATES fixtures)
	list(REMOVE_ITEM fixtures "")
	# the compare test of the recorded original (an open bug): one session
	# of the reference configuration, its observation compared with ID.orig
	# (pass key record=check, ID-record.pass; it writes no file of the
	# source tree); label originals only (in no preset but originals), needs
	# the CD and, for RWE libraries, the reference build.  CHANGED_ORIG: the
	# pass ID-changed.pass of the same against another recorded original
	# (broken input)
	if(NOT noref)
		string(REGEX REPLACE "^bug_orig_" "bug_orig_record_" rec_name "${name}")
		math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
		# the reference build of blitz2.s (not ALL)
		list(APPEND fixtures originals_refbuild)
		set(rec_kinds record)
		set(rec_origs ${dir}/${id}.orig)
		if(arg_CHANGED_ORIG)
			list(APPEND rec_kinds changed)
			list(APPEND rec_origs ${arg_CHANGED_ORIG})
		endif()
		foreach(rk ro IN ZIP_LISTS rec_kinds rec_origs)
			string(REPLACE "@WORK@" "${work}-${rk}" ptext "${rtext}")
			string(REPLACE "orig=${dir}/${id}.orig\n" "orig=${ro}\n" ptext "${ptext}")
			file(GENERATE OUTPUT ${work}-${rk}.pass CONTENT "${ptext}record=check\n")
		endforeach()
		add_test(NAME ${rec_name} COMMAND ${CORPUSRUN} pass ${work}-record.pass)
		set_tests_properties(${rec_name} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
			TIMEOUT ${rec_limit} FIXTURES_REQUIRED "${fixtures}")
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
# Its compare test against a recorded original with a changed line
# (bug_orig_record_changed, label originals) must fail naming the line.
bug_orig_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest/bug-orig-selftest-broken.bas
	bug_orig_selftest_broken WANT 3
	CHANGED_ORIG ${CMAKE_CURRENT_SOURCE_DIR}/selftest/record-changed.orig)
add_test(NAME bug_orig_record_changed COMMAND ${CMAKE_COMMAND} -DRUNNER=${CORPUSRUN} -DSUB=pass
	-DPASS=${CMAKE_CURRENT_BINARY_DIR}/bug-orig-selftest-broken-changed.pass -DWANT=1
	-P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(bug_orig_record_changed PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${rec_limit} FIXTURES_REQUIRED originals_refbuild PASS_REGULAR_EXPRESSION
	"DIFFERS: \\[run\\] line 1: recorded \"hullo\", observed \"hello\"")
# the recorded original (ID.orig) with broken input, before any session (no
# emulator, no files of the pass needed): a file that is missing, not an
# observation or with a wildcard line without a reason is an error (2), one
# that meets the expectation is a broken test (3)
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
# a wildcard line (`*` at the end, `...`) without a "# varies: REASON" line
# before it is an error naming the file and the line (2)
bug_orig_orig_test(bug_orig_wildcard_noreason 2 ${orig_selftest}/wildcard-noreason.orig)
set_tests_properties(bug_orig_wildcard_noreason PROPERTIES PASS_REGULAR_EXPRESSION
	"wildcard-noreason\\.orig:7: a wildcard line \\(`\\*` at the end, `\\.\\.\\.`\\) without a \"# varies: REASON\" line before it")
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

# inject's guardseg option (inject-guardseg.cmake), and the check itself: an
# expect file without the write behind the program's hunk must fail
function(inject_guardseg name expect)
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN}
		-DRESULTS=${RESULTS} -DINJECT=${INJECT} -DSEGWRITE=$<TARGET_FILE:segwrite>
		-DEXPECT=${expect} -DWORK=${work}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/inject-guardseg.cmake)
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT 240)
endfunction()
inject_guardseg(inject_guardseg ${CMAKE_CURRENT_SOURCE_DIR}/inject-guardseg.expect)
inject_guardseg(inject_guardseg_broken ${CMAKE_CURRENT_SOURCE_DIR}/inject-guardseg.broken.expect)
set_tests_properties(inject_guardseg_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"FAIL:[ \n]+results[ \n]+match:[ \n]+[^ \n]+[ \n]+expect[ \n]+line[ \n]+10:[ \n]+\"rc=0\",[ \n]+output[ \n]+line[ \n]+10:[ \n]+\"inject:[ \n]+guard:[ \n]+written[ \n]+behind")

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
# A fixed test has the header "; @fixed #N" (no pair; the bug is fixed in
# TED-C): no ID.orig, no session of the original, judged by the script's
# expectations alone (pass key fixed=1: FIXED, or REGRESSED naming the
# bug).  It is two tests: bug_orig_ID (label emulator, so the presets
# emulator and all) runs Cc, TED-C under blitz2-c with the libraries of the
# build (libs=), and needs no CD; bug_orig_ID_originals (label originals)
# runs C5, TED-C under the CD's blitz2 2.10 with the CD's libraries.
# The scripts without the emulator: ctest bug_orig_ted_lint (host).
set(ted_scripts ${CMAKE_CURRENT_SOURCE_DIR}/ted)
# the two runs of a test in parallel, unless CORPUS_PARALLEL says 1
if(CORPUS_PARALLEL GREATER 1)
	set(ted_parallel 2)
else()
	set(ted_parallel 1)
endif()
# the pass file of a TED bug-orig test: WORK the directory, COMBOS, EXTRA
# further keys
function(bug_orig_ted_pass scripts id name work combos extra)
	set(text "# ${name}: generated by tests/bug-orig/CMakeLists.txt from ${scripts}/${id}.drv\n")
	string(APPEND text "work=${work}\nreport=${work}.txt\nscripts=${scripts}\nfilter=${id}\n"
		"gates=${PROJECT_SOURCE_DIR}/tests/ted-ab/scripts\n"
		"src=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\ncombos=${combos}\nbugorig=1\n"
		"corpus_src=${CMAKE_BINARY_DIR}/tests/corpus/src/tests\n"
		"corpus_out=${CMAKE_BINARY_DIR}/tests/corpus/corpus_originals/ref/vol/out\n"
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
		${extra})
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
endfunction()
function(bug_orig_ted_test scripts id name)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "WANT" "")
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${id})
	# a fixed test: the directive "@fixed #N" alone (the runner refuses a pair)
	file(READ ${scripts}/${id}.drv drv_text)
	string(REPLACE ";" "<semicolon>" drv_text "${drv_text}")
	set(orig_keys "orig=${scripts}/${id}.orig\norig_by=TED 1.1 (the original, ted.s) under the CD's blitz2 2.10 (R5)\n")
	if(drv_text MATCHES "(^|\n)<semicolon> @fixed #[1-9][0-9]*[ \t\r]*(\n|$)")
		set(fixed_test TRUE)
		# Cc with the build's libraries (all), C5 with the CD's (originals)
		bug_orig_ted_pass(${scripts} ${id} ${name} ${work} Cc "fixed=1\nlibs=${BLITZ2_DIR}\n")
		bug_orig_ted_pass(${scripts} ${id} ${name} ${work}_originals C5 "fixed=1\n")
	else()
		set(fixed_test FALSE)
		bug_orig_ted_pass(${scripts} ${id} ${name} ${work} R5,C5 "${orig_keys}")
		# the compare test of the recorded original (record=check): only
		# the reference run (R5), compared with ID.orig, writes no file of
		# the source tree
		bug_orig_ted_pass(${scripts} ${id} ${name} ${work}_record R5,C5 "${orig_keys}record=check\n")
	endif()
	# the recording runs of the target bug-orig-record (record=write, the
	# observation of R5 into the build directory), open or fixed, not for a
	# self test; the second with another memory layout (teddrive's Stack
	# 16384 bytes larger, as corpusrun's layout=stack)
	if(NOT DEFINED arg_WANT)
		set(layout "")
		foreach(k 1 2)
			bug_orig_ted_pass(${scripts} ${id} ${name} ${work}_rec${k} R5,C5
				"${orig_keys}record=write\nrecord_to=${work}_rec${k}.orig\n${layout}")
			set(layout "stack=56384\n")
		endforeach()
		set_property(GLOBAL APPEND PROPERTY BUG_ORIG_RECORD_LIST
			"${id}|ted|${work}_rec1.pass|${work}_rec2.pass|${work}_rec1.orig|${work}_rec2.orig|${scripts}/${id}.orig")
	endif()
	# the compare test of the recorded original: label originals only (a
	# fixed test has none)
	if(NOT fixed_test)
		string(REGEX REPLACE "^bug_orig_" "bug_orig_record_" rec_name "${name}")
		math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
		add_test(NAME ${rec_name} COMMAND ${TEDAB} run ${work}_record.pass)
		# the reference build of ted.s (not ALL)
		set_tests_properties(${rec_name} PROPERTIES LABELS originals SKIP_RETURN_CODE 77
			TIMEOUT ${rec_limit} FIXTURES_REQUIRED originals_refbuild)
	endif()
	# one session (C5 or Cc; the reference R5 only for the recording),
	# maybe repeated once (gates failed), maybe waiting for a slot
	math(EXPR limit "${BUG_ORIG_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
	# the order (see bug_orig_test): the limit of the session
	set(cost ${BUG_ORIG_TIMEOUT})
	if(DEFINED arg_WANT)
		foreach(suffix "" _originals)
			add_test(NAME ${name}${suffix} COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
				-DPASS=${work}${suffix}.pass -DWANT=${arg_WANT}
				-P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
			set_tests_properties(${name}${suffix} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
				TIMEOUT ${limit} COST ${cost})
			if(NOT fixed_test)
				break()
			endif()
		endforeach()
		if(fixed_test)
			set_tests_properties(${name}_originals PROPERTIES LABELS originals)
		endif()
	elseif(fixed_test)
		# it passes (ctest learns its cost): Cc in all (label emulator), C5
		# in originals
		add_test(NAME ${name} COMMAND ${TEDAB} run ${work}.pass)
		set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
			TIMEOUT ${limit} COST ${cost})
		add_test(NAME ${name}_originals COMMAND ${TEDAB} run ${work}_originals.pass)
		set_tests_properties(${name}_originals PROPERTIES LABELS originals SKIP_RETURN_CODE 77
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
		"blitz2_ref=-\nblitz2_c=-\nbugorig=1\n")
	file(GENERATE OUTPUT ${lint} CONTENT "${text}")
	add_test(NAME bug_orig_ted_lint COMMAND ${TEDAB} lint ${lint})
endif()

# the self test of the TED variant: an expectation equal to what the
# original TED does must be reported as broken (exit status 3)
bug_orig_ted_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-bug-orig-selftest-broken
	bug_orig_ted_selftest_broken WANT 3)
# its compare test against a recorded original with a changed state
# (bug_orig_ted_record_changed, label originals) must fail naming the line
bug_orig_ted_pass(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-bug-orig-selftest-broken
	bug_orig_ted_record_changed ${CMAKE_CURRENT_BINARY_DIR}/ted-bug-orig-selftest-broken_changed R5,C5
	"orig=${CMAKE_CURRENT_SOURCE_DIR}/selftest/ted-record-changed.orig\norig_by=TED 1.1\nrecord=check\n")
add_test(NAME bug_orig_ted_record_changed COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
	-DPASS=${CMAKE_CURRENT_BINARY_DIR}/ted-bug-orig-selftest-broken_changed.pass -DWANT=1
	-P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
math(EXPR rec_limit "${BUG_ORIG_TIMEOUT} * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(bug_orig_ted_record_changed PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${rec_limit} FIXTURES_REQUIRED originals_refbuild PASS_REGULAR_EXPRESSION
	"DIFFERS: \\[state\\] line 1: recorded \"FAIL step 9\", observed \"OK\"")

# the fixed form (@fixed #N, pass key fixed=1): a script whose expectations
# hold of TED-C is FIXED (exit status 0); the same with an expectation that
# does not hold is REGRESSED (1, the report names the bug)
bug_orig_ted_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-fixed-selftest-fixed
	bug_orig_ted_fixed_selftest_fixed WANT 0)
bug_orig_ted_test(${CMAKE_CURRENT_SOURCE_DIR}/selftest ted-fixed-selftest-regressed
	bug_orig_ted_fixed_selftest_regressed WANT 1)
# the regular expressions per combination: Cc in all, C5 in originals
foreach(suffix combo IN ZIP_LISTS "<none>;_originals" "Cc;C5")
	if(suffix STREQUAL "<none>")
		set(suffix "")
	endif()
	set_tests_properties(bug_orig_ted_fixed_selftest_regressed${suffix} PROPERTIES
		PASS_REGULAR_EXPRESSION "${combo}: REGRESSED: bug #99999 is fixed")
	set_tests_properties(bug_orig_ted_fixed_selftest_fixed${suffix} PROPERTIES
		PASS_REGULAR_EXPRESSION "${combo}: FIXED #99999")
endforeach()

# the recorded original of a TED test with broken input, before any session
# (no emulator): missing, not an observation or a wildcard line without a
# reason is an error (2)
function(bug_orig_ted_orig_test name orig)
	set(pass ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass)
	string(CONCAT text
		"work=${CMAKE_CURRENT_BINARY_DIR}/${name}\nreport=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt\n"
		"scripts=${CMAKE_CURRENT_SOURCE_DIR}/selftest\nsrc=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
		"combos=R5,C5\nbugorig=1\norig=${orig}\nfilter=ted-bug-orig-selftest-broken\n"
		"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
		"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
		"blitz2_ref=-\nblitz2_c=-\n")
	file(WRITE ${pass} "${text}")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
		-DPASS=${pass} -DWANT=2 -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
endfunction()
bug_orig_ted_orig_test(bug_orig_ted_orig_missing ${CMAKE_CURRENT_SOURCE_DIR}/selftest/nonexistent.orig)
bug_orig_ted_orig_test(bug_orig_ted_orig_garbage ${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig)
bug_orig_ted_orig_test(bug_orig_ted_wildcard_noreason
	${CMAKE_CURRENT_SOURCE_DIR}/selftest/ted-wildcard-noreason.orig)
set_tests_properties(bug_orig_ted_wildcard_noreason PROPERTIES PASS_REGULAR_EXPRESSION
	"ted-wildcard-noreason\\.orig:3: a wildcard line \\(`\\*` at the end, `\\.\\.\\.`\\) without a \"# varies: REASON\" line before it")

# the fixed form with broken input, before any session (no emulator, exit
# status 2 or the lint's message): a pass whose scripts and keys do not fit
# (fixed=1 over an open script, orig= over a fixed one, fixed=1 with orig=,
# fixed=1 without bugorig=1), and the scripts of selftest/ted-fixed-wrong
# (a pair, twice, no #) in the lint beside a correct one
function(bug_orig_ted_pass_test name filter keys regex)
	set(pass ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass)
	string(CONCAT text
		"work=${CMAKE_CURRENT_BINARY_DIR}/${name}\nreport=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt\n"
		"scripts=${CMAKE_CURRENT_SOURCE_DIR}/selftest\nsrc=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
		"combos=C5\nfilter=${filter}\n${keys}"
		"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
		"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
		"blitz2_ref=-\nblitz2_c=-\n")
	file(WRITE ${pass} "${text}")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DRUNNER=${TEDAB} -DSUB=run
		-DPASS=${pass} -DWANT=2 -P ${CMAKE_CURRENT_SOURCE_DIR}/want.cmake)
	set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}")
endfunction()
bug_orig_ted_pass_test(bug_orig_ted_fixed_open_script ted-bug-orig-selftest-broken
	"bugorig=1\nfixed=1\n" "fixed=1, but the script has no @fixed #NNN")
bug_orig_ted_pass_test(bug_orig_ted_open_fixed_script ted-fixed-selftest-fixed
	"combos=R5,C5\nbugorig=1\norig=${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig\n"
	"@fixed #99999: a fixed test is run with fixed=1 and no orig=")
bug_orig_ted_pass_test(bug_orig_ted_fixed_with_orig ted-fixed-selftest-fixed
	"bugorig=1\nfixed=1\norig=${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig\n"
	"fixed=1 belongs to bugorig=1 and has no orig=")
bug_orig_ted_pass_test(bug_orig_ted_fixed_no_bugorig ted-fixed-selftest-fixed
	"fixed=1\n" "fixed=1 belongs to bugorig=1")
bug_orig_ted_pass_test(bug_orig_ted_no_orig_no_fixed ted-fixed-selftest-fixed
	"bugorig=1\n" "bugorig=1 needs orig=")
bug_orig_ted_pass_test(bug_orig_ted_record_no_to ted-bug-orig-selftest-broken
	"combos=R5,C5\nbugorig=1\norig=${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig\nrecord=write\n"
	"record=write and record_to=PATH belong together")
bug_orig_ted_pass_test(bug_orig_ted_record_bad ted-bug-orig-selftest-broken
	"combos=R5,C5\nbugorig=1\norig=${CMAKE_CURRENT_SOURCE_DIR}/selftest/garbage.orig\nrecord=yes\n"
	"record=check or record=write, with orig=")
set(lint ${CMAKE_CURRENT_BINARY_DIR}/ted_fixed_wrong_lint.pass)
string(CONCAT text
	"work=${CMAKE_CURRENT_BINARY_DIR}/ted_fixed_wrong_lint\nreport=${CMAKE_CURRENT_BINARY_DIR}/ted_fixed_wrong_lint.txt\n"
	"scripts=${CMAKE_CURRENT_SOURCE_DIR}/selftest/ted-fixed-wrong\n"
	"src=${PROJECT_SOURCE_DIR}/tests/ted-ab/src\n"
	"corpus_src=-\ncorpus_out=-\nexpect_dir=-\ncd=-\noracle=-\nfonts=-\n"
	"teddrive=-\nteddrivetest=-\ntedprobe=-\nreqtrap=-\nted_ref=-\nted_c=-\n"
	"blitz2_ref=-\nblitz2_c=-\nbugorig=1\n")
file(WRITE ${lint} "${text}")
add_test(NAME bug_orig_ted_fixed_wrong_lint COMMAND ${TEDAB} lint ${lint})
set_tests_properties(bug_orig_ted_fixed_wrong_lint PROPERTIES PASS_REGULAR_EXPRESSION
	"^a-pair line 4: @fixed #NNN \\(no pair[^\n]*\nb-twice line 5: @fixed twice\nc-noissue line 4: @fixed #NNN \\(no pair[^\n]*\ntedab lint: 1 tests, 3 problems\n$")

# The recorded originals (ID.orig) written by the target bug-orig-record
# (record.cmake; not ALL, not a test, reviewed with git diff tests/bug-orig):
# two recording runs of the original per test (corpusrun / tedab with
# record=write, the second with a larger Stack: another memory layout),
# accepted by results bug-orig-accept only if they agree (else
# "test not deterministic", the file unchanged).  Needs Amiberry, the CD
# and the reference builds (blitz2_ref, ted_ref; for @rwe tests the
# reference build of the RWE sources, ctest --preset originals -R
# rwe_refbuild, else they are skipped).  BUG_ORIG_RECORD_FILTER: only these
# tests (IDs or globs, separated by ,); the others are not touched.
set(BUG_ORIG_RECORD_FILTER "" CACHE STRING
	"bug-orig-record: only these tests (IDs or globs with * ?, separated by ,; empty: all)")
set(BUG_ORIG_RECORD_PARALLEL 4 CACHE STRING
	"bug-orig-record: tests recorded at the same time (two emulator sessions each)")
get_property(record_list GLOBAL PROPERTY BUG_ORIG_RECORD_LIST)
list(JOIN record_list "\n" record_list)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bug-orig-record.list CONTENT "${record_list}\n")
set(what "the original (two runs each) -> tests/bug-orig/*/ID.orig")
if(BUG_ORIG_RECORD_FILTER)
	string(APPEND what " (filter ${BUG_ORIG_RECORD_FILTER})")
endif()
add_custom_target(bug-orig-record
	COMMAND ${CMAKE_COMMAND} -E env AMIGA_EMULATION_OWNER=${CMAKE_BINARY_DIR}
		${CMAKE_COMMAND} -DLIST=${CMAKE_CURRENT_BINARY_DIR}/bug-orig-record.list
		-DRESULTS=${RESULTS} -DCORPUSRUN=${CORPUSRUN} -DTEDAB=${TEDAB}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/bug-orig-record "-DFILTER=${BUG_ORIG_RECORD_FILTER}"
		-DPARALLEL=${BUG_ORIG_RECORD_PARALLEL} -P ${CMAKE_CURRENT_SOURCE_DIR}/record.cmake
	USES_TERMINAL
	COMMENT "The recorded originals of the bug-orig tests from ${what}"
	VERBATIM)
add_dependencies(bug-orig-record bug_orig_sources blitz2_ref ted_ref host_tools)
# the orchestration with fake runs (no emulator): a deterministic test is
# written, a non-deterministic one refused with its ID.orig unchanged, a
# test the filter leaves out untouched, a failed run fails the target
add_test(NAME bug_orig_record_target COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS}
	-DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/record.cmake
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/bug_orig_record_target
	-P ${CMAKE_CURRENT_SOURCE_DIR}/selftest/record-target.cmake)
set_tests_properties(bug_orig_record_target PROPERTIES
	PASS_REGULAR_EXPRESSION "record-target: all runs as expected")

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
