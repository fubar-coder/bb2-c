# The Blitz Basic examples of the website (site/content, site/data), run in
# Amiberry (tests/README.md, "Site examples").
#   examples.txt  the manifest: every fenced ```blitzbasic block of the site
#                 once, "run", or "compile" with the reason it is not run, and
#                 the RWE userlibs it needs loaded (tools/siteexamples/README.md)
#   expect/GROUP  the expected output of every example that is run, NAME.out
#                 (the program's output and rc=N; tools/lib/corpusexpect)
# The build step writes the sources from the site (siteexamples extract: it
# fails for a block without an entry in the manifest or an entry without a
# block), tokenises them like TED (tedsrc tok) and the passes run them:
#   site_examples_run      the run examples compiled by blitz2-c with the
#                          C libraries (and the prev build), run, the
#                          output compared with expect/default/NAME.out
#   site_examples_compile  the others compiled only (corpusrun run=off,
#                          must_compile=on: a source that does not compile
#                          fails)
# An example that needs RWE userlibs (rwe=LIB in the manifest) is in the
# group of their variant (rwe-1999, rwe-1999-eval, rwe-1999-ahi): its own
# pass (site_examples_run_GROUP, site_examples_compile_GROUP) with the
# variant's acidlibs and deflibs, the userlibs in blitz2:userlibs/rwe and a
# token list that has their commands.  Plus the host test
# site_examples_manifest (siteexamples check, with the count of run and
# compiled only examples).  The expected files are written by the target
# site-examples-expect-update (from the port; reviewed with git diff
# tests/site-examples/expect), never by a test.
set(SE_SITE ${PROJECT_SOURCE_DIR}/site)
set(SE_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/examples.txt)
set(SE_EXPECT ${CMAKE_CURRENT_SOURCE_DIR}/expect)
set(SE_TEXT ${CMAKE_CURRENT_BINARY_DIR}/text)
set(SE_SRC ${CMAKE_CURRENT_BINARY_DIR}/src)
set(SE_TOKENS ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SE_MANIFEST})

# the manifest: name, mode and userlibs of every entry (the tool checks the
# rest), grouped by the variant of the userlibs
set(se_groups)
set(se_names)
file(STRINGS ${SE_MANIFEST} _se_lines)
foreach(l IN LISTS _se_lines)
	if(l MATCHES "^[ \t]*([a-z0-9-]+)[ \t]+(run|compile)[ \t]+[^ \t]+(.*)$")
		set(name ${CMAKE_MATCH_1})
		set(mode ${CMAKE_MATCH_2})
		set(rest "${CMAKE_MATCH_3}")
		set(group default)
		while(rest MATCHES "^[ \t]+rwe=([a-z0-9-]+)(.*)$")
			set(lib ${CMAKE_MATCH_1})
			set(rest "${CMAKE_MATCH_2}")
			get_property(info GLOBAL PROPERTY RWE_LIB_${lib})
			if(NOT info)
				message(FATAL_ERROR "${SE_MANIFEST}: ${name}: rwe=${lib}: no RWE userlib of that name "
					"(libs/userlibs/rwe/${lib}.c, RWE_LIBS)")
			endif()
			string(REPLACE "|" ";" info "${info}")
			list(GET info 0 obj)
			list(GET info 3 v)
			list(GET info 4 vout)
			if(NOT group STREQUAL "default" AND NOT group STREQUAL v)
				message(FATAL_ERROR "${SE_MANIFEST}: ${name}: the userlibs are of different variants "
					"(${group}, ${v}); one example needs one set of libraries")
			endif()
			set(group ${v})
			set(se_vout_${group} ${vout})
			list(APPEND se_objs_${group} ${obj})
			list(APPEND se_tokdeps_${group} ${v}-${lib}_obj ${v}-acidlibs ${v}-deflibs)
		endwhile()
		list(APPEND se_groups ${group})
		list(APPEND se_names ${name})
		list(APPEND se_${mode}_${group} ${name})
	elseif(NOT l MATCHES "^[ \t]*(#.*)?$")
		message(FATAL_ERROR "${SE_MANIFEST}: cannot read the line \"${l}\" (NAME run|compile FILE#N)")
	endif()
endforeach()
list(REMOVE_DUPLICATES se_groups)
set(se_vout_default ${BLITZ2_DIR})

# the sources: one extract step for all, one tokenise step each (with the
# token list of the group); every site file that can hold a block is a
# dependency (a new or changed example re-runs the check)
file(GLOB_RECURSE _se_site CONFIGURE_DEPENDS ${SE_SITE}/content/* ${SE_SITE}/data/*
	${SE_SITE}/layouts/* ${SE_SITE}/assets/*)
set(_se_text)
foreach(n IN LISTS se_names)
	list(APPEND _se_text ${SE_TEXT}/${n}.bas)
endforeach()
add_custom_command(OUTPUT ${_se_text}
	COMMAND ${SITEEXAMPLES} extract ${SE_SITE} ${SE_MANIFEST} ${SE_TEXT}
	DEPENDS ${SE_MANIFEST} ${_se_site} ${SITEEXAMPLES} host_tools
	COMMENT "Extracting the Blitz Basic examples of the site"
	VERBATIM)
set(_se_tokenised)
foreach(g IN LISTS se_groups)
	if(g STREQUAL "default")
		set(tok ${SE_TOKENS})
	else()
		set(tok ${CMAKE_CURRENT_BINARY_DIR}/tokens-${g}.txt)
		add_custom_command(OUTPUT ${tok}
			COMMAND ${BBSRC} tokens -o ${tok} $<TARGET_FILE:blitz2> ${se_vout_${g}}/acidlibs
				${se_vout_${g}}/deflibs ${se_objs_${g}}
			DEPENDS ${se_objs_${g}} ${se_vout_${g}}/acidlibs ${se_vout_${g}}/deflibs blitz2
				${BBSRC} host_tools
			COMMENT "Token list of the site examples with the userlibs of ${g}"
			VERBATIM)
	endif()
	foreach(n IN LISTS se_run_${g} se_compile_${g})
		add_custom_command(OUTPUT ${SE_SRC}/${g}/${n}.bb2
			COMMAND ${CMAKE_COMMAND} -E make_directory ${SE_SRC}/${g}
			COMMAND ${TEDSRC} tok ${tok} ${SE_TEXT}/${n}.bas ${SE_SRC}/${g}/${n}.bb2
			DEPENDS ${SE_TEXT}/${n}.bas ${tok} ${TEDSRC} host_tools
			COMMENT "Tokenising site example ${n}"
			VERBATIM)
		list(APPEND _se_tokenised ${SE_SRC}/${g}/${n}.bb2)
	endforeach()
endforeach()
add_custom_target(site_examples_sources ALL DEPENDS ${_se_tokenised})
foreach(g IN LISTS se_groups)
	if(se_tokdeps_${g})
		list(REMOVE_DUPLICATES se_tokdeps_${g})
		add_dependencies(site_examples_sources ${se_tokdeps_${g}})
	endif()
endforeach()

# the manifest against the site, with the counts
add_test(NAME site_examples_manifest COMMAND ${SITEEXAMPLES} check ${SE_SITE} ${SE_MANIFEST})
set_tests_properties(site_examples_manifest PROPERTIES
	PASS_REGULAR_EXPRESSION "siteexamples: [0-9]+ examples, [0-9]+ run, [0-9]+ compiled only")

# site_examples_pass(KIND GROUP): the pass file and ctest of the run or
# compile examples of a group (with a prev build also by the previous commit's compiler
# and libraries: the variant's libraries and userlibs of the prev build are the same
# paths below its build directory)
function(site_examples_pass kind group)
	if(group STREQUAL "default")
		set(name site_examples_${kind})
	else()
		string(REPLACE "-" "_" suffix ${group})
		set(name site_examples_${kind}_${suffix})
	endif()
	set(work ${CMAKE_CURRENT_BINARY_DIR}/${name})
	set(vout ${se_vout_${group}})
	set(configs 1)
	string(CONCAT text "# ${name}: generated by tests/site-examples/CMakeLists.txt\n"
		"test=${name}\nwork=${work}\nreport=${work}.txt\nruntime_errors=off\n"
		"timeout=${CORPUS_TIMEOUT}\nparallel=1\nted=$<TARGET_FILE:tedstub>\n")
	if(kind STREQUAL "run")
		string(APPEND text "launcher=$<TARGET_FILE:reqtrap>\nexpect_dir=${SE_EXPECT}/${group}\n")
	else()
		string(APPEND text "run=off\nmust_compile=on\n")
	endif()
	string(APPEND text
		"config=clib|ref|$<TARGET_FILE:blitz2>|${vout}/deflibs|${vout}/acidlibs\n")
	if(PREV_REQUESTED)
		string(REPLACE "${CMAKE_BINARY_DIR}" "${PREV_DIR}" prev_vout "${vout}")
		string(APPEND text
			"config=prev|exe:clib|${PREV_BLITZ2_DIR}/blitz2|${prev_vout}/deflibs|"
			"${prev_vout}/acidlibs\n${PREV_INTENDED_LINES}")
		set(configs 2)
	endif()
	foreach(obj IN LISTS se_objs_${group})
		get_filename_component(f ${obj} NAME)
		string(APPEND text "file=blitz2/userlibs/rwe/${f}|${obj}|clib\n")
		if(PREV_REQUESTED)
			string(REPLACE "${CMAKE_BINARY_DIR}" "${PREV_DIR}" prev_obj "${obj}")
			string(APPEND text "file=blitz2/userlibs/rwe/${f}|${prev_obj}|prev\n")
		endif()
	endforeach()
	foreach(n IN LISTS se_${kind}_${group})
		string(APPEND text "source=${SE_SRC}/${group}/${n}.bb2\n")
	endforeach()
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	add_test(NAME ${name} COMMAND ${CORPUSRUN} pass ${work}.pass)
	math(EXPR limit "${CORPUS_TIMEOUT} * ${configs} + 120 + ${configs} * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
	set_property(GLOBAL APPEND PROPERTY SE_RUN_PASSES ${work}.pass)
endfunction()
foreach(g IN LISTS se_groups)
	foreach(kind run compile)
		if(se_${kind}_${g})
			site_examples_pass(${kind} ${g})
		endif()
	endforeach()
endforeach()

# the expected outputs, from the port (clib), for review: new examples get
# their file, a changed output is refused by the runner (corpusrun pass
# --update-expect, tests/corpus/runner/README.md)
set(se_update)
foreach(g IN LISTS se_groups)
	if(se_run_${g})
		file(MAKE_DIRECTORY ${SE_EXPECT}/${g})
		if(g STREQUAL "default")
			set(pass site_examples_run)
		else()
			string(REPLACE "-" "_" suffix ${g})
			set(pass site_examples_run_${suffix})
		endif()
		list(APPEND se_update COMMAND ${CMAKE_COMMAND} -E env AMIGA_EMULATION_OWNER=${CMAKE_BINARY_DIR}
			${CORPUSRUN} pass --update-expect clib ${CMAKE_CURRENT_BINARY_DIR}/${pass}.pass)
	endif()
endforeach()
if(se_update)
	add_custom_target(site-examples-expect-update ${se_update}
		DEPENDS blitz2 acidlibs deflibs tedstub reqtrap site_examples_sources host_tools
		USES_TERMINAL
		COMMENT "The site examples through the port's blitz2 and C libraries -> tests/site-examples/expect"
		VERBATIM)
endif()

# the checks themselves, with deliberately broken input (emulator): a run
# example whose expected output is wrong must fail with the first line that
# differs, and a source that does not compile must fail a compile pass
set(_se_broken_src ${CMAKE_CURRENT_BINARY_DIR}/selftest/broken.bb2)
add_custom_command(OUTPUT ${_se_broken_src}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/selftest
	COMMAND ${TEDSRC} tok ${SE_TOKENS} ${CMAKE_CURRENT_SOURCE_DIR}/selftest/broken.bas ${_se_broken_src}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/selftest/broken.bas ${SE_TOKENS} ${TEDSRC} host_tools
	COMMENT "Tokenising the broken site example"
	VERBATIM)
add_custom_target(site_examples_selftest_sources ALL DEPENDS ${_se_broken_src})
set(_se_common "timeout=${CORPUS_TIMEOUT}\nparallel=1\nted=$<TARGET_FILE:tedstub>\nruntime_errors=off\n"
	"config=clib|ref|$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n")
string(CONCAT _se_common ${_se_common})
set(work ${CMAKE_CURRENT_BINARY_DIR}/site_examples_run_broken)
file(GENERATE OUTPUT ${work}.pass CONTENT
	"test=site_examples_run_broken\nwork=${work}\nreport=${work}.txt\n${_se_common}launcher=$<TARGET_FILE:reqtrap>\nexpect_dir=${CMAKE_CURRENT_SOURCE_DIR}/selftest/expect-wrong\nsource=${SE_SRC}/default/memacclib-1.bb2\n")
add_test(NAME site_examples_run_broken COMMAND ${CORPUSRUN} pass ${work}.pass)
set(work ${CMAKE_CURRENT_BINARY_DIR}/site_examples_compile_broken)
file(GENERATE OUTPUT ${work}.pass CONTENT
	"test=site_examples_compile_broken\nwork=${work}\nreport=${work}.txt\n${_se_common}run=off\nmust_compile=on\nsource=${_se_broken_src}\n")
add_test(NAME site_examples_compile_broken COMMAND ${CORPUSRUN} pass ${work}.pass)
math(EXPR limit "${CORPUS_TIMEOUT} + 120 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(site_examples_run_broken site_examples_compile_broken PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
set_tests_properties(site_examples_run_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"memacclib-1 +clib: OUTPUT DIFFERS[^\n]*\n[^\n]*selftest/expect-wrong/memacclib-1.out line 1: expected \"1234 Goodbye\", got \"1234 Hello\"")
set_tests_properties(site_examples_compile_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"broken +does not compile \\(must compile: error")
