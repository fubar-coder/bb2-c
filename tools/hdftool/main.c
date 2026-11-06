/*
 * hdftool - read and adapt AmigaDOS hardfiles (see hdf.h): the setup of
 * the emulator's Workbench hardfiles from Amiga Forever
 * (cmake/setup-amiga-forever.cmake, AMIGA-EMULATION-SETUP.md).
 *
 *   hdftool info IMAGE
 *   hdftool list IMAGE [DIR]
 *   hdftool cat IMAGE PATH
 *   hdftool check IMAGE
 *   hdftool edit IN OUT [--partition] OP...
 *     --write PATH TEXT     write a file (create or replace)
 *     --replace PATH OLD NEW  replace text that occurs exactly once
 *     --copy FROM TO        copy a file inside the image
 *
 * edit applies the operations in memory, checks the file system and only
 * then writes OUT, which must not exist (never overwritten); --partition
 * writes only the partition, a raw image without the RDB.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hdf.h"

static const char usage[] =
    "usage: hdftool info IMAGE\n"
    "       hdftool list IMAGE [DIR]\n"
    "       hdftool cat IMAGE PATH\n"
    "       hdftool check IMAGE\n"
    "       hdftool edit IN OUT [--partition] OP...\n"
    "  AmigaDOS hardfiles (RDB with one partition, or a raw partition;\n"
    "  DOS\\0 to DOS\\3).  PATH from the root, separated by '/'.  edit\n"
    "  applies the operations, checks the file system and writes OUT, which\n"
    "  must not exist; --partition writes only the partition (no RDB).\n"
    "  OP: --write PATH TEXT      write a file (create or replace)\n"
    "      --replace PATH OLD NEW replace text that occurs exactly once\n"
    "      --copy FROM TO         copy a file inside the image\n";

static uint8_t *load(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf = NULL;
    long n;

    if (!f || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        if (f)
            fclose(f);
        return NULL;
    }
    buf = malloc(n ? (size_t)n : 1);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "hdftool: %s: cannot read\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

/* OUT, created new (O_EXCL); removed again if it cannot be written whole */
static int save(const char *path, const uint8_t *data, size_t size)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    size_t done = 0;
    ssize_t w;

    if (fd < 0) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        return -1;
    }
    while (done < size) {
        w = write(fd, data + done, size - done);
        if (w <= 0) {
            fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
        done += (size_t)w;
    }
    if (close(fd)) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        unlink(path);
        return -1;
    }
    return 0;
}

static int edit(struct hdf *h, const char *out, int argc, char **argv)
{
    char err[300];
    int i = 0, partition = 0, r;

    if (i < argc && !strcmp(argv[i], "--partition")) {
        partition = 1;
        i++;
    }
    while (i < argc) {
        if (!strcmp(argv[i], "--write") && i + 2 < argc) {
            r = hdf_write(h, argv[i + 1], argv[i + 2], strlen(argv[i + 2]), err, sizeof err);
            i += 3;
        } else if (!strcmp(argv[i], "--replace") && i + 3 < argc) {
            r = hdf_replace(h, argv[i + 1], argv[i + 2], argv[i + 3], err, sizeof err);
            i += 4;
        } else if (!strcmp(argv[i], "--copy") && i + 2 < argc) {
            r = hdf_copy(h, argv[i + 1], argv[i + 2], err, sizeof err);
            i += 3;
        } else {
            fputs(usage, stderr);
            return 2;
        }
        if (r) {
            fprintf(stderr, "hdftool: %s\n", err);
            return 1;
        }
    }
    if (hdf_check(h, err, sizeof err)) {
        fprintf(stderr, "hdftool: after the changes: %s\n", err);
        return 1;
    }
    if (partition)
        r = save(out, h->data + h->offset, (size_t)h->nblocks * HDF_BLOCK);
    else
        r = save(out, h->data, h->size);
    return r ? 1 : 0;
}

int main(int argc, char **argv)
{
    struct hdf h;
    char err[300], name[HDF_NAMEMAX + 2];
    uint8_t *img, *buf;
    size_t size, len;
    int r = 1;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc < 3 || (!strcmp(argv[1], "info") && argc != 3) || (!strcmp(argv[1], "check") && argc != 3)
        || (!strcmp(argv[1], "list") && argc > 4) || (!strcmp(argv[1], "cat") && argc != 4)
        || (!strcmp(argv[1], "edit") && argc < 4)) {
        fputs(usage, stderr);
        return 2;
    }
    if (strcmp(argv[1], "info") && strcmp(argv[1], "check") && strcmp(argv[1], "list")
        && strcmp(argv[1], "cat") && strcmp(argv[1], "edit")) {
        fputs(usage, stderr);
        return 2;
    }
    if (!(img = load(argv[2], &size)))
        return 1;
    if (hdf_open(&h, img, size, err, sizeof err)) {
        fprintf(stderr, "hdftool: %s: %s\n", argv[2], err);
        free(img);
        return 1;
    }
    if (!strcmp(argv[1], "info")) {
        hdf_volume_name(&h, name);
        printf("%s%s%s DOS\\%u blocks %u reserved %u offset %zu root %u free %u volume %s\n",
               h.rdb ? "rdb" : "raw", h.rdb ? " " : "", h.partname, h.dostype & 0xFF, h.nblocks,
               h.reserved, h.offset, h.root, hdf_free_blocks(&h), name);
        r = 0;
    } else if (!strcmp(argv[1], "check")) {
        r = hdf_check(&h, err, sizeof err) != 0;
        if (r)
            fprintf(stderr, "hdftool: %s: %s\n", argv[2], err);
    } else if (!strcmp(argv[1], "list")) {
        r = hdf_list(&h, argc == 4 ? argv[3] : "", stdout, err, sizeof err) != 0;
        if (r)
            fprintf(stderr, "hdftool: %s\n", err);
    } else if (!strcmp(argv[1], "cat")) {
        if (hdf_read(&h, argv[3], &buf, &len, err, sizeof err)) {
            fprintf(stderr, "hdftool: %s\n", err);
        } else {
            r = fwrite(buf, 1, len, stdout) != len;
            free(buf);
        }
    } else {
        r = edit(&h, argv[3], argc - 4, argv + 4);
    }
    hdf_close(&h);
    free(img);
    return r;
}
