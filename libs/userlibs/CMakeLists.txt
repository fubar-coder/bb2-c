# Userlibs: libraries for blitz2:userlibs/DIR/ of a variant, one C file
# each (DIR/NAME.c -> BLITZ_VARIANTS_DIR/VARIANT/userlibs/DIR/NAME.obj),
# with the variant's library numbers.  Every library file is checked in
# its build step (blitzlib check, and no number of acidlibs/deflibs of the
# variant twice); a ctest checks the whole set.
add_subdirectory(rwe)
