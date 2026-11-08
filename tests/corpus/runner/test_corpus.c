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

    /* exe:BASE compares with another configuration instead of the reference */
    CHECK(corpus_parse(BASE CONFIGS "config=clib|output|/a2|/b2|/c2\n"
                       "config=prev|exe:clib|/p|/q|/r\nsource=/s\n", &p, err, sizeof err) == 0);
    CHECK(p.nconfigs == 4 && p.configs[3].expect == CORPUS_EXE && !p.configs[1].base
          && !strcmp(p.configs[3].base, "clib") && !strcmp(p.configs[3].acidlibs, "/r"));
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "config=prev|output:port|/p|/q|/r\nsource=/s\n", &p, err,
                       sizeof err) == 0 && p.configs[2].expect == CORPUS_OUTPUT
          && !strcmp(p.configs[2].base, "port"));
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "config=prev|exe:nobody|/p|/q|/r\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "prev: nobody is no other configuration"));
    CHECK(parse(BASE CONFIGS "config=prev|exe:prev|/p|/q|/r\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "config=prev|exe:|/p|/q|/r\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE "config=ref|ref:port|/a|/b|/c\nconfig=port|exe|/a|/b|/c\nsource=/s\n", err,
                sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "config=prev|ref:port|/p|/q|/r\nsource=/s\n", err, sizeof err) == -1);

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

    /* PREV_INTENDED: test= and intended= (entries of other tests are kept) */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\ntest=corpus\nintended=corpus:t01_int\n"
                       "intended=ted_ab:x:Cc-Pc\n", &p, err, sizeof err) == 0);
    CHECK(!strcmp(p.test, "corpus") && p.intended.n == 2 && !p.intended.all);
    CHECK(prevint_judge(&p.intended, "corpus", "t01_int", NULL, 1) == PREVINT_INTENDED);
    CHECK(prevint_judge(&p.intended, "corpus", "t02", NULL, 1) == PREVINT_DIFFERS);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\ntest=corpus\nintended=ON\n", &p, err,
                       sizeof err) == 0 && p.intended.all);
    corpus_free(&p);
    /* broken: no test=, a malformed entry, a library path, an unknown test */
    CHECK(parse(BASE CONFIGS "source=/s\nintended=corpus:a\n", err, sizeof err) == -1
          && strstr(err, "needs test="));
    CHECK(parse(BASE CONFIGS "source=/s\ntest=corpus\nintended=corpus:\n", err, sizeof err) == -1
          && strstr(err, "line") && strstr(err, "name is empty"));
    CHECK(parse(BASE CONFIGS "source=/s\ntest=corpus\nintended=libs/a.obj\n", err, sizeof err) == -1
          && strstr(err, "library path"));
    CHECK(parse(BASE CONFIGS "source=/s\ntest=corpus\nintended=corpuz:a\n", err, sizeof err) == -1
          && strstr(err, "unknown test"));

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
                       "expect=/e\norig=/o\n", &p, err, sizeof err) == 0);
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
    /* the machine: a known one, the last line wins; none = the default */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0 && !p.machine);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nmachine=a1200\nmachine=a500\nrcput=/rc\n", &p,
                       err, sizeof err) == 0);
    CHECK(p.machine && !strcmp(p.machine, "a500") && !strcmp(p.rcput, "/rc"));
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "source=/s\nmachine=a500-68010\nrcput=/rc\n", err, sizeof err) == 0);
    /* Kickstart 1.x: rcput needed, no gate; rcput does not matter elsewhere */
    CHECK(parse(BASE CONFIGS "source=/s\nmachine=a500\n", err, sizeof err) == -1
          && strstr(err, "rcput="));
    CHECK(parse(BASE CONFIGS "source=/s\nmachine=a500\nrcput=/rc\ngate=/g\n", err, sizeof err) == -1
          && strstr(err, "gate="));
    CHECK(parse(BASE CONFIGS "source=/s\nrcput=/rc\ngate=/g\n", err, sizeof err) == 0);
    CHECK(parse(BASE CONFIGS "source=/s\nmachine=a3000\n", err, sizeof err) == -1 && strstr(err, "machine="));
    CHECK(parse(BASE CONFIGS "source=/s\nmachine=\n", err, sizeof err) == -1);
    /* bug-orig: exactly one source */
    CHECK(parse(BASE CONFIGS "source=/s\nsource=/t\nexpect=/e\norig=/o\n", err, sizeof err) == -1
          && strstr(err, "one source"));
    CHECK(parse(BASE CONFIGS "source=/s\nserial_same_as=/o\nexpect=/e\norig=/o\n", err, sizeof err) == -1
          && strstr(err, "serial_same_as"));
    /* bug-orig with the remote end on the serial port */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nserial=on\nexpect=/e\norig=/o\n", &p, err, sizeof err) == 0);
    CHECK(p.serial && !strcmp(p.expect, "/e"));
    corpus_free(&p);
    /* the recorded original: the normal run drops the reference and the
       files only for it, the recording keeps nothing else */
    {
#define ORIG BASE CONFIGS "config=clib|output|/a3|/b3|/c3\nsource=/s\nexpect=/e\norig=/o\n" \
              "orig_by=blitz2 1.95\nfile=x/ref|/r|ref\nfile=x/port|/p|port\nfile=x/all|/a\n"
        CHECK(corpus_parse(ORIG, &p, err, sizeof err) == 0);
        CHECK(!strcmp(p.orig, "/o") && !strcmp(p.orig_by, "blitz2 1.95") && p.nconfigs == 3);
        corpus_select(&p, 0);
        CHECK(!p.record && p.nconfigs == 2 && !strcmp(p.configs[0].tag, "port")
              && !strcmp(p.configs[1].tag, "clib") && p.configs[0].expect == CORPUS_EXE);
        CHECK(p.nfiles == 2 && !strcmp(p.file_dest[0], "x/port") && !strcmp(p.file_dest[1], "x/all"));
        corpus_free(&p);
        CHECK(corpus_parse(ORIG, &p, err, sizeof err) == 0);
        corpus_select(&p, 1);
        CHECK(p.record && p.nconfigs == 1 && p.configs[0].expect == CORPUS_REF
              && !strcmp(p.configs[0].tag, "ref"));
        CHECK(p.nfiles == 2 && !strcmp(p.file_dest[0], "x/ref") && !strcmp(p.file_dest[1], "x/all"));
        corpus_free(&p);
        /* without the CD: the port (its libraries are missing) is not judged,
           blitz2:defaultdbug is left out; clib is there */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/no/dbug\n"
                           "config=ref|ref|/no/a|/no/b|/no/c\nconfig=port|exe|/|/no/b|/\n"
                           "config=clib|output|/|/|/\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p, 0);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && !strcmp(p.configs[0].tag, "clib") && !p.defaultdbug);
        corpus_free(&p);
        /* with runtime error checking blitz2:defaultdbug stays (the run is skipped) */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/no/dbug\n"
                           "runtime_errors=on\nconfig=ref|ref|/no/a|/no/b|/no/c\n"
                           "config=clib|output|/|/|/\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p, 0);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && p.defaultdbug && !strcmp(p.defaultdbug, "/no/dbug"));
        corpus_free(&p);
        /* none there: the configurations stay (the run is skipped) */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/d\n"
                           "config=ref|ref|/no/a|/no/b|/no/c\nconfig=port|exe|/no/a|/b|/c\n"
                           "config=clib|output|/no/a|/b|/c\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p, 0);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && !strcmp(p.configs[0].tag, "port"));
        corpus_free(&p);
        /* the recording is not touched */
        CHECK(corpus_parse(ORIG, &p, err, sizeof err) == 0);
        corpus_select(&p, 1);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && p.defaultdbug);
        corpus_free(&p);
        /* a pass without orig= is left alone */
        CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0);
        corpus_select(&p, 1);
        CHECK(!p.record && p.nconfigs == 2);
        corpus_free(&p);
        /* broken on purpose: a bug-orig test without orig=, orig= without expect=
           or with fixed= / regression=on */
        CHECK(parse(BASE CONFIGS "source=/s\nexpect=/e\n", err, sizeof err) == -1
              && strstr(err, "orig="));
        CHECK(parse(BASE CONFIGS "source=/s\norig=/o\n", err, sizeof err) == -1
              && strstr(err, "expect="));
        CHECK(parse(BASE "fixed=7\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\norig=/o\n",
                    err, sizeof err) == -1 && strstr(err, "orig="));
        CHECK(parse(BASE "regression=on\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n"
                    "orig=/o\n", err, sizeof err) == -1 && strstr(err, "orig="));
#undef ORIG
    }
    /* an expect test (an extension of blitz2, a port regression test): no
       reference, nothing of the CD, one configuration is enough */
    CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\nregression=on\n"
                       "config=clib|output|/a2|/b2|/c2\nsource=/s\nexpect=/e\n",
                       &p, err, sizeof err) == 0);
    CHECK(p.regression && !p.fixed && p.nconfigs == 1 && p.configs[0].expect == CORPUS_OUTPUT
          && !p.defaultdbug);
    corpus_free(&p);
    /* broken on purpose: a reference, no expectation, not on/off */
    CHECK(parse(BASE CONFIGS "source=/s\nexpect=/e\nregression=on\n", err, sizeof err) == -1
          && strstr(err, "without a reference"));
    CHECK(parse(BASE "regression=on\nconfig=clib|output|/a|/b|/c\nsource=/s\n", err, sizeof err)
          == -1 && strstr(err, "expect="));
    CHECK(parse(BASE "regression=yes\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n",
                err, sizeof err) == -1);
    /* a fixed bug-orig test: no reference, one configuration is enough */
    CHECK(corpus_parse(BASE "fixed=120\nconfig=clib|output|/a2|/b2|/c2\nsource=/s\nexpect=/e\n",
                       &p, err, sizeof err) == 0);
    CHECK(p.fixed == 120 && p.nconfigs == 1 && p.configs[0].expect == CORPUS_OUTPUT);
    corpus_free(&p);
    CHECK(parse(BASE "config=port|exe|/a|/b|/c\nconfig=clib|output|/a|/b|/c\nsource=/s\n"
                "expect=/e\nfixed=7\n", err, sizeof err) == 0);       /* fixed= after them */
    /* no blitz2:defaultdbug (nothing of the CD) */
    CHECK(parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\nfixed=7\n"
                "config=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n", err, sizeof err) == 0);
    CHECK(parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\n" CONFIGS "source=/s\nexpect=/e\n",
                err, sizeof err) == -1 && strstr(err, "defaultdbug"));
    /* broken on purpose: a reference, no expectation, regression, no number */
    CHECK(parse(BASE CONFIGS "source=/s\nexpect=/e\nfixed=120\n", err, sizeof err) == -1
          && strstr(err, "without a reference"));
    CHECK(parse(BASE "fixed=120\nconfig=clib|output|/a|/b|/c\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "expect="));
    CHECK(parse(BASE "fixed=120\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n"
                "regression=on\n", err, sizeof err) == -1 && strstr(err, "regression"));
    CHECK(parse(BASE "fixed=120\nsource=/s\nexpect=/e\n", err, sizeof err) == -1);
    CHECK(parse(BASE "fixed=#120\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n", err,
                sizeof err) == -1);
    CHECK(parse(BASE "fixed=0\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\n", err,
                sizeof err) == -1);
    /* without fixed= the reference is still needed */
    CHECK(parse(BASE "config=clib|output|/a|/b|/c\nconfig=x|output|/a|/b|/c\nsource=/s\n"
                "expect=/e\n", err, sizeof err) == -1 && strstr(err, "first"));

    /* the session script: on the A1200 with $RC and >> (unchanged), on
       Kickstart 1.x in AmigaDOS 1.3 syntax - the redirection right behind
       the command name, rcput instead of $RC and >> */
    {
        static const char want20[] =
            "Assign blitz2: CORPUS:blitz2\n"
            "Stack 40000\n"
            "CORPUS:wrap fail=3 blitz2:blitz2 CORPUS:jobs >CORPUS:out/blitz2.out\n"
            "Echo \"rc=$RC\" >>CORPUS:out/blitz2.out\n"
            "Assign blitz2:\n"
            "If EXISTS CORPUS:out/t01\n"
            "  CORPUS:run CORPUS:runwrap sig CORPUS:out/t01 a b >CORPUS:out/t01.run\n"
            "  Echo \"rc=$RC\" >>CORPUS:out/t01.run\n"
            "EndIf\n";
        static const char want13[] =
            "Assign blitz2: CORPUS:blitz2\n"
            "Stack 40000\n"
            "CORPUS:wrap >CORPUS:out/blitz2.out fail=3 blitz2:blitz2 CORPUS:jobs\n"
            "CORPUS:rcput CORPUS:out/blitz2.out\n"
            "Assign blitz2:\n"
            "If EXISTS CORPUS:out/t01\n"
            "  CORPUS:run >CORPUS:out/t01.run CORPUS:runwrap sig CORPUS:out/t01 a b\n"
            "  CORPUS:rcput CORPUS:out/t01.run\n"
            "EndIf\n";
        static const char want13plain[] =
            "Assign blitz2: CORPUS:blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 >CORPUS:out/blitz2.out CORPUS:jobs\n"
            "CORPUS:rcput CORPUS:out/blitz2.out\n"
            "Assign blitz2:\n"
            "If EXISTS CORPUS:out/t01\n"
            "  CORPUS:run >CORPUS:out/t01.run CORPUS:out/t01\n"
            "  CORPUS:rcput CORPUS:out/t01.run\n"
            "EndIf\n";
        char *t;

        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nwrap=/w|fail=3\nrunwrap=/r|sig\n"
                           "runargs=a b\nrcput=/rc\n", &p, err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want20));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nwrap=/w|fail=3\nrunwrap=/r|sig\n"
                           "runargs=a b\nrcput=/rc\nmachine=a500\n", &p, err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want13));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nrcput=/rc\nmachine=a500-68010\n", &p,
                           err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want13plain) && !strstr(t, "$RC") && !strstr(t, ">>"));
        free(t);
        corpus_free(&p);
    }

    /* compile only (run=off): no launcher, nothing of the runs, the script
       only compiles */
    {
        static const char want[] =
            "Assign blitz2: CORPUS:blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 CORPUS:jobs >CORPUS:out/blitz2.out\n"
            "Echo \"rc=$RC\" >>CORPUS:out/blitz2.out\n"
            "Assign blitz2:\n";
        static const char want13[] =
            "Assign blitz2: CORPUS:blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 >CORPUS:out/blitz2.out CORPUS:jobs\n"
            "CORPUS:rcput CORPUS:out/blitz2.out\n"
            "Assign blitz2:\n";
#define NORUN "work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\nrun=off\n" CONFIGS "source=/s/t01.bb2\n"
        char *t;

        CHECK(corpus_parse(NORUN, &p, err, sizeof err) == 0);
        CHECK(!p.run && !p.launcher);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(NORUN "machine=a500\nrcput=/rc\n", &p, err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want13));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0 && p.run);
        corpus_free(&p);
        CHECK(parse(BASE CONFIGS "source=/s\nrun=off\n", err, sizeof err) == -1
              && strstr(err, "run=off"));
        CHECK(parse(NORUN "gate=/g\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "serial=on\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "runwrap=/r\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "runargs=x\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "expect=/e\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "serial_same_as=/o\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "run=maybe\n", err, sizeof err) == -1);
        /* without run=off the launcher is still needed */
        CHECK(parse("work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\n" CONFIGS "source=/s\n", err,
                    sizeof err) == -1);
#undef NORUN
    }

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
