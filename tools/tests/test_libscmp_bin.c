/*
 * test_libscmp_bin.c - the raw block observations of the library tests
 * (NAME.bin, tools/lib/libscmp, openspec change libs-capture-host-blocks)
 * with files built in memory: every record type formatted as
 * libs/tests/capture.c writes plain cases, the block cases merged into
 * the run's NAME.txt (hash, first=, partial blocks, plain cases between),
 * broken files (truncated, an unknown type, a bad length, a missing file,
 * places and blocks that do not fit, ...), the dump, the first
 * differing observation of two runs, lost inside a block refused by
 * accept, and the directory commands (block listing, libs-dump).
 */
#define _DEFAULT_SOURCE
#include <sys/stat.h>
#include <unistd.h>
#include "testutil.h"
#include "libscmp.h"

static char err[700];

static uint32_t fnv(uint32_t h, const char *s, size_t n)
{
    while (n--)
        h = (h ^ (unsigned char)*s++) * 0x01000193u;
    return h;
}

static void bstr(struct buf *b, const char *s)
{
    putbytes(b, s, strlen(s) + 1);
}

static void head(struct buf *b, const char *name)
{
    char h[100];

    snprintf(h, sizeof h, "# capture-bin %s 1\n", name);
    putbytes(b, h, strlen(h));
}

static void obs(struct buf *b, uint8_t type, const char *check, const char *vec)
{
    put8(b, type);
    bstr(b, check);
    bstr(b, vec);
}

static void num(struct buf *b, const char *check, const char *vec, uint32_t v)
{
    obs(b, 0x10, check, vec);
    put32(b, v);
}

/* text with its block places merged from b (NULL: no file); the result
   of libscmp_bin_merge, the text in *out */
static int merge(const char *text, const struct buf *b, char **out)
{
    size_t len;

    return libscmp_bin_merge(text, strlen(text), "prog.txt", b ? b->p : NULL, b ? b->n : 0,
                             "prog.bin", out, &len, err, sizeof err);
}

/* the merge error of text and b, "" if it merges */
static const char *broken(const char *text, const struct buf *b)
{
    char *out;
    int r = merge(text, b, &out);

    free(out);
    if (!r)
        return "";
    CHECK(r == -2);
    return err;
}

static char *hex8(uint32_t v)
{
    static char s[4][12];
    static int k;

    k = (k + 1) & 3;
    snprintf(s[k], sizeof s[k], "%08lX", (unsigned long)v);
    return s[k];
}

static void test_formats(void)
{
    struct buf b = { 0 };
    unsigned char b64[64], b65[65];
    char lines[4000], want[5000], *out;
    int i;

    for (i = 0; i < 65; i++) {
        b65[i] = (unsigned char)i;
        if (i < 64)
            b64[i] = (unsigned char)i;
    }
    head(&b, "prog");
    put8(&b, 0x01);
    bstr(&b, "rtn");
    num(&b, "finit d0", "x=1", 0x12345678);
    num(&b, "a|b\tc\\d\x01", "v|\t\xff", 0xFFFFFFFF);
    obs(&b, 0x11, "buf", "64");
    put32(&b, 64);
    putbytes(&b, b64, 64);
    obs(&b, 0x11, "buf", "65");
    put32(&b, 65);
    putbytes(&b, b65, 65);
    obs(&b, 0x11, "buf", "0");
    put32(&b, 0);
    obs(&b, 0x13, "s", "");
    put8(&b, 1);
    bstr(&b, "a\"b\\c\td\x80");
    obs(&b, 0x13, "s", "null");
    put8(&b, 0);
    obs(&b, 0x14, "p", "prog");
    put8(&b, 0);
    obs(&b, 0x14, "p", "null");
    put8(&b, 1);
    put32(&b, 0);
    obs(&b, 0x14, "p", "above");
    put8(&b, 2);
    put32(&b, 0x10);
    bstr(&b, "base");
    obs(&b, 0x14, "p", "below");
    put8(&b, 3);
    put32(&b, 0x10);
    bstr(&b, "base");
    obs(&b, 0x15, "regs", "");
    put16(&b, 0);
    obs(&b, 0x15, "regs", "");
    put16(&b, (1u << 3) | (1u << 9) | (1u << 14));
    put8(&b, 0x01);
    bstr(&b, "x\\|y");
    num(&b, "finit d0", "x=1", 5);
    put8(&b, 0x02);

    {
        char hex64[129] = "";

        for (i = 0; i < 64; i++)
            snprintf(hex64 + 2 * i, 3, "%02X", i);
        snprintf(lines, sizeof lines,
                 "rtn|finit d0|x=1|1\t$12345678\n"
                 "rtn|a\\|b\\tc\\\\d\\x01|v\\|\\t\\xFF|2\t$FFFFFFFF\n"
                 "rtn|buf|64|3\tbytes 64 %s\n"
                 "rtn|buf|65|4\tbytes 65 fnv %s\n"
                 "rtn|buf|0|5\tbytes 0\n"
                 "rtn|s||6\t\"a\\\"b\\\\c\\x09d\\x80\"\n"
                 "rtn|s|null|7\tnull\n"
                 "rtn|p|prog|8\tprog\n"
                 "rtn|p|null|9\t$00000000\n"
                 "rtn|p|above|10\tbase+$10\n"
                 "rtn|p|below|11\tbase-$10\n"
                 "rtn|regs||12\tkept\n"
                 "rtn|regs||13\tlost d3 a1 a6\n"
                 "x\\|y|finit d0|x=1|14\t$00000005\n",
                 hex64, hex8(fnv(0x811c9dc5u, (const char *)b65, 65)));
    }
    snprintf(want, sizeof want, "# capture prog 1\nr|c||1\t$0\nBLOCK:1\t14 %s first=rtn|finit d0|x=1|1\n",
             hex8(fnv(0x811c9dc5u, lines, strlen(lines))));
    CHECK(merge("# capture prog 1\nr|c||1\t$0\nBLOCK:1\t14 in prog.bin\n", &b, &out) == 0);
    CHECK(out && !strcmp(out, want));
    if (out && strcmp(out, want))
        fprintf(stderr, "got:\n%s\nwant:\n%s\n", out, want);
    free(out);
    /* the dump of the block: its lines, whose hash is the block's */
    {
        char *d = NULL;
        size_t dl;
        FILE *m = open_memstream(&d, &dl);

        CHECK(libscmp_bin_dump(b.p, b.n, "prog.bin", "BLOCK:1", m, err, sizeof err) == 14);
        fclose(m);
        CHECK(d && !strcmp(d, lines));
        free(d);
        m = open_memstream(&d, &dl);
        CHECK(libscmp_bin_dump(b.p, b.n, "prog.bin", "BLOCK:2", m, err, sizeof err) == 0);
        fclose(m);
        free(d);
    }
    free(b.p);
}

/* 4096 + 3 observations, a plain case between: two block cases */
static void test_blocks(void)
{
    struct buf b = { 0 };
    char *lines1 = malloc(200000), *lines2 = malloc(1000), want[1000], *out, l[100];
    size_t n1 = 0, n2 = 0;
    uint32_t i;

    head(&b, "blk");
    put8(&b, 0x01);
    bstr(&b, "med");
    for (i = 1; i <= LIBSCMP_BLOCK + 3; i++) {
        int k = snprintf(l, sizeof l, "med|obs|v|%lu\t$%s\n", (unsigned long)i, hex8(i));

        num(&b, "obs", "v", i);
        if (i <= LIBSCMP_BLOCK) {
            memcpy(lines1 + n1, l, (size_t)k);
            n1 += (size_t)k;
        } else {
            memcpy(lines2 + n2, l, (size_t)k);
            n2 += (size_t)k;
        }
    }
    put8(&b, 0x02);
    snprintf(want, sizeof want, "# capture blk 1\nmed|before||1\t$00000001\n"
             "BLOCK:1\t4096 %s first=med|obs|v|1\nmed|skip||1\tnot captured: why\n"
             "BLOCK:2\t3 %s first=med|obs|v|4097\nmed|after||1\t$00000002",
             hex8(fnv(0x811c9dc5u, lines1, n1)), hex8(fnv(0x811c9dc5u, lines2, n2)));
    CHECK(merge("# capture blk 1\nmed|before||1\t$00000001\nBLOCK:1\t4096 in blk.bin\n"
                "med|skip||1\tnot captured: why\nBLOCK:2\t3 in blk.bin\nmed|after||1\t$00000002",
                &b, &out) == 0);
    CHECK(out && !strcmp(out, want));
    free(out);
    /* places that do not fit */
    CHECK(strstr(broken("# capture blk 1\nBLOCK:1\t4096 in blk.bin\nBLOCK:2\t4 in blk.bin\n", &b),
                 "prog.txt:3: BLOCK:2 with 4 observations, but block 2 of prog.bin has 3"));
    CHECK(strstr(broken("# capture blk 1\nBLOCK:1\t4096 in blk.bin\n", &b),
                 "prog.bin: block 2 has no place in prog.txt"));
    CHECK(strstr(broken("# capture blk 1\nBLOCK:1\t4096 in blk.bin\nBLOCK:2\t3 in blk.bin\n"
                        "BLOCK:3\t3 in blk.bin\n", &b),
                 "prog.txt:4: BLOCK:3, but prog.bin has only 2 blocks"));
    CHECK(strstr(broken("# capture other 1\n", &b), "prog.bin: the header names blk, the output other"));
    /* the end inside a block: the program stopped */
    b.n--;
    CHECK(strstr(broken("# capture blk 1\nBLOCK:1\t4096 in blk.bin\nBLOCK:2\t3 in blk.bin\n", &b),
                 "the file ends inside block 2"));
    free(b.p);
    free(lines1);
    free(lines2);
}

static void test_broken(void)
{
    static const char text[] = "# capture prog 1\nBLOCK:1\t1 in prog.bin\n";
    struct buf b = { 0 };
    size_t at;
    char key[1100];

    /* no file for the places */
    CHECK(strstr(broken(text, NULL), "prog.txt:2: a block case in prog.bin, which is missing"));
    /* without places and file: the text as it is */
    CHECK(!strcmp(broken("# capture prog 1\nr|c||1\t$0\n", NULL), ""));
    /* no header */
    put8(&b, 0x10);
    CHECK(strstr(broken(text, &b), "prog.bin: offset 0: no header"));
    free(b.p);
    memset(&b, 0, sizeof b);
    putbytes(&b, "# capture-bin prog 2\n", 21);
    CHECK(strstr(broken(text, &b), "(version 1)"));
    free(b.p);
    /* a truncated record */
    memset(&b, 0, sizeof b);
    head(&b, "prog");
    at = b.n;
    num(&b, "c", "", 7);
    b.n -= 2;
    CHECK(strstr(broken(text, &b), "prog.bin: offset 21: the record of type $10 is cut off"));
    CHECK(at == 21);
    b.n = at;
    obs(&b, 0x10, "c", "");
    b.n--;                      /* the vector's 0 byte */
    CHECK(strstr(broken(text, &b), "offset 21: the record of type $10 is cut off"));
    /* an unknown type */
    b.n = at;
    obs(&b, 0x12, "c", "");
    CHECK(strstr(broken(text, &b), "offset 21: unknown record type $12"));
    b.n = at;
    put8(&b, 0x77);
    CHECK(strstr(broken(text, &b), "offset 21: unknown record type $77"));
    /* a bad length */
    b.n = at;
    obs(&b, 0x11, "c", "");
    put32(&b, 1000);
    putbytes(&b, "abc", 3);
    CHECK(strstr(broken(text, &b), "offset 21: bytes of length 1000, beyond the end of the file"));
    /* a bad kind, flag, register set */
    b.n = at;
    obs(&b, 0x14, "c", "");
    put8(&b, 4);
    CHECK(strstr(broken(text, &b), "offset 21: address kind 4 (0-3)"));
    b.n = at;
    obs(&b, 0x13, "c", "");
    put8(&b, 2);
    CHECK(strstr(broken(text, &b), "offset 21: text flag 2 (0 or 1)"));
    b.n = at;
    obs(&b, 0x15, "c", "");
    put16(&b, 0x8000);
    CHECK(strstr(broken(text, &b), "offset 21: register set $8000 (bits 0-14)"));
    /* a key longer than the writer's */
    b.n = at;
    memset(key, 'k', 1020);
    key[1020] = 0;
    num(&b, key, "", 1);
    put8(&b, 0x02);
    CHECK(strstr(broken(text, &b), "offset 21: a key longer than 1023 bytes"));
    /* 1023 bytes are the limit: |k...k||1 */
    b.n = at;
    key[1019] = 0;
    num(&b, key, "", 1);
    put8(&b, 0x02);
    CHECK(!strcmp(broken(text, &b), ""));
    free(b.p);
}

/* the dump of a bytes observation of a block */
static void test_dump_key(void)
{
    struct buf b = { 0 };
    unsigned char v[70];
    char *d = NULL, row[] = "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF", want[400];
    size_t dl;
    FILE *m;
    int i;

    for (i = 0; i < 70; i++)
        v[i] = (unsigned char)(0xA0 + i % 16);
    head(&b, "dmp");
    put8(&b, 0x01);
    bstr(&b, "r");
    for (i = 0; i < 2; i++) {
        obs(&b, 0x11, "buf", "v");
        put32(&b, 70);
        putbytes(&b, v, 70);
    }
    put8(&b, 0x02);
    m = open_memstream(&d, &dl);
    CHECK(libscmp_bin_dump(b.p, b.n, "dmp.bin", "r|buf|v|2", m, err, sizeof err) == 3);
    fclose(m);
    snprintf(want, sizeof want, "r|buf|v|2\tat $0000 %s%s\nr|buf|v|2\tat $0020 %s%s\n"
             "r|buf|v|2\tat $0040 A0A1A2A3A4A5\n", row, row, row, row);
    CHECK(d && !strcmp(d, want));
    free(d);
    free(b.p);
}

/* two runs: one value differs in block 2 */
static void make_run(struct buf *b, uint32_t odd, uint16_t lost)
{
    uint32_t i;

    memset(b, 0, sizeof *b);
    head(b, "prog");
    put8(b, 0x01);
    bstr(b, "r");
    for (i = 1; i <= LIBSCMP_BLOCK + 10; i++)
        num(b, "n", "", i == odd ? 0xBAD : i);
    obs(b, 0x15, "regs", "x");
    put16(b, lost);
    put8(b, 0x02);
}

static const char run_text[] = "# capture prog 1\nr|plain||1\t$1\nBLOCK:1\t4096 in prog.bin\n"
                               "BLOCK:2\t11 in prog.bin\n";

static int run_file(struct libscmp_file *f, const struct buf *b)
{
    return libscmp_parse_run(f, run_text, strlen(run_text), "prog.txt", b->p, b->n, "prog.bin",
                             err, sizeof err);
}

static void test_two_runs(void)
{
    struct buf a, b, l;
    struct libscmp_file fa, fb, fl;
    char *rep = NULL, *out;
    size_t rl;
    FILE *m;

    make_run(&a, 0, 0);
    make_run(&b, LIBSCMP_BLOCK + 5, 0);
    make_run(&l, 0, 1u << 3);
    CHECK(run_file(&fa, &a) == 0);
    CHECK(run_file(&fb, &b) == 0);
    CHECK(run_file(&fl, &l) == 0);
    CHECK(fa.bin && fa.binlen == a.n && !strcmp(fa.binpath, "prog.bin"));
    m = open_memstream(&rep, &rl);
    CHECK(libscmp_bin_first_diff(&fa, &fb, 2, "run 1", "run 2", m, err, sizeof err) == 1);
    CHECK(libscmp_bin_first_diff(&fa, &fb, 1, "run 1", "run 2", m, err, sizeof err) == 0);
    fclose(m);
    CHECK(rep && strstr(rep, "prog: BLOCK:2: first differing observation r|n||4101: run 1 "
                        "$00001005, run 2 $00000BAD\n"));
    CHECK(rep && strstr(rep, "prog: BLOCK:1: no differing observation (run 1 and run 2)\n"));
    free(rep);
    /* accept: not deterministic, with the observation */
    m = open_memstream(&rep, &rl);
    CHECK(libscmp_accept(&fa, &fb, NULL, &out, m) == 1);
    fclose(m);
    CHECK(!out);
    CHECK(rep && strstr(rep, "test not deterministic: prog\nprog: BLOCK:2: run 1 11 ")
          && strstr(rep, "prog: BLOCK:2: first differing observation r|n||4101: run 1 $00001005, "
                    "run 2 $00000BAD\n"));
    free(rep);
    /* accept: lost inside a block refused */
    m = open_memstream(&rep, &rl);
    CHECK(libscmp_accept(&fl, &fl, NULL, &out, m) == 1);
    fclose(m);
    CHECK(!out);
    CHECK(rep && strstr(rep, "prog: r|regs|x|4107 (BLOCK:2): lost d3: the original does not keep "
                        "a stated register") && strstr(rep, "prog: refused: 1 cases lost\n"));
    free(rep);
    /* accept of two equal runs */
    m = open_memstream(&rep, &rl);
    CHECK(libscmp_accept(&fa, &fa, NULL, &out, m) == 0);
    fclose(m);
    CHECK(out && strstr(out, "BLOCK:2\t11 "));
    free(out);
    free(rep);
    libscmp_free(&fa);
    libscmp_free(&fb);
    libscmp_free(&fl);
    free(a.p);
    free(b.p);
    free(l.p);
}

static void put(const char *dir, const char *file, const void *p, size_t n)
{
    char path[300];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", dir, file);
    f = fopen(path, "wb");
    CHECK(f && fwrite(p, 1, n, f) == n);
    if (f)
        fclose(f);
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = calloc(1, 1 << 20);

    if (f) {
        CHECK(fread(s, 1, (1 << 20) - 1, f) > 0);
        fclose(f);
    }
    return s;
}

static void test_dirs(void)
{
    struct buf a, b;
    struct libscmp_file fa;
    char *rep = NULL, *e = NULL, *t, exp[400];
    size_t rl, el;
    const char *only[1] = { "prog" };
    FILE *m, *x;
    int s;

    mkdir("libscmp-bin", 0777);
    mkdir("libscmp-bin/run", 0777);
    mkdir("libscmp-bin/other", 0777);
    mkdir("libscmp-bin/exp", 0777);
    make_run(&a, 0, 0);
    make_run(&b, LIBSCMP_BLOCK + 5, 0);
    /* the expected data from run a */
    CHECK(run_file(&fa, &a) == 0);
    snprintf(exp, sizeof exp, "%.*s", (int)fa.len, fa.text);
    put("libscmp-bin/exp", "prog.txt", exp, strlen(exp));
    libscmp_free(&fa);
    put("libscmp-bin/run", "prog.txt", run_text, strlen(run_text));
    put("libscmp-bin/run", "prog.bin", b.p, b.n);
    put("libscmp-bin/other", "prog.bin", a.p, a.n);
    m = open_memstream(&rep, &rl);
    x = open_memstream(&e, &el);
    s = libscmp_compare_dirs_other("libscmp-bin/run", "libscmp-bin/exp", "libscmp-bin/other",
                                   LIBSCMP_OBJ, NULL, 0, m, x);
    fclose(m);
    fclose(x);
    CHECK(s == 1);
    CHECK(rep && strstr(rep, "prog: BLOCK:2 (observations 4097..4107): expected 11 "));
    CHECK(rep && strstr(rep, "prog: BLOCK:2: its observations in libscmp-bin/run/prog.block-2.obs, "
                        "the first:\nprog:   r|n||4097\t$00001001\nprog:   r|n||4098\t$00001002\n"));
    CHECK(rep && strstr(rep, "prog: BLOCK:2: first differing observation r|n||4101: this run "
                        "$00000BAD, the other run $00001005\n"));
    CHECK(e && !*e);
    free(rep);
    free(e);
    t = slurp("libscmp-bin/run/prog.block-2.obs");
    CHECK(!strncmp(t, "r|n||4097\t$00001001\n", 20) && strstr(t, "r|n||4101\t$00000BAD\n")
          && strstr(t, "r|regs|x|4107\tkept\n"));
    free(t);
    /* without the other run's file */
    m = open_memstream(&rep, &rl);
    x = open_memstream(&e, &el);
    s = libscmp_compare_dirs_other("libscmp-bin/run", "libscmp-bin/exp", "libscmp-bin/exp",
                                   LIBSCMP_OBJ, NULL, 0, m, x);
    fclose(m);
    fclose(x);
    CHECK(s == 1 && rep && strstr(rep, "prog: no libscmp-bin/exp/prog.bin, no comparison"));
    free(rep);
    free(e);
    /* a broken NAME.bin: malformed (2) */
    put("libscmp-bin/run", "prog.bin", b.p, b.n - 3);
    m = open_memstream(&rep, &rl);
    x = open_memstream(&e, &el);
    s = libscmp_compare_dirs("libscmp-bin/run", "libscmp-bin/exp", LIBSCMP_OBJ, NULL, 0, m, x);
    fclose(m);
    fclose(x);
    CHECK(s == 2 && e && strstr(e, "libscmp-bin/run/prog.bin: offset "));
    free(rep);
    free(e);
    /* libs-dump: appended to NAME.dump */
    put("libscmp-bin/run", "prog.bin", b.p, b.n);
    unlink("libscmp-bin/run/prog.dump");
    m = open_memstream(&rep, &rl);
    x = open_memstream(&e, &el);
    s = libscmp_dump_dirs("libscmp-bin/run", "BLOCK:2", NULL, 0, m, x);
    fclose(m);
    fclose(x);
    CHECK(s == 0 && rep && strstr(rep, "libscmp-bin/run/prog.dump: 11 lines of BLOCK:2\n"));
    free(rep);
    free(e);
    t = slurp("libscmp-bin/run/prog.dump");
    CHECK(!strncmp(t, "r|n||4097\t$00001001\n", 20));
    free(t);
    m = open_memstream(&rep, &rl);
    x = open_memstream(&e, &el);
    s = libscmp_dump_dirs("libscmp-bin/run", "r|no||1", only, 1, m, x);
    fclose(m);
    fclose(x);
    CHECK(s == 1 && rep && strstr(rep, "libs-dump r|no||1: in no NAME.bin of libscmp-bin/run"));
    free(rep);
    free(e);
    free(a.p);
    free(b.p);
}

int main(void)
{
    test_formats();
    test_blocks();
    test_broken();
    test_dump_key();
    test_two_runs();
    test_dirs();
    if (failures)
        fprintf(stderr, "test_libscmp_bin: %d failures\n", failures);
    return failures != 0;
}
