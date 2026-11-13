# literallib.obj for blitz2:userlibs/ of the original Blitz 2 (#1018,
# spec libs/literal-library): the library of libs/acidlibs/literallib.c
# (library 33, the names of the literal characters $9081-$90FF) as a
# library file of its own, built with the default variant's numbers, for a
# Blitz 2 whose acidlibs do not have it (the CD's).  It works only if no
# other library numbered 33 is loaded before it (blitz2 and TED take the
# first library of a number; RIDosLib of the CD has 33):
# docs/libs/variants.md.  In the build step: blitzlib check and no number
# of the default deflibs twice.
set(obj ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/literal/literallib.obj)
blitz_library(userlib-literallib VARIANT ${BLITZ_DEFAULT_VARIANT_DIR} OBJ ${obj}
	${PROJECT_SOURCE_DIR}/libs/acidlibs/literallib.c CHECK ${BLITZ2_DIR}/deflibs)
# the same library as in acidlibs, byte for byte
add_test(NAME userlib_literallib_same COMMAND ${CMAKE_COMMAND} -E compare_files ${obj}
	${CMAKE_BINARY_DIR}/libs/acidlibs/literallib.obj)
