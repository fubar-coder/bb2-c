# The committed declaration data of the original libraries
# (tests/data/declarations/): written from the originals (WRITE=ON, the
# target declarations-data) or checked against them (the tests
# NAME_data of the preset originals).
#   cmake -DBLITZLIB=... -DORIGINAL=FILE -DDATA=tests/data/declarations/X.txt
#         [-DWRITE=ON | -DOUT=FILE [-DNO_RUN=ON]] -P decls-data.cmake
# Check: blitzlib decls ORIGINAL into OUT, which must be DATA byte for byte;
# NO_RUN: OUT is there already (the test with broken input).  Exit status
# 0 the same (written), 1 not, 77 skipped: no ORIGINAL (the CD, AmiBlitz3's
# SVN or the RWE reference build).
cmake_minimum_required(VERSION 4.2.3)

foreach(v DATA)
	if(NOT ${v})
		message(FATAL_ERROR "decls-data.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT NO_RUN AND NOT EXISTS "${ORIGINAL}")
	if(WRITE)
		message(FATAL_ERROR "decls-data.cmake: no ${ORIGINAL} (fetch-originals; a reference "
			"build of the RWE userlibs: ctest --preset originals -R '^rwe_refbuild$')")
	endif()
	message(STATUS "no ${ORIGINAL} - skipped")
	cmake_language(EXIT 77)
endif()
if(WRITE)
	get_filename_component(dir ${DATA} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	execute_process(COMMAND ${BLITZLIB} decls ${ORIGINAL} -o ${DATA} RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "blitzlib decls ${ORIGINAL} failed (${rc})")
	endif()
	return()
endif()
if(NOT NO_RUN)
	execute_process(COMMAND ${BLITZLIB} decls ${ORIGINAL} -o ${OUT} RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "blitzlib decls ${ORIGINAL} failed (${rc})")
	endif()
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${DATA} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "the declarations of ${ORIGINAL} differ from ${DATA} (diff against "
		"${OUT}; cmake --build --preset amiga --target declarations-data writes it)")
endif()
message(STATUS "the declaration data ${DATA} are those of the original")
