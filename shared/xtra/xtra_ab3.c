/*
 * xtra_ab3.c - AmiBlitz3's "Embedded .xtra Header" (xtra.h,
 * docs/formats.md 7): the comment lines at the start of an .ab3 text
 * source,
 *
 *   ; XTRA
 *   ; Embedded .xtra Header
 *   ; ...
 *   ; ExePath         = "RAM:"
 *   ; CreateIcon      = 0
 *   ; Max Screen      = 5
 *   ; /XTRA
 *
 * as AmiBlitz3's compiler writes (write_AB3Header) and reads
 * (ParseAB3Header) it, Sourcecodes/Amiblitz3/Compiler/CLIModeSupport.ab3
 * (AmiBlitz3 on GitHub, commit b1a9dee150ef).  Own code after the format;
 * stricter than AmiBlitz3 where AmiBlitz3 guesses (docs/formats.md 7.4).
 * The texts and the "Max" lines stay in the header (views); the writer
 * updates the kept header line by line or writes a new one.
 *
 * No libc, no 32-bit multiply/divide, no writable static data.
 */
#include "xtra_int.h"

/* ------------------------------------------------------------ helpers */

static int lower(int c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

static int blank(int c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/* a piece of the input */
struct span {
    const char *p;
    size_t n;
};

static void trim(struct span *s)
{
    while (s->n && blank(s->p[0])) {
        s->p++;
        s->n--;
    }
    while (s->n && blank(s->p[s->n - 1]))
        s->n--;
}

/* s equals the 0-terminated word w, ignoring case */
static int same(struct span s, const char *w)
{
    size_t i;

    for (i = 0; i < s.n; i++)
        if (!w[i] || lower((unsigned char)s.p[i]) != lower((unsigned char)w[i]))
            return 0;
    return w[i] == 0;
}

/* v * 10 + d without multiply; 0 on overflow of 32 bits */
static int dec(uint32_t *v, unsigned d)
{
    if (*v > 429496729u || (*v == 429496729u && d > 5))
        return 0;
    *v = (*v << 3) + (*v << 1) + d;
    return 1;
}

/* a number: decimal or $hex (AmiBlitz3's Val), 32 bits; a '-' in front
   gives the two's complement (-1, Blitz's True, is in AmiBlitz3's own
   sources: "LabelAll = -1"); 1 if ok */
static int number(struct span s, uint32_t *out)
{
    uint32_t v = 0;
    size_t i = 0;
    unsigned d;

    if (s.n && s.p[0] == '-') {
        s.p++;
        s.n--;
        if (!s.n || s.p[0] == '-' || !number(s, &v) || v > 0x80000000u)
            return 0;
        *out = 0u - v;
        return 1;
    }
    if (s.n && s.p[0] == '$') {
        if (s.n < 2 || s.n > 9)
            return 0;
        for (i = 1; i < s.n; i++) {
            int c = lower((unsigned char)s.p[i]);
            if (c >= '0' && c <= '9')
                d = (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f')
                d = (unsigned)(c - 'a' + 10);
            else
                return 0;
            v = (v << 4) | d;
        }
        *out = v;
        return 1;
    }
    if (!s.n)
        return 0;
    for (i = 0; i < s.n; i++) {
        if (s.p[i] < '0' || s.p[i] > '9' || !dec(&v, (unsigned)(s.p[i] - '0')))
            return 0;
    }
    *out = v;
    return 1;
}

/* a text value: "..." (the quotes removed) or bare; 1 if ok */
static int text(struct span *s)
{
    if (s->n && s->p[0] == '"') {
        if (s->n < 2 || s->p[s->n - 1] != '"')
            return 0;
        s->p++;
        s->n -= 2;
    }
    return 1;
}

/* the text field f: a view of s in the header h */
static int view(struct xtra *x, int f, const uint8_t *h, struct span s)
{
    struct xtra_text *t = &x->text[f];

    if (s.n >= xtra_text_cap(f))
        return XTRA_E_LONG;
    t->in = XTRA_IN_HEADER;
    t->set = NULL;
    t->off = (uint32_t)((const uint8_t *)s.p - h);
    t->len = (uint16_t)s.n;
    return XTRA_OK;
}

static void setsw(struct xtra *x, int sw, uint32_t v)
{
    x->sw[sw] = v ? XTRA_ON : 0;
}

const char *xtra_ab3_keyname(int k)
{
    switch (k) {
    case XTRA_K_EXEPATH: return "ExePath";
    case XTRA_K_EXEFILE: return "ExeFile";
    case XTRA_K_CREATEICON: return "CreateIcon";
    case XTRA_K_RESIDENTS: return "Residents";
    case XTRA_K_STRINGBUFFER: return "StringBuffer";
    case XTRA_K_MAKESMALLEST: return "MakeSmallest";
    case XTRA_K_FUNCOPTIMIZE: return "FuncOptimize";
    case XTRA_K_VERSION: return "Version";
    case XTRA_K_NUMBEROFBUILDS: return "NumberOfBuilds";
    case XTRA_K_CLIARGS: return "CliArgs";
    case XTRA_K_STACKSIZE: return "StackSize";
    case XTRA_K_RUNTIMEDEBUG: return "RuntimeDebug";
    case XTRA_K_DEBUGINFO: return "DebugInfo";
    case XTRA_K_CREATEDBGFILE: return "CreateDbgFile";
    case XTRA_K_OVERFLOWCHECK: return "OverflowCheck";
    case XTRA_K_ASSEMBLERCHECK: return "AssemblerCheck";
    case XTRA_K_INTERRUPTCHECK: return "InterruptCheck";
    case XTRA_K_AUTORUN: return "AutoRun";
    case XTRA_K_CURSORLINE: return "CursorLine";
    case XTRA_K_CURSORCOLUMN: return "CursorColumn";
    case XTRA_K_LABELSEARCH: return "LabelSearch";
    case XTRA_K_LABELREMARK: return "LabelRemark";
    case XTRA_K_LABELALL: return "LabelAll";
    case XTRA_K_LABELCASE: return "LabelCase";
    case XTRA_K_LABELPOSITION: return "LabelPosition";
    case XTRA_K_MAX: return "Max";
    }
    return "";
}

/* the key of a "KEY = VALUE" line: enum xtra_ab3_key, XTRA_K_MAX for
   "Max NAME" (*name: NAME), XTRA_K_COUNT for an unknown key */
static int keyof(struct span key, struct span *name)
{
    int k;

    trim(&key);
    if (key.n >= 3 && same((struct span){key.p, 3}, "max") && (key.n == 3 || blank(key.p[3]))) {
        name->p = key.p + 3;
        name->n = key.n - 3;
        trim(name);
        return XTRA_K_MAX;
    }
    for (k = 0; k < XTRA_K_MAX; k++)
        if (same(key, xtra_ab3_keyname(k)))
            return k;
    return XTRA_K_COUNT;
}

/* Version "MAJOR.MINOR.PATCH", each 0..255 */
static int version(struct span s, uint8_t v[3])
{
    struct span part;
    uint32_t n;
    int i;

    for (i = 0; i < 3; i++) {
        part.p = s.p;
        part.n = 0;
        while (part.n < s.n && s.p[part.n] != '.')
            part.n++;
        if (!number(part, &n) || part.p[0] == '$' || n > 255)
            return 0;
        v[i] = (uint8_t)n;
        if (i < 2) {
            if (part.n == s.n)
                return 0;
            s.p += part.n + 1;
            s.n -= part.n + 1;
        } else if (part.n != s.n) {
            return 0;
        }
    }
    return 1;
}

/* Residents "A,B,...": the parts as given (also empty ones) */
static int residents(struct xtra *x, const uint8_t *h, struct span s)
{
    struct span part;
    int i, rc;

    for (i = 0; i < XTRA_NRES; i++) {
        x->text[XTRA_F_RES0 + i].in = XTRA_IN_NONE;
        x->text[XTRA_F_RES0 + i].len = 0;
    }
    if (!s.n)
        return XTRA_OK;
    for (i = 0;; i++) {
        if (i == XTRA_NRES)
            return XTRA_E_AB3_RESIDENTS;
        part.p = s.p;
        part.n = 0;
        while (part.n < s.n && s.p[part.n] != ',')
            part.n++;
        if ((rc = view(x, XTRA_F_RES0 + i, h, part)))
            return rc;
        if (part.n == s.n)
            return XTRA_OK;
        s.p += part.n + 1;
        s.n -= part.n + 1;
    }
}

/* "Max NAME = N": checked and counted; the line stays in the header */
static int maximum(struct xtra *x, struct span name, struct span value, size_t lstart)
{
    uint32_t v;

    if (!name.n)
        return XTRA_E_AB3_NAME;
    if (name.n > 0xFFFF)
        return XTRA_E_LONG;
    if (!number(value, &v) || v > 0xFFFF)
        return XTRA_E_AB3_NUMBER;
    if (x->max.in != XTRA_IN_HEADER) {
        x->max.in = XTRA_IN_HEADER;
        x->max.off = (uint32_t)lstart;
        x->max.n = 0;
    }
    x->max.n++;
    return XTRA_OK;
}

/* one "KEY = VALUE" line of the header h (starting at lstart) into x */
static int keyvalue(struct xtra *x, const uint8_t *h, struct span key, struct span value, uint32_t line,
                    size_t lstart)
{
    struct span name;
    uint32_t v = 0;
    int k;

    trim(&value);
    k = keyof(key, &name);
    if (k == XTRA_K_MAX) {
        x->ab3.seen |= 1ul << XTRA_K_MAX;
        return maximum(x, name, value, lstart);
    }
    if (k == XTRA_K_COUNT) {
        if (!x->ab3.unknown++)
            x->ab3.unknown_line = line;
        return XTRA_OK;
    }
    switch (k) {
    case XTRA_K_EXEPATH: case XTRA_K_EXEFILE: case XTRA_K_RESIDENTS:
    case XTRA_K_CLIARGS: case XTRA_K_LABELSEARCH:
        if (!text(&value))
            return XTRA_E_AB3_QUOTE;
        break;
    case XTRA_K_VERSION:
        if (!version(value, x->ab3.version))
            return XTRA_E_AB3_VERSION;
        break;
    default:
        if (!number(value, &v))
            return XTRA_E_AB3_NUMBER;
    }
    x->ab3.seen |= 1ul << k;
    switch (k) {
    case XTRA_K_EXEPATH: return view(x, XTRA_F_EXE_PATH, h, value);
    case XTRA_K_EXEFILE: return view(x, XTRA_F_EXE_NAME, h, value);
    case XTRA_K_RESIDENTS: return residents(x, h, value);
    case XTRA_K_CLIARGS: return view(x, XTRA_F_CLI, h, value);
    case XTRA_K_LABELSEARCH: return view(x, XTRA_F_LABEL_SEARCH, h, value);
    case XTRA_K_CREATEICON: x->icon = v ? XTRA_ON : 0; break;
    case XTRA_K_STRINGBUFFER: x->buf[XTRA_BUF_STRING] = v; break;
    case XTRA_K_MAKESMALLEST: setsw(x, XTRA_SW_MAKE_SMALLEST, v); break;
    case XTRA_K_FUNCOPTIMIZE: x->ab3.func_optimize = v != 0; break;
    case XTRA_K_NUMBEROFBUILDS: x->ab3.builds = v; break;
    case XTRA_K_STACKSIZE: x->ab3.stack = v; break;
    case XTRA_K_RUNTIMEDEBUG: setsw(x, XTRA_SW_RUNTIME_ERRORS, v); break;
    case XTRA_K_DEBUGINFO: setsw(x, XTRA_SW_DEBUG_INFO, v); break;
    case XTRA_K_CREATEDBGFILE: x->ab3.create_dbgfile = v != 0; break;
    case XTRA_K_OVERFLOWCHECK: setsw(x, XTRA_SW_OVERFLOW_CHECK, v); break;
    case XTRA_K_ASSEMBLERCHECK: setsw(x, XTRA_SW_ASSEMBLER_CHECK, v); break;
    case XTRA_K_INTERRUPTCHECK: setsw(x, XTRA_SW_INTERRUPT_CHECK, v); break;
    case XTRA_K_AUTORUN: setsw(x, XTRA_SW_AUTO_RUN, v); break;
    case XTRA_K_CURSORLINE:
        x->editor.line = v && v < 0x80000000u ? v - 1 : 0;   /* 1-based; 0 and below: 0 */
        x->editor.flags |= XTRA_ED_CURSOR;
        break;
    case XTRA_K_CURSORCOLUMN:
        x->editor.column = v && v < 0x80000000u ? v - 1 : 0;
        x->editor.flags |= XTRA_ED_CURSOR;
        break;
    case XTRA_K_LABELREMARK: x->ab3.label_remark = (uint16_t)v; break;
    case XTRA_K_LABELALL: x->ab3.label_all = (uint16_t)v; break;
    case XTRA_K_LABELCASE: x->ab3.label_case = (uint16_t)v; break;
    case XTRA_K_LABELPOSITION: x->ab3.label_position = v; break;
    }
    return XTRA_OK;
}

/* the line at *pos (without LF): s; *pos behind its LF */
static void nextline(const uint8_t *buf, size_t size, size_t *pos, struct span *s)
{
    s->p = (const char *)buf + *pos;
    s->n = 0;
    while (*pos < size && buf[*pos] != '\n') {
        (*pos)++;
        s->n++;
    }
    if (*pos < size)
        (*pos)++;
}

/* ";" blanks WORD blanks: the marker lines "; XTRA" and "; /XTRA" */
static int marker(struct span s, const char *word)
{
    trim(&s);
    if (!s.n || s.p[0] != ';')
        return 0;
    s.p++;
    s.n--;
    trim(&s);
    return same(s, word);
}

/* a trimmed line ";KEY=VALUE": key and value (untrimmed); 0 for other
   lines */
static int split(struct span t, struct span *key, struct span *value)
{
    size_t i;

    if (!t.n || t.p[0] != ';')
        return 0;
    for (i = 1; i < t.n && t.p[i] != '='; i++)
        ;
    if (i == t.n)
        return 0;
    key->p = t.p + 1;
    key->n = i - 1;
    value->p = t.p + i + 1;
    value->n = t.n - i - 1;
    return 1;
}

int xtra_ab3_has_header(const uint8_t *buf, size_t size)
{
    struct span s;
    size_t pos = 0;

    if (!buf)
        return 0;
    nextline(buf, size, &pos, &s);
    return marker(s, "XTRA");
}

void xtra_ab3_defaults(struct xtra *x)
{
    x->source = XTRA_SRC_AB3_TEXT;
    x->text[XTRA_F_EXE_PATH].in = XTRA_IN_SET;
    x->text[XTRA_F_EXE_PATH].set = (const uint8_t *)"RAM:";
    x->text[XTRA_F_EXE_PATH].len = 4;
    x->text[XTRA_F_EXE_NAME].in = XTRA_IN_SET;
    x->text[XTRA_F_EXE_NAME].set = (const uint8_t *)"Prog.exe";
    x->text[XTRA_F_EXE_NAME].len = 8;
    x->icon = 0;
    x->sw[XTRA_SW_MAKE_SMALLEST] = 0;
    x->sw[XTRA_SW_RUNTIME_ERRORS] = XTRA_ON;
    x->sw[XTRA_SW_DEBUG_INFO] = 0;
    x->sw[XTRA_SW_OVERFLOW_CHECK] = XTRA_ON;
    x->sw[XTRA_SW_ASSEMBLER_CHECK] = 0;
    x->sw[XTRA_SW_INTERRUPT_CHECK] = XTRA_ON;
    x->sw[XTRA_SW_AUTO_RUN] = XTRA_ON;
    x->buf[XTRA_BUF_STRING] = 10240;            /* #DEFAULT_STRINGBUFFERSIZE */
}

int xtra_ab3_read(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err)
{
    struct span s, key, value;
    size_t pos = 0, lstart;
    uint32_t line = 1;
    int rc;

    xtra_ab3_defaults(x);
    if (!xtra_ab3_has_header(buf, size))
        return XTRA_OK;                         /* no header */
    x->ab3.header = 1;
    nextline(buf, size, &pos, &s);
    for (;;) {
        if (pos >= size)
            return xtra_fail(err, XTRA_E_AB3_END, XTRA_IN_HEADER, 0, line + 1);
        line++;
        lstart = pos;
        nextline(buf, size, &pos, &s);
        trim(&s);
        if (!s.n)
            continue;
        if (s.p[0] != ';')
            return xtra_fail(err, XTRA_E_AB3_LINE, XTRA_IN_HEADER, 0, line);
        if (marker(s, "/XTRA"))
            break;
        if (!split(s, &key, &value))
            continue;                           /* title, ruler, empty */
        rc = keyvalue(x, buf, key, value, line, lstart);
        if (rc)
            return xtra_fail(err, rc, XTRA_IN_HEADER, 0, line);
    }
    x->hdr = (uint32_t)pos;
    return XTRA_OK;
}

int xtra_ab3_next_max(const uint8_t *h, uint32_t *pos, uint32_t end, struct xtra_max *m)
{
    struct span s, key, value, name;
    size_t p = *pos;
    uint32_t v;

    while (p < end) {
        nextline(h, end, &p, &s);
        trim(&s);
        if (!split(s, &key, &value) || keyof(key, &name) != XTRA_K_MAX)
            continue;
        trim(&value);
        if (!number(value, &v))
            v = 0;
        m->name = (const uint8_t *)name.p;
        m->namelen = (uint16_t)name.n;
        m->lib = 0;
        m->max = (uint16_t)v;
        m->flags = XTRA_MAX_NAME;
        *pos = (uint32_t)p;
        return 1;
    }
    *pos = (uint32_t)p;
    return 0;
}

/* ------------------------------------------------------------ writing */

struct out {
    char *p;
    size_t n, cap;
    const char *nl;             /* line end */
};

static void ch(struct out *o, char c)
{
    if (o->n < o->cap)
        o->p[o->n] = c;
    o->n++;
}

static void str(struct out *o, const char *s)
{
    while (*s)
        ch(o, *s++);
}

static void bytes(struct out *o, const void *p, size_t n)
{
    const char *s = (const char *)p;

    while (n--)
        ch(o, *s++);
}

/* decimal, without divide: subtracting powers of ten */
static void num(struct out *o, uint32_t v)
{
    static const uint32_t pow10[] = {
        1000000000u, 100000000u, 10000000u, 1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u
    };
    int i, started = 0;
    char d;

    for (i = 0; i < 10; i++) {
        for (d = '0'; v >= pow10[i]; d++)
            v -= pow10[i];
        if (d != '0' || started || i == 9) {
            ch(o, d);
            started = 1;
        }
    }
}

/* a long as AmiBlitz3's Str$ shows it: signed */
static void snum(struct out *o, uint32_t v)
{
    if (v & 0x80000000u) {
        ch(o, '-');
        v = 0u - v;
    }
    num(o, v);
}

/* a word value as a long: sign-extended */
static uint32_t word(uint16_t v)
{
    return (v & 0x8000u) ? 0xFFFF0000u | v : v;
}

/* "; KEY" padded to 18 columns, "= " (AmiBlitz3's layout) */
static void key(struct out *o, int k)
{
    const char *s = xtra_ab3_keyname(k);
    size_t n = 2;

    str(o, "; ");
    for (; *s; s++, n++)
        ch(o, *s);
    for (; n < 18; n++)
        ch(o, ' ');
    str(o, "= ");
}

static int on(uint16_t v)
{
    return (v & XTRA_ON) != 0;
}

/* the editor's line/column as the header has it (0-based; 0 without) */
static uint32_t cursor(const struct xtra *x, int column)
{
    if (!(x->editor.flags & XTRA_ED_CURSOR))
        return 0;
    return column ? x->editor.column : x->editor.line;
}

/* the value of key k (not XTRA_K_MAX) as AmiBlitz3 writes it */
static int value(struct out *o, int k, const struct xtra *x, const struct xtra_base *b)
{
    const uint8_t *s;
    size_t n, j;
    int i, comma;

    switch (k) {
    case XTRA_K_EXEPATH: case XTRA_K_EXEFILE: case XTRA_K_CLIARGS: case XTRA_K_LABELSEARCH:
        s = xtra_text(x, b,
                      k == XTRA_K_EXEPATH ? XTRA_F_EXE_PATH
                      : k == XTRA_K_EXEFILE ? XTRA_F_EXE_NAME
                      : k == XTRA_K_CLIARGS ? XTRA_F_CLI : XTRA_F_LABEL_SEARCH, &n);
        if (!s)
            return XTRA_E_BASE;
        ch(o, '"');
        bytes(o, s, n);
        ch(o, '"');
        break;
    case XTRA_K_RESIDENTS:
        /* AmiBlitz3's joining: a ',' before each further entry unless the
           text so far ends with one, one trailing ',' removed */
        ch(o, '"');
        comma = 0;                      /* the text so far ends with ',' */
        for (i = 0; i < XTRA_NRES; i++) {
            if (!(s = xtra_text(x, b, XTRA_F_RES0 + i, &n)))
                return XTRA_E_BASE;
            if (i && !comma) {
                ch(o, ',');
                comma = 1;
            }
            for (j = 0; j < n; j++) {
                ch(o, (char)s[j]);
                comma = s[j] == ',';
            }
        }
        if (comma)
            o->n--;
        ch(o, '"');
        break;
    case XTRA_K_CREATEICON: num(o, on(x->icon)); break;
    case XTRA_K_STRINGBUFFER: snum(o, x->buf[XTRA_BUF_STRING]); break;
    case XTRA_K_MAKESMALLEST: num(o, on(x->sw[XTRA_SW_MAKE_SMALLEST])); break;
    case XTRA_K_FUNCOPTIMIZE: num(o, x->ab3.func_optimize != 0); break;
    case XTRA_K_VERSION:
        num(o, x->ab3.version[0]);
        ch(o, '.');
        num(o, x->ab3.version[1]);
        ch(o, '.');
        num(o, x->ab3.version[2]);
        break;
    case XTRA_K_NUMBEROFBUILDS: snum(o, x->ab3.builds); break;
    case XTRA_K_STACKSIZE: snum(o, x->ab3.stack); break;
    case XTRA_K_RUNTIMEDEBUG: num(o, on(x->sw[XTRA_SW_RUNTIME_ERRORS])); break;
    case XTRA_K_DEBUGINFO: num(o, on(x->sw[XTRA_SW_DEBUG_INFO])); break;
    case XTRA_K_CREATEDBGFILE: num(o, x->ab3.create_dbgfile != 0); break;
    case XTRA_K_OVERFLOWCHECK: num(o, on(x->sw[XTRA_SW_OVERFLOW_CHECK])); break;
    case XTRA_K_ASSEMBLERCHECK: num(o, on(x->sw[XTRA_SW_ASSEMBLER_CHECK])); break;
    case XTRA_K_INTERRUPTCHECK: num(o, on(x->sw[XTRA_SW_INTERRUPT_CHECK])); break;
    case XTRA_K_AUTORUN: num(o, on(x->sw[XTRA_SW_AUTO_RUN])); break;
    case XTRA_K_CURSORLINE: snum(o, cursor(x, 0) + 1); break;
    case XTRA_K_CURSORCOLUMN: snum(o, cursor(x, 1) + 1); break;
    case XTRA_K_LABELREMARK: snum(o, word(x->ab3.label_remark)); break;
    case XTRA_K_LABELALL: snum(o, word(x->ab3.label_all)); break;
    case XTRA_K_LABELCASE: snum(o, word(x->ab3.label_case)); break;
    case XTRA_K_LABELPOSITION: snum(o, x->ab3.label_position); break;
    }
    return XTRA_OK;
}

/* a whole line "; KEY = VALUE" */
static int keyline(struct out *o, int k, const struct xtra *x, const struct xtra_base *b)
{
    int rc;

    key(o, k);
    if ((rc = value(o, k, x, b)))
        return rc;
    str(o, o->nl);
    return XTRA_OK;
}

/* 1 if the texts f of x and y have the same bytes */
static int same_text(const struct xtra *x, const struct xtra *y, const struct xtra_base *b, int f)
{
    const uint8_t *s, *t;
    size_t n, m, i;

    s = xtra_text(x, b, f, &n);
    t = xtra_text(y, b, f, &m);
    if (!s || !t || n != m)
        return 0;
    for (i = 0; i < n; i++)
        if (s[i] != t[i])
            return 0;
    return 1;
}

/* 1 if key k (not XTRA_K_MAX) has the same value in x and y, as the
   header shows it (booleans as on/off) */
static int same_value(int k, const struct xtra *x, const struct xtra *y, const struct xtra_base *b)
{
    int i;

    switch (k) {
    case XTRA_K_EXEPATH: return same_text(x, y, b, XTRA_F_EXE_PATH);
    case XTRA_K_EXEFILE: return same_text(x, y, b, XTRA_F_EXE_NAME);
    case XTRA_K_CLIARGS: return same_text(x, y, b, XTRA_F_CLI);
    case XTRA_K_LABELSEARCH: return same_text(x, y, b, XTRA_F_LABEL_SEARCH);
    case XTRA_K_RESIDENTS:
        for (i = 0; i < XTRA_NRES; i++)
            if (!same_text(x, y, b, XTRA_F_RES0 + i))
                return 0;
        return 1;
    case XTRA_K_CREATEICON: return on(x->icon) == on(y->icon);
    case XTRA_K_STRINGBUFFER: return x->buf[XTRA_BUF_STRING] == y->buf[XTRA_BUF_STRING];
    case XTRA_K_MAKESMALLEST: return on(x->sw[XTRA_SW_MAKE_SMALLEST]) == on(y->sw[XTRA_SW_MAKE_SMALLEST]);
    case XTRA_K_FUNCOPTIMIZE: return !x->ab3.func_optimize == !y->ab3.func_optimize;
    case XTRA_K_VERSION:
        return x->ab3.version[0] == y->ab3.version[0] && x->ab3.version[1] == y->ab3.version[1]
            && x->ab3.version[2] == y->ab3.version[2];
    case XTRA_K_NUMBEROFBUILDS: return x->ab3.builds == y->ab3.builds;
    case XTRA_K_STACKSIZE: return x->ab3.stack == y->ab3.stack;
    case XTRA_K_RUNTIMEDEBUG: return on(x->sw[XTRA_SW_RUNTIME_ERRORS]) == on(y->sw[XTRA_SW_RUNTIME_ERRORS]);
    case XTRA_K_DEBUGINFO: return on(x->sw[XTRA_SW_DEBUG_INFO]) == on(y->sw[XTRA_SW_DEBUG_INFO]);
    case XTRA_K_CREATEDBGFILE: return !x->ab3.create_dbgfile == !y->ab3.create_dbgfile;
    case XTRA_K_OVERFLOWCHECK: return on(x->sw[XTRA_SW_OVERFLOW_CHECK]) == on(y->sw[XTRA_SW_OVERFLOW_CHECK]);
    case XTRA_K_ASSEMBLERCHECK: return on(x->sw[XTRA_SW_ASSEMBLER_CHECK]) == on(y->sw[XTRA_SW_ASSEMBLER_CHECK]);
    case XTRA_K_INTERRUPTCHECK: return on(x->sw[XTRA_SW_INTERRUPT_CHECK]) == on(y->sw[XTRA_SW_INTERRUPT_CHECK]);
    case XTRA_K_AUTORUN: return on(x->sw[XTRA_SW_AUTO_RUN]) == on(y->sw[XTRA_SW_AUTO_RUN]);
    case XTRA_K_CURSORLINE: return cursor(x, 0) == cursor(y, 0);
    case XTRA_K_CURSORCOLUMN: return cursor(x, 1) == cursor(y, 1);
    case XTRA_K_LABELREMARK: return x->ab3.label_remark == y->ab3.label_remark;
    case XTRA_K_LABELALL: return x->ab3.label_all == y->ab3.label_all;
    case XTRA_K_LABELCASE: return x->ab3.label_case == y->ab3.label_case;
    case XTRA_K_LABELPOSITION: return x->ab3.label_position == y->ab3.label_position;
    }
    return 1;
}

/* the "Max" lines of the maxima with a name (the others: *skip) */
static void maxlines(struct out *o, const struct xtra *x, const struct xtra_base *b, unsigned *skip)
{
    struct xtra_iter it;
    struct xtra_max m;
    size_t n, k;

    xtra_max_begin(&it, x, b);
    while (xtra_next_max(&it, &m)) {
        if (!(m.flags & XTRA_MAX_NAME)) {
            (*skip)++;
            continue;
        }
        str(o, "; Max ");
        n = 6;
        for (k = 0; k < m.namelen; k++, n++)
            ch(o, m.name[k] == ' ' ? '-' : (char)m.name[k]);
        for (; n < 18; n++)
            ch(o, ' ');
        str(o, "= ");
        num(o, m.max);
        str(o, o->nl);
    }
}

static void section(struct out *o, const char *title)
{
    str(o, "; \n; ");
    str(o, title);
    str(o, "\n; -------------------------------------------------------\n");
}

/* a new header in the layout of write_AB3Header */
static int fresh(struct out *o, const struct xtra *x, const struct xtra_base *b, unsigned *skip)
{
    static const signed char order[] = {
        -1, XTRA_K_EXEPATH, XTRA_K_EXEFILE, XTRA_K_CREATEICON, XTRA_K_RESIDENTS,
        -2, XTRA_K_STRINGBUFFER, XTRA_K_MAKESMALLEST, XTRA_K_FUNCOPTIMIZE, XTRA_K_VERSION,
        XTRA_K_NUMBEROFBUILDS,
        -3, XTRA_K_CLIARGS, XTRA_K_STACKSIZE, XTRA_K_RUNTIMEDEBUG, XTRA_K_DEBUGINFO,
        XTRA_K_CREATEDBGFILE, XTRA_K_OVERFLOWCHECK, XTRA_K_ASSEMBLERCHECK, XTRA_K_INTERRUPTCHECK,
        XTRA_K_AUTORUN,
        -4, XTRA_K_CURSORLINE, XTRA_K_CURSORCOLUMN, XTRA_K_LABELSEARCH, XTRA_K_LABELREMARK,
        XTRA_K_LABELALL, XTRA_K_LABELCASE, XTRA_K_LABELPOSITION,
        -5
    };
    size_t i;
    int rc;

    str(o, "; XTRA\n; Embedded .xtra Header\n");
    for (i = 0; i < sizeof(order); i++) {
        switch (order[i]) {
        case -1: section(o, "General Info"); break;
        case -2: section(o, "Compiler"); break;
        case -3: section(o, "Debugger"); break;
        case -4: section(o, "Editor"); break;
        case -5: section(o, "Blitz Objects"); break;
        default:
            if ((rc = keyline(o, order[i], x, b)))
                return rc;
        }
    }
    maxlines(o, x, b, skip);
    str(o, "; /XTRA\n");
    return XTRA_OK;
}

/* the key of a header line (enum xtra_ab3_key, XTRA_K_COUNT unknown), -1
   for a line without "KEY = VALUE" or the end marker */
static int linekey(struct span s, struct span *key, struct span *value)
{
    struct span name;

    trim(&s);
    if (marker(s, "/XTRA") || !split(s, key, value))
        return -1;
    return keyof(*key, &name);
}

/* 1 if a line of h[pos..end) has key k */
static int later(const uint8_t *h, size_t pos, size_t end, int k)
{
    struct span s, key, value;

    while (pos < end) {
        nextline(h, end, &pos, &s);
        if (linekey(s, &key, &value) == k)
            return 1;
    }
    return 0;
}

/* the kept header h[0..x->hdr), updated line by line */
static int update(struct out *o, const struct xtra *x, const struct xtra_base *b, unsigned *skip)
{
    const uint8_t *h = b->header;
    size_t size = x->hdr, pos = 0, lstart, eq, vs, ve, cend;
    struct span s, t, key, val;
    struct xtra d;              /* what a line says */
    int k, rc, maxset = x->max.in != XTRA_IN_HEADER && x->max.in != XTRA_IN_NONE;

    nextline(h, size, &pos, &s);                /* "; XTRA" */
    bytes(o, h, pos);
    while (pos < size) {
        lstart = pos;
        nextline(h, size, &pos, &s);
        t = s;
        trim(&t);
        if (marker(t, "/XTRA")) {
            /* the line end of the header's last line for added lines */
            o->nl = s.n && s.p[s.n - 1] == '\r' ? "\r\n" : "\n";
            xtra_init(&d);
            xtra_ab3_defaults(&d);
            for (k = 0; k < XTRA_K_MAX; k++)
                if (!(x->ab3.seen & (1ul << k)) && !same_value(k, x, &d, b)
                    && (rc = keyline(o, k, x, b)))
                    return rc;
            if (maxset)
                maxlines(o, x, b, skip);
            bytes(o, h + lstart, pos - lstart);
            continue;
        }
        k = linekey(s, &key, &val);
        if (k < 0 || k == XTRA_K_COUNT || (k == XTRA_K_MAX && !maxset)) {
            bytes(o, h + lstart, pos - lstart);
            continue;
        }
        if (k == XTRA_K_MAX)
            continue;                           /* replaced */
        if (later(h, pos, size, k)) {           /* the last one counts */
            bytes(o, h + lstart, pos - lstart);
            continue;
        }
        xtra_init(&d);
        xtra_ab3_defaults(&d);
        if (keyvalue(&d, h, key, val, 0, lstart) || same_value(k, x, &d, b)) {
            bytes(o, h + lstart, pos - lstart);
            continue;
        }
        /* the new value in place of the old one, the rest kept */
        eq = (size_t)((const uint8_t *)val.p - h) - 1;
        cend = lstart + s.n;
        if (cend > eq + 1 && h[cend - 1] == '\r')
            cend--;
        for (vs = eq + 1; vs < cend && (h[vs] == ' ' || h[vs] == '\t'); vs++)
            ;
        for (ve = cend; ve > vs && blank(h[ve - 1]); ve--)
            ;
        bytes(o, h + lstart, vs - lstart);
        if (vs == eq + 1)
            ch(o, ' ');                         /* AmiBlitz3 drops the first character */
        if ((rc = value(o, k, x, b)))
            return rc;
        bytes(o, h + ve, pos - ve);
    }
    return XTRA_OK;
}

int xtra_write_ab3(const struct xtra *x, const struct xtra_base *b, char *outbuf, size_t cap,
                   size_t *len, unsigned *skipped)
{
    struct out o;
    unsigned skip = 0;
    int rc;

    o.p = outbuf;
    o.n = 0;
    o.cap = outbuf ? cap : 0;
    o.nl = "\n";
    if (x->ab3.header && x->hdr)
        rc = b && b->header ? update(&o, x, b, &skip) : XTRA_E_BASE;
    else
        rc = fresh(&o, x, b, &skip);
    if (skipped)
        *skipped = skip;
    if (len)
        *len = o.n;
    if (rc)
        return rc;
    return o.n <= cap ? XTRA_OK : XTRA_E_SPACE;
}

size_t xtra_ab3_size(const struct xtra *x, const struct xtra_base *b)
{
    size_t len = 0;
    int rc = xtra_write_ab3(x, b, NULL, 0, &len, NULL);

    return rc == XTRA_OK || rc == XTRA_E_SPACE ? len : 0;
}
