/*
 * test_cdtable.c - the CD result table written from a fresh run with
 * tables built in memory: a kept note, a dropped note (the port's hash is
 * the CD's again), a changed row without a note, the reference mode keeping
 * the port's row, and the broken inputs (stale note, note row missing,
 * note without a hash, malformed tables).
 */
#include "testutil.h"
#include "cdtable.h"

#define HDR "path\tresult\tline\tmessage\tsize\tsha256_masked\txtra\tnote\n"
#define H_CD   "3dcc3460e058bbf12447e463f2d24d8738c3cc35353625c0d265d37733e016d6"
#define H_PORT "1c18c4b7421e3a8c99497ead7fa8f8ca2638f45dd985e18ba4c47aa80a2032aa"
#define H_NEW  "9999999999999999999999999999999999999999999999999999999999999999"
#define H_B    "5555555555555555555555555555555555555555555555555555555555555555"
#define NOTE   "#969: the port, Ds storage written as 0 (2.10: " H_CD ")"

static char *out, *rep;

static int run(enum cdtable_mode mode, const char *fresh, const char *old)
{
    int rc;

    free(out);
    free(rep);
    rc = cdtable_merge(mode, fresh, old, &out, &rep);
    return rc;
}

static void test_note_hash(void)
{
    char h[65];

    CHECK(cdtable_note_hash(NOTE, h) == 1 && !strcmp(h, H_CD));
    CHECK(cdtable_note_hash("", h) == 0);
    CHECK(cdtable_note_hash("xtra error: Can't Load Resident", h) == 0);
    CHECK(cdtable_note_hash("#: (2.10: " H_CD ")", h) == 0);
    CHECK(cdtable_note_hash("#969 no colon", h) == 0);
    CHECK(cdtable_note_hash("#969: no hash", h) == -1);
    CHECK(cdtable_note_hash("#969: (2.10: " H_CD "00)", h) == -1);
    CHECK(cdtable_note_hash("#969: (2.10: 3DCC3460E058BBF12447E463F2D24D8738C3CC35353625C0D265D37733E016D6)", h) == -1);
    CHECK(cdtable_note_hash("#969: (2.10: 12)", h) == -1);
}

static void test_update(void)
{
    const char *old = HDR
        "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
        "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t" NOTE "\n"
        "X.BB2\tok\t\t\t7\t" H_B "\town\txtra error: x\n";

    /* the unchanged tree: not a byte changes */
    CHECK(run(CDTABLE_UPDATE, old, old) == 0 && !strcmp(out, old));
    CHECK(strstr(rep, "Q.BB2: note kept") && !strstr(rep, "changed without"));

    /* the fresh row of the note row carries no note: the old note stays */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t\n"
                  "X.BB2\tok\t\t\t7\t" H_B "\town\txtra error: x\n", old) == 0);
    CHECK(!strcmp(out, old));

    /* a kept note, the port's value changed again (another fix) */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219120\t" H_NEW "\town\t\n"
                  "X.BB2\tok\t\t\t7\t" H_B "\town\txtra error: x\n", old) == 0);
    CHECK(strstr(out, "Q.BB2\tok\t\t\t219120\t" H_NEW "\town\t" NOTE "\n"));
    CHECK(strstr(rep, "Q.BB2: note kept"));

    /* a dropped note: the port's executable is the CD's again */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219124\t" H_CD "\town\t\n"
                  "X.BB2\tok\t\t\t7\t" H_B "\town\txtra error: x\n", old) == 0);
    CHECK(strstr(out, "Q.BB2\tok\t\t\t219124\t" H_CD "\town\t\n"));
    CHECK(!strstr(out, "#969") && strstr(rep, "Q.BB2: note dropped"));
    /* and the next run keeps it dropped */
    {
        char *dropped = strdup(out);

        CHECK(run(CDTABLE_UPDATE, dropped, dropped) == 0 && !strcmp(out, dropped));
        CHECK(!strstr(out, "#969") && !strstr(rep, "note"));
        free(dropped);
    }

    /* a row that changed without a note is written and reported with the
     * hash the note must name */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t101\t" H_NEW "\town\t\n"
                  "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t\n"
                  "X.BB2\tok\t\t\t7\t" H_B "\town\txtra error: x\n", old) == 0);
    CHECK(strstr(out, "A.BB2\tok\t\t\t101\t" H_NEW "\town\t\n"));
    CHECK(strstr(rep, "A.BB2: changed without a note") && strstr(rep, "(2.10: " H_B ")"));

    /* a row that is new, a row that is gone */
    CHECK(run(CDTABLE_UPDATE,
              HDR "B.BB2\tok\t\t\t1\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t\n", old) == 0);
    CHECK(strstr(rep, "B.BB2: new row") && strstr(rep, "A.BB2: row removed"));
    CHECK(!strstr(out, "A.BB2") && strstr(out, "B.BB2"));
}

static void test_reference(void)
{
    const char *old = HDR
        "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
        "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t" NOTE "\n";
    /* the original's run: Q.BB2 has the CD's hash, A.BB2 changed */
    const char *fresh = HDR
        "A.BB2\tok\t\t\t102\t" H_NEW "\town\t\n"
        "Q.BB2\tok\t\t\t219000\t" H_CD "\town\t\n";

    CHECK(run(CDTABLE_REFERENCE, fresh, old) == 0);
    CHECK(!strcmp(out, HDR "A.BB2\tok\t\t\t102\t" H_NEW "\town\t\n"
                           "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t" NOTE "\n"));
    CHECK(strstr(rep, "Q.BB2: note kept"));

    /* the original's run of the unchanged CD gives the table back */
    CHECK(run(CDTABLE_REFERENCE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219000\t" H_CD "\town\t\n", old) == 0);
    CHECK(!strcmp(out, old));

    /* a stale note: the original's hash is not the one the note names */
    CHECK(run(CDTABLE_REFERENCE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "Q.BB2\tok\t\t\t219000\t" H_NEW "\town\t\n", old) == 1);
    CHECK(!out && strstr(rep, "stale note") && strstr(rep, "Q.BB2"));
}

static void test_broken(void)
{
    const char *old = HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t" NOTE "\n";
    const char *plain = HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n";

    /* a note row missing in the fresh table (a trial run) */
    CHECK(run(CDTABLE_UPDATE, plain, old) == 1);
    CHECK(!out && strstr(rep, "Q.BB2: note row not in the new table"));
    CHECK(run(CDTABLE_REFERENCE, plain, old) == 1 && !out);

    /* a note row without the CD's hash */
    CHECK(run(CDTABLE_UPDATE, old,
              HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t#969: no hash\n") == 1);
    CHECK(!out && strstr(rep, "without the CD's hash"));

    /* malformed tables: header, field count, a path twice, empty, tab in note */
    CHECK(run(CDTABLE_UPDATE, "path\tresult\n", old) == 1 && !out);
    CHECK(run(CDTABLE_UPDATE, HDR "A.BB2\tok\t\t\t100\n", plain) == 1);
    CHECK(strstr(rep, "new table: line 2"));
    CHECK(run(CDTABLE_UPDATE, plain, HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\tx\n") == 1);
    CHECK(strstr(rep, "old table: line 2"));
    CHECK(run(CDTABLE_UPDATE, plain,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\nA.BB2\tok\t\t\t100\t" H_B "\town\t\n") == 1);
    CHECK(strstr(rep, "A.BB2: twice"));
    CHECK(run(CDTABLE_UPDATE, "", plain) == 1 && strstr(rep, "empty"));
}

int main(void)
{
    test_note_hash();
    test_update();
    test_reference();
    test_broken();
    free(out);
    free(rep);
    return failures != 0;
}
