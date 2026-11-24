# exprtables: readable tables of the type rules of an expression (README.md)
set(EXPRTABLES_SUBCOMMANDS page)
add_executable(exprtables main.cpp cmd_page.cpp)
target_link_libraries(exprtables PRIVATE lib_cli lib_exprtype_tables)
set_target_properties(exprtables PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(exprtables ${EXPRTABLES_SUBCOMMANDS})
host_readme_test(exprtables README.md ${EXPRTABLES_SUBCOMMANDS})

# the page of the tables (docs/blitz2/expression-type-tables.md): the build
# step generates it and checks it against the committed page (exprtables
# page --check); a page that differs fails the step and is not written.
# After an intended change of the rules: build expression_type_tables_update
# (writes the committed page) and commit it.
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(exprtables_page ${repo}/docs/blitz2/expression-type-tables.md)
set(exprtables_out ${CMAKE_CURRENT_BINARY_DIR}/expression-type-tables.md)
add_custom_command(OUTPUT ${exprtables_out}
	COMMAND exprtables page --check ${exprtables_page} -o ${exprtables_out}
	DEPENDS exprtables ${exprtables_page}
	COMMENT "Checking docs/blitz2/expression-type-tables.md against the type rules"
	VERBATIM)
add_custom_target(expression_type_tables ALL DEPENDS ${exprtables_out})
add_custom_target(expression_type_tables_update
	COMMAND exprtables page -o ${exprtables_page}
	COMMENT "Writing docs/blitz2/expression-type-tables.md from the type rules"
	VERBATIM)

# the committed page against the tool's output, the same from the committed
# table of exprtype.c's decisions (programs/blitz2/tests/host/exprtype.tsv),
# and the check with broken input: a copy of the page with a changed cell
# and one without a note must fail it, naming the line, and write nothing
# (page-broken.cmake)
add_test(NAME exprtables_page COMMAND exprtables page --check ${exprtables_page})
add_test(NAME exprtables_page_tsv COMMAND exprtables page --check ${exprtables_page}
	--tsv ${repo}/programs/blitz2/tests/host/exprtype.tsv)
add_test(NAME exprtables_page_broken COMMAND ${CMAKE_COMMAND} -DEXPRTABLES=$<TARGET_FILE:exprtables>
	-DPAGE=${exprtables_page} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/page-broken
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/page-broken.cmake -P ${WITH_WORKDIR})
set_tests_properties(exprtables_page_broken PROPERTIES PASS_REGULAR_EXPRESSION "page-broken: as expected")
