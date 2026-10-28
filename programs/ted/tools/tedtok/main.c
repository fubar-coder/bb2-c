/*
 * tedtok - a source through TED: loaded with fileopen2 and written back
 * with realsave (ted.s:680, ted.s:769), tokenise = 1, by the C core of
 * TED on the host (the same as the original TED for every source of the
 * CD: test_loadsave against the oracle).  A text source comes out
 * tokenised, exactly as TED would save it.
 *
 *   tedtok TOKENS IN OUT       TOKENS: tests/data/ted-tokens.txt
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

int main(int argc, char **argv)
{
    static jmp_buf jb;
    long lines;

    if (argc != 4) {
        fprintf(stderr, "usage: tedtok TOKENS IN OUT\n");
        return 2;
    }
    hostenv_init(argv[1], 25);
    if (!tokens) {
        fprintf(stderr, "tedtok: no token list %s\n", argv[1]);
        return 1;
    }
    host_outmem = &jb;
    if (setjmp(jb)) {
        fprintf(stderr, "tedtok: %s: out of memory\n", argv[2]);
        return 1;
    }
    host_resetcalls();
    fileopen2((UBYTE *)argv[2]);
    if (host_calls[HC_ERROR] || !firstitem) {
        fprintf(stderr, "tedtok: %s: %s\n", argv[2], host_lasterror ? host_lasterror : "not loaded");
        return 1;
    }
    /* a text line longer than TED's line buffer (chcols + 1 = 129
       characters) is cut by readline, the rest becomes a line of its own
       (ted.s:3411): not what the author wrote, so such a source is
       refused */
    lines = text_lines(argv[2]);
    if (lines && numlines != lines) {
        fprintf(stderr, "tedtok: %s: %ld lines, %u after loading: a line is longer than TED "
                        "allows (%u characters)\n", argv[2], lines, (unsigned)numlines,
                (unsigned)chcols + 1);
        return 1;
    }
    remove(argv[3]);
    realsave((UBYTE *)argv[3]);
    if (host_calls[HC_ERROR]) {
        fprintf(stderr, "tedtok: %s: %s\n", argv[3], host_lasterror ? host_lasterror : "not written");
        return 1;
    }
    return 0;
}
