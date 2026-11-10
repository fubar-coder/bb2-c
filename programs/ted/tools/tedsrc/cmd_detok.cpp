/*
 * tedsrc detok - a Blitz 2 source as TED shows it: loaded with fileopen2
 * (ted.s:680, tokenise = 1) by the C core of TED on the host, every line
 * detokenised with detok (ted.s:3455) and written with an LF.  Line N of
 * the output is line N in TED (the same as the original TED for every
 * source of the CD: test_loadsave and test_detok against the oracle).
 *
 *   tedsrc detok TOKENS IN OUT          TOKENS: tests/data/ted-tokens.txt
 *   tedsrc detok --check TOKENS IN...   only the checks, no output
 *   tedsrc detok [--check] --blitz2 BLITZ2 [--lib LIB]... IN OUT | IN...
 *
 * The token list: a list file (tests/data/ted-tokens.txt, or one written
 * by bbsrc tokens), or made from the compiler BLITZ2 and the library
 * or BigLib files LIB in the compiler's order (acidlibs, deflibs,
 * userlibs) as blitz2 builds it for TED (tools/lib/blitztokens).
 *
 * The text is TED's, with its quirks: TAB expanded to tabstop 2, a line
 * longer than TED's line buffer (chcols + 1 = 129 bytes, tokenised) split
 * into several (readline, ted.s:3375; noted on stderr, the numbering is
 * still TED's), a text line tokenised and detokenised again.
 *
 * Exit status 1 if IN cannot be loaded, if a line has a token TED does
 * not know (TED shows "?????") or a token byte without its second byte,
 * or if OUT cannot be written; OUT is then removed.  2: wrong arguments.
 */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "loadsave.h"
}
#include "tedsrc.h"

static const char *me;          /* "tedsrc detok", the prefix of the messages */

/* lines of the file as written: ended by 0 (tokenised) or LF (text), a
   byte with bit 7 takes the next one with it (readline); a last line
   without end counts.  -1 if the file cannot be read. */
static long file_lines(const char *path)
{
    long len, i, n = 0;
    UBYTE *data = host_readfile(path, &len);
    BOOL open = FALSE;

    if (!data)
        return -1;
    for (i = 0; i < len; i++) {
        if (data[i] & 0x80) {
            i++;
            open = TRUE;
        } else if (data[i] == 0 || data[i] == '\n') {
            n++;
            open = FALSE;
        } else
            open = TRUE;
    }
    free(data);
    return n + open;
}

/* the tokens of line a4 as detok reads them: one TED does not know
   (findtoke -> tokeerr) or a token byte whose second byte is the last
   byte of the line (readline at EOF, survey-04 I.3: detok would read on
   behind the line).  FALSE if there is one, reported as IN:LINE. */
static BOOL check_line(const char *in, unsigned long line, const struct item *a4)
{
    const UBYTE *a0 = a4->chars, *end = a4->chars + a4->numchars;
    BOOL ok = TRUE;
    UWORD w;

    while (a0 < end && *a0) {
        if (!(*a0 & 0x80)) {
            a0++;
            continue;
        }
        if (a0 + 2 >= end) {
            fprintf(stderr, "%s: %s:%lu: token byte $%02X without its second byte\n",
                    me, in, line, *a0);
            return FALSE;
        }
        w = (UWORD)((a0[0] << 8) | a0[1]);
        if (findtoke(w) == tokeerr) {
            fprintf(stderr, "%s: %s:%lu: unknown token $%04X\n", me, in, line,
                    (unsigned)w);
            ok = FALSE;
        }
        a0 += 2;
    }
    return ok;
}

/* IN through TED; the text to OUT unless NULL.  FALSE on any error. */
static BOOL detok_file(const char *in, const char *out)
{
    static UBYTE buf[65536];    /* detok: <= 127 tokens of < 256 chars */
    static jmp_buf jb;
    struct item *a4;
    FILE *f = NULL;
    BOOL ok = TRUE;
    long lines;
    unsigned long i;

    host_outmem = &jb;
    if (setjmp(jb)) {
        fprintf(stderr, "%s: %s: out of memory\n", me, in);
        return FALSE;
    }
    host_resetcalls();
    host_lasterror = NULL;
    fileopen2((UBYTE *)in);
    if (host_calls[HC_ERROR] || !firstitem) {
        fprintf(stderr, "%s: %s: %s\n", me, in, host_lasterror ? host_lasterror : "not loaded");
        return FALSE;
    }
    lines = file_lines(in);
    if (lines > 0 && lines != (long)numlines)
        fprintf(stderr, "%s: %s: note: %ld lines in the file, %u in TED (a line longer "
                        "than TED's line buffer is split)\n", me, in, lines, (unsigned)numlines);
    if (out) {
        f = fopen(out, "wb");
        if (!f) {
            fprintf(stderr, "%s: %s: cannot write\n", me, out);
            return FALSE;
        }
    }
    a4 = firstitem;
    for (i = 1; a4 && i <= numlines; i++, a4 = a4->nextitem) {
        UBYTE *e;

        if (!check_line(in, i, a4)) {
            ok = FALSE;
            continue;
        }
        if (!f)
            continue;
        e = detok(a4->chars, buf, buf + sizeof(buf) - 1);  /* - 1: the LF */
        *e++ = '\n';
        if (fwrite(buf, 1, (size_t)(e - buf), f) != (size_t)(e - buf))
            ok = FALSE;
    }
    if (f && fclose(f) != 0)
        ok = FALSE;
    if (out && !ok) {
        fprintf(stderr, "%s: %s: not written\n", me, out);
        remove(out);
    }
    return ok;
}

void cmd_detok(CLI::App &app, int &status)
{
    struct options {
        tedsrc_tokens t;
        bool check = false;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("detok", "a Blitz 2 source (tokenised or text) as TED shows it, one line per line of TED");
    sub->add_flag("--check", o->check, "only the checks of the files IN..., no output");
    tedsrc_token_options(*sub, o->t, "TOKENS IN OUT (TOKENS IN... with --check), without TOKENS with --blitz2");
    sub->footer("TOKENS: the token list file (tests/data/ted-tokens.txt, or one bbsrc tokens "
                "writes). TED's text: TAB expanded to tabstop 2, a line longer than TED's line "
                "buffer split (noted on stderr). Exit status 0, 1 IN cannot be loaded, a line has a "
                "token TED does not know or a token byte without its second byte, a token list "
                "cannot be read or OUT cannot be written (OUT is removed), 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        const std::vector<std::string> &files = o->t.files;
        struct tokennode *list;
        BOOL ok = TRUE;
        int first;

        me = name.c_str();
        if ((first = tedsrc_first(*sub, me, o->t)) < 0
            || (o->check ? files.size() - (size_t)first < 1 : files.size() - (size_t)first != 2)) {
            if (first >= 0)
                fprintf(stderr, "%s: %s%s expected\n", me, first ? "TOKENS " : "",
                        o->check ? "IN..." : "IN OUT");
            fprintf(stderr, "Run with --help for more information.\n");
            status = 2;
            return;
        }
        if (!(list = tedsrc_token_list(*sub, me, o->t))) {
            status = 1;
            return;
        }
        hostenv_init(NULL, 25);
        tokens = list;
        tokenise = 1;
        if (!o->check) {
            status = detok_file(files[(size_t)first].c_str(), files[(size_t)first + 1].c_str()) ? 0 : 1;
            return;
        }
        for (size_t i = (size_t)first; i < files.size(); i++)
            ok &= detok_file(files[i].c_str(), NULL);
        status = ok ? 0 : 1;
    });
}
