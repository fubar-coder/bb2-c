/*
 * bbsrc comment - an edit of the lines of a tokenised Blitz Basic 2 source
 * on its bytes (lib/bblines/bblines.h).
 *
 *   bbsrc comment TOKEN IN OUT
 *
 * Every line whose statement is the token TOKEN (hex, e.g. 80BF = INCDIR)
 * becomes ";"; the other lines stay byte for byte.  The reference build of
 * the RWE sources (libs/userlibs/rwe/refbuild.cmake) neutralises the
 * INCDIR lines with it.  Exit status 1 on an error (OUT is not written),
 * 2 on wrong arguments.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "bblines.h"
}
#include "bbsrc.h"

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

/* the lines of IN whose statement is TOKEN as ";" to OUT */
static int comment(const char *me, const char *word, const char *in, const char *outpath)
{
    unsigned long token;
    uint8_t *src, *out;
    size_t len, outlen, count;
    char err[300], *e;
    FILE *f;

    token = strtoul(word, &e, 16);
    if (!*word || *e || token > 0xFFFF) {
        fprintf(stderr, "%s: %s is not a token word (hex)\n", me, word);
        return 2;
    }
    if (!(src = read_all(in, &len))) {
        fprintf(stderr, "%s: %s: %s\n", me, in, strerror(errno));
        return 1;
    }
    if (bblines_comment(src, len, (uint16_t)token, &out, &outlen, &count, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, in, err);
        free(src);
        return 1;
    }
    free(src);
    if (!(f = fopen(outpath, "wb")) || fwrite(out, 1, outlen, f) != outlen || fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, outpath, strerror(errno));
        remove(outpath);
        free(out);
        return 1;
    }
    free(out);
    return 0;
}

void cmd_comment(CLI::App &app, int &status)
{
    struct options {
        std::string token, in, out;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("comment", "the lines of a tokenised source whose statement is TOKEN as \";\"");
    sub->add_option("TOKEN", o->token, "the token word (hex, e.g. 80BF = INCDIR)")->required();
    sub->add_option("IN", o->in, "the tokenised Blitz 2 source")->required();
    sub->add_option("OUT", o->out, "the source written")->required();
    sub->footer("Every line of IN whose statement is the token TOKEN becomes \";\" in OUT, the "
                "other lines stay byte for byte. Exit status 0, 1 IN cannot be read or is broken, "
                "or OUT cannot be written (OUT is not left), 2 usage error or a TOKEN that is no "
                "hex word.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        status = comment(name.c_str(), o->token.c_str(), o->in.c_str(), o->out.c_str());
    });
}
