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

#   exprtyping the typing pass of rule P+ (src/exprtyping.c, change
#            expr-tree-5-operand-typing, task 3.3): every decision (the
#            format and path of each operation, the conversions of its
#            operands, how the value is taken) against the expression
#            matrix's model with EM_RULES_DECIDED, for every cell of the
#            matrix and the chains of the analysis (the order pairs, the
#            idioms), and the fast path of a 48.16 operation by
#            src/exprarith.c.
add_library(blitz2_exprtyping_host STATIC ${BLITZ2_SRC}/exprtyping.c)
set_target_properties(blitz2_exprtyping_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_exprtyping_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_exprtyping_host)
target_include_directories(blitz2_exprtyping_host PUBLIC ${BLITZ2_SRC})
target_link_libraries(blitz2_exprtyping_host PUBLIC blitz2_exprtype_host)

add_executable(test_exprtyping test_exprtyping.cpp)
target_link_libraries(test_exprtyping PRIVATE blitz2_exprtyping_host blitz2_exprarith_host lib_exprmatrix
	snitch::snitch)
target_compile_definitions(test_exprtyping PRIVATE
	EXPRMATRIX_TXT="${exprtype_repo}/tests/corpus/tests-exprmatrix/matrix.txt")
target_compile_options(test_exprtyping PRIVATE -Wall -Wextra)
own_warnings(test_exprtyping)
set_target_properties(test_exprtyping PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_exprtyping COMMAND test_exprtyping)

#   exprfold the constant folding of whole-number and Quick operations in C
#            (src/exprfold.c, change expr-tree-5-operand-typing, task 3.6):
#            single operations against the values of the 68000 code, what
#            is not folded there, and the matrix's cells of two constants
#            in the Print context against the expression matrix's model
#            (EM_RULES_PORTNOW), with a planted wrong fold.
add_library(blitz2_exprfold_host STATIC ${BLITZ2_SRC}/exprfold.c)
set_target_properties(blitz2_exprfold_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_exprfold_host PRIVATE -Wall -Wextra -pedantic)
own_warnings(blitz2_exprfold_host)
target_include_directories(blitz2_exprfold_host PUBLIC ${BLITZ2_SRC})
target_link_libraries(blitz2_exprfold_host PUBLIC blitz2_exprarith_host blitz2_exprtype_host)

add_executable(test_exprfold test_exprfold.cpp)
target_link_libraries(test_exprfold PRIVATE blitz2_exprfold_host lib_exprmatrix snitch::snitch)
target_compile_definitions(test_exprfold PRIVATE
	EXPRMATRIX_TXT="${exprtype_repo}/tests/corpus/tests-exprmatrix/matrix.txt")
target_compile_options(test_exprfold PRIVATE -Wall -Wextra)
own_warnings(test_exprfold)
set_target_properties(test_exprfold PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_exprfold COMMAND test_exprfold)
