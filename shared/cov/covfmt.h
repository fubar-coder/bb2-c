/*
 * covfmt.h - the format of a coverage file (TEST:cov/<program>.cov), as
 * the coverage variants of the programs write it and tools/covmap reads it.
 *
 * All fields are 32-bit big-endian (the m68k's own order):
 *
 *   offset 0   COVFMT_MAGIC ("BCOV")
 *   offset 4   COVFMT_VERSION
 *   offset 8   the size of the code hunk in bytes (the .text size of the
 *              program's link map)
 *   offset 12  the length of the bitmap in bytes,
 *              COVFMT_BITMAP_LEN(code size)
 *   offset 16  the bitmap
 *
 * Bit i stands for the function that starts at offset 2 * i of the code
 * hunk: byte i / 8, mask 1 << (i % 8).  A set bit means the function was
 * called at least once.
 */
#ifndef COVFMT_H
#define COVFMT_H

#define COVFMT_MAGIC 0x42434F56UL    /* "BCOV" */
#define COVFMT_VERSION 1
#define COVFMT_HEADER_SIZE 16
#define COVFMT_BITMAP_LEN(code_size) (((code_size) / 2 + 7) / 8)

#endif
