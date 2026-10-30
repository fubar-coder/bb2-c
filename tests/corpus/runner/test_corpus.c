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

    /* files of the volume, wrapper, job lines, bug-orig expectation */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nfile=blitz2/userlibs/x/lib|/l\ndir=blitz2/userlibs/y\n"
                       "remove=blitz2/ted\nwrap=/w2|allocfail=64-64\njobline=menu=j1|9|X\n"
                       "expect=/e\n", &p, err, sizeof err) == 0);
    CHECK(p.nfiles == 1 && !strcmp(p.file_dest[0], "blitz2/userlibs/x/lib")
          && !strcmp(p.file_src[0], "/l"));
    CHECK(p.ndirs == 1 && p.nremoves == 1 && !strcmp(p.removes[0], "blitz2/ted"));
    CHECK(!strcmp(p.wrap, "/w2") && !strcmp(p.wrap_args, "allocfail=64-64"));
    CHECK(p.njoblines == 1 && !strcmp(p.joblines[0], "menu=j1|9|X") && !strcmp(p.expect, "/e"));
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nwrap=/w2\n", &p, err, sizeof err) == 0);
    CHECK(!strcmp(p.wrap, "/w2") && !p.wrap_args && !p.runwrap);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nrunwrap=/r|signals\n", &p, err, sizeof err) == 0);
    CHECK(!strcmp(p.runwrap, "/r") && !strcmp(p.runwrap_args, "signals") && !p.wrap);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nrunargs=a.txt b\n", &p, err, sizeof err) == 0);
    CHECK(!strcmp(p.runargs, "a.txt b") && !p.runwrap);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nuae=z3mem_size=512\nuae=a=\n", &p, err, sizeof err) == 0);
    CHECK(p.nuae == 2 && !strcmp(p.uae[0], "z3mem_size=512") && !strcmp(p.uae[1], "a="));
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=/abs|/l\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a/../../x|/l\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=SYS:x|/l\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a//b|/l\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a\n", err, sizeof err) == -1);
    /* a file for one configuration only: the tag must name one */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nfile=u/lib|/r|ref\nfile=u/lib|/c|port\nfile=x|/x\n",
                       &p, err, sizeof err) == 0);
    CHECK(p.nfiles == 3 && !strcmp(p.file_tag[0], "ref") && !strcmp(p.file_src[1], "/c")
          && !strcmp(p.file_tag[1], "port") && !p.file_tag[2]);
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a|/l|x\n", err, sizeof err) == -1
          && strstr(err, "no configuration x"));
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a|/l|\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nfile=a|/l|ref|x\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\ndir=\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nremove=../x\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\njobline=\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nwrap=|x\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nrunwrap=|x\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nrunargs=\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nuae=z3mem_size\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "source=/s\nuae==1\n", err, sizeof err) == -1);
    /* bug-orig: exactly one source */
    CHECK(parse(BASE CONFIGS "source=/s\nsource=/t\nexpect=/e\n", err, sizeof err) == -1
          && strstr(err, "one source"));
    CHECK(parse(BASE CONFIGS "source=/s\nserial_same_as=/o\nexpect=/e\n", err, sizeof err) == -1
          && strstr(err, "serial_same_as"));
    /* bug-orig with the remote end on the serial port */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nserial=on\nexpect=/e\n", &p, err, sizeof err) == 0);
    CHECK(p.serial && !strcmp(p.expect, "/e"));
    corpus_free(&p);

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
