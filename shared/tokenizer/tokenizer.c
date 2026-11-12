/*
 * tokenizer.c - the tokeniser component (tokenizer.h): the token list as
 * a hash table, AmiBlitz3's rules (preproc_Tokenize,
 * Sourcecodes/Amiblitz3/Shared/preprocessor.include.ab3:358 at
 * 58a11658), the ends of AmiBlitz3's embedded headers and the lines of a
 * main source as AmiBlitz3 loads them.  C99, freestanding.
 */
#include "tokenizer.h"

/* ------------------------------------------------------------ the list */

/* the hash of a name: shifts and adds only (no 32-bit multiply on the
   68000 without libgcc); the table only has to find the bytes, AmiBlitz3's
   own hash ((h + c) * 196314165) does not matter */
static uint32_t hash(const uint8_t *s, size_t len)
{
    uint32_t h = 5381;

    while (len--)
        h = ((h << 5) + h) ^ *s++;
    return h ^ (h >> 16);
}

static int same(const uint8_t *a, const uint8_t *b, size_t len)
{
    while (len--)
        if (*a++ != *b++)
            return 0;
    return 1;
}

int tokenizer_init(struct tokenizer *t, int rules, struct tokenizer_entry *entry, uint32_t cap,
                   uint16_t *bucket, uint32_t nbucket)
{
    uint32_t i;

    if (rules != TOKENIZER_RULES_AB3 && rules != TOKENIZER_RULES_AB3_LIT)
        return TOKENIZER_E_RULES;
    if (nbucket == 0 || nbucket > 65536 || (nbucket & (nbucket - 1)))
        return TOKENIZER_E_BUCKETS;
    t->entry = entry;
    t->bucket = bucket;
    t->n = 0;
    t->cap = cap > 65535 ? 65535 : cap;
    t->mask = nbucket - 1;
    t->rules = (uint8_t)rules;
    for (i = 0; i < nbucket; i++)
        bucket[i] = 0;
    return TOKENIZER_OK;
}

/* the entry of the name + 1, 0 if not there */
static uint32_t lookup(const struct tokenizer *t, const uint8_t *name, size_t len)
{
    uint32_t i = t->bucket[hash(name, len) & t->mask];

    while (i) {
        const struct tokenizer_entry *e = &t->entry[i - 1];

        if (e->len == len && same(e->name, name, len))
            return i;
        i = e->next;
    }
    return 0;
}

int tokenizer_add(struct tokenizer *t, const uint8_t *name, size_t len, uint16_t num)
{
    struct tokenizer_entry *e;
    uint16_t *b;

    if (len == 0 || len > 65535)
        return TOKENIZER_E_NAME;
    if (lookup(t, name, len))
        return TOKENIZER_DUPLICATE;     /* the first one wins (QueryTokens) */
    if (t->n >= t->cap)
        return TOKENIZER_E_FULL;
    b = &t->bucket[hash(name, len) & t->mask];
    e = &t->entry[t->n];
    e->name = name;
    e->len = (uint16_t)len;
    e->num = num;
    e->next = *b;
    *b = (uint16_t)++t->n;
    return TOKENIZER_OK;
}

int tokenizer_find(const struct tokenizer *t, const uint8_t *name, size_t len, uint16_t *num)
{
    uint32_t i = len ? lookup(t, name, len) : 0;

    if (i && num)
        *num = t->entry[i - 1].num;
    return i != 0;
}

/* ------------------------------------------------------- AmiBlitz3's rules */

/* the character flags of preproc_Create (preprocessor.include.ab3:155-295) */
#define F_STOP    0x001         /* ends a word: look it up */
#define F_NEW     0x002         /* a token may start behind it */
#define F_IS      0x004         /* belongs to a word */
#define F_FIRST   0x008         /* starts a word */
#define F_COPY    0x010         /* copied to the output */
#define F_QUOT    0x020         /* starts a string */
#define F_COMMENT 0x040         /* starts a comment */
#define F_EOL     0x080         /* ends a line: a 0 */
#define F_TAB     0x100         /* a space */

static unsigned flags(unsigned c)
{
    if (c == 0 || c == 10)
        return F_STOP | F_NEW | F_EOL;
    if (c == 9)
        return F_STOP | F_NEW | F_TAB;
    if (c < 32 || c >= 127)     /* CR, the other control characters, DEL,
                                   128-255: not copied */
        return F_STOP | F_NEW;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        return F_FIRST | F_IS | F_COPY;
    if (c >= '0' && c <= '9')
        return F_IS | F_COPY;
    switch (c) {
    case '"':
        return F_STOP | F_NEW | F_COPY | F_QUOT;
    case ';':
        return F_STOP | F_NEW | F_COPY | F_COMMENT;
    case '#':
    case '$':
        return F_STOP | F_IS | F_COPY;
    case '\'':
    case '.':
    case '@':
    case '\\':
        return F_STOP | F_COPY; /* no token directly behind them */
    case '_':
        return F_IS | F_COPY;
    default:                    /* space and the operators */
        return F_STOP | F_NEW | F_COPY;
    }
}

/* preproc_Tokenize over src[0..len) and, with lf, an LF behind it.  The
   byte behind the text reads as 0, as the 0 AmiBlitz3 has behind its
   buffer (LoadSource: PokeB tempBuffer + byteLength, an include's buffer:
   AllocVec MEMF_CLEAR of 4 bytes more). */
static int run(const struct tokenizer *t, const uint8_t *src, size_t len, int lf, uint8_t *dst,
               size_t cap, size_t *out)
{
    size_t end = len + (lf ? 1 : 0), sp = 0, dp = 0, tl = 0;
    int accept = 1, lit = t->rules == TOKENIZER_RULES_AB3_LIT;
    unsigned c, cf;
    uint32_t found;

#define GET(i) ((i) < len ? src[i] : (lf && (i) == len) ? 10u : 0u)
#define PUT(b) do {                     \
        if (dp >= cap)                  \
            goto space;                 \
        dst[dp++] = (uint8_t)(b);       \
    } while (0)

#define PUTLIT(b) do {                  \
        if (lit && (b) >= 128)          \
            PUT(TOKENIZER_LITHI);       \
        PUT(b);                         \
    } while (0)

    *out = 0;
    if (t->rules != TOKENIZER_RULES_AB3 && t->rules != TOKENIZER_RULES_AB3_LIT)
        return TOKENIZER_E_RULES;
    while (sp < end) {
        c = GET(sp);
        if (c >= 128)
            c = '?';
        cf = flags(c);
        if (tl) {                       /* in a word */
            if (cf & F_STOP) {
                if (c == '$') {         /* name$ first (Str$), then name */
                    if (dp >= cap)
                        goto space;
                    dst[dp] = '$';      /* Poke.b dptr,c: not counted yet */
                    found = lookup(t, dst + dp - tl, tl + 1);
                    if (found)
                        cf = 0;         /* the $ belongs to the token */
                    else
                        found = lookup(t, dst + dp - tl, tl);
                } else
                    found = lookup(t, dst + dp - tl, tl);
                if (found) {
                    uint16_t num = t->entry[found - 1].num;

                    dp -= tl;
                    PUT(num >> 8);
                    PUT(num & 0xFF);
                    tl = 0;
                    accept = 0;
                }
            }
            if (cf & F_IS) {
                if (tl)
                    tl++;
            } else
                tl = 0;
        } else if ((cf & F_FIRST) && accept)
            tl = 1;
        if (cf & (F_QUOT | F_COMMENT | F_EOL | F_TAB)) {
            if (cf & F_QUOT) {          /* a string: bytes as they are */
                do {
                    PUTLIT(c);
                    sp++;
                    c = GET(sp);
                    if (c == 9)
                        c = ' ';
                    cf = flags(c);
                } while (!(cf & (F_QUOT | F_EOL)) && sp < end);
            }
            if (cf & F_COMMENT) {       /* a comment: to the end of the line */
                do {
                    PUTLIT(c);
                    sp++;
                    c = GET(sp);
                    if (c == 9)
                        c = ' ';
                    cf = flags(c);
                } while (!(cf & F_EOL) && sp < end);
            }
            if (cf & F_TAB)
                PUT(' ');
            if (cf & F_EOL)
                PUT(0);
        }
        if (cf & F_NEW) {
            accept = 1;
            tl = 0;
        } else
            accept = 0;
        if (cf & F_COPY)
            PUT(c);
        sp++;
    }
    *out = dp;
    return TOKENIZER_OK;
space:
    *out = dp;
    return TOKENIZER_E_SPACE;
#undef GET
#undef PUT
#undef PUTLIT
}

int tokenizer_text(const struct tokenizer *t, const uint8_t *src, size_t len, uint8_t *dst,
                   size_t cap, size_t *out)
{
    return run(t, src, len, 0, dst, cap, out);
}

int tokenizer_line(const struct tokenizer *t, const uint8_t *src, size_t len, uint8_t *dst,
                   size_t cap, size_t *out)
{
    return run(t, src, len, 1, dst, cap, out);
}

/* ---------------------------------------------------- headers and lines */

size_t tokenizer_ab3_header(const uint8_t *src, size_t len)
{
    static const char key[] = "/xtra";
    size_t pos = 0;

    while (pos < len) {                 /* file_ReadLine */
        size_t i, k = 0, end = pos;
        int ok = 1;

        while (end < len && src[end] != 10)
            end++;
        /* the token: before a '=', letters, '_', '/' and '-' (a space) */
        for (i = pos; i < end && src[i] != '='; i++) {
            unsigned c = src[i];

            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '/'
                  || c == '-'))
                continue;
            if (c >= 'A' && c <= 'Z')
                c += 32;                /* LCase$ */
            if (k >= sizeof key - 1 || c != (unsigned char)key[k])
                ok = 0;
            k++;
        }
        pos = end < len ? end + 1 : len;
        if (ok && k == sizeof key - 1)
            return pos;                 /* offset = file_GetSeekPos */
    }
    return 0;
}

size_t tokenizer_ab3_incheader(const uint8_t *src, size_t len)
{
#define AT(i) ((i) < len ? src[i] : 0u)
    size_t pos = 0;
    int found = 0, comment = 0;

    while (!found && pos < len) {
        switch (src[pos]) {
        case ';':
            comment = 1;
            pos++;
            break;
        case 10:
            if (!comment) {
                found = 1;              /* a line without ';': no header */
                pos = 0;
            } else {
                comment = 0;
                pos++;
            }
            break;
        case '/':
            pos++;
            if ((AT(pos) == 'x' && AT(pos + 1) == 't' && AT(pos + 2) == 'r' && AT(pos + 3) == 'a')
                || (AT(pos) == 'X' && AT(pos + 1) == 'T' && AT(pos + 2) == 'R'
                    && AT(pos + 3) == 'A')
                || (AT(pos) == 'X' && AT(pos + 1) == 't' && AT(pos + 2) == 'r'
                    && AT(pos + 3) == 'a')) {
                pos += 4;
                while (AT(pos) != 10 && pos < len)
                    pos++;
                pos++;
                found = 1;
            }
            break;
        default:
            pos++;
        }
    }
    return pos >= len ? 0 : pos;
#undef AT
}

size_t tokenizer_lines(const uint8_t *src, size_t len)
{
    size_t i, n = 0;

    for (i = 0; i < len; i++)
        if (src[i] == 10)
            n++;
    if (len && src[len - 1] != 10)
        n++;
    return n;
}

size_t tokenizer_ab3_mainline(const uint8_t *tok, size_t len, size_t pos, size_t *keep)
{
    size_t end = pos;

    while (end < len && tok[end])
        end++;
    *keep = end - pos > TOKENIZER_AB3_COLUMNS ? TOKENIZER_AB3_COLUMNS : end - pos;
    return end < len ? end + 1 : len;
}

size_t tokenizer_ab3_mainline_lit(const uint8_t *tok, size_t len, size_t pos, size_t *keep)
{
    size_t end = pos, n = 0, cut = pos;

    while (end < len && tok[end]) {
        size_t w = tok[end] == TOKENIZER_LITHI && end + 1 < len && tok[end + 1] >= 128 ? 2 : 1;

        if (n < TOKENIZER_AB3_COLUMNS && cut + w - pos <= TOKENIZER_AB3_COLUMNS && cut == end) {
            n++;
            cut = end + w;
        }
        end += w;
    }
    if (end > len)
        end = len;
    *keep = cut - pos;
    return end < len ? end + 1 : len;
}

/* ------------------------------------------------------------ file names */

size_t tokenizer_ab3_getext(const char *name, size_t len)
{
    size_t i = len;

    while (i--) {
        /* the character at m is read, then m - 1 = 0 or more than 10
           characters behind it end the search without a result */
        if (i == 0 || len - i > 10)
            return len;
        if (name[i] == '.')
            return i + 1;
    }
    return len;
}

/* dos_RemExt: the name without the extension behind the last '.' among
   the last 8 characters (not a '.' at the start); the length kept */
static size_t remext(const char *name, size_t len)
{
    size_t i = len;

    while (i--) {
        if (i == 0 || len - i > 8)
            return len;
        if (name[i] == '.')
            return i;
    }
    return len;
}

/* dos_SetExt into out[cap] (out may be name): name without its extension,
   '.', ext; the length, 0 if it does not fit */
static size_t setext(const char *name, size_t len, const char *e1, size_t n1, const char *e2,
                     char *out, size_t cap)
{
    size_t keep = remext(name, len), n2 = 0, i;

    while (e2 && e2[n2])
        n2++;
    if (keep + 1 + n1 + n2 + 1 > cap)
        return 0;
    for (i = 0; i < keep; i++)
        out[i] = name[i];
    out[i++] = '.';
    while (n1--)
        out[i++] = *e1++;
    while (e2 && *e2)
        out[i++] = *e2++;
    out[i] = 0;
    return i;
}

size_t tokenizer_ab3_incname(const char *name, size_t len, int k, char *out, size_t cap)
{
    static const char *const ext[] = { "ab3", "ab2", "bb2" };
    size_t e = tokenizer_ab3_getext(name, len), n = len, i;
    int j;

    if (k < 0 || k >= TOKENIZER_AB3_INCNAMES || len + 1 > cap)
        return 0;
    for (i = 0; i < len; i++)           /* the name as given */
        out[i] = name[i];
    out[len] = 0;
    /* each try replaces the extension of the one before (filename =
       dos_SetExt{filename, ...}); the 4th puts the old extension back in
       front of .ab3 */
    for (j = 1; j <= k && n; j++)
        n = j == 4 ? setext(out, n, name + e, len - e, ".ab3", out, cap)
                   : setext(out, n, ext[j < 4 ? j - 1 : j - 4], 3, 0, out, cap);
    return n;
}

int tokenizer_ab3_inctype(const char *name, size_t len)
{
    const char *t = name + len - 3;

    if (len < 3)
        return TOKENIZER_AB3_REFUSED;
    if ((t[0] == 'b' && t[1] == 'b' && t[2] == '2') || (t[0] == '.' && t[1] == 'b' && t[2] == 'b'))
        return TOKENIZER_AB3_TOKENISED;
    if ((t[0] == 'a' && t[1] == 'b' && (t[2] == '3' || t[2] == '2'))
        || (t[0] == 'A' && t[1] == 'B' && t[2] == '3'))
        return TOKENIZER_AB3_TEXT;
    return TOKENIZER_AB3_REFUSED;
}

/* the extension equals ext (lower case) in any case */
static int isext(const char *name, size_t len, const char *ext)
{
    size_t e = tokenizer_ab3_getext(name, len);

    for (; e < len && *ext; e++, ext++) {
        char c = name[e];

        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (c != *ext)
            return 0;
    }
    return e == len && !*ext;
}

int tokenizer_ab3_source(const char *name, size_t len)
{
    if (isext(name, len, "bb") || isext(name, len, "bb2"))
        return 0;
    if (isext(name, len, "ab2"))
        return TOKENIZER_AB3_TEXT;
    return TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER;
}
