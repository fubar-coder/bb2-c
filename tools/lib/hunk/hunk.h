/*
 * hunk.h - reading AmigaDOS load files (hunk format) on the host.
 *
 * load_parse() checks the structure (bounds, known hunk types) and lists
 * the hunks with their RELOC32 relocations; symbol, debug and name hunks
 * are skipped.  Memory flags (bits 30/31) are removed from types and sizes.
 */
#ifndef HUNK_H
#define HUNK_H

#include <stddef.h>
#include <stdint.h>

#define HUNK_NAME          0x3E8
#define HUNK_CODE          0x3E9
#define HUNK_DATA          0x3EA
#define HUNK_BSS           0x3EB
#define HUNK_RELOC32       0x3EC
#define HUNK_SYMBOL        0x3F0
#define HUNK_DEBUG         0x3F1
#define HUNK_END           0x3F2
#define HUNK_HEADER        0x3F3
#define HUNK_DREL32        0x3F7      /* in load files: RELOC32SHORT (V37) */
#define HUNK_RELOC32SHORT  0x3FC
#define HUNK_RELRELOC32    0x3FD

struct hunk {
    uint32_t type;              /* HUNK_CODE, HUNK_DATA or HUNK_BSS */
    const uint8_t *data;        /* contents (NULL for BSS) */
    uint32_t size;              /* in bytes */
    uint32_t nrelocs;           /* relocations in this hunk: */
    uint32_t *reloc_offset;     /*   offset of the long in this hunk */
    uint32_t *reloc_target;     /*   hunk it points into */
};

struct loadfile {
    const uint8_t *data;        /* the whole file */
    size_t size;
    uint8_t *buf;               /* owned by load_read, else NULL */
    uint32_t table_size;        /* number of hunks in the header */
    uint32_t nhunks;            /* hunks found in the file */
    struct hunk *hunks;
};

uint16_t be16(const uint8_t *p);
uint32_t be32(const uint8_t *p);

/* 0 = ok; -1 = error, message in err (lf is empty then) */
int load_parse(const uint8_t *buf, size_t size, struct loadfile *lf, char *err, size_t errlen);
int load_read(const char *path, struct loadfile *lf, char *err, size_t errlen);
void load_free(struct loadfile *lf);

#endif
