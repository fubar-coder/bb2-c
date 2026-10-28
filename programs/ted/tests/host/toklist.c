/*
 * toklist.c - blitz2's token list for the host tests, built from
 * tests/data/ted-tokens.txt (the TED oracle: the list as blitz2 1.95 gave
 * it to TED, "XXXX name" per line, word with bit 15, in list order).
 * Node layout as in blitz2: {next, num, name\0, help\0} (no help texts in
 * the file: empty).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hostenv.h"

struct tokennode *host_loadtokens(const char *path)
{
    char line[300];
    struct tokennode *first = NULL, **link = &first, *n;
    FILE *f = fopen(path, "r");

    if (!f)
        return NULL;
    while (fgets(line, sizeof(line), f)) {
        char *name = line + 5, *e;
        unsigned long w = strtoul(line, &e, 16);
        size_t len;

        if (e != line + 4 || *e != ' ')
            continue;
        len = strcspn(name, "\r\n");
        n = calloc(1, sizeof(*n) + len + 2);
        /* blitz2 stores the numbers without bit 15: findtoke does
           bclr #15,d0 / cmp 4(a3),d0 */
        n->num = (UWORD)(w & 0x7FFF);
        memcpy(n->name, name, len);         /* + 0, help "" */
        *link = n;
        link = &n->next;
    }
    fclose(f);
    return first;
}
