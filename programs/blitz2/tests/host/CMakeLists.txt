# Host tests of blitz2-ab3's pure code (programs/blitz2/src, no AmigaOS
# call), built with the native compiler by host/CMakeLists.txt: one snitch
# program per test file.
#   ab3date  dos.library's day number as a date (src/ab3date.c, the
#            escapes \\__DATE__ and \\__DATE_GER__)
set(BLITZ2_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_library(blitz2_ab3date_host STATIC ${BLITZ2_SRC}/ab3date.c)
set_target_properties(blitz2_ab3date_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_ab3date_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_ab3date_host)
target_include_directories(blitz2_ab3date_host PUBLIC ${BLITZ2_SRC})

add_executable(test_ab3date test_ab3date.cpp)
target_link_libraries(test_ab3date PRIVATE blitz2_ab3date_host snitch::snitch)
target_compile_options(test_ab3date PRIVATE -Wall -Wextra)
own_warnings(test_ab3date)
set_target_properties(test_ab3date PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_ab3date COMMAND test_ab3date)

#   exprtype the type decisions of an expression (src/exprtype.c, issue
#            #1180, change expr-tree-1-typing): every decision against the
#            committed table exprtype.tsv, a changed row of it, and the
#            two-operand cells of the expression matrix against its model
#            (tools/lib/exprmatrix).  EXPRTYPE_TSV_WRITE=FILE writes the
#            table instead of reading it (the generator mode).
add_library(blitz2_exprtype_host STATIC ${BLITZ2_SRC}/exprtype.c)
set_target_properties(blitz2_exprtype_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_exprtype_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_exprtype_host)
target_include_directories(blitz2_exprtype_host PUBLIC ${BLITZ2_SRC})

get_filename_component(exprtype_repo ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)
add_executable(test_exprtype test_exprtype.cpp)
target_link_libraries(test_exprtype PRIVATE blitz2_exprtype_host lib_exprmatrix snitch::snitch)
target_compile_definitions(test_exprtype PRIVATE
	EXPRTYPE_TSV="${CMAKE_CURRENT_SOURCE_DIR}/exprtype.tsv"
	EXPRMATRIX_TXT="${exprtype_repo}/tests/corpus/tests-exprmatrix/matrix.txt")
target_compile_options(test_exprtype PRIVATE -Wall -Wextra)
own_warnings(test_exprtype)
set_target_properties(test_exprtype PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_exprtype COMMAND test_exprtype)

#   exprtree the nodes of an expression and the walk over them
#            (src/exprtree.c, issue #1180, change expr-tree-2-operators):
#            the order of the emission, the limit of the counted nodes,
#            the bound of the arena, and a chain of 100 000 operators of
#            one precedence on a stack that does not grow.  The hooks of
#            the walk are the test's own small evaluator.
add_library(blitz2_exprtree_host STATIC ${BLITZ2_SRC}/exprtree.c)
set_target_properties(blitz2_exprtree_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_exprtree_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_exprtree_host)
target_include_directories(blitz2_exprtree_host PUBLIC ${BLITZ2_SRC})

add_executable(test_exprtree test_exprtree.cpp)
target_link_libraries(test_exprtree PRIVATE blitz2_exprtree_host snitch::snitch)
target_compile_options(test_exprtree PRIVATE -Wall -Wextra)
own_warnings(test_exprtree)
set_target_properties(test_exprtree PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_exprtree COMMAND test_exprtree)

#   exprarith the arithmetic of the formats of rule P+ (src/exprarith.c,
#            change expr-tree-5-operand-typing, task 3.4): whole numbers,
#            16.16 and 48.16 bit for bit against the values of the
#            expression matrix's model (tools/lib/exprmatrix), with P+
#            (EM_RULES_DECIDED) and today's rules.
add_library(blitz2_exprarith_host STATIC ${BLITZ2_SRC}/exprarith.c)
set_target_properties(blitz2_exprarith_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_exprarith_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_exprarith_host)
target_include_directories(blitz2_exprarith_host PUBLIC ${BLITZ2_SRC})

add_executable(test_exprarith test_exprarith.cpp)
target_link_libraries(test_exprarith PRIVATE blitz2_exprarith_host lib_exprmatrix snitch::snitch)
target_compile_options(test_exprarith PRIVATE -Wall -Wextra)
own_warnings(test_exprarith)
set_target_properties(test_exprarith PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_exprarith COMMAND test_exprarith)
