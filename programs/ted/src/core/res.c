/*
 * res.c - the binary includes of ted.s (incbin) as C arrays, PLAN.md 3.4.
 *
 * #embed reads the original files at build time (--embed-dir = the
 * directory of ted.s, see CMakeLists.txt); no copies in the repository.
 * keymap.h declares the sizes: a file of another size gives a conflicting
 * type, the static_asserts say which one.
 */
#include "keymap.h"

const UBYTE sleep_[] __attribute__((aligned(4))) = {    /* ted.s:316 */
#embed "sleep.bin"
};
static_assert(sizeof sleep_ == 704, "sleep.bin: 704 bytes expected");

const UBYTE pointer[] __attribute__((aligned(4))) = {   /* ted.s:319 */
#embed "pointer.bin"
};
static_assert(sizeof pointer == 64, "pointer.bin: 64 bytes expected");

const UBYTE srcinfo[] __attribute__((aligned(4))) = {   /* ted.s:827 */
#embed "src.info"
};
static_assert(sizeof srcinfo == 396, "src.info: 396 bytes expected");

const UBYTE font6[] __attribute__((aligned(4))) = {     /* ted.s:8070 */
#embed "chars.b"
};
static_assert(sizeof font6 == 768, "chars.b: 768 bytes expected");

const UBYTE font8[] __attribute__((aligned(4))) = {     /* ted.s:8071 */
#embed "chars8.b"
};
static_assert(sizeof font8 == 1024, "chars8.b: 1024 bytes expected");
