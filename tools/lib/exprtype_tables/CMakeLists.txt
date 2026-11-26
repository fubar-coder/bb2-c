# the readable tables of the type rules of an expression (tools/exprtables):
# links blitz2's exprtype.c and exprtyping.c as the host tests do
# (blitz2_exprtype_host, blitz2_exprtyping_host, programs/blitz2/tests/host);
# the candidate rules of expr-tree-5 and rule P+ (cand.c) from the expression
# matrix's model (lib_exprmatrix)
add_library(lib_exprtype_tables STATIC rules.c tsv.c page.c cand.c)
set_target_properties(lib_exprtype_tables PROPERTIES OUTPUT_NAME exprtype_tables)
target_include_directories(lib_exprtype_tables PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_exprtype_tables PUBLIC blitz2_exprtype_host blitz2_exprtyping_host lib_exprmatrix)
