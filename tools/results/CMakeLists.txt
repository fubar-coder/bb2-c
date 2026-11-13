# results: a result against its expectation or the previous result (README.md)
set(RESULTS_SUBCOMMANDS match prev cd-update cd-reference ted-oracle libs-compare libs-accept libs-update libs-dump fix-scope bug-orig-accept corpus-accept)
add_executable(results main.cpp cmd_match.cpp cmd_prev.cpp cmd_cd.cpp cmd_ted_oracle.cpp cmd_libs.cpp cmd_fix_scope.cpp cmd_bugorig.cpp cmd_corpus.cpp)
target_link_libraries(results PRIVATE lib_cli lib_outmatch lib_prevcmp lib_previntended lib_cdtable
	lib_oracletable lib_libscmp lib_bugorigcmp lib_corpusexpect)
set_target_properties(results PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(results ${RESULTS_SUBCOMMANDS})
host_readme_test(results README.md ${RESULTS_SUBCOMMANDS})
