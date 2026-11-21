/*
 * results exe-same - two executables of the same source, the parameter blocks of their traps
 * masked (lib/prevcmp/prevcmp.h prevcmp_exe_same).
 *
 *   results exe-same A B
 *
 * Exit status 0 = the same, 1 = they differ (the offset of the first difference on stderr),
 * 2 = a file cannot be read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "prevcmp.h"
}
#include "results.h"

static const char *me; /* "results exe-same", the prefix of the messages */

/* a whole file, NULL if it cannot be read */
static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (d = (uint8_t *)malloc((size_t)n + 1)) && fread(d, 1, (size_t)n, f) == (size_t)n) {
        *len = (size_t)n;
    } else {
        free(d);
        d = NULL;
    }
    fclose(f);
    return d;
}

void cmd_exe_same(CLI::App &app, int &status)
{
    struct options {
        std::string a, b;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("exe-same", "two executables of the same source, the parameter blocks of their traps masked");
    sub->add_option("A", o->a, "the first executable")->required();
    sub->add_option("B", o->b, "the second executable")->required();
    sub->footer("The 8 bytes behind each trap #1 (4E41) and trap #4 (4E44) of a program with runtime "
                "error checking or debugging hold addresses of the compiler's memory and are "
                "zeroed in both before the comparison. Exit status 0 the same, 1 they differ "
                "(the offset of the first difference on stderr), 2 a file cannot be read.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        size_t alen = 0, blen = 0, first = 0;
        uint8_t *a, *b;

        me = name.c_str();
        a = read_file(o->a.c_str(), &alen);
        b = a ? read_file(o->b.c_str(), &blen) : NULL;
        if (!a || !b) {
            fprintf(stderr, "%s: cannot read %s\n", me, a ? o->b.c_str() : o->a.c_str());
            free(a);
            status = 2;
            return;
        }
        if (prevcmp_exe_same(a, alen, b, blen, &first)) {
            status = 0;
        } else {
            fprintf(stderr, "%s: %s and %s differ (%zu and %zu bytes), first at offset %zu\n", me,
                    o->a.c_str(), o->b.c_str(), alen, blen, first);
            status = 1;
        }
        free(a);
        free(b);
    });
}
