/*
 * biglib - join Blitz Basic 2 library files into one BigLib file
 * (blitz2:acidlibs, blitz2:deflibs; format: lib/biglib/biglib.h).
 *
 *   biglib join OUT LIB.obj...
 *
 * Every input must be a well-formed AmigaDOS load file (lib/hunk).
 */
#include <stdlib.h>
#include <string.h>
#include "biglib.h"
#include "hunk.h"

static int join(const char *out, int n, char **files)
{
    struct loadfile *lf = calloc(n ? (size_t)n : 1, sizeof *lf);
    struct biglib_entry *e = calloc(n ? (size_t)n : 1, sizeof *e);
    FILE *f;
    int i, rc = 1;

    if (!lf || !e) {
        fprintf(stderr, "biglib: out of memory\n");
        goto done;
    }
    for (i = 0; i < n; i++) {
        char err[200];

        if (load_read(files[i], &lf[i], err, sizeof err)) {
            fprintf(stderr, "biglib: %s: %s\n", files[i], err);
            goto done;
        }
        e[i].data = lf[i].data;
        e[i].size = (uint32_t)lf[i].size;
    }
    f = fopen(out, "wb");
    if (!f) {
        perror(out);
        goto done;
    }
    if (biglib_write(f, e, (size_t)n)) {
        perror(out);
        fclose(f);
        remove(out);
        goto done;
    }
    if (fclose(f)) {
        perror(out);
        remove(out);
        goto done;
    }
    printf("%s: %d libs\n", out, n);
    rc = 0;
done:
    for (i = 0; lf && i < n; i++)
        load_free(&lf[i]);
    free(lf);
    free(e);
    return rc;
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "join"))
        return join(argv[2], argc - 3, argv + 3);
    fprintf(stderr, "usage: biglib join OUT LIB.obj...\n");
    return 2;
}
