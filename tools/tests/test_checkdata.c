/*
 * test_checkdata.c - checkdata: the table, equal and differing bytes,
 * relocated longs of the reference skipped, zeros behind the initialised
 * part and in BSS, missing symbols and labels.
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

static int run(const char *table, const uint8_t *cdata, uint32_t csize, char *msg, size_t len)
{
    struct hunk ref = { HUNK_CODE, refcode, sizeof refcode, 1, &refreloc, &reftarget, 0, NULL, 0 };
    struct listing l = { NULL, 0, labels, 4, NULL, 0 };
    struct hunk_symbol data_syms[] = { { "_abcd", 5, 0 }, { "_ef", 3, 4 }, { "_ptr", 4, 6 } };
    struct hunk_symbol bss_syms[] = { { "_zero", 5, 0 } };
    struct hunk hunks[2] = {
        { HUNK_DATA, cdata, csize, 0, NULL, NULL, 3, data_syms, 0 },
        { HUNK_BSS, NULL, 16, 0, NULL, NULL, 1, bss_syms, 0 },
    };
    struct loadfile c = { NULL, 0, NULL, 2, 2, hunks };
    struct checkdata_table t;
    char err[200];
    FILE *out = tmpfile();
    int r;

    if (checkdata_table_parse(table, strlen(table), &t, err, sizeof err)) {
        snprintf(msg, len, "parse: %s", err);
        fclose(out);
        return -1;
    }
    r = (int)checkdata_run(&t, &l, &ref, &c, 0, out);
    rewind(out);
    msg[fread(msg, 1, len - 1, out)] = 0;
    fclose(out);
    checkdata_table_free(&t);
    return r;
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
    return failures != 0;
}
