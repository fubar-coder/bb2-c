# tedoracle - the original TED (ref/ted) as a library for the oracle data
# of the TED tests (oracle.c).  Runs as blitz2:ted under the original
# blitz2 1.95 (ref/blitz2); ted-oracle.cmake runs it in Amiberry.
set(ORACLE_LABELS
	tokens loadxtra savextra cleanup external returncode int dos dosopen
	chcols dosbuff charbuff charbuff2 charbuff3 charbuff4 tokenise
	defaultreqga9 error donew detok tokit2 fileopen2 realsave
	pointeron sleepon showfname beepem showall showtopitem showbotitem
	showthisitem blokprnt2 scrollup scrupall scrolldown scrdownall cls
	scrollrite scrollleft refresh showhprop showvprop)
set(gen ${CMAKE_CURRENT_BINARY_DIR}/gen)
add_custom_command(
	OUTPUT ${gen}/ted_labels.h
	COMMAND ${CMAKE_COMMAND} -DLABELS=${REF_DIR}/ted.labels -DOUT=${gen}/ted_labels.h
		"-DNAMES=${ORACLE_LABELS}" -P ${CMAKE_CURRENT_SOURCE_DIR}/labels.cmake
	DEPENDS ${REF_DIR}/ted.labels labels.cmake
	COMMENT "Labels of the reference TED for the oracle"
	VERBATIM)

# not ALL: only the target ted-oracle and ctest ted_oracle_reference use it
# (ctest originals_refbuild builds it)
add_executable(tedoracle EXCLUDE_FROM_ALL $<TARGET_OBJECTS:edstub> oracle.c ${gen}/ted_labels.h)
target_include_directories(tedoracle PRIVATE ${gen} ${EDSTUB_DIR})
target_compile_options(tedoracle PRIVATE ${EDSTUB_COMPILE_OPTIONS})
target_link_options(tedoracle PRIVATE ${EDSTUB_LINK_OPTIONS})
set_target_properties(tedoracle PROPERTIES SUFFIX "")
# hunk layout: +0 moveq #20,d0 (shell start fails), +4 movem.l (outside)
add_dependencies(tedoracle host_tools)
add_custom_command(TARGET tedoracle POST_BUILD
	COMMAND ${HUNKCHECK} start $<TARGET_FILE:tedoracle> 0:7014 4:48e7
	VERBATIM)

# regenerating the oracle data in tests/data (not part of ALL: needs the CD
# and Amiberry, runs some minutes); ORACLE_LIMIT=N: trial run on the first
# N sources, tests/data stays as it is
set(ORACLE_LIMIT "" CACHE STRING "ted-oracle: only the first N sources (trial run)")
add_custom_target(ted-oracle
	COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN} -DRESULTS=${RESULTS}
		-DBLITZ2=${CMAKE_BINARY_DIR}/programs/blitz2/ref/blitz2 -DTED=${REF_DIR}/ted
		-DORACLE=$<TARGET_FILE:tedoracle>
		-DCD=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/run -DDATA=${PROJECT_SOURCE_DIR}/tests/data
		-DLIMIT=${ORACLE_LIMIT} -P ${CMAKE_CURRENT_SOURCE_DIR}/ted-oracle.cmake
	DEPENDS tedoracle ted_ref blitz2_ref host_tools
	USES_TERMINAL
	COMMENT "TED oracle data (Amiberry, original blitz2 1.95 and TED 1.1)"
	VERBATIM)

# ctest ted_oracle_reference (label originals, preset originals): the same
# run into the build directory, the committed oracle data must come out
# (ted-oracle-check.cmake); the check with broken input: a hash that is not
# the committed one (no emulator), and the same files pass
set(oracle_check ${CMAKE_CURRENT_SOURCE_DIR}/ted-oracle-check.cmake)
add_test(NAME ted_oracle_reference COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DRESULTS=${RESULTS}
	-DBLITZ2=${CMAKE_BINARY_DIR}/programs/blitz2/ref/blitz2 -DTED=${REF_DIR}/ted
	-DORACLE=$<TARGET_FILE:tedoracle>
	-DCD=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/check-run -DCHECKDIR=${CMAKE_CURRENT_BINARY_DIR}/check
	-DDATA=${PROJECT_SOURCE_DIR}/tests/data -P ${oracle_check})
math(EXPR limit "1800 + 120 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(ted_oracle_reference PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${limit} FIXTURES_REQUIRED originals_refbuild)
foreach(name right wrong)
	add_test(NAME ted_oracle_reference_check_${name} COMMAND ${CMAKE_COMMAND} -DNO_RUN=ON
		-DDATA=${CMAKE_CURRENT_SOURCE_DIR}/selftest/right
		-DCHECKDIR=${CMAKE_CURRENT_SOURCE_DIR}/selftest/${name} -P ${oracle_check})
endforeach()
set_tests_properties(ted_oracle_reference_check_right PROPERTIES
	PASS_REGULAR_EXPRESSION "the oracle data of[ \n]+[^ \n]*selftest/right[ \n]+are the original's")
set_tests_properties(ted_oracle_reference_check_wrong PROPERTIES
	PASS_REGULAR_EXPRESSION "differs[ \n]+from[ \n]+[^ \n]*selftest/right[ \n]+in[ \n]+ted-oracle.sha256[ \n]+\\(")
