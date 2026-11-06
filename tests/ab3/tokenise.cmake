# ctest ab3_tokenise*: the tokens of AB3 mode against AmiBlitz3's (spec
# ab3-compat/tokenizer, "The same tokens as AmiBlitz3"; change ab3-mode-g2
# task 1.3).  One emulator session lets AmiBlitz3 v3.14.0's own compiler
# load every source and write its lines (CLI mode, Blitz3:Amiblitz3 -s
# SOURCE -save TEST:out/NAME.bb2: the editor's lines, each with its 0);
# on the host ab3tok (shared/tokenizer, the code ab3cli tokenises with)
# loads the same source with the token list of AB3 mode - blitz2-ab3's
# AmiBlitz3 list followed by AmiBlitz3's BigLibs (blitztokens, as the
# test ab3_tokens has it equal to AmiBlitz3's) - and compares.
#   cmake -DAMIGA_RUN=... -DBLITZTOKENS=... -DAB3TOK=... -DBLITZ2=... -DAB3=DIR
#         -DSOURCES=FILE;... -DWORK=DIR [-DBREAK=NAME] [-DTIMEOUT=SEC]
#         -P tokenise.cmake
# The sources: SOURCES and AmiBlitz3's test programs (AB3/Sourcecodes/
# Amiblitz3/Tests/*.ab3).
# BREAK: the check itself - the token of NAME changed in our list, the
# comparison must fail.  Exit status 0 if every source is equal; 77
# (skipped) without the emulator or the fetched AmiBlitz3 files.
cmake_minimum_required(VERSION 4.2.3)

foreach(v AMIGA_RUN BLITZTOKENS AB3TOK BLITZ2 AB3 SOURCES WORK)
	if(NOT ${v})
		message(FATAL_ERROR "tokenise.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 600)
endif()
set(biglibs baseLibs utilLibs OsLibs csLibs)
set(need ${AB3}/Amiblitz3 ${AB3}/Libs/ab3pft.library ${AB3}/BlitzLibs/Residents/all.res
	${AB3}/Sourcecodes/Amiblitz3/Tests/atoi.ab3)
foreach(f IN LISTS biglibs)
	list(APPEND need ${AB3}/BlitzLibs/${f})
endforeach()
file(GLOB ab3_tests ${AB3}/Sourcecodes/Amiblitz3/Tests/*.ab3)
list(APPEND SOURCES ${ab3_tests})
foreach(f IN LISTS need SOURCES)
	if(NOT EXISTS ${f})
		message(STATUS "tokenise.cmake: no ${f} - skipped (fetch-originals)")
		cmake_language(EXIT 77)
	endif()
endforeach()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
# Libs/: AmiBlitz3 adds Blitz3:Libs to LIBS: (without the directory the
# lock 0 of the boot volume, docs/ab3/variant.md) and opens its
# ab3pft.library from there - the system has none
file(MAKE_DIRECTORY ${test}/out ${test}/src ${test}/Blitz3/BlitzLibs/Residents
	${test}/Blitz3/BlitzLibs/UserLibs ${test}/Blitz3/Libs)
file(COPY_FILE ${AB3}/Libs/ab3pft.library ${test}/Blitz3/Libs/ab3pft.library)
foreach(f IN LISTS biglibs)
	file(COPY_FILE ${AB3}/BlitzLibs/${f} ${test}/Blitz3/BlitzLibs/${f})
	list(APPEND libs ${AB3}/BlitzLibs/${f})
endforeach()
file(COPY_FILE ${AB3}/BlitzLibs/Residents/all.res ${test}/Blitz3/BlitzLibs/Residents/all.res)
file(COPY_FILE ${AB3}/Amiblitz3 ${test}/Blitz3/Amiblitz3)

# the session: one -save per source, each with its own Assign Blitz3: -
# AmiBlitz3 removes the assign Blitz3: when it ends (after Blitz3:Libs
# from LIBS:, AmiBlitz3.ab3 1428-1440 at @58a11658), also one it did not
# make
set(script "FailAt 21\nCd TEST:src\n")
set(names)
foreach(f IN LISTS SOURCES)
	get_filename_component(file ${f} NAME)
	get_filename_component(name ${f} NAME_WE)
	if(name IN_LIST names)
		message(FATAL_ERROR "tokenise.cmake: two sources named ${name}")
	endif()
	list(APPEND names ${name})
	file(COPY_FILE ${f} ${test}/src/${file})
	string(APPEND script "Assign Blitz3: TEST:Blitz3\nBlitz3:Amiblitz3 >TEST:out/${name}.log -s ${file} -save TEST:out/${name}.bb2\n")
endforeach()
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "tokenise.cmake: the emulator session did not end (${WORK})")
endif()

# our token list, and the check itself
execute_process(COMMAND ${BLITZTOKENS} -last 80DB -o ${WORK}/tokens.txt ${BLITZ2} ${libs}
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitztokens ${BLITZ2}: exit ${rc}")
endif()
if(BREAK)
	file(STRINGS ${WORK}/tokens.txt list)
	list(FILTER list EXCLUDE REGEX "^.... ${BREAK}$")
	list(INSERT list 0 "7FFF ${BREAK}")
	list(JOIN list "\n" list)
	file(WRITE ${WORK}/tokens.txt "${list}\n")
endif()

# the comparison
set(failed 0)
list(LENGTH SOURCES n)
foreach(f IN LISTS SOURCES)
	get_filename_component(name ${f} NAME_WE)
	get_filename_component(file ${f} NAME)
	set(theirs ${test}/out/${name}.bb2)
	if(NOT EXISTS ${theirs})
		message(STATUS "FAIL ${name}: AmiBlitz3 wrote no ${name}.bb2 (${test}/out/${name}.log)")
		math(EXPR failed "${failed} + 1")
		continue()
	endif()
	execute_process(COMMAND ${AB3TOK} -cmp ${WORK}/tokens.txt ${test}/src/${file} ${theirs}
		RESULT_VARIABLE r ERROR_VARIABLE msg)
	string(STRIP "${msg}" msg)
	if(r)
		message(STATUS "FAIL ${name}: ${msg}")
		math(EXPR failed "${failed} + 1")
	else()
		message(STATUS "ok   ${name}")
	endif()
endforeach()
if(failed)
	message(FATAL_ERROR "tokenise.cmake: ${failed} of ${n} source(s) differ from AmiBlitz3's")
endif()
message(STATUS "tokenise.cmake: ${n} sources, the same tokens as AmiBlitz3's")
