/*
 * ab3tok - a source tokenised as AmiBlitz3 v3.14.0 loads it in CLI mode
 * (shared/tokenizer, AmiBlitz3's rules), in the form of its "-save
 * NAME.bb2": every line of the editor with its 0.
 *
 *   ab3tok TOKENS SOURCE OUT          write the lines to OUT
 *   ab3tok -cmp TOKENS SOURCE THEIRS  compare them with THEIRS (AmiBlitz3's
 *                                     -save output): exit 1 and the first
 *                                     line that differs if not equal
 *
 * TOKENS: the token list, one "XXXX name" per line in the compiler's
 * order (tools/blitztokens: the compiler's tokens, then the libraries').
 * SOURCE is loaded as AmiBlitz3's LoadSource does (CLIModeSupport.ab3:437
 * at 58a11658): a .bb or .bb2 as it is, any other name tokenised as text -
 * without its embedded header (not for .ab2) - and cut into lines of at
 * most 250 characters.  Unlike ab3cli, the header is not kept as empty
 * lines: the output is AmiBlitz3's.  Exit 2 on a usage or file error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: ab3tok TOKENS SOURCE OUT\n"
            "       ab3tok -cmp TOKENS SOURCE THEIRS\n"
            "SOURCE tokenised as AmiBlitz3 v3.14.0 loads it (its -save NAME.bb2 form:\n"
            "every line with its 0), with the token list TOKENS (\"XXXX name\" per\n"
            "line, as tools/blitztokens writes it). -cmp: compare with THEIRS, exit 1\n"
            "and the first line that differs if not equal.\n");
}

static unsigned char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

static struct tokenizer_entry *e;     /* the memory of the tokeniser */
static uint16_t *b;

/* the token list into t; the names point into list (kept) */
static int load_tokens(struct tokenizer *t, unsigned char *list, size_t len, const char *path)
{
    size_t pos = 0, n = 0, line = 0;

    for (size_t i = 0; i < len; i++)
        n += list[i] == '\n';
    e = calloc(n + 1, sizeof *e);
    b = calloc(65536, sizeof *b);
    if (!e || !b || tokenizer_init(t, TOKENIZER_RULES_AB3, e, (uint32_t)n + 1, b, 65536)) {
        fprintf(stderr, "ab3tok: out of memory\n");
        return -1;
    }
    while (pos < len) {
        size_t end = pos;
        unsigned long num;
        char *stop;
        int r;

        while (end < len && list[end] != '\n')
            end++;
        line++;
        list[end < len ? end : len] = 0;
        num = strtoul((char *)list + pos, &stop, 16);
        if (stop != (char *)list + pos + 4 || *stop != ' ' || num > 0xFFFF) {
            fprintf(stderr, "ab3tok: %s:%zu: not \"XXXX name\"\n", path, line);
            return -1;
        }
        r = tokenizer_add(t, (unsigned char *)stop + 1, (size_t)(list + end - (unsigned char *)stop - 1),
                          (uint16_t)num);
        if (r != TOKENIZER_OK && r != TOKENIZER_DUPLICATE) {
            fprintf(stderr, "ab3tok: %s:%zu: token not added (%d)\n", path, line, r);
            return -1;
        }
        pos = end + 1;
    }
    return 0;
}

/* the lines of the source as AmiBlitz3 loads it; *olen bytes */
static unsigned char *load_source(const struct tokenizer *t, const char *name,
                                  const unsigned char *src, size_t len, size_t *olen)
{
    int how = tokenizer_ab3_source(name, strlen(name));
    size_t hdr = 0, tlen = len, pos, keep, n = 0;
    unsigned char *tok = (unsigned char *)src, *out;

    if (how & TOKENIZER_AB3_TEXT) {
        if (how & TOKENIZER_AB3_HEADER)
            hdr = tokenizer_ab3_header(src, len);
        tok = malloc(TOKENIZER_BOUND(len - hdr));
        if (!tok || tokenizer_text(t, src + hdr, len - hdr, tok, TOKENIZER_BOUND(len - hdr),
                                   &tlen) != TOKENIZER_OK) {
            fprintf(stderr, "ab3tok: out of memory\n");
            return NULL;
        }
    }
    out = malloc(tlen + 1);
    if (!out) {
        fprintf(stderr, "ab3tok: out of memory\n");
        return NULL;
    }
    for (pos = 0; pos < tlen; ) {
        size_t start = pos;

        pos = tokenizer_ab3_mainline(tok, tlen, pos, &keep);
        memcpy(out + n, tok + start, keep);
        n += keep;
        out[n++] = 0;
    }
    if (tok != src)
        free(tok);
    *olen = n;
    return out;
}

/* a line for a message: text as it is, a token as {XXXX}, other bytes \xNN */
static void show(FILE *f, const unsigned char *s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (s[i] & 0x80 && i + 1 < len) {
            fprintf(f, "{%02X%02X}", s[i], s[i + 1]);
            i++;
        } else if (s[i] >= 32 && s[i] < 127)
            fputc(s[i], f);
        else
            fprintf(f, "\\x%02X", s[i]);
    }
}

/* 0 if equal, else 1 and the first line that differs on stderr */
static int compare(const char *source, const unsigned char *a, size_t alen, const char *theirs,
                   const unsigned char *b, size_t blen)
{
    size_t pa = 0, pb = 0, line = 1;

    while (pa < alen || pb < blen) {
        size_t ea = pa, eb = pb;

        while (ea < alen && a[ea])
            ea++;
        while (eb < blen && b[eb])
            eb++;
        if (pa >= alen || pb >= blen || ea - pa != eb - pb || memcmp(a + pa, b + pb, ea - pa)) {
            fprintf(stderr, "ab3tok: %s: line %zu differs from AmiBlitz3's (%s):\n  ours:      ",
                    source, line, theirs);
            if (pa < alen)
                show(stderr, a + pa, ea - pa);
            else
                fputs("(no line)", stderr);
            fputs("\n  AmiBlitz3: ", stderr);
            if (pb < blen)
                show(stderr, b + pb, eb - pb);
            else
                fputs("(no line)", stderr);
            fputc('\n', stderr);
            return 1;
        }
        pa = ea + 1;
        pb = eb + 1;
        line++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct tokenizer t;
    unsigned char *list = NULL, *src = NULL, *out = NULL, *theirs = NULL;
    size_t llen, slen, olen, tlen;
    int cmp = 0, rc = 2;
    FILE *f;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "-cmp")) {
        cmp = 1;
        argv++;
        argc--;
    }
    if (argc != 4) {
        usage(stderr);
        return 2;
    }
    if (!(list = read_all(argv[1], &llen))) {
        perror(argv[1]);
        goto done;
    }
    if (load_tokens(&t, list, llen, argv[1]))
        goto done;
    if (!(src = read_all(argv[2], &slen))) {
        perror(argv[2]);
        goto done;
    }
    if (!(out = load_source(&t, argv[2], src, slen, &olen)))
        goto done;
    if (cmp) {
        if (!(theirs = read_all(argv[3], &tlen))) {
            perror(argv[3]);
            goto done;
        }
        rc = compare(argv[2], out, olen, argv[3], theirs, tlen);
        goto done;
    }
    if (!(f = fopen(argv[3], "wb")) || fwrite(out, 1, olen, f) != olen || fclose(f)) {
        perror(argv[3]);
        goto done;
    }
    rc = 0;
done:
    free(list);
    free(src);
    free(out);
    free(theirs);
    free(e);
    free(b);
    return rc;
}
