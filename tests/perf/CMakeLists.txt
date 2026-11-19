# The benchmark of the libraries (spec testing/performance-budgets,
# tests/README.md "The benchmark"): one For loop per command in the
# cycle-exact A500, timed with a CIA-B timer cascade, the empty loop
# subtracted; the cycles per call of the C libraries.
#   bench/    the Blitz sources, one program per area (text, tokenised here
#             with tedsrc tok like the corpus); a program with the header
#             line "; @rwe NAME..." uses RWE userlibs (below), one with
#             "; @file DEST|PATH" reads the file PATH as CORPUS:DEST
#             (@ANCIENT@ in PATH: archive/ancient/github-1491a35a, from
#             cmake/fetch-originals.cmake; @CD@: the CD's directory
#             archive/cd/ultimate-blitz-basic-2.1; the program is left
#             out without the file)
#   budgets.txt the budgets: the highest allowed cycles per call of each
#             command, and the original's value as the target
#   perf.cmake  the test script: corpusrun pass --observe, then results perf
#   selftest/   an observation and budgets for the checks of the budgets

# --------------------------------------------------------- the sources
set(PERF_SRC ${CMAKE_CURRENT_BINARY_DIR}/src)
set(PERF_ANCIENT ${PROJECT_SOURCE_DIR}/archive/ancient/github-1491a35a)
set(PERF_CD ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1)
set(TED_TOKENS ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
file(GLOB bas CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bench/*.bas)
set(perf_sources)
set(perf_rwe_deps)
foreach(b IN LISTS bas)
	get_filename_component(name ${b} NAME_WE)
	set(out ${PERF_SRC}/${name}.bb2)
	# RWE userlibs (; @rwe NAME..., as in tests/bug-orig): the program is
	# tokenised with a token list of blitz2-c, the libraries' variant and
	# the named libraries; clib gets the variant's acidlibs and deflibs and
	# the C build of each library in blitz2:userlibs/rwe/, orig the CD's
	# libraries and the reference (perf_NAME_RWE_* below)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${b})
	# data files (; @file DEST|PATH): file= lines of its passes; without
	# the file the program is left out
	# (the line's ";" is CMake's list separator: matched without it)
	file(STRINGS ${b} pfiles REGEX "^; @file ")
	set(perf_${name}_DATA "")
	set(missing "")
	foreach(l IN LISTS pfiles)
		string(REGEX MATCH "@file ([^|]+)\\|(.+)$" l "${l}")
		if(NOT l)
			continue()
		endif()
		string(REPLACE "@ANCIENT@" "${PERF_ANCIENT}" path "${CMAKE_MATCH_2}")
		string(REPLACE "@CD@" "${PERF_CD}" path "${path}")
		if(NOT EXISTS ${path})
			set(missing ${path})
		endif()
		string(APPEND perf_${name}_DATA "file=${CMAKE_MATCH_1}|${path}\n")
	endforeach()
	if(missing)
		message(STATUS "perf: ${name} left out: no ${missing} (cmake -P cmake/fetch-originals.cmake)")
		continue()
	endif()
	file(STRINGS ${b} rwe REGEX "^; @rwe ")
	set(tok ${TED_TOKENS})
	if(rwe)
		string(REGEX MATCH "@rwe (.+)$" rwe "${rwe}")
		separate_arguments(rwe UNIX_COMMAND "${CMAKE_MATCH_1}")
		set(variant "")
		set(objs "")
		set(files "")
		foreach(n IN LISTS rwe)
			get_property(info GLOBAL PROPERTY RWE_LIB_${n})
			if(NOT info)
				message(FATAL_ERROR "perf: ${b}: @rwe ${n}: no RWE userlib of that name")
			endif()
			string(REPLACE "|" ";" info "${info}")
			list(GET info 0 obj)
			list(GET info 1 reffile)
			list(GET info 3 v)
			list(GET info 4 vout)
			if(variant AND NOT v STREQUAL variant)
				message(FATAL_ERROR "perf: ${b}: @rwe: ${n} is built for the variant ${v}, "
					"the others for ${variant}")
			endif()
			set(variant ${v})
			string(APPEND files "file=blitz2/userlibs/rwe/${n}.obj|${obj}|clib\n"
				"file=blitz2/userlibs/rwe/${n}.obj|${reffile}|orig\n")
			list(APPEND objs ${obj})
			list(APPEND perf_rwe_deps ${v}-${n}_obj)
		endforeach()
		list(APPEND perf_rwe_deps ${variant}-acidlibs ${variant}-deflibs)
		set(perf_${name}_RWE_DIR ${vout})
		set(perf_${name}_RWE_FILES "${files}")
		set(tok ${PERF_SRC}/${name}-tokens.txt)
		add_custom_command(OUTPUT ${tok}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PERF_SRC}
			COMMAND ${BBSRC} tokens -o ${tok} $<TARGET_FILE:blitz2> ${vout}/acidlibs
				${vout}/deflibs ${objs}
			DEPENDS ${objs} ${vout}/acidlibs ${vout}/deflibs blitz2 ${BBSRC} host_tools
			COMMENT "perf ${name}: token list with the RWE userlibs"
			VERBATIM)
	endif()
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PERF_SRC}
		COMMAND ${TEDSRC} tok ${tok} ${b} ${out}
		DEPENDS ${b} ${tok} ${TEDSRC} host_tools
		COMMENT "Tokenising perf/${name}.bas"
		VERBATIM)
	list(APPEND perf_sources ${out})
endforeach()
# the modules bmod plays: the corpus's descriptions (tests/corpus/data)
set(PERF_DATA ${CMAKE_CURRENT_BINARY_DIR}/data)
set(perf_data)
foreach(name t111_song.mod t112_song.med)
	set(out ${PERF_DATA}/${name})
	add_custom_command(OUTPUT ${out}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PERF_DATA}
		COMMAND ${MKTESTDATA} ${PROJECT_SOURCE_DIR}/tests/corpus/data/${name}.spec ${out}
		DEPENDS ${PROJECT_SOURCE_DIR}/tests/corpus/data/${name}.spec ${MKTESTDATA} host_tools
		COMMENT "Generating perf data ${name}"
		VERBATIM)
	list(APPEND perf_data ${out})
endforeach()
add_custom_target(perf_sources ALL DEPENDS ${perf_sources} ${perf_data})
if(perf_rwe_deps)
	list(REMOVE_DUPLICATES perf_rwe_deps)
	add_dependencies(perf_sources ${perf_rwe_deps})
endif()

# ------------------------------------------------------------ the pass
# One configuration at a time (--observe): clib = blitz2-c with the C
# libraries of this build (the test), orig = blitz2-c with the CD's
# libraries (the target perf-original: the original's values).  The
# machine of the measurement: a500 (68000, Kickstart 1.3, ECS) with
# cycle-exact CPU, memory and blitter, runtime error checking off.  The
# emulated machine runs in warp mode (warp=true: no wait for the real time,
# the chipset and CPU timing stays exact), so a session takes a fraction of
# the emulated time; its cycle values are those of the real-speed run
# (tests/README.md, "The benchmark").  perf.pass has all programs in one
# session (the target perf-original), perf-NAME.pass one program (the test
# perf_NAME: the sessions of the programs run side by side).  A program
# with RWE userlibs needs other libraries for clib and orig: it is not in
# perf.pass, its pass has them (LIBDIR: the variant's directory, FILES: the
# file= lines of the libraries); its target is measured with that pass and
# the tag orig, after the reference build (ctest -R rwe_refbuild).
set(PERF_TIMEOUT 900 CACHE STRING "perf: time limit of the emulator session (seconds)")
set(CD_BLITZ2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
# the pass file of the sources SOURCES (a list) under the name NAME, the
# libraries of clib from LIBDIR (default: this build's), FILES added: the text
# before the sources is the same for every pass
function(perf_pass name out)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "LIBDIR;FILES" "SOURCES")
	set(libdir ${BLITZ2_DIR})
	if(arg_LIBDIR)
		set(libdir ${arg_LIBDIR})
	endif()
	string(CONCAT text "# perf: generated by tests/perf/CMakeLists.txt\n"
		"test=${name}\nwork=${CMAKE_CURRENT_BINARY_DIR}/${name}\n"
		"report=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt\nruntime_errors=off\nserial=off\n"
		"timeout=${PERF_TIMEOUT}\nparallel=1\nted=$<TARGET_FILE:tedstub>\n"
		"launcher=$<TARGET_FILE:reqtrap>\nrcput=$<TARGET_FILE:rcput>\n"
		"config=clib|ref|$<TARGET_FILE:blitz2>|${libdir}/deflibs|${libdir}/acidlibs\n"
		"config=orig|output|$<TARGET_FILE:blitz2>|${CD_BLITZ2}/DEFLIBS|${CD_BLITZ2}/ACIDLIBS\n"
		"machine=a500\nuae=cpu_cycle_exact=true\nuae=cpu_memory_cycle_exact=true\n"
		"uae=blitter_cycle_exact=true\nuae=cycle_exact=true\nuae=cpu_speed=real\n"
		"uae=warp=true\n")
	foreach(f IN LISTS perf_data)
		get_filename_component(n ${f} NAME)
		string(APPEND text "file=data/${n}|${f}\n")
	endforeach()
	string(APPEND text "${arg_FILES}")
	foreach(s IN LISTS arg_SOURCES)
		string(APPEND text "source=${s}\n")
	endforeach()
	file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass CONTENT "${text}")
	set(${out} ${CMAKE_CURRENT_BINARY_DIR}/${name}.pass PARENT_SCOPE)
endfunction()
set(perf_all)
set(perf_all_data "")
foreach(src IN LISTS perf_sources)
	get_filename_component(prog ${src} NAME_WE)
	if(NOT DEFINED perf_${prog}_RWE_DIR)
		list(APPEND perf_all ${src})
		string(APPEND perf_all_data "${perf_${prog}_DATA}")
	endif()
endforeach()
perf_pass(perf PERF_PASS FILES "${perf_all_data}" SOURCES ${perf_all})
set(PERF_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/perf.cmake)

# ------------------------------------------------------------ the tests
# perf_NAME: one program of the benchmark (its own pass, its own emulator
# session) against its budgets, the PROGRAM.* lines of budgets.txt.  The
# sessions are independent (the cycle values do not depend on what else the
# host does), so ctest runs them side by side (the preset perf, jobs 7).
set(PERF_BUDGETS ${CMAKE_CURRENT_SOURCE_DIR}/budgets.txt)
math(EXPR limit "${PERF_TIMEOUT} + 120 + ${EMULATION_QUEUE_WAIT}")
foreach(src IN LISTS perf_sources)
	get_filename_component(prog ${src} NAME_WE)
	if(DEFINED perf_${prog}_RWE_DIR)
		perf_pass(perf_${prog} pass LIBDIR ${perf_${prog}_RWE_DIR}
			FILES "${perf_${prog}_RWE_FILES}${perf_${prog}_DATA}" SOURCES ${src})
	else()
		perf_pass(perf_${prog} pass FILES "${perf_${prog}_DATA}" SOURCES ${src})
	endif()
	add_test(NAME perf_${prog} COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=${CORPUSRUN} -DRESULTS=${RESULTS}
		-DPASS=${pass} -DTAG=clib -DPROGRAM=${prog} -DBUDGETS=${PERF_BUDGETS}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/perf_${prog}-run
		-DTEST_SCRIPT=${PERF_SCRIPT} -P ${WITH_WORKDIR})
	# label perf: only the preset perf runs them (all, host, emulator and the
	# others exclude it); skipped (77) without Amiberry or the emulation data
	set_tests_properties(perf_${prog} PROPERTIES LABELS "perf;emulator" SKIP_RETURN_CODE 77
		TIMEOUT ${limit})
endforeach()
# the target perf-original (not ALL, not a test; needs the CD and Amiberry):
# the same benchmark with the CD's libraries, the table of the original's
# cycles per call (the TARGET column of budgets.txt), no budgets
add_custom_target(perf-original
	COMMAND ${CMAKE_COMMAND} -E env AMIGA_EMULATION_OWNER=${CMAKE_BINARY_DIR}
		${CMAKE_COMMAND} -DCORPUSRUN=${CORPUSRUN} -DRESULTS=${RESULTS} -DPASS=${PERF_PASS}
		-DTAG=orig -DWORK=${CMAKE_CURRENT_BINARY_DIR}/perf-original
		-DTEST_SCRIPT=${PERF_SCRIPT} -P ${WITH_WORKDIR}
	DEPENDS blitz2 tedstub reqtrap rcput perf_sources host_tools
	USES_TERMINAL
	COMMENT "The benchmark with the CD's libraries (the original's cycles per call)"
	VERBATIM)

# the check of the budgets with broken input (no emulator: the committed
# observation selftest/obs, Peek.b at 217.0 cycles per call, through
# perf.cmake): a budget below the measurement fails with the command and
# both values, the right budget passes
foreach(t ok low)
	if(t STREQUAL "ok")
		set(test perf_budget_ok)
		set(want)
	else()
		set(test perf_budget_broken)
		set(want -DWANT_FAIL=ON)
	endif()
	add_test(NAME ${test} COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS}
		-DOBS=${CMAKE_CURRENT_SOURCE_DIR}/selftest/obs
		-DBUDGETS=${CMAKE_CURRENT_SOURCE_DIR}/selftest/budgets-${t}.txt ${want}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${test} -DTEST_SCRIPT=${PERF_SCRIPT} -P ${WITH_WORKDIR})
endforeach()
set_tests_properties(perf_budget_ok PROPERTIES
	PASS_REGULAR_EXPRESSION "results perf: every command within its budget \\(1 measured\\)")
set_tests_properties(perf_budget_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "results perf: bmath\\.peekb: measured 217\\.0 cycles per call, budget 130\n.*perf: a command over its budget")

# the filter of one program (-DPROGRAM, perf_NAME): only the lines of that
# program are judged.  The same observation and budgets: bmath passes,
# bstr has a budget without a measurement, bdisp has no budget line at all
foreach(t ok bstr bdisp)
	set(test perf_budget_program_${t})
	set(want)
	if(t STREQUAL "ok")
		set(prog bmath)
	else()
		set(prog ${t})
		set(want -DWANT_FAIL=ON)
	endif()
	add_test(NAME ${test} COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS}
		-DOBS=${CMAKE_CURRENT_SOURCE_DIR}/selftest/obs -DPROGRAM=${prog}
		-DBUDGETS=${CMAKE_CURRENT_SOURCE_DIR}/selftest/budgets-two.txt ${want}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${test} -DTEST_SCRIPT=${PERF_SCRIPT} -P ${WITH_WORKDIR})
endforeach()
set_tests_properties(perf_budget_program_ok PROPERTIES
	PASS_REGULAR_EXPRESSION "results perf: every command within its budget \\(1 measured\\)")
set_tests_properties(perf_budget_program_bstr PROPERTIES
	PASS_REGULAR_EXPRESSION "bstr\\.len: not measured.*perf: a command over its budget")
set_tests_properties(perf_budget_program_bdisp PROPERTIES
	PASS_REGULAR_EXPRESSION "perf: the budgets have no line of the program bdisp")
