/*
 * outmatch - the output of a test run against an expect file
 * (lib/outmatch/outmatch.h: literal lines, '~' and '+' patterns, sections).
 *
 *   outmatch [--section NAME] EXPECT OUTPUT
 *
 * Exit status 0 if OUTPUT matches EXPECT, 1 if not (the first difference
 * on stderr), 2 on wrong arguments, a file that cannot be read or a
 * broken expect file.  A 0 byte in OUTPUT counts as '?'.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "outmatch.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: outmatch [--section NAME] EXPECT OUTPUT\n"
               "Compare the text OUTPUT with the expect file EXPECT line by line: a\n"
               "line is literal, \"~PATTERN\" one line, \"+PATTERN\" one or more lines\n"
               "matching PATTERN ('*' any characters, '#' a decimal number, \"\\c\" c),\n"
               "\"\\TEXT\" the literal TEXT.  --section: only the lines after \"[NAME]\"\n"
               "up to the next \"[...]\" line in both.\n");
}

/* a whole file as text, 0 bytes as '?' */
static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n, i;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
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

int main(int argc, char **argv)
{
    const char *section = NULL;
    char msg[512], *expect, *output;
    int a = 1, r;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc > 2 && !strcmp(argv[1], "--section")) {
        section = argv[2];
        a = 3;
    }
    if (argc - a != 2) {
        usage(stderr);
        return 2;
    }
    if (!(expect = read_text(argv[a]))) {
        perror(argv[a]);
        return 2;
    }
    if (!(output = read_text(argv[a + 1]))) {
        perror(argv[a + 1]);
        free(expect);
        return 2;
    }
    r = outmatch(expect, output, section, msg, sizeof msg);
    if (r)
        fprintf(stderr, "outmatch: %s: %s\n", r < 0 ? argv[a] : argv[a + 1], msg);
    free(expect);
    free(output);
    return r < 0 ? 2 : r;
}
