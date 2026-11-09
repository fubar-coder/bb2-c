/*
 * results match - the output of a test run against an expect file
 * (lib/outmatch/outmatch.h: literal lines, '~' and '+' patterns, sections).
 *
 *   results match [--section NAME] EXPECT OUTPUT
 *
 * Exit status 0 if OUTPUT matches EXPECT, 1 if not (the first difference
 * on stderr), 2 on wrong arguments, a file that cannot be read or a
 * broken expect file.  A 0 byte in OUTPUT counts as '?'.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "outmatch.h"
}
#include "results.h"

static const char *me; /* "results match", the prefix of the messages */

/* a whole file as text, 0 bytes as '?' */
static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n, i;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        for (i = 0; i < n; i++)
            if (!s[i])
                s[i] = '?';
        s[n] = 0;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

void cmd_match(CLI::App &app, int &status)
{
    struct options {
        std::string section, expect, output;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("match", "the text OUTPUT against the expect file EXPECT, line by line");
    sub->add_option("--section", o->section, "only the lines after \"[NAME]\" up to the next \"[...]\" line in both")
        ->type_name("NAME");
    sub->add_option("EXPECT", o->expect, "the expect file")->required();
    sub->add_option("OUTPUT", o->output, "the output of a test run")->required();
    sub->footer("A line of EXPECT is literal, \"~PATTERN\" one line, \"+PATTERN\" one or more lines "
                "matching PATTERN ('*' any characters, '#' a decimal number, \"\\c\" c), \"\\TEXT\" "
                "the literal TEXT. Exit status 0 OUTPUT matches, 1 it does not (the first "
                "difference on stderr), 2 usage error, a file that cannot be read or a broken "
                "EXPECT.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        const char *section = sub->count("--section") ? o->section.c_str() : NULL;
        const char *expect_path = o->expect.c_str(), *output_path = o->output.c_str();
        char msg[512], *expect, *output;
        int r;

        me = name.c_str();
        if (!(expect = read_text(expect_path))) {
            fprintf(stderr, "%s: %s: %s\n", me, expect_path, strerror(errno));
            status = 2;
            return;
        }
        if (!(output = read_text(output_path))) {
            fprintf(stderr, "%s: %s: %s\n", me, output_path, strerror(errno));
            free(expect);
            status = 2;
            return;
        }
        r = outmatch(expect, output, section, msg, sizeof msg);
        if (r)
            fprintf(stderr, "%s: %s: %s\n", me, r < 0 ? expect_path : output_path, msg);
        free(expect);
        free(output);
        status = r < 0 ? 2 : r;
    });
}
