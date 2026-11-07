/*
 * test_checkdata.c - checkdata: the table, equal and differing bytes,
 * relocated longs of the reference skipped, zeros behind the initialised
 * part and in BSS, missing symbols and labels; the list of intended
 * differences (programs/ted/checkdata-intended.txt): correct, stale, an
 * unlisted difference, broken lists.
 */
#include "testutil.h"
#include "checkdata.h"

/* reference code hunk: "ABCD" at 0, "EF" + 2 zeros at 4, a relocated
   long at 8 */
static const uint8_t refcode[12] = { 'A', 'B', 'C', 'D', 'E', 'F', 0, 0, 1, 2, 3, 4 };
static uint32_t refreloc = 8, reftarget = 0;

static struct listing_symbol labels[] = {
    { "abcd", 0 }, { "ef", 4 }, { "efend", 8 }, { "ptr", 8 },
};

/* the checks of table, with the intended differences of intended (NULL:
   none); -1 = table or list does not parse (message in msg) */
static int run_intended(const char *table, const char *intended, const uint8_t *cdata,
                        uint32_t csize, char *msg, size_t len)
{
    struct hunk ref = { HUNK_CODE, refcode, sizeof refcode, 1, &refreloc, &reftarget, 0, NULL, 0, 0 };
    struct listing l = { NULL, 0, labels, 4, NULL, 0 };
    struct hunk_symbol data_syms[] = { { "_abcd", 5, 0 }, { "_ef", 3, 4 }, { "_ptr", 4, 6 } };
    struct hunk_symbol bss_syms[] = { { "_zero", 5, 0 } };
    struct hunk hunks[2] = {
        { HUNK_DATA, cdata, csize, 0, NULL, NULL, 3, data_syms, 0, 0 },
        { HUNK_BSS, NULL, 16, 0, NULL, NULL, 1, bss_syms, 0, 0 },
    };
    struct loadfile c = { NULL, 0, NULL, 2, 2, hunks };
    struct checkdata_table t;
    struct checkdata_intended in = { NULL, 0 };
    char err[200];
    FILE *out = tmpfile();
    int r;

    if (checkdata_table_parse(table, strlen(table), &t, err, sizeof err)) {
        snprintf(msg, len, "parse: %s", err);
        fclose(out);
        return -1;
    }
    if (intended && checkdata_intended_parse(intended, strlen(intended), &in, err, sizeof err)) {
        snprintf(msg, len, "intended: %s", err);
        checkdata_table_free(&t);
        fclose(out);
        return -1;
    }
    r = (int)checkdata_run(&t, &l, &ref, &c, intended ? &in : NULL, 0, out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    checkdata_table_free(&t);
    checkdata_intended_free(&in);
    return r;
}

static int run(const char *table, const uint8_t *cdata, uint32_t csize, char *msg, size_t len)
{
    return run_intended(table, NULL, cdata, csize, msg, len);
}

int main(void)
{
    /* C data: ABCD, EF (only 6 bytes initialised: zeros behind), then the
       pointer at 6 with other bytes than the reference */
    static const uint8_t good[8] = { 'A', 'B', 'C', 'D', 'E', 'F', 9, 9 };
    static const uint8_t bad[8] = { 'A', 'X', 'C', 'Y', 'E', 'F', 9, 9 };
    static const char table[] =
        "# comment\n"
        "\n"
        "_abcd\tabcd\t4\tsize\n"
        "_ef\tef\tefend\tend label, zeros behind the data\n"
        "_ptr\tptr\t4\trelocated: skipped\n"
        "_zero\tef\t2\tBSS is zero\n";
    char msg[1000];

    CHECK(run(table, good, 6, msg, sizeof msg) == 1);       /* BSS != "EF" */
    CHECK(strstr(msg, "FAIL    _zero          ef: 2 of 2 bytes differ (first at +0) - BSS is zero\n")
          != NULL);
    CHECK(run("_abcd\tabcd\t4\n_ef\tef\tefend\n_ptr\tptr\t4\n", good, 6, msg, sizeof msg) == 0);
    CHECK(strstr(msg, "checkdata: 3 checks, 0 failed") != NULL);

    /* differing bytes: count and first */
    CHECK(run("_abcd\tabcd\t4\tn\n", bad, 8, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "FAIL    _abcd          abcd: 2 of 4 bytes differ (first at +1) - n\n") != NULL);
    /* zeros behind the initialised part: "EF\0\0" against data ending at 6 */
    CHECK(run("_ef\tef\tefend\n", good, 6, msg, sizeof msg) == 0);
    CHECK(run("_ef\tef\tefend\n", good, 8, msg, sizeof msg) == 1);
    /* missing symbol, missing label */
    CHECK(run("_nope\tabcd\t4\tx\n", good, 8, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "MISSING _nope          (abcd, x)\n") != NULL);
    CHECK(run("_abcd\tabcd\tnolabel\n", good, 8, msg, sizeof msg) == 1);
    CHECK(strstr(msg, "abcd/nolabel: label not in the listing (table line 1)") != NULL);
    /* broken table */
    CHECK(run("_abcd\tabcd\n", good, 8, msg, sizeof msg) == -1);
    CHECK(strstr(msg, "line 1: expected") != NULL);

    /* intended differences: _abcd differs (2 bytes), _ef is equal, _zero
       always differs (BSS against "EF") */
    {
        static const char t2[] = "_abcd\tabcd\t4\tn\n_ef\tef\tefend\tm\n";
        static const char t3[] = "_abcd\tabcd\t4\tn\n_zero\tef\t2\tz\n";

        /* correct: the difference of _abcd is listed */
        CHECK(run_intended(t2, "# fixed original bugs\n\n_abcd\t#123\tthe fix of #123\n", bad, 6,
                           msg, sizeof msg) == 0);
        CHECK(strstr(msg, "intended _abcd         abcd: 2 of 4 bytes differ (first at +1) - #123\n")
              != NULL);
        CHECK(strstr(msg, "checkdata: 2 checks, 0 failed, 1 intended\n") != NULL);
        /* the note is optional; an empty list changes nothing */
        CHECK(run_intended(t2, "_abcd\t#123\n", bad, 6, msg, sizeof msg) == 0);
        CHECK(run_intended(t2, "", good, 6, msg, sizeof msg) == 0);
        CHECK(strstr(msg, "checkdata: 2 checks, 0 failed, 0 intended\n") != NULL);
        /* broken: a stale entry - the check is equal (the fix is gone) */
        CHECK(run_intended(t2, "_abcd\t#123\n", good, 6, msg, sizeof msg) == 1);
        CHECK(strstr(msg, "STALE   _abcd          intended difference (#123) not found "
                          "(intended line 1)\n") != NULL);
        CHECK(run_intended(t2, "_abcd\t#123\n_ef\t#7\n", bad, 6, msg, sizeof msg) == 1);
        CHECK(strstr(msg, "STALE   _ef            intended difference (#7)") != NULL);
        /* broken: a stale entry - no check has the symbol */
        CHECK(run_intended(t2, "_abcd\t#123\n_nope\t#8\n", bad, 6, msg, sizeof msg) == 1);
        CHECK(strstr(msg, "STALE   _nope") != NULL);
        /* broken: a difference the list does not name */
        CHECK(run_intended(t3, "_abcd\t#123\n", bad, 6, msg, sizeof msg) == 1);
        CHECK(strstr(msg, "FAIL    _zero          ef: 2 of 2 bytes differ") != NULL);
        CHECK(strstr(msg, "checkdata: 2 checks, 1 failed, 1 intended\n") != NULL);
        CHECK(run_intended(t2, "", bad, 6, msg, sizeof msg) == 1);
        /* a missing symbol is no intended difference */
        CHECK(run_intended("_nope\tabcd\t4\tx\n", "_nope\t#9\n", good, 8, msg, sizeof msg) == 2);
        CHECK(strstr(msg, "MISSING _nope") != NULL && strstr(msg, "STALE   _nope") != NULL);
        /* broken lists */
        CHECK(run_intended(t2, "_abcd\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(strstr(msg, "intended: line 1: expected CSYM #ISSUE [NOTE]") != NULL);
        CHECK(run_intended(t2, "_abcd\t123\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(run_intended(t2, "_abcd\t#12x\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(run_intended(t2, "_abcd\t#\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(run_intended(t2, "_abcd\t#0\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(run_intended(t2, "\t#1\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(run_intended(t2, "# c\n_abcd\t#1\n_abcd\t#2\n", bad, 6, msg, sizeof msg) == -1);
        CHECK(strstr(msg, "intended: line 3: _abcd listed twice (line 2)") != NULL);
    }
    return failures != 0;
}
