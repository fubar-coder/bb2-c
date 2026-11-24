# the readable tables of the type rules of an expression (tools/exprtables):
# links blitz2's exprtype.c as the host test does (blitz2_exprtype_host,
# programs/blitz2/tests/host)
add_library(lib_exprtype_tables STATIC rules.c tsv.c page.c)
set_target_properties(lib_exprtype_tables PROPERTIES OUTPUT_NAME exprtype_tables)
target_include_directories(lib_exprtype_tables PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_exprtype_tables PUBLIC blitz2_exprtype_host)
