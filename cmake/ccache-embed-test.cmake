# ccache-embed-test.cmake - a compiler launcher (ccache, cmake/ccache.cmake)
# and #embed: compiles a source that embeds a file, with the launcher and
# a cache of its own (CCACHE_DIR in WORK):
#   1. the source: stored in the cache
#   2. the same again: must be a cache hit (else the test proves nothing)
#   3. the embedded file changed (same size): must give the object the
#      compiler gives without the launcher, not the cached one of 1
#
#   cmake -DLAUNCHER=PROGRAM|KEY=VALUE|... -DCC=COMPILER [-DFLAGS=F1|F2...]
#         [-DBROKEN=ON] -DWORK=DIR -P ccache-embed-test.cmake
# The lists are separated by | (a ctest argument cannot hold a ;).  An
# empty LAUNCHER (BB2C_CCACHE=OFF) is skipped (77).  BROKEN=ON (the test
# of the check): step 3 writes the embedded file unchanged, which must
# fail as a stale cache hit.
foreach(v CC WORK)
	if(NOT ${v})
		message(FATAL_ERROR "ccache-embed-test.cmake: ${v} missing")
	endif()
endforeach()
if(NOT LAUNCHER)
	message("ccache-embed-test: no compiler launcher (BB2C_CCACHE=OFF): skipped")
	cmake_language(EXIT 77)
endif()
string(REPLACE "|" ";" launcher "${LAUNCHER}")
string(REPLACE "|" ";" flags "${FLAGS}")
list(GET launcher 0 ccache)

file(MAKE_DIRECTORY ${WORK}/cache)
set(ENV{CCACHE_DIR} ${WORK}/cache)
file(WRITE ${WORK}/embed.c [=[
static const unsigned char data[] = {
#embed "data.bin"
};
static_assert(sizeof data == 16, "data.bin: 16 bytes");
const unsigned char *embedded(void) { return data; }
]=])

# compile(OUT): WORK/embed.o with the launcher (always the same -o: the
# Amiga object format names its output file), copied to WORK/OUT; NOLAUNCHER:
# without it
function(compile out)
	cmake_parse_arguments(PARSE_ARGV 1 arg "NOLAUNCHER" "" "")
	set(cmd ${launcher} ${CC})
	if(arg_NOLAUNCHER)
		set(cmd ${CC})
	endif()
	file(REMOVE ${WORK}/embed.o)
	execute_process(COMMAND ${cmd} ${flags} -c ${WORK}/embed.c -o embed.o
		WORKING_DIRECTORY ${WORK} RESULT_VARIABLE rc OUTPUT_VARIABLE log ERROR_VARIABLE log)
	if(rc OR NOT EXISTS ${WORK}/embed.o)
		message(FATAL_ERROR "ccache-embed-test: ${cmd} ${flags} -c embed.c failed:\n${log}")
	endif()
	file(RENAME ${WORK}/embed.o ${WORK}/${out})
endfunction()

# hits(VAR): the cache hits so far (ccache --print-stats: direct and
# preprocessed)
function(hits var)
	execute_process(COMMAND ${ccache} --print-stats OUTPUT_VARIABLE stats RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "ccache-embed-test: ${ccache} --print-stats failed")
	endif()
	set(n 0)
	foreach(key direct_cache_hit preprocessed_cache_hit)
		if(stats MATCHES "(^|\n)${key}\t([0-9]+)")
			math(EXPR n "${n} + ${CMAKE_MATCH_2}")
		endif()
	endforeach()
	set(${var} ${n} PARENT_SCOPE)
endfunction()

file(WRITE ${WORK}/data.bin "AAAAAAAAAAAAAAAA")
compile(first.o)
hits(h1)
compile(again.o)
hits(h2)
if(NOT h2 GREATER h1)
	message(FATAL_ERROR "ccache-embed-test: the same compilation again was no cache hit "
		"(${h1} -> ${h2} hits): the launcher \"${LAUNCHER}\" does not cache, the test proves nothing")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${WORK}/first.o ${WORK}/again.o
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "ccache-embed-test: the cache hit differs from the compiled object")
endif()

if(BROKEN)
	file(WRITE ${WORK}/data.bin "AAAAAAAAAAAAAAAA")
else()
	file(WRITE ${WORK}/data.bin "BBBBBBBBBBBBBBBB")
endif()
compile(changed.o)
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${WORK}/first.o ${WORK}/changed.o
	RESULT_VARIABLE rc)
if(NOT rc)
	message(FATAL_ERROR "ccache-embed-test: stale cache hit: the object did not change with "
		"the embedded file (launcher \"${LAUNCHER}\")")
endif()
compile(direct.o NOLAUNCHER)
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${WORK}/direct.o ${WORK}/changed.o
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "ccache-embed-test: the object with the launcher differs from the one "
		"without it")
endif()
message(STATUS "ccache-embed-test: a changed embedded file gives a new object (${CC})")
