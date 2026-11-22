# exprmatrix: the expression matrix of blitz2 (README.md)
set(EXPRMATRIX_SUBCOMMANDS generate classify expect)
add_executable(exprmatrix main.cpp cmd_generate.cpp cmd_classify.cpp cmd_expect.cpp)
target_link_libraries(exprmatrix PRIVATE lib_cli lib_exprmatrix)
set_target_properties(exprmatrix PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(exprmatrix ${EXPRMATRIX_SUBCOMMANDS})
host_readme_test(exprmatrix README.md ${EXPRMATRIX_SUBCOMMANDS})

# the committed tests of the matrix (exprmatrix expect, the target
# expr-matrix-update of tests/corpus) against a fresh generation from the
# committed description and measured values; and the same on a copy of the
# files with one of them changed, which must fail naming it
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(exprmatrix_desc ${repo}/tests/corpus/tests-exprmatrix/matrix.txt)
set(exprmatrix_meas ${repo}/tests/corpus/tests-exprmatrix/measured.tsv)
add_test(NAME exprmatrix_committed COMMAND exprmatrix expect ${exprmatrix_desc} ${exprmatrix_meas}
	-o ${repo} --check)
set_tests_properties(exprmatrix_committed PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ files, 0 differ")
add_test(NAME exprmatrix_committed_broken COMMAND ${CMAKE_COMMAND} -DEXPRMATRIX=$<TARGET_FILE:exprmatrix>
	-DREPO=${repo} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/committed-broken
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/committed-broken.cmake -P ${WITH_WORKDIR})
set_tests_properties(exprmatrix_committed_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "committed-broken: as expected")
