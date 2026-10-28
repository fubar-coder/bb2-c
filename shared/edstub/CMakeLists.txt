# edstub - frame of a headless editor that blitz2 loads as blitz2:ted
# (edstub.h).  An object library: start.c is its first object and must stay
# the first of the program's link (blitz2 calls hunk offset 4), so a program
# lists $<TARGET_OBJECTS:edstub> before its own sources (not
# target_link_libraries: that would add the objects a second time, behind
# the program's) and compiles and links with EDSTUB_COMPILE_OPTIONS /
# EDSTUB_LINK_OPTIONS.  No C runtime: -nostdlib, no libgcc.
set(EDSTUB_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(EDSTUB_COMPILE_OPTIONS -fomit-frame-pointer -fno-builtin -ffreestanding
	-Wall -Wno-pointer-sign)
set(EDSTUB_COMPILE_OPTIONS ${EDSTUB_COMPILE_OPTIONS} PARENT_SCOPE)
set(EDSTUB_LINK_OPTIONS -nostdlib -nostartfiles PARENT_SCOPE)
add_library(edstub OBJECT start.c edstub.c)
target_compile_options(edstub PRIVATE ${EDSTUB_COMPILE_OPTIONS})
