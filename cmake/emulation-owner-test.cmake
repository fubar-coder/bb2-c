# emulation-owner-test.cmake - checks emulation_snapshot_owner and
# emulation_coverage (cmake/emulation-queue.cmake): every test that runs
# Amiberry gets AMIGA_EMULATION_OWNER in its environment, a test that does
# not gets none; every test of the label emulator but host_emulator gets
# AMIGA_EMULATION_COVERAGE=BIN/coverage/NAME, the others none.  Configures
# a project in WORK with tests of every label that uses the emulator
# (emulator, originals, oracle; one of them in a subdirectory), two that do
# not, and tests of the BASIC test model (cmake/basic-test-model.cmake,
# basic_test_register: testreg declares them at build time, the port and
# the originals side of a test and a test of an open bug, label bug-orig);
# builds it, lists the tests with ctest --show-only=json-v1 and reads their
# environments.
#
#   cmake -DQUEUE=emulation-queue.cmake -DMODEL=basic-test-model.cmake
#         -DTESTREG=testreg -DCMAKE_GENERATOR=G [-DBROKEN=ON|model] -DWORK=DIR
#         -P emulation-owner-test.cmake
# BROKEN=ON (the test of the check): the project calls neither function,
# so no test gets the owner, which must fail.  BROKEN=model: the script of
# the environment of the tests of the model sets none, so they lack it,
# which must fail.
foreach(v QUEUE MODEL TESTREG CMAKE_GENERATOR WORK)
	if(NOT ${v})
		message(FATAL_ERROR "emulation-owner-test.cmake: ${v} missing")
	endif()
endforeach()
set(owner /the/owner)
set(call "emulation_snapshot_owner(${owner})\nemulation_coverage()")
if(BROKEN STREQUAL "ON")
	set(call "")
endif()
file(MAKE_DIRECTORY ${WORK}/src/sub ${WORK}/src/model)
file(WRITE ${WORK}/src/sub/CMakeLists.txt [=[
add_test(NAME t_sub_originals COMMAND ${CMAKE_COMMAND} -E true)
set_tests_properties(t_sub_originals PROPERTIES LABELS "originals")
]=])
# the tests of the model: a port and an originals side (libs=cd needs the
# CD), and an open bug (a deviation of the original, label bug-orig)
file(WRITE ${WORK}/src/model/tests.toml "root = true\nexclude = [\"compiler=orig195,libs=clib\"]\n"
	"[matrix]\ncompiler = [\"port\", \"orig195\"]\nlibs = [\"cd\", \"clib\"]\n")
file(WRITE ${WORK}/src/model/plain.test.bas "!TestCase{c}\n  !Check{1}\n!EndTestCase\n")
file(WRITE ${WORK}/src/model/open.test.bas "!TestCase{c}\n  !CheckEq{1,1,\"w\"}\n!EndTestCase\n")
file(WRITE ${WORK}/src/model/open.test.toml "[deviations.\"libs=cd\"]\n\"check.c.w\" = \"== fail\"\n")
file(WRITE ${WORK}/src/CMakeLists.txt "
cmake_minimum_required(VERSION 3.20)
project(owner NONE)
enable_testing()
include(${QUEUE})
include(${MODEL})
foreach(t t_emulator t_originals t_oracle t_perf t_plain t_site host_emulator)
	add_test(NAME \${t} COMMAND \${CMAKE_COMMAND} -E true)
endforeach()
set_tests_properties(t_emulator PROPERTIES LABELS emulator)
set_tests_properties(t_originals PROPERTIES LABELS originals)
set_tests_properties(t_oracle PROPERTIES LABELS oracle)
set_tests_properties(t_perf PROPERTIES LABELS \"perf;emulator\")
set_tests_properties(t_site PROPERTIES LABELS site)
set_tests_properties(host_emulator PROPERTIES LABELS emulator)
add_subdirectory(sub)
# what basic_test_register needs of the build: the environment file and the
# token list of the model, the tools
set(TESTREG ${TESTREG})
set(CORPUSRUN ${CMAKE_COMMAND})
file(WRITE \${BASIC_TEST_ENV} \"top = \\\"${WORK}/src/model\\\"\\n\")
file(WRITE \${BASIC_TEST_TOKENS} \"8001 End\\n\")
add_custom_target(basic_test_tokens)
add_custom_target(host_tools)
basic_test_register(m_ ${WORK}/src/model \"*.test.bas\")
${call}
")
execute_process(COMMAND ${CMAKE_COMMAND} -S ${WORK}/src -B ${WORK}/bin -G "${CMAKE_GENERATOR}"
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(rc)
	message(FATAL_ERROR "emulation-owner-test: configure failed:\n${out}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build ${WORK}/bin RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(rc)
	message(FATAL_ERROR "emulation-owner-test: build failed:\n${out}")
endif()
if(BROKEN STREQUAL "model")
	file(WRITE ${WORK}/bin/m-model-env.cmake "function(basic_test_environment name labels)\nendfunction()\n")
endif()
execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${WORK}/bin --show-only=json-v1
	RESULT_VARIABLE rc OUTPUT_VARIABLE json ERROR_VARIABLE out)
if(rc OR out)
	message(FATAL_ERROR "emulation-owner-test: ctest failed:\n${out}")
endif()

set(want_owner t_emulator t_originals t_oracle t_perf t_sub_originals host_emulator
	m_plain__c m_plain__c_originals m_open__c m_open__c_originals)
set(want_none t_plain t_site)
set(want_coverage t_emulator t_perf m_plain__c m_open__c)
string(JSON n LENGTH "${json}" tests)
set(failed FALSE)
set(seen "")
math(EXPR last "${n} - 1")
foreach(i RANGE ${last})
	string(JSON name GET "${json}" tests ${i} name)
	list(APPEND seen ${name})
	set(has FALSE)
	set(cov "")
	set(labels "")
	string(JSON np LENGTH "${json}" tests ${i} properties)
	math(EXPR plast "${np} - 1")
	foreach(p RANGE ${plast})
		string(JSON pname GET "${json}" tests ${i} properties ${p} name)
		if(pname STREQUAL "LABELS")
			string(JSON labels GET "${json}" tests ${i} properties ${p} value)
		elseif(pname STREQUAL "ENVIRONMENT")
			string(JSON ne LENGTH "${json}" tests ${i} properties ${p} value)
			math(EXPR elast "${ne} - 1")
			foreach(e RANGE ${elast})
				string(JSON env GET "${json}" tests ${i} properties ${p} value ${e})
				if(env STREQUAL "AMIGA_EMULATION_OWNER=${owner}")
					set(has TRUE)
				elseif(env MATCHES "^AMIGA_EMULATION_COVERAGE=(.*)")
					set(cov "${CMAKE_MATCH_1}")
				endif()
			endforeach()
		endif()
	endforeach()
	if(name IN_LIST want_owner AND NOT has)
		message(SEND_ERROR "emulation-owner-test: ${name} has no AMIGA_EMULATION_OWNER")
		set(failed TRUE)
	elseif(name IN_LIST want_none AND has)
		message(SEND_ERROR "emulation-owner-test: ${name} has AMIGA_EMULATION_OWNER but runs no emulator")
		set(failed TRUE)
	endif()
	if(NOT BROKEN STREQUAL "ON")
		if(name IN_LIST want_coverage AND NOT cov STREQUAL "${WORK}/bin/coverage/${name}")
			message(SEND_ERROR "emulation-owner-test: ${name} has not AMIGA_EMULATION_COVERAGE="
				"${WORK}/bin/coverage/${name} (${cov})")
			set(failed TRUE)
		elseif(NOT name IN_LIST want_coverage AND cov)
			message(SEND_ERROR "emulation-owner-test: ${name} has AMIGA_EMULATION_COVERAGE (${cov})")
			set(failed TRUE)
		endif()
	endif()
	if(name STREQUAL "m_open__c" AND NOT labels MATCHES "bug-orig")
		message(SEND_ERROR "emulation-owner-test: m_open__c is no test of the label bug-orig: ${labels}")
		set(failed TRUE)
	endif()
endforeach()
foreach(t IN LISTS want_owner want_none)
	if(NOT t IN_LIST seen)
		message(SEND_ERROR "emulation-owner-test: test ${t} not listed")
		set(failed TRUE)
	endif()
endforeach()
if(failed)
	message(FATAL_ERROR "emulation-owner-test: failed")
endif()
message("emulation-owner-test: OK")
