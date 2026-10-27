/*
 * toklist.c - blitz2's token list for the host tests, built from
 * FIXDIR/tokens.txt (mkfixtures.py: "WORD NAME" per line, in the order of
 * bb2tok.load_tokens() = built-in tokens, deflibs, acidlibs - the order of
 * the compiler's list).  Node layout as in blitz2: {next, num, name\0,
 * help\0} (no help texts in the tables: empty).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hostenv.h"

struct tokennode *host_loadtokens(const char *fixdir)
{
    char path[1024], name[256];
    unsigned w;
    struct tokennode *first = NULL, **link = &first, *n;
    FILE *f;

    snprintf(path, sizeof(path), "%s/tokens.txt", fixdir);
    f = fopen(path, "r");
    if (!f)
        return NULL;
    while (fscanf(f, "%x %255s\n", &w, name) == 2) {
        size_t len = strlen(name);
        n = calloc(1, sizeof(*n) + len + 2);
        /* blitz2 stores the numbers without bit 15: findtoke does
           bclr #15,d0 / cmp 4(a3),d0 */
        n->num = (UWORD)(w & 0x7FFF);
        memcpy(n->name, name, len + 1);     /* + help "" */
        *link = n;
        link = &n->next;
    }
    fclose(f);
    return first;
}
