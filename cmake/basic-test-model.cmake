# The Blitz tests of the BASIC test model (tests/README.md, "Tests that judge
# themselves"; openspec change basic-test-runner): a source ID.test.bas
# whose program checks itself with the test library, its run configuration
# in ID.test.toml and the tests.toml files above it (root: tests/tests.toml).
# CMake reads none of these files: it globs their names, and the build step
# of a directory's tests (tools/testreg) reads them, checks them and writes
# the ctest include file with one test per case and side
# (TEST_INCLUDE_FILES, as gtest_discover_tests: `ctest -N` shows them after
# the build).  The tests run `corpusrun case` (tests/corpus/runner/caserun.h)
# with the environment file of the build.
#
# basic_test_model_env()
#   the environment file BASIC_TEST_ENV (tests/model-env.toml in the build
#   directory) and the token list BASIC_TEST_TOKENS (blitz2 with the default
#   library sets, the test library and the bug-orig user library); once, by
#   the top CMakeLists.txt after the directories that define the programs
# basic_test_register(PREFIX DIR GLOB... [EXCLUDE REGEX] [REQUIRE_DEVIATION]
#                     [PREV REGEX [PREV_EXCLUDE ID...]] [ORDER FILE])
#   the tests of the sources DIR/GLOB (e.g. "*/*.test.bas"; those whose path
#   below DIR matches REGEX left out): ctest
#   PREFIX + ID + "__" + CASE (and "_originals" for the side with original
#   material); REQUIRE_DEVIATION: bug-orig tests (a test without a declared
#   deviation is broken).  PREV: the sources whose path below DIR matches
#   REGEX (but the ids PREV_EXCLUDE) are compared with the prev build
#   (basic_test_driver, cmake/prev.cmake prev_compile_model_test): ctest
#   PREFIX + ID + "_prev".  ORDER: the order file of bug_orig_declare_tests
#   (cmake/bug-orig-order.cmake): its tests are declared with the tests of
#   the label bug-orig in one list, the largest COST first.  The tests get
#   the environment of the tests CMake declares (emulation_test_environment,
#   cmake/emulation-queue.cmake: the snapshot owner, the coverage
#   directory): testreg --test-env, the script PREFIX-model-env.cmake that
#   the end of the top directory writes, which ctest reads with the include
#   file and calls for each test with its labels.  With BASIC_TEST_BUNDLE
#   (ON; off in a coverage build, BLITZ2_COVERAGE) testreg also registers
#   the bundle tests PREFIX + "bundle_" + N: the cases that may share a
#   session with other sources, 16 at most per bundle, each case test
#   served from its bundle and DEPENDS on it (tools/testreg/README.md)
# basic_test_driver(SOURCE OUT [CONFS...])
#   the build writes the driver of all cases of SOURCE, tokenised, to OUT
#   (NAME.bb2; corpusrun case --write-driver), a step of the target that
#   adds OUT to its DEPENDS; CONFS: the configuration files it may read

set(BASIC_TEST_ENV ${CMAKE_BINARY_DIR}/tests/model-env.toml)
set(BASIC_TEST_TOKENS ${CMAKE_BINARY_DIR}/tests/model-tokens.txt)
set(BASIC_TEST_WORK ${CMAKE_BINARY_DIR}/tests/model-runs)
set(BASIC_TEST_TIMEOUT 90 CACHE STRING "BASIC test model: the time limit of an emulator session without timeout (seconds)")
set(BASIC_TEST_RUN_HANG_AFTER 8 CACHE STRING
	"BASIC test model: seconds of silence of a program run after which a session ends (0: only the time limit)")
option(BASIC_TEST_BUNDLE
	"BASIC test model: bundle tests (16 cases of other sources in one emulator session; off in a coverage build)" ON)

function(basic_test_model_env)
	set(cd ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1)
	set(bo ${CMAKE_BINARY_DIR}/tests/bug-orig)
	set(userlib ${bo}/amiga/bugorig_userlib.obj)
	set(rwe "")
	get_property(names GLOBAL PROPERTY RWE_LIB_NAMES)
	foreach(n IN LISTS names)
		get_property(info GLOBAL PROPERTY RWE_LIB_${n})
		string(REPLACE "|" ";" info "${info}")
		list(GET info 0 obj)
		list(GET info 1 ref)
		list(GET info 4 vout)
		string(APPEND rwe "rwe-${n} = \"${obj}|${ref}|${vout}\"\n")
	endforeach()
	file(GENERATE OUTPUT ${BASIC_TEST_ENV} CONTENT
"# the environment of the tests of the BASIC test model (cmake/basic-test-model.cmake)
top = \"${PROJECT_SOURCE_DIR}\"
[placeholders]
TESTDATA = \"${bo}/testdata\"
USERLIB = \"${userlib}\"
INJECT = \"$<TARGET_FILE:inject>\"
WBRUN = \"$<TARGET_FILE:wbrun>\"
WBRUNT = \"$<TARGET_FILE:wbrunt>\"
REQLIB = \"${PROJECT_SOURCE_DIR}/archive/aminet/ReqLib28/req.library\"
ANCIENT = \"${PROJECT_SOURCE_DIR}/archive/ancient/github-1491a35a\"
CD = \"${cd}\"
CD_BLITZ2 = \"${cd}/BLITZ2\"
[runner]
blitz2 = \"$<TARGET_FILE:blitz2>\"
orig195 = \"${CMAKE_BINARY_DIR}/programs/blitz2/ref/blitz2\"
orig210 = \"${cd}/BLITZ2/BLITZ2\"
cd-deflibs = \"${cd}/BLITZ2/DEFLIBS\"
cd-acidlibs = \"${cd}/BLITZ2/ACIDLIBS\"
cd-defaultdbug = \"${cd}/BLITZ2/DEFAULTDBUG\"
clib-deflibs = \"${BLITZ2_DIR}/deflibs\"
clib-acidlibs = \"${BLITZ2_DIR}/acidlibs\"
ted = \"$<TARGET_FILE:tedstub>\"
launcher = \"$<TARGET_FILE:reqtrap>\"
rcput = \"$<TARGET_FILE:rcput>\"
inject = \"$<TARGET_FILE:inject>\"
testlib = \"${TESTLIB_OBJ}\"
macros = \"${TESTLIB_MACROS}\"
tedsrc = \"${TEDSRC}\"
mktestdata = \"${MKTESTDATA}\"
bbsrc = \"${BBSRC}\"
tokens = \"${BASIC_TEST_TOKENS}\"
work = \"${BASIC_TEST_WORK}\"
timeout = \"${BASIC_TEST_TIMEOUT}\"
run-hang-after = \"${BASIC_TEST_RUN_HANG_AFTER}\"
run-dir = \"${CMAKE_BINARY_DIR}/Testing/runs/default\"
${rwe}")
	# the names the driver is tokenised with: blitz2-c's, the default library
	# sets', the test library's and the bug-orig user library's
	add_custom_command(OUTPUT ${BASIC_TEST_TOKENS}
		COMMAND ${BBSRC} tokens -o ${BASIC_TEST_TOKENS} $<TARGET_FILE:blitz2> ${BLITZ2_DIR}/acidlibs
			${BLITZ2_DIR}/deflibs ${TESTLIB_OBJ} ${userlib}
		DEPENDS blitz2 ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs ${TESTLIB_OBJ} ${userlib}
			userlib-testlib_obj bugorig_userlib_obj ${BBSRC} host_tools
		COMMENT "Token list of the tests of the BASIC test model"
		VERBATIM)
	add_custom_target(basic_test_tokens ALL DEPENDS ${BASIC_TEST_TOKENS})
endfunction()

function(basic_test_driver src out)
	add_custom_command(OUTPUT ${out}
		COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV} --source ${src} --write-driver ${out}
		DEPENDS ${src} ${ARGN} ${PROJECT_SOURCE_DIR}/tests/tests.toml ${BASIC_TEST_ENV} ${BASIC_TEST_TOKENS}
			basic_test_tokens ${TESTLIB_MACROS} ${CORPUSRUN} ${TEDSRC} host_tools
		COMMENT "The driver of all cases of ${src}"
		VERBATIM)
endfunction()

function(basic_test_register prefix dir)
	cmake_parse_arguments(PARSE_ARGV 2 arg "REQUIRE_DEVIATION" "EXCLUDE;PREV;ORDER" "PREV_EXCLUDE")
	set(srcs "")
	set(confs "")
	foreach(g IN LISTS arg_UNPARSED_ARGUMENTS)
		file(GLOB s RELATIVE ${dir} CONFIGURE_DEPENDS ${dir}/${g})
		foreach(f IN LISTS s)
			if(NOT arg_EXCLUDE OR NOT f MATCHES "${arg_EXCLUDE}")
				list(APPEND srcs ${dir}/${f})
			endif()
		endforeach()
		# a new configuration file next to a source is seen at once (the
		# depfile names only the files that existed)
		string(REGEX REPLACE "\\.test\\.bas$" ".test.toml" t "${g}")
		get_filename_component(gd "${g}" DIRECTORY)
		file(GLOB c CONFIGURE_DEPENDS ${dir}/${t} ${dir}/${gd}/tests.toml ${dir}/tests.toml)
		list(APPEND confs ${c})
		# and so is a file of a test's directory ID.test/ (its data, change
		# basic-test-local-data)
		string(REGEX REPLACE "\\.test\\.bas$" ".test" td "${g}")
		file(GLOB_RECURSE c CONFIGURE_DEPENDS ${dir}/${td}/*)
		list(APPEND confs ${c})
	endforeach()
	list(SORT srcs)
	if(NOT srcs)
		# the order file is a declaration of its own, sorted (cmake/bug-orig-order.cmake)
		if(arg_ORDER)
			set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${arg_ORDER})
		endif()
		return()
	endif()
	string(REGEX REPLACE "_$" "" stem "${prefix}")
	set(inc ${CMAKE_CURRENT_BINARY_DIR}/${stem}-model-tests.cmake)
	set(tenv ${CMAKE_CURRENT_BINARY_DIR}/${stem}-model-env.cmake)
	set_property(GLOBAL APPEND PROPERTY BASIC_TEST_ENV_SCRIPTS "${tenv}|${CMAKE_CURRENT_BINARY_DIR}")
	get_property(deferred GLOBAL PROPERTY BASIC_TEST_ENV_DEFERRED)
	if(NOT deferred)
		set_property(GLOBAL PROPERTY BASIC_TEST_ENV_DEFERRED ON)
		cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _basic_test_write_env)
	endif()
	set(cmd --command ${CORPUSRUN} --command case --command --env --command ${BASIC_TEST_ENV})
	if(arg_REQUIRE_DEVIATION)
		list(APPEND cmd --command --require-deviation)
	endif()
	math(EXPR extra "60 + ${EMULATION_QUEUE_WAIT}")
	set(order "")
	get_property(costs GLOBAL PROPERTY TEST_COSTS_FILE)
	if(costs AND EXISTS ${costs})
		list(APPEND order --costs ${costs})
	else()
		set(costs "")
	endif()
	if(arg_ORDER)
		list(APPEND order --order ${arg_ORDER})
	endif()
	# the bundle tests: none when switched off, and none in a coverage build
	# (a served case would have no tracefile of its own, the bundle's would
	# hold the functions of all its members)
	if(NOT BASIC_TEST_BUNDLE OR BLITZ2_COVERAGE)
		list(APPEND order --no-bundle)
	endif()
	add_custom_command(OUTPUT ${inc}
		COMMAND ${TESTREG} --prefix ${prefix} --output ${inc} --depfile ${inc}.d --env ${BASIC_TEST_ENV}
			--tokens ${BASIC_TEST_TOKENS} --timeout-default ${BASIC_TEST_TIMEOUT} --timeout-extra ${extra}
			--cell-fixture "compiler=orig195|originals_refbuild"
			--cell-fixture "compiler=orig210|rwe_refbuild" --test-env ${tenv} ${order}
			${cmd} ${srcs}
		DEPENDS ${srcs} ${confs} ${BASIC_TEST_ENV} ${BASIC_TEST_TOKENS} basic_test_tokens ${TESTREG} host_tools
			${arg_ORDER} ${costs}
		DEPFILE ${inc}.d
		COMMENT "Registering the tests of the BASIC test model in ${dir} (${prefix}*)"
		VERBATIM)
	# the comparison with the prev build: the driver of all cases of each source
	# (written by every build, so a source whose driver cannot be written fails it)
	set(drivers "")
	if(DEFINED arg_PREV)
		foreach(s IN LISTS srcs)
			file(RELATIVE_PATH rel ${dir} ${s})
			get_filename_component(id ${s} NAME)
			string(REGEX REPLACE "\\.test\\.bas$" "" id "${id}")
			if(NOT rel MATCHES "${arg_PREV}" OR id IN_LIST arg_PREV_EXCLUDE)
				continue()
			endif()
			set(drv ${CMAKE_CURRENT_BINARY_DIR}/model-drivers/${id}.bb2)
			basic_test_driver(${s} ${drv} ${confs})
			list(APPEND drivers ${drv})
			string(REPLACE "-" "_" name "${prefix}${id}_prev")
			prev_compile_model_test(${name} ${s} ${drv})
		endforeach()
	endif()
	add_custom_target(${stem}_model_tests ALL DEPENDS ${inc} ${drivers})
	set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${inc})
endfunction()

# the scripts of the tests' environment (testreg --test-env), at the end of
# the top directory: after emulation_snapshot_owner and emulation_coverage,
# whose settings they hand to emulation_test_environment, the rules of the
# tests CMake declares (cmake/emulation-queue.cmake), which ctest reads; a
# script is written only when it changes (the build step does not read it)
function(_basic_test_write_env)
	get_property(scripts GLOBAL PROPERTY BASIC_TEST_ENV_SCRIPTS)
	get_property(owner GLOBAL PROPERTY EMULATION_SNAPSHOT_OWNER)
	get_property(cov GLOBAL PROPERTY EMULATION_COVERAGE)
	set(queue ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/emulation-queue.cmake)
	foreach(s IN LISTS scripts)
		string(REPLACE "|" ";" s "${s}")
		list(GET s 0 file)
		list(GET s 1 bin)
		string(CONCAT text
			"# the environment of the tests of the model in this directory (cmake/basic-test-model.cmake):\n"
			"# basic_test_environment(NAME LABELS), called by testreg's include file for each test\n"
			"set_property(GLOBAL PROPERTY EMULATION_SNAPSHOT_OWNER [==[${owner}]==])\n"
			"set_property(GLOBAL PROPERTY EMULATION_COVERAGE [==[${cov}]==])\n"
			"include([==[${queue}]==])\n"
			"function(basic_test_environment name labels)\n"
			"\temulation_test_environment(env \${name} \"\${labels}\" [==[${bin}]==])\n"
			"\tif(env)\n"
			"\t\tset_tests_properties(\${name} PROPERTIES ENVIRONMENT \"\${env}\")\n"
			"\tendif()\n"
			"endfunction()\n")
		file(CONFIGURE OUTPUT ${file} CONTENT "${text}" @ONLY)
	endforeach()
endfunction()
