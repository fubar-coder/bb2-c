# The step of the `prev` build (cmake/prev.cmake): builds the Amiga side of
# another commit of this repository in PREV_DIR.
#   cmake -DREPO=SOURCE -DREV=REV -DPREV_DIR=DIR -DGENERATOR=G
#         [-DCONFIGURE_ARGS="-D...|-D..."] -P prev-build.cmake
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
