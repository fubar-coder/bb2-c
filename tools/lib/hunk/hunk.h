/*
 * hunk.h - reading AmigaDOS load files (hunk format) on the host.
 *
 * load_parse() checks the structure (bounds, known hunk types) and lists
 * the hunks with their RELOC32 relocations and their symbols (HUNK_SYMBOL);
 * debug and name hunks are skipped.  Memory flags (bits 30/31) are removed from types and sizes;
 * those of the header's size table are kept per hunk (memflags).
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

#define HUNKF_CHIP         0x40000000   /* memory flags of a hunk */
#define HUNKF_FAST         0x80000000

/** @brief A symbol: the name points into the file (len bytes, not
    0-terminated). */
struct hunk_symbol {
    const char *name;
    uint32_t len;
    uint32_t value;             /* offset in the hunk */
};

/** @brief A hunk with its relocations and symbols. */
struct hunk {
    uint32_t type;              /* HUNK_CODE, HUNK_DATA or HUNK_BSS */
    const uint8_t *data;        /* contents (NULL for BSS) */
    uint32_t size;              /* in bytes */
    uint32_t nrelocs;           /* relocations in this hunk: */
    uint32_t *reloc_offset;     /*   offset of the long in this hunk */
    uint32_t *reloc_target;     /*   hunk it points into */
    uint32_t nsyms;             /* HUNK_SYMBOL entries of this hunk: */
    struct hunk_symbol *syms;
    uint32_t memflags;          /* bits 30/31 of its size in the header:
                                   HUNKF_CHIP, HUNKF_FAST */
};

/** @brief A load file: the file and its hunks (load_free). */
struct loadfile {
    const uint8_t *data;       /* the whole file */
    size_t size;
    uint8_t *buf;               /* owned by load_read, else NULL */
    uint32_t table_size;        /* number of hunks in the header */
    uint32_t nhunks;            /* hunks found in the file */
    struct hunk *hunks;
};

/** @brief The big-endian word at p. */
uint16_t be16(const uint8_t *p);
/** @brief The big-endian long at p. */
uint32_t be32(const uint8_t *p);

/**
 * @brief Parse a load file in memory (see the top).
 * @param buf     the file (lf points into it: it must stay)
 * @param size    its size
 * @param lf      out: the load file (empty on an error)
 * @param err     the message of an error
 * @param errlen  the size of err
 * @return 0 = ok, -1 = error
 */
int load_parse(const uint8_t *buf, size_t size, struct loadfile *lf, char *err, size_t errlen);

/** @brief Read a load file from disk and parse it; as load_parse() (the
    buffer is owned by lf). */
int load_read(const char *path, struct loadfile *lf, char *err, size_t errlen);

/**
 * @brief Find a symbol in any hunk (the last definition wins).
 * @param lf     the load file
 * @param name   the symbol (0-terminated)
 * @param hunk   out: its hunk index
 * @param value  out: its value
 * @return 0 = found, -1 = not there
 */
int load_symbol(const struct loadfile *lf, const char *name, uint32_t *hunk, uint32_t *value);

/** @brief Free what load_parse/load_read filled in. */
void load_free(struct loadfile *lf);

#endif
