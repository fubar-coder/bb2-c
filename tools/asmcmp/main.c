/*
 * asmcmp - assembler routines of a C library against the original,
 * instruction by instruction (see asmcmp.h).
 *
 *   asmcmp OBJDUMP ORIGINAL LIBNUM NAME.linked NAME:ORIGADDR:CSYMBOL:COUNT...
 *
 * ORIGINAL: the BigLib file with the original library (e.g. the CD's
 * BLITZ2/ACIDLIBS), LIBNUM its number; NAME.linked: the C library linked
 * with symbols.  Exit status 1 if a routine differs, 77 (skipped) if
 * ORIGINAL does not exist (the CD is not in the repository:
 * fetch-originals).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "asmcmp.h"
#include "biglib.h"

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    struct biglib_entry *e;
    struct loadfile orig, c;
    unsigned long libnum;
    char err[200];
    uint8_t *big;
    size_t size, n, i;
    int bad = 0, found = 0;

    if (argc < 6) {
        fprintf(stderr, "usage: asmcmp OBJDUMP ORIGINAL LIBNUM NAME.linked "
                        "NAME:ORIGADDR:CSYMBOL:COUNT...\n");
        return 2;
    }
    if (access(argv[2], F_OK)) {
        printf("asmcmp: no %s - skipped (fetch-originals)\n", argv[2]);
        return 77;
    }
    libnum = strtoul(argv[3], NULL, 0) & 0xFFFF;
    if (!(big = read_all(argv[2], &size)) || biglib_read(big, size, &e, &n, err, sizeof err)) {
        fprintf(stderr, "asmcmp: %s: %s\n", argv[2], big ? err : "cannot read");
        return 1;
    }
    for (i = 0; i < n && !found; i++) {
        if (load_parse(e[i].data, e[i].size, &orig, err, sizeof err)) {
            fprintf(stderr, "asmcmp: %s: library %zu: %s\n", argv[2], i + 1, err);
            return 1;
        }
        found = orig.nhunks && orig.hunks[0].size >= 6 && be16(orig.hunks[0].data + 4) == libnum;
        if (!found)
            load_free(&orig);
    }
    if (!found) {
        fprintf(stderr, "asmcmp: %s: no library %lu\n", argv[2], libnum);
        return 1;
    }
    if (load_read(argv[4], &c, err, sizeof err)) {
        fprintf(stderr, "asmcmp: %s: %s\n", argv[4], err);
        return 1;
    }
    for (i = 5; i < (size_t)argc; i++)
        if (asmcmp_routine(argv[1], &orig.hunks[0], &c, argv[i], stdout))
            bad = 1;
    load_free(&c);
    load_free(&orig);
    free(e);
    free(big);
    return bad;
}
