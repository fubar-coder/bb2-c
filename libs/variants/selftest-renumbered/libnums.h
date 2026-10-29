/* libnums.h of the variant selftest-renumbered: fadelib 157 -> 3 and
   inputoutputlib 164 -> 4 (both free in the default build), to check that
   a number changes the header, the tokens and the calls of other
   libraries (debuglib calls inputoutputlib; ctest
   variant_selftest_renumbered) */
#ifndef VARIANT_LIBNUMS_H
#define VARIANT_LIBNUMS_H
#include "../../include/libnums.h"
#undef LIB_fadelib_header
#define LIB_fadelib_header 3
#undef LIB_inputoutputlib
#define LIB_inputoutputlib 4
#endif
