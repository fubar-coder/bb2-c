/*
 * test_oracletable.c - the oracle data of the TED tests written from a
 * fresh run of the original TED, with files built in memory: no notes
 * (the fresh run as it is), a kept note row, the parts of a note, and the
 * broken inputs (stale note, note row missing from the run, a note
 * without the original's hash or naming the row's own, malformed rows).
 */
#include "testutil.h"
#include "oracletable.h"

#define H_ORIG "3dcc3460e058bbf12447e463f2d24d8738c3cc35353625c0d265d37733e016d6"
#define H_PORT "1c18c4b7421e3a8c99497ead7fa8f8ca2638f45dd985e18ba4c47aa80a2032aa"
#define H_A    "5555555555555555555555555555555555555555555555555555555555555555"
#define H_B    "9999999999999999999999999999999999999999999999999999999999999999"
#define H_NEW  "0000000000000000000000000000000000000000000000000000000000000000"
#define NOTE   "#123: the port reads no byte behind the line (TED 1.1: " H_ORIG ")"

/* the original's run: three outputs of two sources */
#define FRESH H_A "  detok/X/A.BB2\n" H_ORIG "  save/X/A.BB2\n" H_B "  save/X/B.BB\n"

static char *out, *rep;

static int run(const char *fresh, const char *old)
{
    free(out);
    free(rep);
    return oracletable_merge(fresh, old, &out, &rep);
}

static void test_note_hash(void)
{
    char h[65];

    CHECK(oracletable_note_hash(NOTE, h) == 1 && !strcmp(h, H_ORIG));
    /* the first well formed part counts */
    CHECK(oracletable_note_hash("#1: (TED 1.1: x) (TED 1.1: " H_A ")", h) == 1 && !strcmp(h, H_A));
    CHECK(oracletable_note_hash("", h) == -1);
    CHECK(oracletable_note_hash("#: (TED 1.1: " H_A ")", h) == -1);
    CHECK(oracletable_note_hash("#12 (TED 1.1: " H_A ")", h) == -1);
    CHECK(oracletable_note_hash("123: (TED 1.1: " H_A ")", h) == -1);
    CHECK(oracletable_note_hash("#12: no hash", h) == -1);
    CHECK(oracletable_note_hash("#12: (TED 1.1: " H_A, h) == -1);
    CHECK(oracletable_note_hash("#12: (TED 1.1: 3DCC3460E058BBF12447E463F2D24D8738C3CC35353625C0D265D37733E016D6)",
                                h) == -1);
    CHECK(oracletable_note_hash("#12: (2.10: " H_A ")", h) == -1);
}

int main(void)
{
    test_note_hash();

    /* no notes: the fresh run as it is (a regenerated file equals the
       committed one) */
    CHECK(run(FRESH, FRESH) == 0 && !strcmp(out, FRESH) && strstr(rep, "0 note rows kept"));
    CHECK(run(FRESH, H_NEW "  save/X/A.BB2\n") == 0 && !strcmp(out, FRESH));
    CHECK(run(FRESH, "") == 0 && !strcmp(out, FRESH));

    /* a kept note row: the port's hash and the note stay, the other rows
       are the fresh run's */
    {
        static const char old[] = H_NEW "  detok/X/A.BB2\n"
                                  H_PORT "  save/X/A.BB2\t" NOTE "\n" H_B "  save/X/B.BB\n";
        static const char want[] = H_A "  detok/X/A.BB2\n"
                                   H_PORT "  save/X/A.BB2\t" NOTE "\n" H_B "  save/X/B.BB\n";

        CHECK(run(FRESH, old) == 0 && !strcmp(out, want));
        CHECK(strstr(rep, "save/X/A.BB2: note kept (#123)") && strstr(rep, "1 note rows kept"));
        /* again on its own output: nothing changes */
        CHECK(run(FRESH, want) == 0 && !strcmp(out, want));
    }

    /* stale: the original gives another hash than the note names */
    CHECK(run(H_A "  detok/X/A.BB2\n" H_B "  save/X/A.BB2\n" H_B "  save/X/B.BB\n",
              H_PORT "  save/X/A.BB2\t" NOTE "\n") == 1
          && !out && strstr(rep, "save/X/A.BB2: stale note"));
    /* a note row the run does not have */
    CHECK(run(FRESH, H_PORT "  save/X/C.BB2\t" NOTE "\n") == 1 && !out
          && strstr(rep, "save/X/C.BB2: note row, but not in the fresh run"));
    /* a note without the original's hash, one naming the row's own hash */
    CHECK(run(FRESH, H_PORT "  save/X/A.BB2\t#123: the port\n") == 1 && !out
          && strstr(rep, "the note is not"));
    CHECK(run(FRESH, H_ORIG "  save/X/A.BB2\t" NOTE "\n") == 1 && !out
          && strstr(rep, "own hash"));
    CHECK(run(FRESH, H_PORT "  save/X/A.BB2\t\n") == 1 && strstr(rep, "empty note"));
    CHECK(run(FRESH, H_PORT "  save/X/A.BB2\t" NOTE "\tx\n") == 1 && strstr(rep, "second tab"));

    /* malformed files */
    CHECK(run(FRESH, "xyz  save/X/A.BB2\n") == 1 && strstr(rep, "OLD: line 1"));
    CHECK(run(H_A " save/X/A.BB2\n", "") == 1 && strstr(rep, "NEW: line 1"));
    CHECK(run(H_A "  other/X/A.BB2\n", "") == 1 && strstr(rep, "NEW: line 1"));
    CHECK(run(H_A "  save/\n", "") == 1 && strstr(rep, "NEW: line 1"));
    CHECK(run(H_A "  save/X/A.BB2", "") == 1 && strstr(rep, "no LF"));
    CHECK(run(H_A "  save/X/A.BB2\n" H_B "  save/X/A.BB2\n", "") == 1 && strstr(rep, "twice"));
    CHECK(run(H_A "  save/X/A.BB2\t" NOTE "\n", "") == 1 && strstr(rep, "a note in the fresh run"));
    CHECK(run(FRESH, H_A "  save/X/A.BB2\n" H_B "  save/X/A.BB2\n") == 1 && strstr(rep, "OLD"));

    free(out);
    free(rep);
    return failures != 0;
}
