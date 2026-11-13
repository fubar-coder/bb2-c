# corpusrun: the corpus passes (pass) and the CD runs (cd) in Amiberry, host
# side (README.md)
# strings and files, and the run directories (runwork.h), also for the TED
# A/B runner (tests/ted-ab/runner)
add_library(lib_runner_util STATIC util.c runwork.c)
target_include_directories(lib_runner_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(lib_runner_util PRIVATE -Wall -Wextra)
set_target_properties(lib_runner_util PROPERTIES C_STANDARD 23)

add_library(lib_corpus STATIC corpus.c serial.c bugorig.c)
target_include_directories(lib_corpus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(lib_corpus PUBLIC lib_runner_util lib_amiga_emulation lib_statline lib_previntended lib_corpusexpect
	lib_bugorigcmp Threads::Threads)
target_compile_options(lib_corpus PRIVATE -Wall -Wextra)
set_target_properties(lib_corpus PROPERTIES C_STANDARD 23)
set(CORPUSRUN_SUBCOMMANDS pass cd)
add_executable(corpusrun main.cpp cmd_pass.cpp cmd_cd.cpp cd.c)
target_link_libraries(corpusrun PRIVATE lib_cli lib_corpus lib_cdcache lib_sha256)
target_compile_options(corpusrun PRIVATE -Wall -Wextra)
set_target_properties(corpusrun PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

add_executable(test_corpus test_corpus.c)
target_link_libraries(test_corpus PRIVATE lib_corpus)
target_compile_options(test_corpus PRIVATE -Wall -Wextra)
add_test(NAME corpus_runner COMMAND test_corpus)
# the verdict of the bug-orig tests (tests/bug-orig), with broken inputs
add_executable(test_bugorig test_bugorig.c)
target_link_libraries(test_bugorig PRIVATE lib_corpus)
target_compile_options(test_bugorig PRIVATE -Wall -Wextra)
add_test(NAME bugorig_verdict COMMAND test_bugorig)

# corpusrun cd: every source of the CD compiled by one compiler (reference table /
# comparison with it)
# the cache of a pass (cache=)
add_library(lib_cdcache STATIC cdcache.c)
target_include_directories(lib_cdcache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_cdcache PUBLIC lib_runner_util lib_sha256)
target_compile_options(lib_cdcache PRIVATE -Wall -Wextra)
set_target_properties(lib_cdcache PROPERTIES C_STANDARD 23)
add_executable(test_cdcache test_cdcache.c)
target_link_libraries(test_cdcache PRIVATE lib_cdcache)
target_compile_options(test_cdcache PRIVATE -Wall -Wextra)
add_test(NAME cdrun_cache COMMAND test_cdcache)
# its source= lines with broken input (no emulator needed)
add_test(NAME cdrun_source COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=$<TARGET_FILE:corpusrun>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/cdrun_source
	-P ${CMAKE_CURRENT_SOURCE_DIR}/check-cdrun-source.cmake)

host_help_test(corpusrun ${CORPUSRUN_SUBCOMMANDS})
host_readme_test(corpusrun README.md ${CORPUSRUN_SUBCOMMANDS})
# the run directories of corpusrun pass and tedab run (runwork.h): runs of
# their own, the leftovers, work/last, the free-space check
add_executable(test_runwork test_runwork.c)
target_link_libraries(test_runwork PRIVATE lib_runner_util)
target_compile_options(test_runwork PRIVATE -Wall -Wextra)
add_test(NAME runner_runwork COMMAND test_runwork ${CMAKE_CURRENT_BINARY_DIR}/runner_runwork)
