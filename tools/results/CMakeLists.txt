# results: a result against its expectation or the previous result (README.md)
set(RESULTS_SUBCOMMANDS match prev cd-update cd-reference ted-oracle)
add_executable(results main.cpp cmd_match.cpp cmd_prev.cpp cmd_cd.cpp cmd_ted_oracle.cpp)
target_link_libraries(results PRIVATE lib_cli lib_outmatch lib_prevcmp lib_previntended lib_cdtable
	lib_oracletable)
set_target_properties(results PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(results CLI ${RESULTS_SUBCOMMANDS})
host_readme_test(results README.md CLI ${RESULTS_SUBCOMMANDS})
