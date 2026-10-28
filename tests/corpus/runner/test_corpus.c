/*
 * test_corpus.c - the pass file (valid and broken) and the masking of the
 * trap #1 / trap #4 parameter blocks (the emulator part runs in the
 * corpus_* ctests).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corpus.h"

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%s:%d: CHECK failed: %s\n", \
                                          __FILE__, __LINE__, #c); failures++; } } while (0)

static int parse(const char *text, char *err, size_t len)
{
    struct corpus_pass p;
    int r = corpus_parse(text, &p, err, len);

    if (!r)
        corpus_free(&p);
    return r;
}

#define BASE "work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/d\n"
#define CONFIGS "config=ref|ref|/a|/b|/c\nconfig=port|exe|/a2|/b|/c\n"

int main(void)
{
    struct corpus_pass p;
    char err[300], *n;

    /* a valid pass */
    CHECK(corpus_parse("# comment\n\n" BASE "runtime_errors=on\ntimeout=90\nparallel=3\n" CONFIGS
                       "config=clib|output|/a2|/b2|/c2\nsource=/s/t01_int.bb2\n"
                       "source=/s/T02_float.BB2\n", &p, err, sizeof err) == 0);
    CHECK(p.runtime_errors == 1 && p.timeout == 90 && p.parallel == 3 && p.stack == 40000);
    CHECK(p.nconfigs == 3 && p.configs[1].expect == CORPUS_EXE
          && p.configs[2].expect == CORPUS_OUTPUT && !strcmp(p.configs[2].acidlibs, "/c2"));
    CHECK(p.nsources == 2 && !p.gate);
    corpus_free(&p);
    CHECK((n = corpus_name("/s/T02_float.BB2")) && !strcmp(n, "T02_float"));
    free(n);

    /* broken */
    CHECK(parse(BASE CONFIGS, err, sizeof err) == -1);                      /* no source */
    CHECK(parse(BASE "config=ref|ref|/a|/b|/c\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE "config=port|exe|/a|/b|/c\nconfig=ref|ref|/a|/b|/c\nsource=/s\n", err,
                sizeof err) == -1 && strstr(err, "first"));
    CHECK(parse(BASE CONFIGS "config=PORT|output|/a|/b|/c\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "twice"));
    CHECK(parse(BASE CONFIGS "source=/s/a.bb2\nsource=/t/A.bb2\n", err, sizeof err) == -1
          && strstr(err, "two sources"));
    CHECK(parse(BASE CONFIGS "config=x|maybe|/a|/b|/c\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "config=x|output|/a|/b\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "parallel=0\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "runtime_errors=yes\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "colour=red\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "unknown key"));
    CHECK(parse("work=/w\n" CONFIGS "source=/s\n", err, sizeof err) == -1);

    /* read-only volumes */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nmount=BB2|/cd\nmount=X|/x\n", &p, err,
                       sizeof err) == 0);
    CHECK(p.nmounts == 2 && !strcmp(p.mount_name[0], "BB2") && !strcmp(p.mount_dir[1], "/x"));
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "source=/s\nmount=BB2:|/cd\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nmount=BB2\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nmount=BB2|/cd|x\n", err, sizeof err) == -1);

    /* masking: 8 bytes behind each trap #1 / #4, the search goes on behind
       the block; first every trap #1, then every trap #4 (as bb2's
       mask_trap1: the trap #1 at 14 also clears the $4E71, the trap #4 at
       12 then its own 8 bytes); a trap without 8 bytes behind it stays */
    {
        uint8_t d[] = { 0x4E, 0x41, 1, 2, 3, 4, 5, 6, 7, 8, 0x4E, 0x75,
                        0x4E, 0x44, 0x4E, 0x41, 9, 9, 9, 9, 9, 9, 0x4E, 0x71,
                        0x4E, 0x41, 1, 2 };
        static const uint8_t want[] = { 0x4E, 0x41, 0, 0, 0, 0, 0, 0, 0, 0, 0x4E, 0x75,
                                        0x4E, 0x44, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0x4E, 0x41, 1, 2 };

        corpus_mask_traps(d, sizeof d);
        CHECK(!memcmp(d, want, sizeof d));
    }
    return failures != 0;
}
