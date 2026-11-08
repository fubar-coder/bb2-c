# tedsrc: sources through the C core of TED (host side): tok (tokenised as
# TED saves them), detok (as TED shows them); README.md.  bblibsrc.cmake:
# the build target bblibsrc (included by the top-level CMakeLists.txt)
set(TEDSRC_SUBCOMMANDS tok detok)
add_executable(tedsrc main.c cmd_tok.c cmd_detok.c)
target_link_libraries(tedsrc PRIVATE lib_subcmd "$<LINK_GROUP:RESCAN,tedhost,tedcore_host>")
target_compile_options(tedsrc PRIVATE -Wall)
set_target_properties(tedsrc PROPERTIES C_STANDARD 23 C_EXTENSIONS ON)
host_help_test(tedsrc ${TEDSRC_SUBCOMMANDS})
host_readme_test(tedsrc README.md)

get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)
set(tokens ${repo}/tests/data/ted-tokens.txt)
set(t ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(cd ${repo}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)

# ---------------------------------------------------------------- tok
# the line check: 129 characters pass, 130 are refused (TED would cut)
foreach(n 129 130)
	add_test(NAME tedtok_line${n} COMMAND tedsrc tok ${tokens}
		${t}/line${n}.bas ${CMAKE_CURRENT_BINARY_DIR}/line${n}.bb2)
endforeach()
set_tests_properties(tedtok_line130 PROPERTIES WILL_FAIL TRUE)

# the token list made from the CD's compiler and libraries: the same file
# as with the list file (skipped without the CD)
add_test(NAME tedtok_libs COMMAND ${CMAKE_COMMAND} -DTEDSRC=$<TARGET_FILE:tedsrc>
	-DTOKENS=${tokens} -DCD=${cd} -DIN=${t}/tokens.bas
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens -P ${t}/libs.cmake)
set_tests_properties(tedtok_libs PROPERTIES SKIP_RETURN_CODE 77)

# -------------------------------------------------------------- detok
# tests: tests/*.bb2 (tokenised, written byte by byte), the expected text
# next to them; unknown token and half token must fail without output
set(run -DTEDSRC=$<TARGET_FILE:tedsrc> -DTOKENS=${tokens})
# good: tokens, TAB (tabstop 2), empty line, a text line (LF: tokenised on
# loading), last line
add_test(NAME teddetok_good COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/good.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/good.txt -DEXPECT=${t}/good.txt -P ${t}/run.cmake)
# long: a line of 135 characters is two lines in TED (127, 3 lost, 5)
add_test(NAME teddetok_long COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/long.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/long.txt -DEXPECT=${t}/long.txt
	"-DNOTE=2 lines in the file, 3 in TED" -P ${t}/run.cmake)
# broken: a token TED does not know, a token byte at the end of the file
add_test(NAME teddetok_unknown COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/unknown.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/unknown.txt "-DERROR=unknown.bb2:2: unknown token \\$9001"
	-P ${t}/run.cmake)
add_test(NAME teddetok_half COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/half.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/half.txt "-DERROR=half.bb2:1: token byte \\$DD without"
	-P ${t}/run.cmake)
add_test(NAME teddetok_missing COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/missing.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/missing.txt "-DERROR=missing.bb2" -P ${t}/run.cmake)
# the whole bblibsrc set (skipped without the Aminet archive)
add_test(NAME teddetok_bblibsrc COMMAND ${CMAKE_COMMAND} ${run} -DREPO=${repo}
	-P ${t}/bblibsrc.cmake)
set_tests_properties(teddetok_bblibsrc PROPERTIES SKIP_RETURN_CODE 77)

# the token list made from the CD's compiler and libraries instead of the
# list file: the same text (skipped without the CD); a compiler that is no
# load file must fail
# ($<SEMICOLON>: the lists must reach run.cmake as one argument each)
set(S $<SEMICOLON>)
set(cdlibs -DTEDSRC=$<TARGET_FILE:tedsrc>
	-DTOKENS=--blitz2${S}${cd}/BLITZ2${S}--lib${S}${cd}/ACIDLIBS${S}--lib${S}${cd}/DEFLIBS
	-DREQUIRE=${cd}/BLITZ2${S}${cd}/ACIDLIBS${S}${cd}/DEFLIBS)
add_test(NAME teddetok_good_libs COMMAND ${CMAKE_COMMAND} ${cdlibs} -DIN=${t}/good.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/good_libs.txt -DEXPECT=${t}/good.txt -P ${t}/run.cmake)
add_test(NAME teddetok_unknown_libs COMMAND ${CMAKE_COMMAND} ${cdlibs} -DIN=${t}/unknown.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/unknown_libs.txt "-DERROR=unknown.bb2:2: unknown token \\$9001"
	-P ${t}/run.cmake)
add_test(NAME teddetok_bblibsrc_libs COMMAND ${CMAKE_COMMAND} ${cdlibs} -DREPO=${repo}
	-P ${t}/bblibsrc.cmake)
add_test(NAME teddetok_badcompiler COMMAND ${CMAKE_COMMAND} -DTEDSRC=$<TARGET_FILE:tedsrc>
	-DTOKENS=--blitz2${S}${tokens} -DIN=${t}/good.bb2 -DOUT=${CMAKE_CURRENT_BINARY_DIR}/bad.txt
	"-DERROR=ted-tokens.txt: not a load file" -P ${t}/run.cmake)
set_tests_properties(teddetok_good_libs teddetok_unknown_libs teddetok_bblibsrc_libs
	PROPERTIES SKIP_RETURN_CODE 77)
