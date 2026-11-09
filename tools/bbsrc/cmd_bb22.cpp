/*
 * bbsrc bb22, bbsrc bb22-info - a Blitz 2 source in the "BB22" format
 * (tools/lib/bb22/bb22.h, docs/rwelibs/README.md 4.1) in TED's format.
 *
 *   bbsrc bb22 IN OUT      convert: OUT = the lines of IN, each ended by 0
 *   bbsrc bb22-info IN     version, header words, line count, longest line
 *
 * Exit status 1 if IN cannot be read or is refused (message with the
 * offset or line; OUT is then not written) or OUT cannot be written
 * (OUT is removed); 2: wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "bb22.h"
}
#include "bbsrc.h"

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (d = (uint8_t *)malloc((size_t)n + 1)) && fread(d, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(d);
        d = NULL;
    }
    fclose(f);
    return d;
}

/* bb22 (OUT given) and bb22-info (OUT NULL) */
static int bb22(const char *me, const char *in, const char *out)
{
    struct bb22 b;
    char err[200];
    uint8_t *data;
    size_t len;
    FILE *f;
    int bad;

    if (!(data = read_file(in, &len))) {
        fprintf(stderr, "%s: cannot read %s\n", me, in);
        return 1;
    }
    if (bb22_parse(data, len, &b, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", in, err);
        free(data);
        return 1;
    }
    if (!out) {
        printf("version %u (header %zu bytes, line prefix %zu)\n", b.version, b.header_size,
               b.prefix_size);
        printf("cursor: column %u, row %u, top line %u (line %u, 0-based)\n", b.column, b.row,
               b.top, b.top + b.row);
        printf("lines %zu, longest %zu bytes\n", b.nlines, b.max_len);
        bb22_free(&b);
        free(data);
        return 0;
    }
    f = fopen(out, "wb");
    bad = !f || bb22_write_ted(&b, data, f);
    if (f && fclose(f))
        bad = 1;
    bb22_free(&b);
    free(data);
    if (bad) {
        fprintf(stderr, "%s: cannot write %s\n", me, out);
        remove(out);
        return 1;
    }
    return 0;
}

void cmd_bb22(CLI::App &app, int &status)
{
    struct options {
        std::string in, out;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("bb22", "a Blitz 2 source in the BB22 format in TED's format");
    sub->add_option("IN", o->in, "the BB22 source (some of RWE's library sources, 1997)")->required();
    sub->add_option("OUT", o->out, "the source written: lines of tokenised text, each ended by 0")->required();
    sub->footer("Refuses whatever it does not know. Exit status 0, 1 IN cannot be read or is "
                "refused (OUT is not written) or OUT cannot be written, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        status = bb22(name.c_str(), o->in.c_str(), o->out.c_str());
    });
}

void cmd_bb22_info(CLI::App &app, int &status)
{
    auto in = std::make_shared<std::string>();
    CLI::App *sub = app.add_subcommand("bb22-info", "the header of a BB22 source, its line count and its longest line");
    sub->add_option("IN", *in, "the BB22 source")->required();
    sub->footer("The version, the cursor of the header, the line count and the longest line, "
                "on stdout. Refuses whatever it does not know. Exit status 0, 1 IN cannot be "
                "read or is refused, 2 usage error.");
    sub->callback([sub, in, &status] {
        const std::string name = cli::name(*sub);

        status = bb22(name.c_str(), in->c_str(), NULL);
    });
}
