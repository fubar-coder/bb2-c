/* libnums.h of the test variant selftest-renumbered: bb2-2.10 with
   fadelib 157 -> 3 and inputoutputlib 164 -> 4 (both free there), to
   check that a number changes the header, the tokens and the calls of
   other libraries (debuglib calls inputoutputlib; ctest
   variant_selftest_renumbered) */
#ifndef VARIANT_LIBNUMS_H
#define VARIANT_LIBNUMS_H
#include "../../../variants/bb2-2.10/libnums.h"
#undef LIB_fadelib_header
#define LIB_fadelib_header 3
#undef LIB_inputoutputlib
#define LIB_inputoutputlib 4
#endif
