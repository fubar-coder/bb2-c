/*
 * tedtok - a source through TED: loaded with fileopen2 and written back
 * with realsave (ted.s:680, ted.s:769), tokenise = 1, by the C core of
 * TED on the host (the same as the original TED for every source of the
 * CD: test_loadsave against the oracle).  A text source comes out
 * tokenised, exactly as TED would save it.
 *
 *   tedtok TOKENS IN OUT       TOKENS: tests/data/ted-tokens.txt
 *   tedtok --blitz2 BLITZ2 [--lib LIB]... IN OUT
 *
 * The token list: a list file (tests/data/ted-tokens.txt, or one written
 * by tools/blitztokens), or made from the compiler BLITZ2 and the library
 * or BigLib files LIB in the compiler's order (acidlibs, deflibs,
 * userlibs) as blitz2 builds it for TED (tools/lib/blitztokens).
 *
 * Exit status 1 if IN cannot be loaded, if a line of a text source is
 * longer than TED allows (TED would cut it), or if OUT is not written.
 */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hostenv.h"
#include "loadsave.h"

/* lines of a text file (the last one may lack its LF); 0 if the file has
   a 0 byte (tokenised) or is missing */
static long text_lines(const char *path)
{
    long len, i, n = 0;
    UBYTE *text = host_readfile(path, &len);

    if (text && !memchr(text, 0, (size_t)len)) {
        n = len && text[len - 1] != '\n';
        for (i = 0; i < len; i++)
            n += text[i] == '\n';
    }
    free(text);
    return n;
}

static const char usage[] =
    "usage: tedtok TOKENS IN OUT\n"
    "       tedtok --blitz2 BLITZ2 [--lib LIB]... IN OUT\n"
    "  a source through TED (load and save, tokenised) on the host; the\n"
    "  token list from the file TOKENS or made from the compiler and its\n"
    "  libraries (acidlibs, deflibs, userlibs, in this order)\n";

int main(int argc, char **argv)
{
    static jmp_buf jb;
    struct tokennode *list;
    char *in, *out;
    long lines;
    int a;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    list = host_tokens_args("tedtok", argc, argv, &a);
    if (!list && a < 0)
        return 1;
    if (!list || argc - a != 2) {
        fputs(usage, stderr);
        return 2;
    }
    in = argv[a];
    out = argv[a + 1];
    hostenv_init(NULL, 25);
    tokens = list;
    tokenise = 1;
    host_outmem = &jb;
    if (setjmp(jb)) {
        fprintf(stderr, "tedtok: %s: out of memory\n", in);
        return 1;
    }
    host_resetcalls();
    fileopen2((UBYTE *)in);
    if (host_calls[HC_ERROR] || !firstitem) {
        fprintf(stderr, "tedtok: %s: %s\n", in, host_lasterror ? host_lasterror : "not loaded");
        return 1;
    }
    /* a text line longer than TED's line buffer (chcols + 1 = 129
       characters) is cut by readline, the rest becomes a line of its own
       (ted.s:3411): not what the author wrote, so such a source is
       refused */
    lines = text_lines(in);
    if (lines && numlines != lines) {
        fprintf(stderr, "tedtok: %s: %ld lines, %u after loading: a line is longer than TED "
                        "allows (%u characters)\n", in, lines, (unsigned)numlines,
                (unsigned)chcols + 1);
        return 1;
    }
    remove(out);
    realsave((UBYTE *)out);
    if (host_calls[HC_ERROR]) {
        fprintf(stderr, "tedtok: %s: %s\n", out, host_lasterror ? host_lasterror : "not written");
        return 1;
    }
    return 0;
}
