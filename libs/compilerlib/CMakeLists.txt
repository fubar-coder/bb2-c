# compilerlib - the port's library of the routines of generated code that
# no library set has (#1178), system library 64030 ($D600-$D602).  Not a
# file of blitz2:acidlibs: a BigLib of its own (COMPILERLIB_DIR/
# compilerlib.big) that programs/blitz2 embeds and links first in its
# library list (asmlibs.c, loadlibs), in both modes.  `blitzlib numbers`
# checks that its number and tokens are free in acidlibs and deflibs.
set(COMPILERLIB_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "the BigLib compilerlib.big")
blitz_library(compilerlib compilerlib.c)
biglib_join(compilerlib_big ${CMAKE_CURRENT_BINARY_DIR}/compilerlib.big
	${CMAKE_CURRENT_BINARY_DIR}/compilerlib.obj
	CHECK ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs)
