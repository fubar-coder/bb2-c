/*
 * tokref.c - the token list of the host tool tedsrc (tok, detok) from
 * their arguments: a token list file (tests/data/ted-tokens.txt, or one
 * written by bbsrc tokens) or the files it is made of - the compiler and
 * its libraries, as blitz2 builds the list for TED (tools/lib/blitztokens).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blitztokens.h"
#include "hostenv.h"

/* the nodes of TED's list from blitztokens' list (no help texts) */
static struct tokennode *nodes(const struct blitztokens *l)
{
    struct tokennode *first = NULL, **link = &first, *n;
    size_t i, len;

    for (i = 0; i < l->n; i++) {
        len = strlen(l->t[i].name);
        n = calloc(1, sizeof(*n) + len + 2);
        if (!n)
            return NULL;
        n->num = (UWORD)(l->t[i].num & 0x7FFF);     /* as toklist.c */
        memcpy(n->name, l->t[i].name, len);
        *link = n;
        link = &n->next;
    }
    return first;
}

static int from_files(const char *prog, const char *blitz2, const char *const *libs, int nlibs,
                      struct tokennode **out)
{
    struct blitztokens l = { 0 };
    UBYTE **bufs = calloc((size_t)nlibs + 1, sizeof *bufs);
    int i, ok = 0;

    if (!bufs)
        return 0;
    for (i = 0; i <= nlibs; i++) {
        const char *path = i ? libs[i - 1] : blitz2;
        char err[300];
        long len;
        int r;

        if (!(bufs[i] = host_readfile(path, &len))) {
            fprintf(stderr, "%s: cannot read %s\n", prog, path);
            goto done;
        }
        r = i ? blitztokens_libraries(bufs[i], (size_t)len, &l, err, sizeof err)
              : blitztokens_compiler(bufs[i], (size_t)len, &l, err, sizeof err);
        if (r) {
            fprintf(stderr, "%s: %s: %s\n", prog, path, err);
            goto done;
        }
    }
    if (!(*out = nodes(&l)))
        fprintf(stderr, "%s: no tokens\n", prog);
    else
        ok = 1;
done:
    for (i = 0; i <= nlibs; i++)
        free(bufs[i]);
    free(bufs);
    blitztokens_free(&l);
    return ok;
}

struct tokennode *host_tokens(const char *prog, const char *list, const char *blitz2,
                              const char *const *libs, int nlibs)
{
    struct tokennode *t = NULL;

    if (!blitz2) {
        if (!(t = host_loadtokens(list)))
            fprintf(stderr, "%s: no token list %s\n", prog, list);
        return t;
    }
    return from_files(prog, blitz2, libs, nlibs, &t) ? t : NULL;
}
