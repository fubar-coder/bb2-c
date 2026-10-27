/*
 * biglib - join Blitz Basic 2 library files into one BigLib file
 * (blitz2:acidlibs, blitz2:deflibs).
 *
 *   biglib join OUT LIB.obj...
 *
 * BigLib format (blitz2.s loadlibgroup): for every library its length as a
 * big-endian long and the library file (an AmigaDOS load file), in the
 * given order; a long 0 ends the file.  Every input must be a load file
 * (starts with HUNK_HEADER).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUNK_HEADER 0x3F3UL

static unsigned char *readfile(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;

    if (!f || fseek(f, 0, SEEK_END) || (*size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        perror(path);
        exit(1);
    }
    buf = malloc(*size ? *size : 1);
    if (!buf || fread(buf, 1, *size, f) != (size_t)*size) {
        perror(path);
        exit(1);
    }
    fclose(f);
    return buf;
}

static void putlong(FILE *f, unsigned long v)
{
    unsigned char b[4] = { v >> 24 & 0xFF, v >> 16 & 0xFF, v >> 8 & 0xFF, v & 0xFF };

    fwrite(b, 1, 4, f);
}

static int join(const char *out, int n, char **files)
{
    FILE *f = fopen(out, "wb");
    int i;

    if (!f) {
        perror(out);
        return 1;
    }
    for (i = 0; i < n; i++) {
        long size;
        unsigned char *exe = readfile(files[i], &size);

        if (size < 4 || ((unsigned long)exe[0] << 24 | exe[1] << 16 | exe[2] << 8 | exe[3])
                        != HUNK_HEADER) {
            fprintf(stderr, "biglib: %s is not a load file\n", files[i]);
            fclose(f);
            remove(out);
            return 1;
        }
        putlong(f, (unsigned long)size);
        fwrite(exe, 1, size, f);
        free(exe);
    }
    putlong(f, 0);
    if (fclose(f)) {
        perror(out);
        remove(out);
        return 1;
    }
    printf("%s: %d libs\n", out, n);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "join"))
        return join(argv[2], argc - 3, argv + 3);
    fprintf(stderr, "usage: biglib join OUT LIB.obj...\n");
    return 2;
}
