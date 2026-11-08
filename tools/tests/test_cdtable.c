/*
 * test_cdtable.c - the CD result table written from a fresh run with
 * tables built in memory: a kept note, a dropped note (the port's hash is
 * the CD's again), a changed row without a note, the reference mode keeping
 * the port's row, notes naming an error of the CD's compiler, and the broken
 * inputs (stale note, note row missing, note without the CD's result, a
 * malformed error part, malformed tables).
 */
#include "testutil.h"
#include "cdtable.h"

#define HDR "path\tresult\tline\tmessage\tsize\tsha256_masked\txtra\tnote\n"
#define H_CD   "3dcc3460e058bbf12447e463f2d24d8738c3cc35353625c0d265d37733e016d6"
#define H_PORT "1c18c4b7421e3a8c99497ead7fa8f8ca2638f45dd985e18ba4c47aa80a2032aa"
#define H_NEW  "9999999999999999999999999999999999999999999999999999999999999999"
#define H_B    "5555555555555555555555555555555555555555555555555555555555555555"
#define NOTE   "#969: the port, Ds storage written as 0 (2.10: " H_CD ")"
#define ENOTE  "#940: the port, End Statement in a Function (2.10: error 383)"
#define CD_ERR "B.BB2\terror\t383\tToken Not Found : $3383. |   {$B383} 290:tm+1\t\t\town\t"
#define PORT_ERR "B.BB2\terror\t195\tIllegal End Procedure | End Statement\t\t\town\t"

static char *out, *rep;

static int run(enum cdtable_mode mode, const char *fresh, const char *old)
{
    int rc;

    free(out);
    free(rep);
    rc = cdtable_merge(mode, fresh, old, &out, &rep);
    return rc;
}

static void test_note_ref(void)
{
    char h[CDTABLE_REF_MAX];

    CHECK(cdtable_note_ref(NOTE, h) == 1 && !strcmp(h, H_CD));
    CHECK(cdtable_note_ref(ENOTE, h) == 1 && !strcmp(h, "error 383"));
    CHECK(cdtable_note_ref("#940: (2.10: error 1)", h) == 1 && !strcmp(h, "error 1"));
    CHECK(cdtable_note_ref("#940: (2.10: error 123456789)", h) == 1
          && !strcmp(h, "error 123456789"));
    /* the first well formed part counts */
    CHECK(cdtable_note_ref("#940: x (2.10: y) (2.10: error 12) (2.10: " H_CD ")", h) == 1
          && !strcmp(h, "error 12"));
    /* malformed error parts */
    CHECK(cdtable_note_ref("#940: (2.10: error)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error )", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 0)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 012)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 12x)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 12", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error -12)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error  12)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: Error 12)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 1234567890)", h) == -1);
    CHECK(cdtable_note_ref("#940: (2.10: error 12 Token Not Found)", h) == -1);
    CHECK(cdtable_note_ref("", h) == 0);
    CHECK(cdtable_note_ref("xtra error: Can't Load Resident", h) == 0);
    CHECK(cdtable_note_ref("#: (2.10: " H_CD ")", h) == 0);
    CHECK(cdtable_note_ref("#969 no colon", h) == 0);
    CHECK(cdtable_note_ref("#969: no hash", h) == -1);
    CHECK(cdtable_note_ref("#969: (2.10: " H_CD "00)", h) == -1);
    CHECK(cdtable_note_ref("#969: (2.10: 3DCC3460E058BBF12447E463F2D24D8738C3CC35353625C0D265D37733E016D6)", h) == -1);
    CHECK(cdtable_note_ref("#969: (2.10: 12)", h) == -1);
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

/* a source the CD's compiler refuses and the port refuses at another line
 * (#940: BALLBLITZ.BB2), and a source the CD compiles and the port refuses */
static void test_error_note(void)
{
    const char *old = HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n" PORT_ERR ENOTE "\n";
    const char *cdtab = HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n" CD_ERR "\n";
    const char *o2 = HDR "Q.BB2\terror\t86\tIllegal End Procedure | End Statement\t\t\town\t" NOTE
                         "\n";

    /* the unchanged tree: not a byte changes, in both modes */
    CHECK(run(CDTABLE_UPDATE, old, old) == 0 && !strcmp(out, old));
    CHECK(strstr(rep, "B.BB2: note kept"));
    CHECK(run(CDTABLE_REFERENCE, cdtab, old) == 0 && !strcmp(out, old));
    CHECK(strstr(rep, "B.BB2: note kept"));

    /* the port refuses it at a third line: the note stays */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "B.BB2\terror\t17\tSyntax Error | x\t\t\town\t\n", old) == 0);
    CHECK(strstr(out, "B.BB2\terror\t17\tSyntax Error | x\t\t\town\t" ENOTE "\n"));
    /* the port compiles it: the note stays */
    CHECK(run(CDTABLE_UPDATE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "B.BB2\tok\t\t\t5000\t" H_NEW "\town\t\n", old) == 0);
    CHECK(strstr(out, "B.BB2\tok\t\t\t5000\t" H_NEW "\town\t" ENOTE "\n"));

    /* the port refuses it at the CD's line again: the note is dropped */
    CHECK(run(CDTABLE_UPDATE, cdtab, old) == 0 && !strcmp(out, cdtab));
    CHECK(strstr(rep, "B.BB2: note dropped"));

    /* changed from the CD's error without a note: the report names the
     * error part the note needs */
    CHECK(run(CDTABLE_UPDATE, HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n" PORT_ERR "\n", cdtab)
          == 0);
    CHECK(strstr(rep, "B.BB2: changed without a note") && strstr(rep, "(2.10: error 383)"));

    /* the original refuses it at another line, or compiles it: stale */
    CHECK(run(CDTABLE_REFERENCE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "B.BB2\terror\t384\tToken Not Found : $3383. | x\t\t\town\t\n", old) == 1);
    CHECK(!out && strstr(rep, "stale note") && strstr(rep, "error 384"));
    CHECK(run(CDTABLE_REFERENCE,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "B.BB2\tok\t\t\t5000\t" H_NEW "\town\t\n", old) == 1);
    CHECK(!out && strstr(rep, "stale note"));
    /* a hash note where the original refuses the source: stale */
    CHECK(run(CDTABLE_REFERENCE, cdtab,
              HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n"
                  "B.BB2\tok\t\t\t5000\t" H_NEW "\town\t" NOTE "\n") == 1);
    CHECK(!out && strstr(rep, "stale note") && strstr(rep, "error 383"));

    /* the port refuses a source the CD compiled (a hash note on an error
     * row, #940's DARTS_POLY.BB): kept, dropped when the port gives the
     * CD's hash again */
    CHECK(run(CDTABLE_UPDATE, o2, o2) == 0 && !strcmp(out, o2));
    CHECK(run(CDTABLE_REFERENCE, HDR "Q.BB2\tok\t\t\t219000\t" H_CD "\town\t\n", o2) == 0
          && !strcmp(out, o2));
    CHECK(run(CDTABLE_UPDATE, HDR "Q.BB2\tok\t\t\t219000\t" H_CD "\town\t\n", o2) == 0
          && strstr(rep, "Q.BB2: note dropped"));
}

static void test_broken(void)
{
    const char *old = HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t" NOTE "\n";
    const char *plain = HDR "A.BB2\tok\t\t\t100\t" H_B "\town\t\n";

    /* a note row missing in the fresh table (a trial run) */
    CHECK(run(CDTABLE_UPDATE, plain, old) == 1);
    CHECK(!out && strstr(rep, "Q.BB2: note row not in the new table"));
    CHECK(run(CDTABLE_REFERENCE, plain, old) == 1 && !out);

    /* a note row without the CD's result */
    CHECK(run(CDTABLE_UPDATE, old,
              HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t#969: no hash\n") == 1);
    CHECK(!out && strstr(rep, "without the CD's result"));
    /* a note row with a malformed error part */
    CHECK(run(CDTABLE_UPDATE, old,
              HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t#940: x (2.10: error 38x)\n") == 1);
    CHECK(!out && strstr(rep, "without the CD's result") && strstr(rep, "error 38x"));
    CHECK(run(CDTABLE_REFERENCE, old,
              HDR "Q.BB2\tok\t\t\t219116\t" H_PORT "\town\t#940: x (2.10: error)\n") == 1);
    CHECK(!out && strstr(rep, "without the CD's result"));
    /* an error note row missing in the fresh table */
    CHECK(run(CDTABLE_UPDATE, plain, HDR PORT_ERR ENOTE "\n") == 1);
    CHECK(!out && strstr(rep, "B.BB2: note row not in the new table"));

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
    test_note_ref();
    test_update();
    test_reference();
    test_error_note();
    test_broken();
    free(out);
    free(rep);
    return failures != 0;
}
