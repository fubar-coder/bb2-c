# Build target bblibsrc (included by the top-level CMakeLists.txt): the
# Blitz sources of the libraries from bblibsrc.lha as TED shows them,
#   build/<preset>/bblibsrc/NAME     line N = line N in TED
# for archive/aminet/amibl2src/bblibsrc/NAME (see bblibsrc_files.cmake).
# Part of ALL when the archive is there (cmake -P
# cmake/fetch-originals.cmake; the glob notices it at the next build),
# else the target only says so.  tedsrc detok fails on a token TED does not
# know: the file is then not written and the step fails.
include(${CMAKE_CURRENT_LIST_DIR}/bblibsrc_files.cmake)
set(BBLIBSRC_DIR ${CMAKE_BINARY_DIR}/bblibsrc)
bblibsrc_files(${PROJECT_SOURCE_DIR} bblibsrc_in bblibsrc_names CONFIGURE_DEPENDS)
if(bblibsrc_names)
	set(tokens ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
	set(outs)
	file(MAKE_DIRECTORY ${BBLIBSRC_DIR})
	foreach(name IN LISTS bblibsrc_names)
		add_custom_command(OUTPUT ${BBLIBSRC_DIR}/${name}
			COMMAND ${TEDSRC} detok ${tokens} ${bblibsrc_in}/${name} ${BBLIBSRC_DIR}/${name}
			DEPENDS ${bblibsrc_in}/${name} ${tokens} ${TEDSRC} host_tools
			COMMENT "Detokenising bblibsrc/${name}"
			VERBATIM)
		list(APPEND outs ${BBLIBSRC_DIR}/${name})
	endforeach()
	add_custom_target(bblibsrc ALL DEPENDS ${outs})
else()
	add_custom_target(bblibsrc
		COMMAND ${CMAKE_COMMAND} -E echo "bblibsrc: no ${bblibsrc_in} (cmake -P cmake/fetch-originals.cmake): skipped"
		VERBATIM)
endif()
