/*
 * test_hunk.c - load_parse: structure, relocations, errors.
 */
#include "testutil.h"

int main(void)
{
    struct buf code = { 0 }, f, g = { 0 };
    struct loadfile lf;
    char err[200];
    uint32_t relocs[2] = { 0, 4 };

    put32(&code, 0x70004E75);
    put32(&code, 0x12345678);
    put16(&code, 0x4E75);               /* 10 bytes: padded to 12 */

    /* one code hunk, two relocations */
    f = loadfile1(1, HUNK_CODE, &code, relocs, 2, 0);
    CHECK(load_parse(f.p, f.n, &lf, err, sizeof err) == 0);
    CHECK(lf.table_size == 1 && lf.nhunks == 1);
    CHECK(lf.hunks[0].type == HUNK_CODE && lf.hunks[0].size == 12);
    CHECK(lf.hunks[0].nrelocs == 2 && lf.hunks[0].reloc_offset[1] == 4);
    CHECK(be32(lf.hunks[0].data) == 0x70004E75);
    load_free(&lf);

    /* memory flags in the type are ignored */
    f.p[28] |= 0x40;                    /* HUNK_CODE | MEMF_CHIP */
    CHECK(load_parse(f.p, f.n, &lf, err, sizeof err) == 0 && lf.hunks[0].type == HUNK_CODE);
    load_free(&lf);

    /* truncated */
    CHECK(load_parse(f.p, f.n - 8, &lf, err, sizeof err) == -1);
    CHECK(strstr(err, "ends inside") != NULL);

    /* not a load file */
    CHECK(load_parse(code.p, code.n, &lf, err, sizeof err) == -1);
    CHECK(strstr(err, "not a load file") != NULL);

    /* RELOC32SHORT (word format, padded) and a symbol hunk are read */
    put32(&g, HUNK_HEADER); put32(&g, 0); put32(&g, 1); put32(&g, 0); put32(&g, 0); put32(&g, 2);
    put32(&g, HUNK_CODE); put32(&g, 2); put32(&g, 0); put32(&g, 0);
    put32(&g, HUNK_RELOC32SHORT); put16(&g, 2); put16(&g, 0); put16(&g, 2); put16(&g, 4);
    put16(&g, 0);                       /* 5 words: */
    put16(&g, 0);                       /* padding to a long */
    put32(&g, HUNK_SYMBOL);
    put32(&g, 1); putbytes(&g, "abc", 4); put32(&g, 4);             /* padded name */
    put32(&g, 2); putbytes(&g, "longname", 8); put32(&g, 6);        /* no 0 byte */
    put32(&g, 0);
    put32(&g, HUNK_END);
    CHECK(load_parse(g.p, g.n, &lf, err, sizeof err) == 0);
    CHECK(lf.nhunks == 1 && lf.hunks[0].nrelocs == 2 && lf.hunks[0].reloc_offset[1] == 4);
    CHECK(lf.hunks[0].nsyms == 2 && lf.hunks[0].syms[1].len == 8);
    {
        uint32_t hunk = 9, value = 9;

        CHECK(load_symbol(&lf, "abc", &hunk, &value) == 0 && hunk == 0 && value == 4);
        CHECK(load_symbol(&lf, "longname", &hunk, &value) == 0 && value == 6);
        CHECK(load_symbol(&lf, "ab", &hunk, &value) == -1);
        CHECK(load_symbol(&lf, "longnam", &hunk, &value) == -1);
    }
    load_free(&lf);

    /* unknown hunk type */
    g.p[g.n - 1] = 0x99;
    CHECK(load_parse(g.p, g.n, &lf, err, sizeof err) == -1);
    CHECK(strstr(err, "unknown hunk type") != NULL);

    free(code.p);
    free(f.p);
    free(g.p);
    if (failures)
        fprintf(stderr, "test_hunk: %d failures\n", failures);
    return failures != 0;
}
