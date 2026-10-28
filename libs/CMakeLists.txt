# The libraries blitz2 reads at startup: blitz2:acidlibs (command
# libraries) and blitz2:deflibs (AmigaOS call declarations), each one BigLib
# file joined from one library file per C source (cmake/BlitzLibrary.cmake).
add_subdirectory(acidlibs)
add_subdirectory(deflibs)

# the declarations of every library (header, entries, !libfin; code
# offsets only set or not) against the originals of the CD, same libraries
# in the same order (the compiler's token list order); skipped without the
# CD (archive/cd, cmake --build --preset amiga --target fetch-originals)
foreach(group acidlibs deflibs)
	string(TOUPPER ${group} orig)
	add_test(NAME declarations_${group} COMMAND ${BLITZLIBCOMPARE}
		${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/${orig}
		${BLITZ2_DIR}/${group})
	set_tests_properties(declarations_${group} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
