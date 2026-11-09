/*
 * refcheck asm - assembler routines of a C library against the original,
 * instruction by instruction (see asmcmp.h).
 *
 *   refcheck asm OBJDUMP ORIGINAL LIBNUM NAME.linked NAME:ORIGADDR:CSYMBOL:COUNT...
 *
 * ORIGINAL: the BigLib file with the original library (e.g. the CD's
 * BLITZ2/ACIDLIBS), LIBNUM its number; NAME.linked: the C library linked
 * with symbols.  Exit status 1 if a routine differs, 77 (skipped) if
 * ORIGINAL does not exist (the CD is not in the repository:
 * fetch-originals).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "asmcmp.h"
#include "biglib.h"
}
#include "refcheck.h"

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

const char cmd_asm_usage[] =
    "usage: refcheck asm OBJDUMP ORIGINAL LIBNUM NAME.linked NAME:ORIGADDR:CSYMBOL:COUNT...\n"
    "  asm routines of a C library against the original, instruction by instruction\n";

static const char *me; /* "refcheck asm", the prefix of the messages */

struct asm_options {
    std::string objdump, original, libnum, linked;
    std::vector<std::string> routines;
};

static int run(const asm_options &o)
{
    struct biglib_entry *e;
    struct loadfile orig, c;
    unsigned long libnum;
    char err[200];
    uint8_t *big;
    size_t size, n, i;
    int bad = 0, found = 0;
    const char *original = o.original.c_str();

    if (access(original, F_OK)) {
        printf("%s: no %s - skipped (fetch-originals)\n", me, original);
        return 77;
    }
    libnum = strtoul(o.libnum.c_str(), NULL, 0) & 0xFFFF;
    if (!(big = read_all(original, &size)) || biglib_read(big, size, &e, &n, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, original, big ? err : "cannot read");
        return 1;
    }
    for (i = 0; i < n && !found; i++) {
        if (load_parse(e[i].data, e[i].size, &orig, err, sizeof err)) {
            fprintf(stderr, "%s: %s: library %zu: %s\n", me, original, i + 1, err);
            return 1;
        }
        found = orig.nhunks && orig.hunks[0].size >= 6 && be16(orig.hunks[0].data + 4) == libnum;
        if (!found)
            load_free(&orig);
    }
    if (!found) {
        fprintf(stderr, "%s: %s: no library %lu\n", me, original, libnum);
        return 1;
    }
    if (load_read(o.linked.c_str(), &c, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, o.linked.c_str(), err);
        return 1;
    }
    for (const std::string &r : o.routines)
        if (asmcmp_routine(o.objdump.c_str(), &orig.hunks[0], &c, r.c_str(), stdout))
            bad = 1;
    load_free(&c);
    load_free(&orig);
    free(e);
    free(big);
    return bad;
}

void cmd_asm(CLI::App &app, int &status)
{
    auto o = std::make_shared<asm_options>();
    CLI::App *sub = app.add_subcommand("asm", "asm routines of a C library against the original, instruction by instruction");
    sub->add_option("OBJDUMP", o->objdump, "the m68k objdump")->required();
    sub->add_option("ORIGINAL", o->original,
                    "the BigLib file with the original library (e.g. the CD's BLITZ2/ACIDLIBS)")
        ->required();
    sub->add_option("LIBNUM", o->libnum, "the number of the library in ORIGINAL")->required();
    sub->add_option("LINKED", o->linked, "NAME.linked, the C library linked with symbols")->required();
    sub->add_option("ROUTINE", o->routines,
                    "NAME:ORIGADDR:CSYMBOL:COUNT, a routine: its name, its address in the original, "
                    "the C symbol and the number of instructions")
        ->required();
    sub->footer("Exit status 0 every routine is equal, 1 a routine differs or an input cannot be "
                "read, 2 usage error, 77 ORIGINAL does not exist (skipped: the CD is not in the "
                "repository, fetch-originals).");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = run(*o);
    });
}
