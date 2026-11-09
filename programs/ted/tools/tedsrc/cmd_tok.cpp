/*
 * tedsrc tok - a source through TED: loaded with fileopen2 and written
 * back with realsave (ted.s:680, ted.s:769), tokenise = 1, by the C core
 * of TED on the host (the same as the original TED for every source of
 * the CD: test_loadsave against the oracle).  A text source comes out
 * tokenised, exactly as TED would save it.
 *
 *   tedsrc tok TOKENS IN OUT       TOKENS: tests/data/ted-tokens.txt
 *   tedsrc tok --blitz2 BLITZ2 [--lib LIB]... IN OUT
 *
 * The token list: a list file (tests/data/ted-tokens.txt, or one written
 * by bbsrc tokens), or made from the compiler BLITZ2 and the library
 * or BigLib files LIB in the compiler's order (acidlibs, deflibs,
 * userlibs) as blitz2 builds it for TED (tools/lib/blitztokens).  The
 * token list of tedsrc detok is made here too (tedsrc.h).
 *
 * Exit status 1 if IN cannot be loaded, if a line of a text source is
 * longer than TED allows (TED would cut it), or if OUT is not written.
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
#include "loadsave.h"
}
#include "tedsrc.h"

void tedsrc_token_options(CLI::App &sub, tedsrc_tokens &o, const std::string &files)
{
    sub.add_option("--blitz2", o.blitz2, "the token list made from this compiler and the LIBs instead of TOKENS")
        ->type_name("BLITZ2");
    cli::add_repeated(sub, "--lib", o.libs, 1,
                      "with --blitz2: a library or BigLib file, in the compiler's order (acidlibs, "
                      "deflibs, userlibs)")
        ->type_name("LIB");
    sub.add_option("FILE", o.files, files)->required();
}

int tedsrc_first(const CLI::App &sub, const char *me, const tedsrc_tokens &o)
{
    if (sub.count("--blitz2"))
        return 0;
    if (!o.libs.empty()) {
        fprintf(stderr, "%s: --lib without --blitz2\n", me);
        return -1;
    }
    return 1;
}

struct tokennode *tedsrc_token_list(const CLI::App &sub, const char *me, const tedsrc_tokens &o)
{
    std::vector<const char *> libs;

    if (!sub.count("--blitz2"))
        return host_tokens(me, o.files[0].c_str(), NULL, NULL, 0);
    for (const std::string &lib : o.libs)
        libs.push_back(lib.c_str());
    return host_tokens(me, NULL, o.blitz2.c_str(), libs.data(), (int)libs.size());
}

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

static const char *me;          /* "tedsrc tok", the prefix of the messages */

/* IN through TED, saved to OUT with the token list LIST */
static int tok(struct tokennode *list, const char *in, const char *out)
{
    static jmp_buf jb;
    long lines;

    hostenv_init(NULL, 25);
    tokens = list;
    tokenise = 1;
    host_outmem = &jb;
    if (setjmp(jb)) {
        fprintf(stderr, "%s: %s: out of memory\n", me, in);
        return 1;
    }
    host_resetcalls();
    fileopen2((UBYTE *)in);
    if (host_calls[HC_ERROR] || !firstitem) {
        fprintf(stderr, "%s: %s: %s\n", me, in, host_lasterror ? host_lasterror : "not loaded");
        return 1;
    }
    /* a text line longer than TED's line buffer (chcols + 1 = 129
       characters) is cut by readline, the rest becomes a line of its own
       (ted.s:3411): not what the author wrote, so such a source is
       refused */
    lines = text_lines(in);
    if (lines && numlines != lines) {
        fprintf(stderr, "%s: %s: %ld lines, %u after loading: a line is longer than TED "
                        "allows (%u characters)\n", me, in, lines, (unsigned)numlines,
                (unsigned)chcols + 1);
        return 1;
    }
    remove(out);
    realsave((UBYTE *)out);
    if (host_calls[HC_ERROR]) {
        fprintf(stderr, "%s: %s: %s\n", me, out, host_lasterror ? host_lasterror : "not written");
        return 1;
    }
    return 0;
}

void cmd_tok(CLI::App &app, int &status)
{
    auto o = std::make_shared<tedsrc_tokens>();
    CLI::App *sub = app.add_subcommand("tok", "a source through TED (load and save, tokenised) on the host");
    tedsrc_token_options(*sub, *o, "TOKENS IN OUT, or IN OUT with --blitz2");
    sub->footer("TOKENS: the token list file (tests/data/ted-tokens.txt, or one bbsrc tokens "
                "writes). A text source comes out tokenised, as TED saves it. Exit status 0, 1 IN "
                "cannot be loaded, a line of a text source is longer than TED allows, a token list "
                "cannot be read or OUT is not written, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        struct tokennode *list;
        int first;

        me = name.c_str();
        if ((first = tedsrc_first(*sub, me, *o)) < 0 || o->files.size() - (size_t)first != 2) {
            if (first >= 0)
                fprintf(stderr, "%s: %s\n", me, first ? "TOKENS IN OUT expected" : "IN OUT expected");
            fprintf(stderr, "Run with --help for more information.\n");
            status = 2;
            return;
        }
        if (!(list = tedsrc_token_list(*sub, me, *o))) {
            status = 1;
            return;
        }
        status = tok(list, o->files[(size_t)first].c_str(), o->files[(size_t)first + 1].c_str());
    });
}
