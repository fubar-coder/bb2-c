# Generating the oracle data of the TED tests: tedoracle (oracle.c) as
# blitz2:ted under the original blitz2 1.95 in Amiberry, on every .bb2/.bb
# of the CD (archive/cd, cmake/fetch-originals.cmake).
#   cmake -DAMIGA_RUN=... -DRESULTS=results -DBLITZ2=ref/blitz2 -DTED=ref/ted
#         -DORACLE=tedoracle
#         -DCD=archive/cd/ultimate-blitz-basic-2.1 -DWORK=DIR -DDATA=tests/data
#         [-DLIMIT=N] [-DTIMEOUT=SEC] -P ted-oracle.cmake
#
#   WORK/vol/           the volume ORACLE: (blitz2: directory, the job
#                       file, the outputs in out/, oracle.log, blitz2.out)
#   WORK/emu/           Amiberry's work directory (amiga_run --dir)
#   DATA/ted-tokens.txt     the token list (out/tokens.txt)
#   DATA/ted-oracle.sha256  SHA-256 of every output, by source:
#                           "SHA  detok/PATH", "tokall/PATH", "save/PATH"
#                           (PATH relative to the CD, sorted); the note
#                           rows of the file stay (results ted-oracle,
#                           tools/lib/oracletable)
# LIMIT: only the first N sources, DATA is not written (trial run).
# The libraries are the CD's (BLITZ2/DEFLIBS, ACIDLIBS, DEFAULTDBUG); the
# CD has no user libraries.
cmake_minimum_required(VERSION 4.2.3)

foreach(v AMIGA_RUN RESULTS BLITZ2 TED ORACLE CD WORK DATA)
	if(NOT ${v})
		message(FATAL_ERROR "ted-oracle.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT IS_DIRECTORY ${CD}/BLITZ2)
	message(FATAL_ERROR "${CD}: no CD - cmake --build --preset amiga --target fetch-originals")
endif()
if(NOT TIMEOUT)
	set(TIMEOUT 1800)
endif()

# the sources: every .bb2/.bb (any case), sorted by path
file(GLOB_RECURSE sources RELATIVE ${CD} ${CD}/*)
list(FILTER sources INCLUDE REGEX "\\.[Bb][Bb]2?$")
list(SORT sources)
if(LIMIT)
	list(SUBLIST sources 0 ${LIMIT} sources)
endif()
list(LENGTH sources nsources)

set(vol ${WORK}/vol)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${vol}/blitz2/userlibs ${vol}/out/detok ${vol}/out/tokall
	${vol}/out/save ${WORK}/emu)
file(COPY_FILE ${BLITZ2} ${vol}/blitz2/blitz2)
file(COPY_FILE ${ORACLE} ${vol}/blitz2/ted)
foreach(f DEFLIBS ACIDLIBS DEFAULTDBUG)
	string(TOLOWER ${f} lower)
	file(COPY_FILE ${CD}/BLITZ2/${f} ${vol}/blitz2/${lower})
endforeach()
file(COPY_FILE ${TED} ${vol}/ted.ref)

set(jobs "ted=ORACLE:ted.ref\nout=ORACLE:out/\nlog=ORACLE:oracle.log\n")
foreach(s IN LISTS sources)
	string(APPEND jobs "src=UBB21:${s}\n")
endforeach()
file(WRITE ${vol}/jobs "${jobs}")
file(WRITE ${WORK}/run.script "Assign blitz2: ORACLE:blitz2
Stack 200000
blitz2:blitz2 ORACLE:jobs >ORACLE:blitz2.out
Echo \"rc=$RC\" >>ORACLE:blitz2.out
Assign blitz2:
")

message(STATUS "ted-oracle: ${nsources} sources in Amiberry (time limit ${TIMEOUT} s)")
execute_process(
	COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK}/emu
		--mount ORACLE=${vol} --mount-ro UBB21=${CD} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	message(STATUS "ted-oracle: no Amiberry or emulation data - skipped")
	cmake_language(EXIT 77)
elseif(rc)
	message(FATAL_ERROR "ted-oracle: Amiberry run failed (${WORK}/emu, ${vol}/oracle.log)")
endif()

# out/index.txt: NNNN kind result errors - one line per source, all ok
set(index ${vol}/out/index.txt)
if(NOT EXISTS ${index})
	message(FATAL_ERROR "ted-oracle: no ${index} (${vol}/oracle.log)")
endif()
file(STRINGS ${index} rows)
list(LENGTH rows nrows)
if(NOT nrows EQUAL nsources)
	message(FATAL_ERROR "ted-oracle: ${nrows} of ${nsources} sources done (${vol}/oracle.log)")
endif()
set(manifest "")
set(ntok 0)
set(bad "")
foreach(row IN LISTS rows)
	string(REPLACE "\t" ";" row "${row}")
	list(GET row 0 nnnn)
	list(GET row 1 kind)
	list(GET row 2 result)
	list(GET row 3 errors)
	math(EXPR i "${nnnn}")
	list(GET sources ${i} rel)
	if(NOT result STREQUAL "ok" OR NOT errors EQUAL 0)
		list(APPEND bad "${rel}: ${result}, ${errors} TED errors")
	endif()
	set(outs save/${nnnn}.bb2)
	if(kind STREQUAL "tok")
		math(EXPR ntok "${ntok} + 1")
		list(PREPEND outs detok/${nnnn}.txt tokall/${nnnn}.bb2)
	endif()
	foreach(o IN LISTS outs)
		string(REGEX MATCH "^[a-z]+" what ${o})
		if(NOT EXISTS ${vol}/out/${o})
			list(APPEND bad "${rel}: no ${o}")
			continue()
		endif()
		file(SHA256 ${vol}/out/${o} sha)
		list(APPEND manifest "${what}/${rel}|${sha}")
	endforeach()
endforeach()
if(bad)
	list(JOIN bad "\n  " bad)
	message(FATAL_ERROR "ted-oracle: not every source went through:\n  ${bad}")
endif()
message(STATUS "ted-oracle: ${nsources} sources (${ntok} tokenised), all ok")

if(LIMIT)
	message(STATUS "ted-oracle: trial run (LIMIT), ${DATA} not written; outputs in ${vol}/out")
	return()
endif()
list(SORT manifest)
list(TRANSFORM manifest REPLACE "^([^|]+)\\|(.*)$" "\\2  \\1")
list(JOIN manifest "\n" manifest)
# the note rows of the committed file stay (results ted-oracle: the port's
# hash after a fix, "#NNN: ... (TED 1.1: HASH)"; the note must name the
# hash of this run)
file(WRITE ${WORK}/ted-oracle.sha256 "${manifest}\n")
if(EXISTS ${DATA}/ted-oracle.sha256)
	execute_process(
		COMMAND ${RESULTS} ted-oracle ${WORK}/ted-oracle.sha256 ${DATA}/ted-oracle.sha256
			${DATA}/ted-oracle.sha256
		RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "ted-oracle: ${DATA}/ted-oracle.sha256 not written (results ted-oracle)")
	endif()
else()
	file(COPY_FILE ${WORK}/ted-oracle.sha256 ${DATA}/ted-oracle.sha256)
endif()
file(COPY_FILE ${vol}/out/tokens.txt ${DATA}/ted-tokens.txt)
message(STATUS "ted-oracle: ${DATA}/ted-oracle.sha256, ted-tokens.txt written")
