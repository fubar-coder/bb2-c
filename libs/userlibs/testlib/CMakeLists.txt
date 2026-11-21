# testlib.obj, the Blitz test library (openspec change basic-testlib,
# spec testing/basic-testlib): user library 81 (LIB_testlib) for
# blitz2:userlibs/testlib/ of the default variant, built with the default
# variant's numbers and checked in its build step (blitzlib check, no
# number of the default acidlibs and deflibs twice).  Test infrastructure:
# no part of acidlibs or deflibs.  testlib.c is the Amiga side, tlcore.c
# the logic (its host tests: tests/host, added by host/CMakeLists.txt);
# the entries are written by the codec of shared/basictest.
set(TESTLIB_OBJ ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/testlib/testlib.obj
	CACHE INTERNAL "the Blitz test library (libs/userlibs/testlib)")
blitz_library(userlib-testlib VARIANT ${BLITZ_DEFAULT_VARIANT_DIR} OBJ ${TESTLIB_OBJ}
	${CMAKE_CURRENT_SOURCE_DIR}/testlib.c ${CMAKE_CURRENT_SOURCE_DIR}/tlcore.c
	${PROJECT_SOURCE_DIR}/shared/basictest/basictest_write.c
	CHECK ${BLITZ2_DIR}/deflibs ${BLITZ2_DIR}/acidlibs)
target_include_directories(userlib-testlib PRIVATE ${PROJECT_SOURCE_DIR}/shared/basictest
	${CMAKE_CURRENT_SOURCE_DIR})
