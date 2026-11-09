/*
 * bbsrc detok - a tokenised Blitz 2 / AmiBlitz source as plain text, line N
 * of the text = line N of the file (lib/bbdetok/bbdetok.h), for reading
 * and diffing sources written with other editors and libraries.
 *
 *   bbsrc detok [--ted] [--strict] [--unknown] TOKENS... IN [OUT]
 *
 * TOKENS, in the compiler's order (the first entry of a number wins):
 *   --tokens FILE   a token list file ("XXXX name" per line: bbsrc
 *                   tokens, tests/data/ted-tokens.txt)
 *   --blitz2 FILE   a compiler's own tokens (blitz2, an AmiBlitz compiler)
 *   --last XXXX     for the next --blitz2: of its several lists (blitz2-ab3)
 *                   the one whose last token is $XXXX
 *   --lib FILE      a library or BigLib file (acidlibs, deflibs, userlibs)
 * --ted: TED's rules (every byte from $80 a token, also in strings and
 * comments), else AmiBlitz's (PED, AmiBlitz 2/3: strings and comments as
 * they are).  A token not in the list is written as {L:N}; the count goes
 * to stderr, --unknown lists every such number with its count, --strict
 * makes it an error.  Without OUT the text goes to stdout.
 *
 * Exit status 1 if a file cannot be read, a token list is refused, the
 * source ends with a token byte or (--strict) has unknown tokens, or OUT
 * cannot be written (OUT is then removed); 2: wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "bbdetok.h"
}
#include "bbsrc.h"

static const char *me;          /* "bbsrc detok", the prefix of the messages */

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (uint8_t *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* the token list of the TOKENS options in the order given; 0, 1 (a file
 * refused) or 2 (a usage error) */
static int load(const CLI::App &sub, struct blitztokens *list, std::vector<uint8_t *> &bufs)
{
    unsigned long last = 0;
    int ntok = 0;
    char err[300];

    for (const cli::occurrence &oc : cli::occurrences(sub)) {
        const std::string o = oc.option->get_name();
        const char *v = oc.values.empty() ? "" : oc.values[0].c_str();
        size_t len;
        int r2;

        if (o == "--last") {
            char *end;

            last = strtoul(v, &end, 16);
            if (*end || !*v || !last || last > 0xFFFF) {
                fprintf(stderr, "%s: --last: not a token number: %s\n", me, v);
                return 2;
            }
            continue;
        }
        if (o != "--tokens" && o != "--blitz2" && o != "--lib")
            continue;
        bufs.push_back(read_all(v, &len));
        if (!bufs.back()) {
            fprintf(stderr, "%s: cannot read %s\n", me, v);
            return 1;
        }
        if (o == "--tokens")
            r2 = blitztokens_read((char *)bufs.back(), len, list, err, sizeof err);
        else if (o == "--blitz2") {
            r2 = blitztokens_compiler_last(bufs.back(), len, (uint16_t)last, list, err, sizeof err);
            last = 0;
        } else
            r2 = blitztokens_libraries(bufs.back(), len, list, err, sizeof err);
        if (r2) {
            fprintf(stderr, "%s: %s: %s\n", me, v, err);
            return 1;
        }
        ntok++;
    }
    if (!ntok) {
        fprintf(stderr, "%s: no token list (--tokens, --blitz2 or --lib)\n", me);
        return 2;
    }
    if (last) {
        fprintf(stderr, "%s: --last without a --blitz2 after it\n", me);
        return 2;
    }
    return 0;
}

static int detok(const CLI::App &sub, int rules, bool strict, bool unknown, const char *in,
                 const char *outpath)
{
    static struct bbdetok_result r;
    struct blitztokens list = {};
    struct bbdetok d = {};
    std::vector<uint8_t *> bufs;
    int rc = 1, i;
    uint8_t *src = NULL;
    char err[300], *text = NULL;
    size_t len, textlen;
    FILE *f;

    if ((rc = load(sub, &list, bufs)) != 0) {
        outpath = NULL;                 /* an OUT of an earlier run is left as it is */
        goto done;
    }
    rc = 1;
    if (bbdetok_init(&d, &list)) {
        fprintf(stderr, "%s: out of memory\n", me);
        goto done;
    }
    if (!(src = read_all(in, &len))) {
        fprintf(stderr, "%s: cannot read %s\n", me, in);
        goto done;
    }
    if (bbdetok_text(&d, rules, src, len, &text, &textlen, &r, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, in, err);
        goto done;
    }
    if (r.open)
        fprintf(stderr, "%s: %s: note: the last line has no 0 byte (a text file?)\n", me, in);
    if (r.unknown)
        fprintf(stderr, "%s: %s: %zu of %zu tokens unknown (%zu different numbers)\n", me, in,
                r.unknown, r.tokens, r.kinds);
    if (unknown)
        for (i = 0; i < 32768; i++)
            if (r.count[i])
                fprintf(stderr, "{%d:%d} $%04X %u\n", i >> 7, i & 0x7F, i | 0x8000,
                        (unsigned)r.count[i]);
    if (strict && r.unknown)
        goto done;
    f = outpath ? fopen(outpath, "wb") : stdout;
    if (!f) {
        fprintf(stderr, "%s: cannot write %s\n", me, outpath);
        goto done;
    }
    i = fwrite(text, 1, textlen, f) != textlen;
    if ((outpath ? fclose(f) : fflush(f)) || i) {
        fprintf(stderr, "%s: cannot write %s\n", me, outpath ? outpath : "stdout");
        goto done;
    }
    rc = 0;
done:
    if (rc && outpath)
        remove(outpath);                /* no OUT of an earlier run stays */
    for (uint8_t *buf : bufs)
        free(buf);
    free(src);
    free(text);
    bbdetok_free(&d);
    blitztokens_free(&list);
    return rc;
}

void cmd_detok(CLI::App &app, int &status)
{
    struct options {
        bool ted = false, strict = false, unknown = false;
        std::vector<std::string> tokens, blitz2, last, lib;
        std::string in, out;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("detok", "a tokenised Blitz 2 / AmiBlitz source as plain text, line for line");
    sub->add_flag("--ted", o->ted,
                  "TED's rules: every byte from $80 a token, also in strings and comments (default: "
                  "AmiBlitz's, strings and comments as they are)");
    sub->add_flag("--strict", o->strict, "fail on a token the list does not have (else written as {L:N})");
    sub->add_flag("--unknown", o->unknown, "list the unknown tokens on stderr: {L:N} $XXXX count");
    cli::add_repeated(*sub, "--tokens", o->tokens, 1, "a token list file (\"XXXX name\" per line, bbsrc tokens)")
        ->type_name("FILE");
    cli::add_repeated(*sub, "--blitz2", o->blitz2, 1, "a compiler's own tokens")->type_name("FILE");
    cli::add_repeated(*sub, "--last", o->last, 1, "for the next --blitz2: its list that ends with $XXXX")
        ->type_name("XXXX");
    cli::add_repeated(*sub, "--lib", o->lib, 1, "a library or BigLib file (acidlibs, deflibs, userlibs)")
        ->type_name("FILE");
    sub->add_option("IN", o->in, "the tokenised source")->required();
    sub->add_option("OUT", o->out, "the text written (default: stdout)");
    sub->footer("TOKENS: at least one of --tokens, --blitz2, --lib, in the compiler's order (the "
                "first entry of a number wins). A token not in the list is written as {L:N}, the "
                "count goes to stderr. Exit status 0, 1 a file cannot be read, a token list is "
                "refused, the source ends with a token byte or (--strict) has unknown tokens, or "
                "OUT cannot be written (OUT is removed), 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = detok(*sub, o->ted ? BBDETOK_TED : BBDETOK_AB, o->strict, o->unknown, o->in.c_str(),
                       sub->count("OUT") ? o->out.c_str() : NULL);
    });
}
