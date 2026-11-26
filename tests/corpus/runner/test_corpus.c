/*
 * test_corpus.c - the pass file (valid and broken), the masking of the
 * trap #1 / trap #4 parameter blocks and the compare of the recorded
 * original (record=check: it reads the file and never writes it) (the
 * emulator part runs in the corpus_* ctests).
 */
#define _XOPEN_SOURCE 700       /* open_memstream */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "corpus.h"
#include "util.h"

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
#define NORUN0 "work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\nrun=off\n"

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

    /* exeinfo: the output is judged, the executable only compared for information;
       it has no BASE */
    CHECK(corpus_parse(BASE "config=ref|ref|/a|/b|/c\nconfig=port|exeinfo|/a2|/b|/c\n"
                       "source=/s\n", &p, err, sizeof err) == 0
          && p.configs[1].expect == CORPUS_EXEINFO && !p.configs[1].base);
    corpus_free(&p);
    CHECK(parse(BASE "config=ref|ref|/a|/b|/c\nconfig=port|exeinfo:ref|/a2|/b|/c\n"
                "source=/s\n", err, sizeof err) == -1);
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

    /* hang_after=: seconds, not given = 0; zero, a non-number or a negative one is refused */
    CHECK(corpus_parse(BASE CONFIGS "hang_after=20\nsource=/s\n", &p, err, sizeof err) == 0
          && p.hang_after == 20 && p.timeout == 300);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0 && p.hang_after == 0);
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "hang_after=0\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "hang_after needs a number > 0"));
    CHECK(parse(BASE CONFIGS "hang_after=soon\nsource=/s\n", err, sizeof err) == -1);
    CHECK(parse(BASE CONFIGS "hang_after=\nsource=/s\n", err, sizeof err) == -1);

    /* hang_from=run: the silence counts from the first program run; only with hang_after= and
       runs, only the value run */
    CHECK(corpus_parse(BASE CONFIGS "hang_after=8\nhang_from=run\nsource=/s\n", &p, err,
                       sizeof err) == 0 && p.hang_from_run == 1 && p.hang_after == 8);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "hang_after=8\nsource=/s\n", &p, err, sizeof err) == 0
          && p.hang_from_run == 0);
    corpus_free(&p);
    CHECK(parse(BASE CONFIGS "hang_from=run\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "hang_from=run needs hang_after="));
    CHECK(parse(BASE CONFIGS "hang_after=8\nhang_from=start\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "hang_from=run"));
    CHECK(parse("work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\nrun=off\n" CONFIGS
                "hang_after=8\nhang_from=run\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "hang_from=run needs hang_after="));

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
    CHECK(parse(BASE CONFIGS "rtg=yes\nsource=/s\n", err, sizeof err) == -1 && strstr(err, "rtg=on"));
    CHECK(parse(BASE CONFIGS "rtg=\nsource=/s\n", err, sizeof err) == -1);
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

    /* unjudged=TEST:CONFIG:NAME|REASON: an entry of this pass, its configuration and source */
#define UJ BASE CONFIGS "source=/s/t01_int.bb2\ntest=corpus_x\nexpect_dir=/e\n"
    CHECK(corpus_parse(UJ "unjudged=corpus_x:port:t01_int|why: both\n", &p, err, sizeof err) == 0);
    CHECK(p.nunjudged == 1 && !strcmp(p.unjudged[0].entry, "corpus_x:port:t01_int")
          && !strcmp(p.unjudged[0].config, "port") && !strcmp(p.unjudged[0].name, "t01_int")
          && !strcmp(p.unjudged[0].reason, "why: both") && !p.unjudged[0].seen);
    corpus_free(&p);
    CHECK(parse(UJ "unjudged=corpus_x:port:t01_int\n", err, sizeof err) == -1
          && strstr(err, "TEST:CONFIG:NAME|REASON"));
    CHECK(parse(UJ "unjudged=corpus_x:port:t01_int|\n", err, sizeof err) == -1
          && strstr(err, "TEST:CONFIG:NAME|REASON"));
    CHECK(parse(UJ "unjudged=corpus_x::t01_int|r\n", err, sizeof err) == -1
          && strstr(err, "TEST:CONFIG:NAME|REASON"));
    CHECK(parse(UJ "unjudged=corpus_x:port:t01:int|r\n", err, sizeof err) == -1
          && strstr(err, "TEST:CONFIG:NAME|REASON"));
    CHECK(parse(UJ "unjudged=corpus_y:port:t01_int|r\n", err, sizeof err) == -1
          && strstr(err, "not an entry of the pass corpus_x"));
    CHECK(parse(UJ "unjudged=corpus_x:clib:t01_int|r\n", err, sizeof err) == -1
          && strstr(err, "no configuration clib"));
    CHECK(parse(UJ "unjudged=corpus_x:port:t02_float|r\n", err, sizeof err) == -1
          && strstr(err, "no program t02_float"));
    CHECK(parse(UJ "unjudged=corpus_x:port:t01_int|r\nunjudged=corpus_x:port:t01_int|s\n", err,
                sizeof err) == -1 && strstr(err, "twice"));
    CHECK(parse(BASE CONFIGS "source=/s/t01_int.bb2\ntest=corpus_x\n"
                "unjudged=corpus_x:port:t01_int|r\n", err, sizeof err) == -1
          && strstr(err, "needs expect_dir="));
#undef UJ

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
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nlauncherargs=-harden\n", &p, err, sizeof err) == 0);
    CHECK(!strcmp(p.launcherargs, "-harden") && !p.runargs);
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
    CHECK(parse(BASE CONFIGS "source=/s\nlauncherargs=\n", err, sizeof err) == -1
          && strstr(err, "launcherargs"));
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
    /* bug-orig with the remote end on the serial port */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nserial=on\nexpect=/e\norig=/o\n", &p, err, sizeof err) == 0);
    CHECK(p.serial && !strcmp(p.expect, "/e"));
    corpus_free(&p);
    /* the RTG variant: off unless rtg=on, the last line wins */
    CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0 && !p.rtg);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nrtg=on\n", &p, err, sizeof err) == 0 && p.rtg);
    corpus_free(&p);
    CHECK(corpus_parse(BASE CONFIGS "source=/s\nrtg=on\nrtg=off\n", &p, err, sizeof err) == 0
          && !p.rtg);
    corpus_free(&p);
    /* the recorded original: the normal run drops the reference and the
       files only for it, the recording keeps nothing else */
    {
#define ORIG BASE CONFIGS "config=clib|output|/a3|/b3|/c3\nsource=/s\nexpect=/e\norig=/o\n" \
              "orig_by=blitz2 1.95\nfile=x/ref|/r|ref\nfile=x/port|/p|port\nfile=x/all|/a\n"
        CHECK(corpus_parse(ORIG, &p, err, sizeof err) == 0);
        CHECK(!strcmp(p.orig, "/o") && !strcmp(p.orig_by, "blitz2 1.95") && p.nconfigs == 3);
        corpus_select(&p);
        CHECK(!p.record && p.nconfigs == 2 && !strcmp(p.configs[0].tag, "port")
              && !strcmp(p.configs[1].tag, "clib") && p.configs[0].expect == CORPUS_EXE);
        CHECK(p.nfiles == 2 && !strcmp(p.file_dest[0], "x/port") && !strcmp(p.file_dest[1], "x/all"));
        corpus_free(&p);
        /* --observe is not for a bug-orig pass: refused, nothing dropped */
        CHECK(corpus_parse(ORIG, &p, err, sizeof err) == 0);
        CHECK(corpus_observe(&p, "clib", "/o", 0, err, sizeof err) == -1 && strstr(err, "orig=")
              && p.nconfigs == 3 && !p.observe_to);
        CHECK(corpus_observe(&p, "ref", "/o", 0, err, sizeof err) == -1 && strstr(err, "orig=")
              && p.nconfigs == 3 && !p.observe_to);
        corpus_free(&p);
        CHECK(corpus_parse(ORIG "record=check\n", &p, err, sizeof err) == 0);
        CHECK(corpus_observe(&p, "ref", "/o", 0, err, sizeof err) == -1 && strstr(err, "record=")
              && p.nconfigs == 3);
        corpus_free(&p);
        CHECK(corpus_parse(ORIG "record=check\n", &p, err, sizeof err) == 0);
        corpus_select(&p);
        CHECK(p.record == CORPUS_RECORD_CHECK && p.nconfigs == 1 && p.configs[0].expect == CORPUS_REF
              && !strcmp(p.configs[0].tag, "ref"));
        CHECK(p.nfiles == 2 && !strcmp(p.file_dest[0], "x/ref") && !strcmp(p.file_dest[1], "x/all"));
        corpus_free(&p);
        /* without the CD: the port (its libraries are missing) is not judged,
           blitz2:defaultdbug is left out; clib is there */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/no/dbug\n"
                           "config=ref|ref|/no/a|/no/b|/no/c\nconfig=port|exe|/|/no/b|/\n"
                           "config=clib|output|/|/|/\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && !strcmp(p.configs[0].tag, "clib") && !p.defaultdbug);
        corpus_free(&p);
        /* with runtime error checking too (the program links rundebuglib,
           never the module): left out, the run goes on */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/no/dbug\n"
                           "runtime_errors=on\nconfig=ref|ref|/no/a|/no/b|/no/c\n"
                           "config=clib|output|/|/|/\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && !p.defaultdbug);
        corpus_free(&p);
        /* none there: the configurations stay (the run is skipped) */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/d\n"
                           "config=ref|ref|/no/a|/no/b|/no/c\nconfig=port|exe|/no/a|/b|/c\n"
                           "config=clib|output|/no/a|/b|/c\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && !strcmp(p.configs[0].tag, "port"));
        corpus_free(&p);
        /* the compare test of the recorded original is not touched */
        CHECK(corpus_parse(ORIG "record=check\n", &p, err, sizeof err) == 0);
        corpus_select(&p);
        corpus_skip_missing(&p, NULL);
        CHECK(p.nconfigs == 1 && p.defaultdbug);
        corpus_free(&p);
        /* a pass without orig= is left alone */
        CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0);
        corpus_select(&p);
        CHECK(!p.record && p.nconfigs == 2);
        corpus_free(&p);
        /* broken on purpose: record= without orig=, another value */
        CHECK(parse(BASE CONFIGS "source=/s\nrecord=check\n", err, sizeof err) == -1
              && strstr(err, "record= needs orig="));
        CHECK(parse(ORIG "record=yes\n", err, sizeof err) == -1 && strstr(err, "record=check"));
        /* the recording run: record_to=, the layout with a larger Stack */
        CHECK(corpus_parse(ORIG "record=write\nrecord_to=/b/run1\n", &p, err, sizeof err) == 0);
        corpus_select(&p);
        CHECK(p.record == CORPUS_RECORD_WRITE && p.nconfigs == 1 && !strcmp(p.record_to, "/b/run1"));
        CHECK(corpus_layout(&p, NULL) == 0 && p.stack == 40000);
        corpus_free(&p);
        CHECK(corpus_parse(ORIG "record=write\nrecord_to=/b/run2\nlayout=stack\n", &p, err,
                           sizeof err) == 0);
        CHECK(corpus_layout(&p, NULL) == 0 && p.stack == 40000 + CORPUS_LAYOUT_STACK);
        {
            char *sc = corpus_script(&p);

            CHECK(sc && strstr(sc, "Stack 56384\n"));
            free(sc);
        }
        corpus_free(&p);
        /* broken on purpose: record=write without record_to=, record_to=
           without it, layout=stack without record=write, another layout */
        CHECK(parse(ORIG "record=write\n", err, sizeof err) == -1 && strstr(err, "record_to="));
        CHECK(parse(ORIG "record_to=/b/x\n", err, sizeof err) == -1 && strstr(err, "record_to="));
        CHECK(parse(ORIG "layout=stack\n", err, sizeof err) == -1 && strstr(err, "layout=stack"));
        CHECK(parse(ORIG "record=write\nrecord_to=/b/x\nlayout=big\n", err, sizeof err) == -1);
        /* broken on purpose: a bug-orig test without orig=, orig= without expect=
           or with fixed= / regression=on */
        CHECK(parse(BASE CONFIGS "source=/s\nexpect=/e\n", err, sizeof err) == -1
              && strstr(err, "orig="));
        CHECK(parse(BASE CONFIGS "source=/s\norig=/o\n", err, sizeof err) == -1
              && strstr(err, "expect="));
        /* a fixed test keeps its recorded original (only checked), the
           configurations stay */
        CHECK(corpus_parse(BASE "fixed=7\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\norig=/o\n",
                           &p, err, sizeof err) == 0);
        corpus_select(&p);
        corpus_skip_missing(&p, NULL);
        CHECK(p.fixed == 7 && p.nconfigs == 1 && !strcmp(p.configs[0].tag, "clib"));
        corpus_free(&p);
        CHECK(parse(BASE "fixed=7\nconfig=clib|output|/a|/b|/c\nsource=/s\nexpect=/e\norig=/o\n"
                    "record=check\n", err, sizeof err) == -1 && strstr(err, "record="));
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
    /* defaultdbug= is optional in every pass (the module is never part of
       a program); a pass without work= still fails */
    CHECK(parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\n" CONFIGS
                "source=/s\nexpect=/e\norig=/o\n", err, sizeof err) == 0);
    CHECK(parse("work=/w\nreport=/r\nted=/t\nlauncher=/x\nruntime_errors=on\n"
                "config=clib|ref|/a|/b|/c\nexpect_dir=/e\nsource=/s\n", err, sizeof err) == 0);
    CHECK(parse("report=/r\nted=/t\nlauncher=/x\n" CONFIGS "source=/s\nexpect=/e\norig=/o\n",
                err, sizeof err) == -1
          && strstr(err, "work, report, ted and launcher are needed"));
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
        /* launcherargs: behind the launcher, before runwrap (both scripts) */
        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nlauncherargs=-harden\nrunwrap=/r|sig\n"
                           "rcput=/rc\n", &p, err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && strstr(t, "CORPUS:run -harden CORPUS:runwrap sig CORPUS:out/t01 >CORPUS:out/t01.run"));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nlauncherargs=-harden\nrcput=/rc\n"
                           "machine=a500\n", &p, err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && strstr(t, "CORPUS:run >CORPUS:out/t01.run -harden CORPUS:out/t01"));
        free(t);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "source=/s/t01.bb2\nrcput=/rc\nmachine=a500-68010\n", &p,
                           err, sizeof err) == 0);
        t = corpus_script(&p);
        CHECK(t && !strcmp(t, want13plain) && !strstr(t, "$RC") && !strstr(t, ">>"));
        free(t);
        corpus_free(&p);
        /* the halves of corpusrun case (caserun.h): the compile and the runs,
           the one followed by the other is the script (Kickstart 2 and 1.3, with
           a gate, compile only) */
        {
            static const char *const passes[] = {
                BASE CONFIGS "source=/s/t01.bb2\nwrap=/w|fail=3\nrunwrap=/r|sig\nrunargs=a b\nrcput=/rc\n",
                BASE CONFIGS "source=/s/t01.bb2\nwrap=/w|fail=3\nrunwrap=/r|sig\nrunargs=a b\nrcput=/rc\n"
                             "machine=a500\n",
                BASE CONFIGS "source=/s/t01.bb2\nsource=/s/t02.bb2\ngate=/g\n",
                NORUN0 CONFIGS "source=/s/t01.bb2\n",
            };

            for (size_t k = 0; k < sizeof passes / sizeof passes[0]; k++) {
                char *c, *r, *both;

                CHECK(corpus_parse(passes[k], &p, err, sizeof err) == 0);
                t = corpus_script(&p);
                c = corpus_script_compile(&p);
                r = corpus_script_run(&p);
                both = c && r ? str_printf("%s%s", c, r) : NULL;
                CHECK(t && both && !strcmp(t, both));
                CHECK(c && strstr(c, "blitz2:blitz2") && !strstr(c, "CORPUS:run"));
                CHECK(r && !strstr(r, "blitz2:blitz2"));
                CHECK(r && (p.run ? strstr(r, "CORPUS:run") != NULL : !*r));
                free(t);
                free(c);
                free(r);
                free(both);
                corpus_free(&p);
            }
        }
        /* cell=on: one configuration and one source, nothing that judges */
        CHECK(corpus_parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\n"
                           "config=c0|ref|/b|/d|/a\nsource=/s/test.bb2\n", &p, err, sizeof err) == 0);
        CHECK(p.cell && p.nconfigs == 1);
        corpus_free(&p);
        CHECK(parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n", err,
                    sizeof err) == -1 && strstr(err, "cell=on: one configuration and one source"));
        CHECK(parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n"
                    "config=c1|output|/b|/d|/a\nsource=/s/test.bb2\n", err, sizeof err) == -1
              && strstr(err, "cell=on"));
        CHECK(parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n"
                    "source=/s/test.bb2\nexpect=/e\n", err, sizeof err) == -1 && strstr(err, "cell=on"));
        CHECK(parse("cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\n"
                    "source=/s/test.bb2\nmachine=a500\n", err, sizeof err) == -1 && strstr(err, "rcput"));
        CHECK(parse("cell=maybe\n", err, sizeof err) == -1 && strstr(err, "cell=on or cell=off"));
    }

    /* build=, build_job= (cell=on; change basic-test-local-data): the parts
       before the cell's compile, the jobs in the job lists, broken lines */
    {
#define CELL "cell=on\nwork=/w\nreport=/r\nted=/t\nlauncher=/l\nconfig=c0|ref|/b|/d|/a\nsource=/s/test.bb2\n"
        static const char one[] =
            "Assign blitz2: CORPUS:x.test/.build-1/blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 CORPUS:x.test/.build-1/jobs >CORPUS:x.test/.build-1/blitz2.out\n"
            "Echo \"rc=$RC\" >>CORPUS:x.test/.build-1/blitz2.out\n"
            "Assign blitz2:\n"
            "Assign blitz2: CORPUS:blitz2\n";
        static const char two13[] =
            "Assign blitz2: CORPUS:x.test/.build-1/blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 >CORPUS:x.test/.build-1/blitz2.out CORPUS:x.test/.build-1/jobs\n"
            "CORPUS:rcput CORPUS:x.test/.build-1/blitz2.out\n"
            "Assign blitz2:\n"
            "Assign blitz2: CORPUS:x.test/.build-2/blitz2\n"
            "Stack 40000\n"
            "blitz2:blitz2 >CORPUS:x.test/.build-2/blitz2.out CORPUS:x.test/.build-2/jobs\n"
            "CORPUS:rcput CORPUS:x.test/.build-2/blitz2.out\n"
            "Assign blitz2:\n"
            "Assign blitz2: CORPUS:blitz2\n";
        char *t, *r;

        /* none: the script halves as before */
        CHECK(corpus_parse(CELL, &p, err, sizeof err) == 0 && !p.nbuilds && !p.nbjobs);
        t = corpus_script_compile(&p);
        CHECK(t && !strncmp(t, "Assign blitz2: CORPUS:blitz2\n", 29));
        free(t);
        corpus_free(&p);
        /* one part and a job of the cell's list */
        CHECK(corpus_parse(CELL "build=x.test/.build-1|/o/blitz2|/o/def|/o/acid|off\n"
                           "build_job=x.test/.build-1|/t/b0.bb2|x.test/child\nbuild_job=-|/t/b1.bb2|x.test/sub/own\n",
                           &p, err, sizeof err) == 0);
        CHECK(p.nbuilds == 1 && !strcmp(p.builds[0].dir, "x.test/.build-1") && !p.builds[0].runtime_errors
              && !strcmp(p.builds[0].blitz2, "/o/blitz2") && !strcmp(p.builds[0].acidlibs, "/o/acid"));
        CHECK(p.nbjobs == 2 && !strcmp(p.bjobs[0].part, "x.test/.build-1") && !p.bjobs[1].part
              && !strcmp(p.bjobs[1].dest, "x.test/sub/own"));
        t = corpus_script_compile(&p);
        r = corpus_script_run(&p);
        CHECK(t && !strncmp(t, one, sizeof one - 1));
        CHECK(r && !strstr(r, "build"));
        free(t);
        free(r);
        corpus_free(&p);
        /* two parts on Kickstart 1.3, runtime error checking on */
        CHECK(corpus_parse(CELL "machine=a500\nrcput=/rc\nbuild=x.test/.build-1|/o/b|/o/d|/o/a|on\n"
                           "build=x.test/.build-2|/o/b|/o/d|/o/a|off\n", &p, err, sizeof err) == 0);
        CHECK(p.nbuilds == 2 && p.builds[0].runtime_errors && !p.builds[1].runtime_errors);
        t = corpus_script_compile(&p);
        CHECK(t && !strncmp(t, two13, sizeof two13 - 1));
        free(t);
        corpus_free(&p);
        /* broken lines */
        CHECK(parse(CELL "build=x.test/.b|/o/b|/o/d|/o/a\n", err, sizeof err) == -1
              && strstr(err, "build=DIR|BLITZ2|DEFLIBS|ACIDLIBS|on|off"));
        CHECK(parse(CELL "build=x.test/.b|/o/b|/o/d|/o/a|maybe\n", err, sizeof err) == -1
              && strstr(err, "build=DIR"));
        CHECK(parse(CELL "build=../b|/o/b|/o/d|/o/a|on\n", err, sizeof err) == -1 && strstr(err, "inside the volume"));
        CHECK(parse(CELL "build=x.test/.b||/o/d|/o/a|on\n", err, sizeof err) == -1);
        CHECK(parse(CELL "build=x.test/.b|/o/b|/o/d|/o/a|on|x\n", err, sizeof err) == -1);
        CHECK(parse(CELL "build=x.test/.b|/o/b|/o/d|/o/a|on\nbuild=X.test/.b|/o/b|/o/d|/o/a|on\n", err,
                    sizeof err) == -1 && strstr(err, "twice"));
        CHECK(parse(CELL "build_job=-|/t/b.bb2\n", err, sizeof err) == -1 && strstr(err, "build_job=PART|SRC|DEST"));
        CHECK(parse(CELL "build_job=-|/t/b.bb2|CORPUS:x\n", err, sizeof err) == -1);
        CHECK(parse(CELL "build_job=-||x.test/c\n", err, sizeof err) == -1);
        CHECK(parse(CELL "build_job=x.test/.b|/t/b.bb2|x.test/c\n", err, sizeof err) == -1
              && strstr(err, "no build=x.test/.b"));
        CHECK(parse(BASE CONFIGS "source=/s\nbuild_job=-|/t/b.bb2|x.test/c\n", err, sizeof err) == -1
              && strstr(err, "need cell=on"));
        CHECK(parse(BASE CONFIGS "source=/s\nbuild=x.test/.b|/o/b|/o/d|/o/a|on\n", err, sizeof err) == -1
              && strstr(err, "need cell=on"));
    }

    /* the staging of build= and build_job=: the part's compiler, its job
       list, the sources, the jobs of the cell's list */
    {
        char dir[] = "/tmp/test-corpus-build-XXXXXX";
        char *pt = NULL, *f;
        size_t len;

        CHECK(mkdtemp(dir) != NULL);
        f = str_printf("%s/in", dir);
        CHECK(f && mkdir_p(f) == 0);
        free(f);
        {
            const char *names[] = { "blitz2", "def", "acid", "ted", "drv.bb2", "b0.bb2", "b1.bb2", "ob2", "run", "rc" };

            for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
                f = str_printf("%s/in/%s", dir, names[i]);
                CHECK(f && write_file(f, names[i], strlen(names[i])) == 0);
                free(f);
            }
        }
        pt = str_printf("cell=on\nwork=%s/w\nreport=%s/w/r.txt\nted=%s/in/ted\nlauncher=%s/in/run\n"
                        "config=c0|ref|%s/in/blitz2|%s/in/def|%s/in/acid\nsource=%s/in/drv.bb2\n"
                        "build=x.test/.build-1|%s/in/ob2|%s/in/def|%s/in/acid|on\n"
                        "build_job=x.test/.build-1|%s/in/b0.bb2|x.test/child\nbuild_job=-|%s/in/b1.bb2|x.test/sub/own\n",
                        dir, dir, dir, dir, dir, dir, dir, dir, dir, dir, dir, dir, dir);
        CHECK(pt && corpus_parse(pt, &p, err, sizeof err) == 0);
        free(pt);
        CHECK(corpus_stage(&p, 0, stderr) == 0);
        {
            static const struct { const char *path, *text; } want[] = {
                { "x.test/.build-1/blitz2/blitz2", "ob2" }, { "x.test/.build-1/blitz2/ted", "ted" },
                { "x.test/.build-1/blitz2/deflibs", "def" }, { "x.test/.build-1/blitz2/acidlibs", "acid" },
                { "x.test/.build-1/src/b000.bb2", "b0.bb2" }, { "src/b001.bb2", "b1.bb2" },
                { "x.test/.build-1/jobs",
                  "log=CORPUS:x.test/.build-1/session.log\ndefaults=CORPUS:x.test/.build-1/defaults.xtra\n"
                  "job=b000|CORPUS:x.test/.build-1/src/b000.bb2|CORPUS:x.test/child\n" },
                { "jobs",
                  "log=CORPUS:out/session.log\ndefaults=CORPUS:out/defaults.xtra\nrunerr=off\n"
                  "job=j000|CORPUS:src/drv.bb2|CORPUS:out/drv\n"
                  "job=b001|CORPUS:src/b001.bb2|CORPUS:x.test/sub/own\n" },
            };

            for (size_t i = 0; i < sizeof want / sizeof want[0]; i++) {
                char *got;

                f = str_printf("%s/w/c0/vol/%s", dir, want[i].path);
                got = f ? (char *)read_file(f, &len) : NULL;
                if (!got || strcmp(got, want[i].text))
                    fprintf(stderr, "%s: %s\n", want[i].path, got ? got : "(none)");
                CHECK(got && !strcmp(got, want[i].text));
                free(got);
                free(f);
            }
            f = str_printf("%s/w/c0/vol/x.test/sub", dir);
            {
                struct stat st;

                CHECK(f && stat(f, &st) == 0 && S_ISDIR(st.st_mode));
            }
            free(f);
            f = str_printf("%s/w/c0/vol/x.test/.build-1/blitz2/userlibs", dir);
            {
                struct stat st;

                CHECK(f && stat(f, &st) == 0 && S_ISDIR(st.st_mode));
            }
            free(f);
        }
        corpus_free(&p);
        rm_tree(dir);
    }

    /* editrun=on (cell=on; change basic-test-editor-run): the run= line
       behind the job list, no run of the program in the second half, the
       refusals */
    {
        char dir[] = "/tmp/test-corpus-editrun-XXXXXX";
        char *pt = NULL, *f, *t, *r, *got;
        size_t len;

        CHECK(corpus_parse(CELL "editrun=on\nhang_after=8\nhang_from=run\n", &p, err, sizeof err) == 0);
        CHECK(p.editrun && p.cell);
        t = corpus_script_compile(&p);
        r = corpus_script_run(&p);
        CHECK(t && strstr(t, "blitz2:blitz2 CORPUS:jobs >CORPUS:out/blitz2.out"));
        CHECK(r && !strstr(r, "CORPUS:run") && !strstr(r, "test.run"));
        free(t);
        free(r);
        corpus_free(&p);
        /* the same pass without it runs the program from the shell */
        CHECK(corpus_parse(CELL, &p, err, sizeof err) == 0 && !p.editrun);
        r = corpus_script_run(&p);
        CHECK(r && strstr(r, "CORPUS:run CORPUS:out/test >CORPUS:out/test.run"));
        free(r);
        corpus_free(&p);
        CHECK(parse(CELL "editrun=on\nrunwrap=/r|guard\n", err, sizeof err) == -1
              && strstr(err, "editrun=on: the program runs from the editor; no runwrap="));
        CHECK(parse(CELL "editrun=on\nrunargs=a\n", err, sizeof err) == -1 && strstr(err, "no runargs="));
        CHECK(parse(CELL "editrun=on\nlauncherargs=-harden\n", err, sizeof err) == -1
              && strstr(err, "no launcherargs="));
        CHECK(parse(BASE CONFIGS "source=/s\neditrun=on\n", err, sizeof err) == -1
              && strstr(err, "editrun=on needs cell=on"));
        CHECK(parse(CELL "editrun=yes\n", err, sizeof err) == -1 && strstr(err, "editrun=on or editrun=off"));
        /* the job list: the jobs, the job lines, then the run */
        CHECK(mkdtemp(dir) != NULL);
        f = str_printf("%s/in", dir);
        CHECK(f && mkdir_p(f) == 0);
        free(f);
        {
            const char *names[] = { "blitz2", "def", "acid", "ted", "drv.bb2", "run" };

            for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
                f = str_printf("%s/in/%s", dir, names[i]);
                CHECK(f && write_file(f, names[i], strlen(names[i])) == 0);
                free(f);
            }
        }
        pt = str_printf("cell=on\neditrun=on\nruntime_errors=on\nwork=%s/w\nreport=%s/w/r.txt\nted=%s/in/ted\n"
                        "launcher=%s/in/run\nconfig=c0|ref|%s/in/blitz2|%s/in/def|%s/in/acid\nsource=%s/in/drv.bb2\n"
                        "jobline=progress=on\n", dir, dir, dir, dir, dir, dir, dir, dir);
        CHECK(pt && corpus_parse(pt, &p, err, sizeof err) == 0);
        free(pt);
        CHECK(corpus_stage(&p, 0, stderr) == 0);
        f = str_printf("%s/w/c0/vol/jobs", dir);
        got = f ? (char *)read_file(f, &len) : NULL;
        CHECK(got && !strcmp(got, "log=CORPUS:out/session.log\ndefaults=CORPUS:out/defaults.xtra\n"
                                  "job=j000|CORPUS:src/drv.bb2|CORPUS:out/drv\nprogress=on\n"
                                  "run=r001|CORPUS:src/drv.bb2|CORPUS:out/drv.con\n"));
        if (got && !strstr(got, "run=r001"))
            fprintf(stderr, "jobs: %s\n", got);
        free(got);
        free(f);
        corpus_free(&p);
        rm_tree(dir);
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
        CHECK(parse(NORUN "launcherargs=-harden\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        CHECK(parse(NORUN "expect=/e\n", err, sizeof err) == -1 && strstr(err, "run=off"));
        /* mask_traps=on / off, anything else is an error */
        CHECK(corpus_parse(NORUN "mask_traps=on\n", &p, err, sizeof err) == 0 && p.mask_traps);
        corpus_free(&p);
        CHECK(corpus_parse(NORUN "mask_traps=off\n", &p, err, sizeof err) == 0 && !p.mask_traps);
        corpus_free(&p);
        CHECK(corpus_parse(NORUN "mask_traps=1\n", &p, err, sizeof err) == -1
              && strstr(err, "mask_traps=on or mask_traps=off"));
        /* must_compile=on: only with run=off */
        CHECK(corpus_parse(NORUN "must_compile=on\n", &p, err, sizeof err) == 0 && p.must_compile);
        corpus_free(&p);
        CHECK(corpus_parse(NORUN, &p, err, sizeof err) == 0 && !p.must_compile);
        corpus_free(&p);
        CHECK(parse(BASE CONFIGS "source=/s\nmust_compile=on\n", err, sizeof err) == -1
              && strstr(err, "must_compile=on needs run=off"));
        CHECK(parse(NORUN "must_compile=1\n", err, sizeof err) == -1
              && strstr(err, "must_compile=on or must_compile=off"));
        /* the reference alone is enough for must_compile=on, not without */
        CHECK(corpus_parse("work=/w\nreport=/r\nted=/t\nrun=off\nmust_compile=on\nconfig=ref|ref|/a|/b|/c\nsource=/s\n",
                           &p, err, sizeof err) == 0);
        corpus_free(&p);
        CHECK(parse("work=/w\nreport=/r\nted=/t\nrun=off\nconfig=ref|ref|/a|/b|/c\nsource=/s\n", err,
                    sizeof err) == -1
              && strstr(err, "must_compile=on the reference alone"));
        /* the key serial_same_as is gone (the committed .serial files are compared) */
        CHECK(parse(NORUN "serial_same_as=/o\n", err, sizeof err) == -1 && strstr(err, "unknown key"));
        /* publish= and run_dir= */
        CHECK(corpus_parse(NORUN "publish=fx\nrun_dir=/r\n", &p, err, sizeof err) == 0 && !strcmp(p.publish, "fx") && !strcmp(p.run_dir, "/r"));
        corpus_free(&p);
        CHECK(parse(NORUN "run=maybe\n", err, sizeof err) == -1);
        /* without run=off the launcher is still needed */
        CHECK(parse("work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\n" CONFIGS "source=/s\n", err,
                    sizeof err) == -1);
#undef NORUN
    }

    /* expect_dir=: the expected outputs */
    {
        static const char out[] = "a\nb\nc\nrc=0\n";
        char what[300];
        FILE *log;
        char *logtext = NULL;
        size_t loglen = 0;

        CHECK(corpus_parse(BASE CONFIGS "config=clib|output|/a2|/b2|/c2\n"
                           "expect_dir=/nonexistent/expect/corpus\nsource=/s/t01_int.bb2\n",
                           &p, err, sizeof err) == 0);
        CHECK(!strcmp(p.expect_dir, "/nonexistent/expect/corpus") && !p.update);
        CHECK((n = corpus_expect_path(&p, 0, ".out"))
              && !strcmp(n, "/nonexistent/expect/corpus/t01_int.out"));
        free(n);
        /* a missing expected directory: named, and the run fails before any
           session (also without the emulator and the CD) */
        CHECK(corpus_load_expect(&p, err, sizeof err) == -1
              && strstr(err, "/nonexistent/expect/corpus: cannot read the directory"));
        CHECK((log = open_memstream(&logtext, &loglen)) != NULL);
        if (log) {
            CHECK(corpus_run(&p, log) == 2);
            fclose(log);
            CHECK(logtext && strstr(logtext, "/nonexistent/expect/corpus: cannot read"));
            free(logtext);
        }
        /* the update: only the configuration named runs, compared with
           nothing (a directory that does not exist yet is empty) */
        CHECK(corpus_update(&p, "nobody", NULL, 0, err, sizeof err) == -1
              && strstr(err, "nobody"));
        CHECK(corpus_update(&p, "clib", NULL, 0, err, sizeof err) == 0);
        CHECK(p.update == CORPUS_UPDATE_OUT && p.nconfigs == 1 && !strcmp(p.configs[0].tag, "clib")
              && p.configs[0].expect == CORPUS_OUTPUT && !p.configs[0].base
              && !strcmp(p.expected.pass, "corpus") && !p.expected.n);
        corpus_free(&p);
        CHECK(corpus_parse(BASE CONFIGS "config=clib|output|/a2|/b2|/c2\n"
                           "config=prev|exe:clib|/p|/q|/r\nexpect_dir=/e\nsource=/s\n",
                           &p, err, sizeof err) == 0);
        CHECK(corpus_update(&p, "clib", NULL, 0, err, sizeof err) == 0 && p.nconfigs == 1
              && !strcmp(p.configs[0].tag, "clib") && !strcmp(p.configs[0].blitz2, "/a2"));
        corpus_free(&p);
        /* exeinfo (port against the original): updates only write outputs, so the
           configuration runs alone like any other */
        CHECK(corpus_parse(BASE "config=ref|ref|/a|/b|/c\nconfig=port|exeinfo|/a2|/b|/c\n"
                           "expect_dir=/nonexistent/corpus\nsource=/s/t01_int.bb2\n", &p, err,
                           sizeof err) == 0);
        CHECK(corpus_update(&p, "port", NULL, 0, err, sizeof err) == 0
              && p.update == CORPUS_UPDATE_OUT && p.nconfigs == 1
              && !strcmp(p.configs[0].tag, "port") && p.nsources == 1);
        corpus_free(&p);
        /* the observation of one configuration; only some sources */
        CHECK(corpus_parse(BASE CONFIGS "config=clib|output|/a2|/b2|/c2\n"
                           "expect_dir=/e\nsource=/s/t01_int.bb2\nsource=/s/t02.bb2\n", &p, err,
                           sizeof err) == 0);
        CHECK(corpus_observe(&p, "none", "/o", 1, err, sizeof err) == -1 && strstr(err, "none"));
        CHECK(corpus_observe(&p, "ref", "/o", 1, err, sizeof err) == 0 && p.nconfigs == 1
              && !strcmp(p.configs[0].tag, "ref") && p.layout_stack
              && !strcmp(p.observe_to, "/o") && !strcmp(p.work, "/o.work")
              && !strcmp(p.report, "/o.txt"));
        {
            const char *only[] = { "t02" }, *bad[] = { "t03" };

            CHECK(corpus_only(&p, bad, 1, err, sizeof err) == -1
                  && strstr(err, "t03: no source of the pass"));
            CHECK(corpus_only(&p, only, 1, err, sizeof err) == 0 && p.nsources == 1
                  && !strcmp(p.sources[0], "/s/t02.bb2"));
        }
        corpus_free(&p);
        /* serial=on: NAME.serial too */
        CHECK(corpus_parse(BASE CONFIGS "serial=on\nexpect_dir=/nonexistent\nsource=/s/x.bb2\n",
                           &p, err, sizeof err) == 0);
        CHECK((n = corpus_expect_path(&p, 0, ".serial")) && !strcmp(n, "/nonexistent/x.serial"));
        free(n);
        corpus_free(&p);
        /* without expect_dir= there is nothing to update */
        CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0);
        CHECK(corpus_load_expect(&p, err, sizeof err) == 0);
        CHECK(corpus_update(&p, "port", NULL, 0, err, sizeof err) == -1
              && strstr(err, "expect_dir"));
        corpus_free(&p);
        /* broken: empty, compile only, a bug-orig test */
        CHECK(parse(BASE CONFIGS "source=/s\nexpect_dir=\n", err, sizeof err) == -1
              && strstr(err, "expect_dir"));
        CHECK(parse("work=/w\nreport=/r\nted=/t\ndefaultdbug=/d\nrun=off\n" CONFIGS
                    "source=/s\nexpect_dir=/e\n", err, sizeof err) == -1
              && strstr(err, "expect_dir"));
        CHECK(parse(BASE CONFIGS "source=/s\nexpect=/x\norig=/o\nexpect_dir=/e\n", err,
                    sizeof err) == -1 && strstr(err, "expect_dir"));

        /* the comparison: matching output */
        CHECK(corpus_expect_diff((const uint8_t *)out, strlen(out), (const uint8_t *)out,
                                 strlen(out), what, sizeof what) == 0);
        /* differing output: the first differing line */
        CHECK(corpus_expect_diff((const uint8_t *)out, strlen(out),
                                 (const uint8_t *)"a\nB\nc\nrc=0\n", 11, what, sizeof what) == 1
              && !strcmp(what, "line 2: expected \"b\", got \"B\""));
        CHECK(corpus_expect_diff((const uint8_t *)out, strlen(out),
                                 (const uint8_t *)"a\nb\nc\nrc=5\n", 11, what, sizeof what) == 1
              && !strcmp(what, "line 4: expected \"rc=0\", got \"rc=5\""));
        /* a line missing, a line too many, the LF at the end */
        CHECK(corpus_expect_diff((const uint8_t *)out, strlen(out), (const uint8_t *)"a\nb\n", 4,
                                 what, sizeof what) == 1
              && !strcmp(what, "line 3: expected \"c\", got the end"));
        CHECK(corpus_expect_diff((const uint8_t *)"a\n", 2, (const uint8_t *)"a\nb\n", 4, what,
                                 sizeof what) == 1
              && !strcmp(what, "line 2: expected the end, got \"b\""));
        CHECK(corpus_expect_diff((const uint8_t *)"a\n", 2, (const uint8_t *)"a", 1, what,
                                 sizeof what) == 1
              && !strcmp(what, "line 1: expected \"a\", got \"a\" (no LF)"));
        /* no output at all, an empty one, characters outside 32..126 */
        CHECK(corpus_expect_diff((const uint8_t *)out, strlen(out), NULL, 0, what,
                                 sizeof what) == 1 && !strcmp(what, "no output"));
        CHECK(corpus_expect_diff((const uint8_t *)"", 0, (const uint8_t *)"", 0, what,
                                 sizeof what) == 0);
        CHECK(corpus_expect_diff((const uint8_t *)"x\n", 2, (const uint8_t *)"\x05\"\xE4\0\n", 5,
                                 what, sizeof what) == 1
              && !strcmp(what, "line 1: expected \"x\", got \"\\x05\\x22\\xE4\\x00\""));
        /* a long line is cut */
        {
            char lw[200], lg[200];

            memset(lw, 'x', 150);
            lw[150] = '\n';
            memcpy(lg, lw, 151);
            lg[10] = 'y';
            CHECK(corpus_expect_diff((const uint8_t *)lw, 151, (const uint8_t *)lg, 151, what,
                                     sizeof what) == 1
                  && strstr(what, "line 1: expected \"xxxx") && strstr(what, "...\", got \"xxxxxxxxxxy")
                  && strlen(what) < 170);
        }
    }

    /* a pass of the configurations without the original (clib, prev): the
       reference alone is enough with expect_dir= */
    CHECK(corpus_parse(BASE "config=clib|ref|/a|/b|/c\nexpect_dir=/e\nsource=/s\n", &p, err,
                       sizeof err) == 0 && p.nconfigs == 1);
    corpus_free(&p);
    CHECK(parse(BASE "config=clib|ref|/a|/b|/c\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "another configuration"));
    CHECK(parse(BASE "expect_dir=/e\nsource=/s\n", err, sizeof err) == -1);

    /* the keys of the notes are gone (the pairs of tests/corpus/expect) */
    CHECK(parse(BASE CONFIGS "expect_dir=/e\nnotes=/n.tsv\nsource=/s\n", err, sizeof err) == -1
          && strstr(err, "unknown key notes"));

    /* floppy=N|SRC: parse, the emulator changes and the copy per configuration */
    {
        struct amiga_emulation_option ch[CORPUS_FLOPPY_CHANGES];
        char *paths[CORPUS_FLOPPIES], dir[] = "/tmp/corpus-floppy-XXXXXX";
        char *src, *want, *got, text[600];
        size_t len, nch, i;

        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=1\n", err, sizeof err) == -1
              && strstr(err, "floppy=N|SRC"));
        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=1|\n", err, sizeof err) == -1);
        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=4|/x\n", err, sizeof err) == -1);
        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=a|/x\n", err, sizeof err) == -1);
        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=10|/x\n", err, sizeof err) == -1);
        CHECK(parse(BASE CONFIGS "source=/s\nfloppy=1|/x\nfloppy=1|/y\n", err, sizeof err) == -1
              && strstr(err, "once per drive"));
        CHECK(corpus_parse(BASE CONFIGS "source=/s\n", &p, err, sizeof err) == 0);
        CHECK(corpus_floppy_changes(&p, 0, ch, paths) == 0 && !paths[0] && !paths[1]);
        corpus_free(&p);
        /* DF1 only: the drive is switched on, DF0 stays empty */
        CHECK(corpus_parse(BASE CONFIGS "source=/s\nfloppy=1|/img/a.adf\n", &p, err,
                           sizeof err) == 0);
        CHECK(!p.floppy[0] && !strcmp(p.floppy[1], "/img/a.adf"));
        nch = corpus_floppy_changes(&p, 1, ch, paths);
        CHECK(nch == 3 && !paths[0] && paths[1] && !strcmp(paths[1], "/w/port/emu/df1.adf"));
        CHECK(nch == 3 && !strcmp(ch[0].key, "floppy1type") && !strcmp(ch[0].value, "0")
              && !strcmp(ch[1].key, "floppy1") && ch[1].value == paths[1]
              && !strcmp(ch[2].key, "nr_floppies") && !strcmp(ch[2].value, "2"));
        for (i = 0; i < CORPUS_FLOPPIES; i++)
            free(paths[i]);
        corpus_free(&p);
        /* DF0 and DF3: four drives, no type for DF0 */
        CHECK(corpus_parse(BASE CONFIGS "source=/s\nfloppy=3|/c\nfloppy=0|/d\n", &p, err,
                           sizeof err) == 0);
        nch = corpus_floppy_changes(&p, 0, ch, paths);
        CHECK(nch == 4 && !strcmp(ch[0].key, "floppy0") && !strcmp(ch[1].key, "floppy3type")
              && !strcmp(ch[2].key, "floppy3") && !strcmp(ch[3].key, "nr_floppies")
              && !strcmp(ch[3].value, "4"));
        for (i = 0; i < CORPUS_FLOPPIES; i++)
            free(paths[i]);
        corpus_free(&p);

        /* the copy: one per configuration, in WORK/TAG/emu; the image itself stays */
        CHECK(mkdtemp(dir) != NULL);
        src = str_printf("%s/a.adf", dir);
        snprintf(text, sizeof text, "work=%s/w\nreport=/r\nted=/t\nlauncher=/x\ndefaultdbug=/d\n"
                 CONFIGS "source=/s\nfloppy=1|%s\n", dir, src);
        CHECK(write_file(src, "DISK", 4) == 0);
        CHECK(corpus_parse(text, &p, err, sizeof err) == 0);
        CHECK(corpus_floppy_stage(&p, 0, err, sizeof err) == -1 && strstr(err, "df1.adf"));
        CHECK((want = str_printf("%s/w/ref/emu", dir)) && mkdir_p(want) == 0);
        free(want);
        CHECK((want = str_printf("%s/w/port/emu", dir)) && mkdir_p(want) == 0);
        free(want);
        CHECK(corpus_floppy_stage(&p, 0, err, sizeof err) == 0);
        CHECK(corpus_floppy_stage(&p, 1, err, sizeof err) == 0);
        CHECK((want = corpus_floppy_path(&p, 0, 1)) != NULL);
        got = want ? (char *)read_file(want, &len) : NULL;
        CHECK(got && len == 4 && !memcmp(got, "DISK", 4));
        free(got);
        /* a write of the session reaches the copy only */
        CHECK(write_file(want, "WRIT", 4) == 0);
        free(want);
        CHECK((want = corpus_floppy_path(&p, 1, 1)) != NULL);
        got = (char *)read_file(want, &len);
        CHECK(got && len == 4 && !memcmp(got, "DISK", 4));
        free(got);
        free(want);
        got = (char *)read_file(src, &len);
        CHECK(got && len == 4 && !memcmp(got, "DISK", 4));
        free(got);
        /* a missing image: the stage fails and names it */
        CHECK(remove(src) == 0);
        CHECK(corpus_floppy_stage(&p, 0, err, sizeof err) == -1 && strstr(err, "the floppy image") && strstr(err, "a.adf"));
        corpus_free(&p);
        free(src);
        rm_tree(dir);
    }

    /* the expected data, read when the pass runs; the files of the test in
       a directory of its own */
    {
        char dir[] = "/tmp/corpus-test-XXXXXX";
        char *good, *bad, *src1, *src2, *dbug, *edir;
        char pass[2048];

        if (!mkdtemp(dir)) {
            perror("mkdtemp");
            return 1;
        }
        good = str_printf("%s/corpus/t01_int.out", dir);
        bad = str_printf("%s/corpus/t01_int.fixed-6.out", dir);
        edir = str_printf("%s/corpus", dir);
        src1 = str_printf("%s/t110_audio.bb2", dir);
        src2 = str_printf("%s/t01_int.bb2", dir);
        dbug = str_printf("%s/DEFAULTDBUG", dir);
        CHECK(mkdir_p(edir) == 0 && write_file(good, "1\n", 2) == 0);
        CHECK(write_file(src1, "LoadSound 0,\"BB2:sounds/x\"\n", 27) == 0);
        CHECK(write_file(src2, "NPrint 1\n", 9) == 0);

        snprintf(pass, sizeof pass, BASE CONFIGS "expect_dir=%s\nsource=%s\n", edir, src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        CHECK(corpus_load_expect(&p, err, sizeof err) == 0 && p.expected.n == 1);
        corpus_free(&p);
        /* serial=on: the serial file is missing */
        snprintf(pass, sizeof pass, BASE CONFIGS "serial=on\nexpect_dir=%s\nsource=%s\n", edir,
                 src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        CHECK(corpus_load_expect(&p, err, sizeof err) == -1
              && strstr(err, "/corpus/t01_int.serial (the expected output"));
        corpus_free(&p);
        /* malformed: an unqualified and a qualified file, named */
        CHECK(write_file(bad, "2\n", 2) == 0);
        snprintf(pass, sizeof pass, BASE CONFIGS "expect_dir=%s\nsource=%s\n", edir, src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        CHECK(corpus_load_expect(&p, err, sizeof err) == -1
              && strstr(err, "/corpus/t01_int.out and t01_int.fixed-6.out: an unqualified"));
        corpus_free(&p);
        /* a lone fixed file: malformed for the judge */
        unlink(good);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        CHECK(corpus_load_expect(&p, err, sizeof err) == -1
              && strstr(err, "t01_int.fixed-6.out: without its [orig] file"));
        corpus_free(&p);

        /* without the CD: no blitz2:defaultdbug, and the source that names
           BB2: is left out */
        snprintf(pass, sizeof pass, "work=/w\nreport=/r\nted=/t\nlauncher=/x\n"
                 "defaultdbug=%s\nconfig=clib|ref|/a|/b|/c\nexpect_dir=/e\n"
                 "mount=BB2|%s/no-cd\nmount=OTHER|%s\nsource=%s\nsource=%s\n", dbug, dir, dir,
                 src1, src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        corpus_skip_missing(&p, NULL);
        CHECK(!p.defaultdbug && p.nsources == 1 && !strcmp(p.sources[0], src2)
              && p.nmounts == 1 && !strcmp(p.mount_name[0], "OTHER"));
        corpus_free(&p);
        /* with runtime error checking the same: the module is left out, the
           run goes on */
        snprintf(pass, sizeof pass, "work=/w\nreport=/r\nted=/t\nlauncher=/x\nruntime_errors=on\n"
                 "defaultdbug=%s\nconfig=clib|ref|/a|/b|/c\nexpect_dir=/e\nsource=%s\n", dbug,
                 src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        corpus_skip_missing(&p, NULL);
        CHECK(!p.defaultdbug && p.nsources == 1);
        corpus_free(&p);
        /* the CD is there: nothing is left out */
        CHECK(write_file(dbug, "x", 1) == 0);
        snprintf(pass, sizeof pass, "work=/w\nreport=/r\nted=/t\nlauncher=/x\n"
                 "defaultdbug=%s\nconfig=clib|ref|/a|/b|/c\nexpect_dir=/e\n"
                 "mount=BB2|%s\nsource=%s\nsource=%s\n", dbug, dir, src1, src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        corpus_skip_missing(&p, NULL);
        CHECK(p.defaultdbug && p.nsources == 2 && p.nmounts == 1);
        corpus_free(&p);
        /* the update leaves everything (it needs the CD) */
        snprintf(pass, sizeof pass, "work=/w\nreport=/r\nted=/t\nlauncher=/x\n"
                 "defaultdbug=%s/none\nconfig=clib|ref|/a|/b|/c\nexpect_dir=/e\n"
                 "mount=BB2|%s/no-cd\nsource=%s\nsource=%s\n", dir, dir, src1, src2);
        CHECK(corpus_parse(pass, &p, err, sizeof err) == 0);
        CHECK(corpus_update(&p, "clib", NULL, 0, err, sizeof err) == 0);
        corpus_skip_missing(&p, NULL);
        CHECK(p.defaultdbug && p.nsources == 2 && p.nmounts == 1);
        corpus_free(&p);

        rm_tree(dir);
        free(good);
        free(bad);
        free(src1);
        free(src2);
        free(dbug);
        free(edir);
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

    /* record=check: the observation of the original against the recorded
       one, a read-only file that stays as it is (it is never opened for
       writing: that would fail); its wildcard lines match, a changed line
       is named, a missing or malformed file is an error */
    {
        static const char orig[] = "# the original\n[session]\ndone\n[run]\n"
                                   "# varies: an address\nptr=$*\nrc=0\n";
        char dir[] = "/tmp/corpus-record-XXXXXX", msg[600], *path, *bad, *back;
        struct stat before, after;
        size_t len;

        if (!mkdtemp(dir)) {
            perror("mkdtemp");
            return 1;
        }
        path = str_printf("%s/t.orig", dir);
        bad = str_printf("%s/bad.orig", dir);
        CHECK(write_file(path, orig, strlen(orig)) == 0 && chmod(path, 0444) == 0);
        CHECK(write_file(bad, "[run]\nx*\n", 9) == 0);
        CHECK(stat(path, &before) == 0);
        CHECK(corpus_record_check(path, "# the original\n[session]\ndone\n[run]\nptr=$1234\nrc=0\n",
                                  msg, sizeof msg) == 0);
        CHECK(corpus_record_check(path, "# the original\n[session]\ndone\n[run]\nptr=$1234\nrc=5\n",
                                  msg, sizeof msg) == 1);
        CHECK(!strcmp(msg, "[run] line 2: recorded \"rc=0\", observed \"rc=5\""));
        CHECK(corpus_record_check(path, "[session]\ntimeout\n[run]\nptr=$1\nrc=0\n", msg, sizeof msg) == 1);
        CHECK(!strncmp(msg, "[session] line 1:", 17));
        CHECK(stat(path, &after) == 0 && after.st_mtime == before.st_mtime
              && after.st_size == before.st_size && (after.st_mode & 0777) == 0444);
        back = (char *)read_file(path, &len);
        CHECK(back && len == strlen(orig) && !memcmp(back, orig, len));
        free(back);
        CHECK(corpus_record_check(bad, "[run]\nx\n", msg, sizeof msg) == 2
              && strstr(msg, "bad.orig:2: a wildcard line"));
        unlink(bad);
        CHECK(corpus_record_check(bad, "[run]\nx\n", msg, sizeof msg) == 2
              && strstr(msg, "the target bug-orig-record"));
        chmod(path, 0644);
        unlink(path);
        rmdir(dir);
        free(path);
        free(bad);
    }
    return failures != 0;
}
