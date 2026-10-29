# Host tests of the TED core (src/core: no AmigaOS calls), built with the
# native compiler by host/CMakeLists.txt:
#   tedcore_host  the core with TED_HOST (resources via #embed as on Amiga)
#   tedhost       the test environment: hook stubs (stubs.c), token list
#                 (toklist.c), see hostenv.h
#   oracle data   tests/data (ted-oracle.sha256, ted-tokens.txt: the
#                 original TED, programs/ted/tests/oracle) and the CD
#                 sources (archive/cd); without the CD the corpus tests
#                 skip (snitch SKIP -> ctest "skipped")
#   test_*.cpp    one snitch program each - every file starts with the
#                 core's initial global state (test_state checks it)
set(TED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
get_filename_component(REPO_DIR ${TED_DIR}/../.. ABSOLUTE)
include(${TED_DIR}/sources.cmake)

set(core_sources)
foreach(m IN LISTS TED_CORE_MODULES)
	list(APPEND core_sources ${TED_DIR}/src/core/${m}.c)
endforeach()
add_library(tedcore_host STATIC ${core_sources} ${TED_DIR}/src/core/res.c)
target_compile_definitions(tedcore_host PUBLIC TED_HOST)
target_include_directories(tedcore_host PUBLIC ${TED_DIR}/src/core)
target_compile_options(tedcore_host PRIVATE -Wall -Wno-pointer-sign -Wno-parentheses)
set_target_properties(tedcore_host PROPERTIES C_STANDARD 23 C_EXTENSIONS ON)
set_source_files_properties(${TED_DIR}/src/core/res.c PROPERTIES
	COMPILE_OPTIONS --embed-dir=${TED_DIR}/../../archive/sources/ted-1.1)

add_library(tedhost STATIC stubs.c toklist.c)
target_include_directories(tedhost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tedhost PUBLIC tedcore_host)
target_compile_options(tedhost PRIVATE -Wall)
set_target_properties(tedhost PROPERTIES C_STANDARD 23 C_EXTENSIONS ON)

file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)
foreach(t IN LISTS tests)
	get_filename_component(name ${t} NAME_WE)
	string(REGEX REPLACE "^test_" "" short ${name})
	add_executable(ted_${name} ${t})
	# the core and the stubs reference each other (core -> hooks, stubs ->
	# core): link them as a group
	target_link_libraries(ted_${name} PRIVATE "$<LINK_GROUP:RESCAN,tedhost,tedcore_host>"
		snitch::snitch lib_sha256)
	target_compile_definitions(ted_${name} PRIVATE
		TED_TEST_DATA_DIR="${REPO_DIR}/tests/data"
		TED_CD_DIR="${REPO_DIR}/archive/cd/ultimate-blitz-basic-2.1")
	target_compile_options(ted_${name} PRIVATE -Wall)
	set_target_properties(ted_${name} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
	add_test(NAME ted_core_${short} COMMAND ted_${name})
	set_tests_properties(ted_core_${short} PROPERTIES LABELS ted SKIP_REGULAR_EXPRESSION "test cases skipped")
endforeach()
