# The step of the `prev` build (cmake/prev.cmake): builds the Amiga side of
# another commit of this repository in PREV_DIR.
#   cmake -DREPO=SOURCE -DREV=REV -DPREV_DIR=DIR -DGENERATOR=G
#         [-DCONFIGURE_ARGS="-D...|-D..."] [-DSNITCH_SOURCE_DIR=SNITCH -DSNITCH_PIN=SHA256]
#         [-DCLI11_SOURCE_DIR=CLI11 -DCLI11_PIN=SHA256]
#         -P prev-build.cmake
# DIR/src is `git archive REV` (the checkout is not touched); the data of
# the checkout that git does not hold (archive/cd, ...) are linked into it.
# DIR/commit names the commit DIR/src holds; when REV points to another
# one (HEAD~1 moves with every commit), DIR is emptied and made again - the
# files of an archive carry the commit's time, so a build directory of
# another commit would take them for unchanged.  The results are in
# DIR/blitz2/ (acidlibs, deflibs, blitz2, ted, ...) as in a build
# directory of the commit.

foreach(v REPO REV PREV_DIR GENERATOR)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "prev-build.cmake: -D${v}= is needed")
	endif()
endforeach()

execute_process(COMMAND git -C ${REPO} rev-parse --verify --quiet ${REV}^{commit}
	OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc)
if(rc OR NOT commit)
	message(FATAL_ERROR "BLITZ2_PREV: ${REV} is no commit of ${REPO}")
endif()

set(have "")
if(EXISTS ${PREV_DIR}/commit AND IS_DIRECTORY ${PREV_DIR}/src)
	file(READ ${PREV_DIR}/commit have)
	string(STRIP "${have}" have)
endif()
if(NOT have STREQUAL commit)
	message(STATUS "prev: ${REV} = ${commit}")
	file(REMOVE_RECURSE ${PREV_DIR})
	file(MAKE_DIRECTORY ${PREV_DIR}/src)
	execute_process(COMMAND git -C ${REPO} archive --format=tar ${commit}
		COMMAND tar -x -C ${PREV_DIR}/src
		RESULTS_VARIABLE rcs ERROR_VARIABLE err)
	list(GET rcs 0 rc1)
	list(GET rcs 1 rc2)
	if(rc1 OR rc2)
		file(REMOVE_RECURSE ${PREV_DIR})
		message(FATAL_ERROR "prev: git archive ${commit} failed: ${err}")
	endif()
	# the data git does not hold: the checkout's own
	foreach(d archive/cd archive/aminet archive/amiblitz3 archive/bernd .cache)
		if(EXISTS ${REPO}/${d})
			get_filename_component(parent ${PREV_DIR}/src/${d} DIRECTORY)
			file(MAKE_DIRECTORY ${parent})
			file(CREATE_LINK ${REPO}/${d} ${PREV_DIR}/src/${d} SYMBOLIC)
		endif()
	endforeach()
	file(WRITE ${PREV_DIR}/commit "${commit}\n")
endif()

string(REPLACE "|" ";" CONFIGURE_ARGS "${CONFIGURE_ARGS}")

# snitch without the network: the current build's source (SNITCH_SOURCE_DIR,
# pinned to SNITCH_PIN) if the commit pins the same SHA256 in its
# cmake/Snitch.cmake.  Its top-level configure gets it as
# FETCHCONTENT_SOURCE_DIR_SNITCH (and passes it on to its host
# ExternalProject); for the host ExternalProject of a commit that does not
# pass it on yet, DIR/ab3c/build/amiga-release/_deps/snitch-src links to it:
# Snitch.cmake takes a snitch that ab3c downloaded from there (relative to
# DIR/src).  With another pin, the commit downloads its own.
set(snitch_link ${PREV_DIR}/ab3c/build/amiga-release/_deps/snitch-src)
set(prev_pin "")
if(EXISTS ${PREV_DIR}/src/cmake/Snitch.cmake)
	file(STRINGS ${PREV_DIR}/src/cmake/Snitch.cmake prev_pin REGEX "URL_HASH +SHA256=")
	string(REGEX REPLACE ".*SHA256=([0-9a-fA-F]+).*" "\\1" prev_pin "${prev_pin}")
endif()
if(SNITCH_PIN AND prev_pin STREQUAL SNITCH_PIN AND EXISTS ${SNITCH_SOURCE_DIR}/CMakeLists.txt)
	set(target "")
	if(IS_SYMLINK ${snitch_link})
		file(READ_SYMLINK ${snitch_link} target)
	endif()
	if(NOT target STREQUAL SNITCH_SOURCE_DIR)
		file(REMOVE ${snitch_link})
		get_filename_component(parent ${snitch_link} DIRECTORY)
		file(MAKE_DIRECTORY ${parent})
		file(CREATE_LINK ${SNITCH_SOURCE_DIR} ${snitch_link} SYMBOLIC)
	endif()
	list(APPEND CONFIGURE_ARGS -DFETCHCONTENT_SOURCE_DIR_SNITCH=${SNITCH_SOURCE_DIR})
else()
	message(STATUS "prev: ${commit} pins snitch SHA256=${prev_pin}, this build "
		"SHA256=${SNITCH_PIN} (source ${SNITCH_SOURCE_DIR}): the prev build downloads its own")
endif()

# CLI11 the same way: the current build's source (CLI11_SOURCE_DIR, pinned
# to CLI11_PIN) if the commit pins the same SHA256 in its cmake/CLI11.cmake;
# its top-level configure passes FETCHCONTENT_SOURCE_DIR_CLI11 on to its
# host ExternalProject.  A commit without cmake/CLI11.cmake needs none.
if(EXISTS ${PREV_DIR}/src/cmake/CLI11.cmake)
	file(STRINGS ${PREV_DIR}/src/cmake/CLI11.cmake prev_pin REGEX "URL_HASH +SHA256=")
	string(REGEX REPLACE ".*SHA256=([0-9a-fA-F]+).*" "\\1" prev_pin "${prev_pin}")
	if(CLI11_PIN AND prev_pin STREQUAL CLI11_PIN AND IS_DIRECTORY ${CLI11_SOURCE_DIR}/include/CLI)
		list(APPEND CONFIGURE_ARGS -DFETCHCONTENT_SOURCE_DIR_CLI11=${CLI11_SOURCE_DIR})
	else()
		message(STATUS "prev: ${commit} pins CLI11 SHA256=${prev_pin}, this build "
			"SHA256=${CLI11_PIN} (source ${CLI11_SOURCE_DIR}): the prev build downloads its own")
	endif()
endif()

if(NOT EXISTS ${PREV_DIR}/build.ninja AND NOT EXISTS ${PREV_DIR}/Makefile)
	execute_process(COMMAND ${CMAKE_COMMAND} -S ${PREV_DIR}/src -B ${PREV_DIR} -G ${GENERATOR}
		-DCMAKE_TOOLCHAIN_FILE=${PREV_DIR}/src/cmake/m68k-amigaos.cmake
		-DBLITZ2_CLANGD=OFF ${CONFIGURE_ARGS}
		RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "prev: configuring ${commit} failed")
	endif()
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build ${PREV_DIR} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "prev: building ${commit} failed")
endif()
