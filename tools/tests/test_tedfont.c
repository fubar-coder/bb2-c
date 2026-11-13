/*
 * test_tedfont.c - tedfont_parse, tedfont_build, tedfont_check with
 * sources and archive fonts built in memory: a good source at both
 * heights (strips 0-7 the archive's, $80-$FF and the 6 px glyph 26 the
 * source's); broken sources (no height, a missing glyph, a duplicate, a
 * row of 7 or 9 columns or with another character, too few and too many
 * rows, a code outside $80-$FF, a glyph 27, a glyph 26 at 8 px); an
 * archive font of the wrong size; fonts that fail the check (size, an
 * original glyph, a new glyph).  A failed build leaves the output as it
 * was.
 */
#include "testutil.h"
#include "tedfont.h"

/* a growing text */
struct text {
    char *p;
    size_t n, cap;
};

static void add(struct text *t, const char *s)
{
    size_t l = strlen(s);

    if (t->n + l + 1 > t->cap) {
        t->cap = (t->n + l + 1) * 2;
        t->p = realloc(t->p, t->cap);
        if (!t->p)
            abort();
    }
    memcpy(t->p + t->n, s, l + 1);
    t->n += l;
}

/* the row of glyph c, row r: a pattern of its own */
static uint8_t pattern(unsigned c, unsigned r)
{
    return (uint8_t)(c * 7 + r * 13 + 1);
}

static void addrow(struct text *t, uint8_t b, int width)
{
    char row[16];
    int i;

    for (i = 0; i < width; i++)
        row[i] = (b >> (7 - (i & 7))) & 1 ? '#' : '.';
    row[width] = '\n';
    row[width + 1] = 0;
    add(t, row);
}

/* the kinds of broken source */
enum broken {
    GOOD, NO_HEIGHT, MISSING_E9, DUPLICATE, ROW7, ROW9, BADCHAR, FEW_ROWS, MANY_ROWS,
    CODE_7F, GLYPH_27, GLYPH_26
};

/* a source of height h: $80-$FF with pattern(); at h = 6 also @1A */
static struct text source(int h, enum broken how)
{
    struct text t = { 0 };
    unsigned c;
    int r;
    char line[40];

    add(&t, "; a test source\n\n");
    if (how != NO_HEIGHT) {
        snprintf(line, sizeof line, "h=%d\n", h);
        add(&t, line);
    }
    if (h == 6 || how == GLYPH_26) {
        add(&t, "@1A ; redrawn\n");
        for (r = 0; r < h; r++)
            addrow(&t, pattern(26, (unsigned)r) ^ 0xFF, 8);
    }
    if (how == GLYPH_27) {
        add(&t, "@1B\n");
        for (r = 0; r < h; r++)
            addrow(&t, 0, 8);
    }
    if (how == CODE_7F) {
        add(&t, "@7F\n");
        for (r = 0; r < h; r++)
            addrow(&t, 0, 8);
    }
    for (c = 0x80; c < 0x100; c++) {
        int rows = h;

        if (c == 0xE9 && how == MISSING_E9)
            continue;
        snprintf(line, sizeof line, "\n@%02X\n", c);
        add(&t, line);
        if (c == 0xC0 && how == FEW_ROWS)
            rows--;
        if (c == 0xC0 && how == MANY_ROWS)
            rows++;
        for (r = 0; r < rows; r++) {
            int width = 8;

            if (c == 0xA5 && r == 2 && how == ROW7)
                width = 7;
            if (c == 0xA5 && r == 2 && how == ROW9)
                width = 9;
            if (c == 0xA5 && r == 2 && how == BADCHAR) {
                add(&t, "..#x#...\n");
                continue;
            }
            addrow(&t, pattern(c, (unsigned)r), width);
        }
    }
    if (how == DUPLICATE) {
        add(&t, "\n@A0\n");
        for (r = 0; r < h; r++)
            addrow(&t, 0, 8);
    }
    return t;
}

/* an archive font of height h: 128 glyphs, byte i = i * 3 + 5 */
static uint8_t *archive(int h)
{
    uint8_t *a = malloc((size_t)128 * h);
    size_t i;

    if (!a)
        abort();
    for (i = 0; i < (size_t)128 * h; i++)
        a[i] = (uint8_t)(i * 3 + 5);
    return a;
}

/* parse a broken source: fails, the message holds want */
static void bad_source(int h, enum broken how, const char *want)
{
    struct text t = source(h, how);
    struct tedfont_src src;
    char err[300] = "";

    CHECK(tedfont_parse(t.p, t.n, "glyphs.txt", &src, err, sizeof err) == -1);
    if (!strstr(err, want)) {
        fprintf(stderr, "message \"%s\" lacks \"%s\"\n", err, want);
        failures++;
    }
    free(t.p);
}

static void good(int h)
{
    struct text t = source(h, GOOD);
    struct tedfont_src src;
    uint8_t *a = archive(h), *font = malloc(tedfont_size(h));
    char err[300] = "";
    unsigned c, r;
    int same = 1;

    CHECK(tedfont_size(h) == (size_t)(h == 8 ? 2048 : 1536));
    CHECK(tedfont_parse(t.p, t.n, "glyphs.txt", &src, err, sizeof err) == 0);
    CHECK(src.height == h);
    CHECK(tedfont_build(&src, a, (size_t)128 * h, font, err, sizeof err) == 0);
    for (c = 0; c < 256; c++)
        for (r = 0; r < (unsigned)h; r++) {
            uint8_t want;
            size_t at = tedfont_at(h, c, r);

            if (c >= 0x80)
                want = pattern(c, r);
            else if (c == 26 && h == 6)
                want = pattern(26, r) ^ 0xFF;
            else
                want = a[at];
            if (font[at] != want)
                same = 0;
        }
    CHECK(same);
    /* strips 0-7 are the first half: the archive file (except glyph 26 at 6 px) */
    CHECK(h == 6 || memcmp(font, a, (size_t)128 * h) == 0);
    CHECK(tedfont_at(h, 0x80, 0) == (size_t)128 * h);
    CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h), err, sizeof err) == 0);

    /* the check: a font of the wrong size, a changed original glyph (27,
       and 26 at 8 px), a changed new glyph */
    CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h) - 1, err, sizeof err) == -1);
    CHECK(strstr(err, "bytes, ") != NULL);
    font[tedfont_at(h, 27, 1)] ^= 1;
    CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h), err, sizeof err) == -1);
    CHECK(strstr(err, "glyph $1B row 1") != NULL && strstr(err, "archive") != NULL);
    font[tedfont_at(h, 27, 1)] ^= 1;
    if (h == 8) {
        font[tedfont_at(h, 26, 0)] ^= 0x80;
        CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h), err, sizeof err) == -1);
        CHECK(strstr(err, "glyph $1A row 0") != NULL);
        font[tedfont_at(h, 26, 0)] ^= 0x80;
    }
    font[tedfont_at(h, 0xE9, h - 1)] ^= 0x10;
    CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h), err, sizeof err) == -1);
    CHECK(strstr(err, "glyph $E9") != NULL && strstr(err, "source") != NULL);
    font[tedfont_at(h, 0xE9, h - 1)] ^= 0x10;
    CHECK(tedfont_check(&src, a, (size_t)128 * h, font, tedfont_size(h), err, sizeof err) == 0);

    /* an archive font of the wrong size: build and check fail, the output
       stays as it was */
    memset(font, 0xAA, tedfont_size(h));
    CHECK(tedfont_build(&src, a, (size_t)128 * h + 1, font, err, sizeof err) == -1);
    CHECK(strstr(err, "archive font has") != NULL);
    CHECK(tedfont_build(&src, a, 1024 + 768 - (size_t)128 * h, font, err, sizeof err) == -1);
    CHECK(font[0] == 0xAA && font[tedfont_size(h) - 1] == 0xAA);
    CHECK(tedfont_check(&src, a, 0, font, tedfont_size(h), err, sizeof err) == -1);
    free(font);
    free(a);
    free(t.p);
}

int main(void)
{
    good(8);
    good(6);

    bad_source(8, NO_HEIGHT, "glyphs.txt:4: expected h=8 or h=6");
    bad_source(8, MISSING_E9, "no block @E9");
    bad_source(6, MISSING_E9, "no block @E9");
    bad_source(8, DUPLICATE, "@A0 given twice");
    bad_source(8, ROW7, "a row of 7 characters, 8 expected");
    bad_source(6, ROW9, "a row of 9 characters, 8 expected");
    bad_source(8, BADCHAR, "'x' in a row");
    bad_source(8, FEW_ROWS, "@C0 has 7 rows, 8 expected");
    bad_source(6, MANY_ROWS, "a row too many: @C0 has 6 rows");
    bad_source(8, CODE_7F, "@7F: glyph $7F is TED's own");
    bad_source(8, GLYPH_27, "@1B: glyph $1B is TED's own");
    bad_source(6, GLYPH_27, "@1B: glyph $1B is TED's own, only $80-$FF and $1A");
    bad_source(8, GLYPH_26, "@1A: glyph $1A is TED's own, only $80-$FF may");

    /* the messages name file and line: the row of 7 columns is in the
       block @A5 (line 3 is h=8; each block 1 blank, @XX, 8 rows) */
    {
        char want[40];

        snprintf(want, sizeof want, "glyphs.txt:%d:", 3 + (0xA5 - 0x80) * 10 + 2 + 1 + 2);
        bad_source(8, ROW7, want);
    }
    return failures != 0;
}
