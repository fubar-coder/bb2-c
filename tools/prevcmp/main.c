/*
 * prevcmp - the library files of this build against those of the build of
 * the previous commit (lib/prevcmp/prevcmp.h).
 *
 *   prevcmp [--all] [--intended PATH]... CUR PREV
 *
 * PATH is a library file.  An entry with a colon (corpus:NAME,
 * ted_ab:CASE, ... - PREV_INTENDED, lib/previntended) belongs to the
 * corpus and ted_ab: prevcmp only checks its syntax and ignores it.
 *
 * CUR and PREV are build directories (build/<preset>, build/<preset>/prev).
 * Exit status 0 = the library files are the same or differ as intended,
 * 1 = a difference not intended or an intended entry that does not differ,
 * 77 = PREV does not exist (no prev build: skipped), 2 = error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "prevcmp.h"
#include "previntended.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: prevcmp [--all] [--intended PATH]... CUR PREV\n"
               "Compare the library files (blitz2/acidlibs, blitz2/deflibs and every .obj\n"
               "below libs/ and variants/) of the build directory CUR byte by byte with\n"
               "those of PREV, the build of the previous commit.  A difference fails unless\n"
               "--all (every difference is intended) or --intended PATH (relative to the\n"
               "build directory, repeatable; it must differ) names it.  An entry with a colon\n"
               "(corpus:NAME, ted_ab:CASE ...) is one of the corpus and ted_ab: checked for\n"
               "its syntax, otherwise ignored.  Exit status 77: PREV does not exist.\n");
}

int main(int argc, char **argv)
{
    struct prevcmp_options o = { 0 };
    struct prevint others = { 0 };
    const char **intended = calloc((size_t)argc + 1, sizeof *intended);
    char err[600];
    struct stat st;
    int a = 1, r;

    if (!intended)
        return 2;
    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        free(intended);
        return 0;
    }
    for (; a < argc && argv[a][0] == '-'; a++) {
        if (!strcmp(argv[a], "--all"))
            o.all = 1;
        else if (!strcmp(argv[a], "--intended") && a + 1 < argc) {
            int k = prevint_add(&others, argv[a + 1], err, sizeof err);

            if (k < 0) {
                fprintf(stderr, "prevcmp: %s\n", err);
                prevint_free(&others);
                free(intended);
                return 2;
            }
            if (k)
                intended[o.nintended++] = argv[a + 1];
            a++;
        } else {
            usage(stderr);
            free(intended);
            return 2;
        }
    }
    if (argc - a != 2) {
        usage(stderr);
        free(intended);
        return 2;
    }
    o.intended = intended;
    prevint_free(&others);
    if (stat(argv[a + 1], &st) || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "prevcmp: %s: no prev build\n", argv[a + 1]);
        free(intended);
        return 77;
    }
    r = prevcmp(argv[a], argv[a + 1], &o, stdout, err, sizeof err);
    if (r == 2)
        fprintf(stderr, "prevcmp: %s\n", err);
    free(intended);
    return r;
}
