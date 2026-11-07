/*
 * corpusrun - one pass of the corpus check in Amiberry (see corpus.h).
 *
 *   corpusrun PASSFILE
 *
 * Exit status 0 = no differences, 1 = differences (report), 77 = skipped
 * (no emulator, or the CD's libraries are missing), 2 = error.  With the
 * environment variable BUGORIG_RECORD=1 a bug-orig pass (orig=) records the
 * original's observation instead (corpus.h).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corpus.h"

static const char usage[] =
    "usage: corpusrun PASSFILE\n"
    "  one pass of the corpus check in Amiberry\n";

int main(int argc, char **argv)
{
    struct corpus_pass p;
    char err[300], *text;
    FILE *f;
    long n;
    int rc;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc != 2) {
        fputs(usage, stderr);
        return 2;
    }
    if (!(f = fopen(argv[1], "rb")) || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0
        || fseek(f, 0, SEEK_SET) || !(text = malloc((size_t)n + 1))
        || fread(text, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "corpusrun: cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);
    text[n] = 0;
    if (corpus_parse(text, &p, err, sizeof err)) {
        fprintf(stderr, "corpusrun: %s: %s\n", argv[1], err);
        return 2;
    }
    free(text);
    /* BUGORIG_RECORD=1: the recording run of a bug-orig test (orig=) */
    corpus_select(&p, getenv("BUGORIG_RECORD") && !strcmp(getenv("BUGORIG_RECORD"), "1"));
    corpus_skip_missing(&p, stdout);
    rc = corpus_run(&p, stdout);
    corpus_free(&p);
    return rc;
}
