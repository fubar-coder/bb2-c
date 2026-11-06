/*
 * hdf.h - AmigaDOS hardfiles on the host: an image with a Rigid Disk Block
 * (one partition) or a raw partition image, and its OFS/FFS file system
 * (DOS\0 to DOS\3, 512-byte blocks): reading files, writing a file, the
 * consistency of blocks and bitmap.  Used by hdftool to adapt the
 * Workbench hardfiles of Amiga Forever for the emulator tests
 * (AMIGA-EMULATION-SETUP.md, cmake/setup-amiga-forever.cmake).
 *
 * The image is held in memory and changed there; nothing is written to a
 * file here.  Writing keeps the image reproducible: no host clock, a new
 * file takes the date of its directory (a copy that of its source), blocks
 * are taken from the bitmap in a fixed order, the directories' dates stay.
 * The directory caches of DOS\4/DOS\5 are not supported (refused).
 */
#ifndef HDF_H
#define HDF_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HDF_BLOCK 512u                  /**< bytes of a block */
#define HDF_HT 72u                      /**< entries of a hash table (512-byte blocks) */
#define HDF_NAMEMAX 30u                 /**< longest file name */

/** @brief An image in memory and the partition found in it. */
struct hdf {
    uint8_t *data;              /**< the whole image (not owned) */
    size_t size;                /**< its bytes */
    int rdb;                    /**< 1: Rigid Disk Block, 0: raw partition */
    char partname[32];          /**< the partition's name (RDB), else "" */
    size_t offset;              /**< byte offset of the partition */
    uint32_t nblocks;           /**< blocks of the partition */
    uint32_t reserved;          /**< reserved blocks at its start */
    uint32_t dostype;           /**< DOS\0 .. DOS\3 */
    uint32_t root;              /**< the root block (partition-relative) */
    uint32_t *bm;               /**< the bitmap blocks, in order */
    size_t nbm;                 /**< their number */
};

/**
 * @brief Find the partition and its file system in an image.
 *
 * An RDB (RDSK in one of the first 16 blocks) must have valid checksums,
 * 512-byte blocks and exactly one partition; without RDB the image is one
 * partition with 2 reserved blocks.  The file system must be DOS\0 to
 * DOS\3 with a valid root block and a valid bitmap.
 * @param h      out: the image (borrows data; hdf_close frees the rest)
 * @param data   the image's bytes (changed in place by the writing calls)
 * @param size   their number
 * @param err    out: the reason if it is refused
 * @param errlen size of err
 * @return 0, or -1 with err set
 */
int hdf_open(struct hdf *h, uint8_t *data, size_t size, char *err, size_t errlen);

/** @brief Free what hdf_open allocated (not the image). */
void hdf_close(struct hdf *h);

/**
 * @brief Read a file.
 * @param path  its path from the root, components separated by '/'
 *              (case-insensitive as AmigaDOS compares names)
 * @param out   out: its contents (malloc'd, a 0 after them; free())
 * @param len   out: their bytes
 * @return 0, or -1 with err set (not found, a directory, a damaged block)
 */
int hdf_read(struct hdf *h, const char *path, uint8_t **out, size_t *len, char *err, size_t errlen);

/**
 * @brief Write a file: replace the contents of an existing one (its date,
 *        protection and comment stay) or create it in an existing
 *        directory (with the directory's date).
 * @return 0, or -1 with err set (the image may then be half changed)
 */
int hdf_write(struct hdf *h, const char *path, const void *data, size_t len, char *err,
              size_t errlen);

/**
 * @brief Write a file whose contents are given as hexadecimal digits (two
 *        per byte, upper or lower case, no separators): binary files
 *        that cannot be an argument text.
 * @return 0, or -1 with err set (odd number of digits, a character that
 *         is no hexadecimal digit, or as hdf_write)
 */
int hdf_write_hex(struct hdf *h, const char *path, const char *hex, char *err, size_t errlen);

/**
 * @brief Copy a file inside the image (the copy gets the source's date
 *        and protection bits); an existing target is replaced.
 */
int hdf_copy(struct hdf *h, const char *from, const char *to, char *err, size_t errlen);

/**
 * @brief Replace text in a file: old must occur exactly once.
 */
int hdf_replace(struct hdf *h, const char *path, const char *old, const char *repl, char *err,
                size_t errlen);

/**
 * @brief Check the file system: checksums and types of all blocks reachable
 *        from the root, no block used twice, every used block allocated in
 *        the bitmap and every allocated block used.
 * @return 0, or -1 with err set (the first problem)
 */
int hdf_check(struct hdf *h, char *err, size_t errlen);

/** @brief The free blocks of the bitmap. */
uint32_t hdf_free_blocks(const struct hdf *h);

/** @brief The volume name (from the root block, 0-terminated). */
void hdf_volume_name(const struct hdf *h, char name[HDF_NAMEMAX + 2]);

/**
 * @brief List a directory: one line per entry, "NAME/" for directories,
 *        "NAME SIZE" for files, in hash table order.
 */
int hdf_list(struct hdf *h, const char *path, FILE *out, char *err, size_t errlen);

/**
 * @brief Format a raw partition image (2 reserved blocks, boot block with
 *        the DOS type, root in the middle, bitmap after the root): for
 *        tests.
 * @param data     the image (size bytes, a multiple of 512, at least 8 blocks)
 * @param dostype  DOS\0 .. DOS\3
 * @param name     the volume name
 * @param days     the date of the root (days since 1978-01-01)
 */
int hdf_format(uint8_t *data, size_t size, uint32_t dostype, const char *name, uint32_t days,
               char *err, size_t errlen);

#ifdef __cplusplus
}
#endif

#endif
