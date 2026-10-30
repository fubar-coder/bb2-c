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
 *
 * No libc, no 32-bit multiply/divide, no writable static data.
 */
#include "xtra_ab3.h"

/* ------------------------------------------------------------ helpers */

static void zero(void *p, size_t n)
{
    uint8_t *d = (uint8_t *)p;

    while (n--)
        *d++ = 0;
}

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

static int fail(struct xtra_error *err, int code, uint32_t line)
{
    if (err) {
        err->code = code;
        err->offset = 0;
        err->line = line;
    }
    return code;
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

/* s into the field to[0..size): 0-terminated, the rest 0; 1 if it fits */
static int put(void *to, size_t size, struct span s)
{
    uint8_t *d = (uint8_t *)to;
    size_t i;

    if (s.n >= size)
        return 0;
    zero(d, size);
    for (i = 0; i < s.n; i++)
        d[i] = (uint8_t)s.p[i];
    return 1;
}

static void setsw(struct xtra *x, int sw, uint32_t v)
{
    x->sw[sw] = v ? XTRA_ON : 0;
}

/* the key names; index = enum xtra_ab3_key (XTRA_K_MAX is "Max NAME") */
static const char *keyname(int k)
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
    }
    return "";
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
static int residents(struct xtra *x, struct span s)
{
    struct span part;
    int i;

    for (i = 0; i < XTRA_NRES; i++)
        zero(&x->res[i], sizeof(x->res[i]));
    if (!s.n)
        return XTRA_OK;
    for (i = 0;; i++) {
        if (i == XTRA_NRES)
            return XTRA_E_AB3_RESIDENTS;
        part.p = s.p;
        part.n = 0;
        while (part.n < s.n && s.p[part.n] != ',')
            part.n++;
        if (!put(x->res[i].name, XTRA_RESNAME, part))
            return XTRA_E_AB3_LONG;
        if (part.n == s.n)
            return XTRA_OK;
        s.p += part.n + 1;
        s.n -= part.n + 1;
    }
}

/* "Max NAME = N": the object NAME ('-' for ' ', as AmiBlitz3 writes it);
   a NAME given twice: the last value counts */
static int maximum(struct xtra *x, struct span name, struct span value)
{
    char n[XTRA_MAXNAME];
    uint32_t v;
    size_t i;
    unsigned k;

    trim(&name);
    if (!name.n)
        return XTRA_E_AB3_NAME;
    if (name.n >= XTRA_MAXNAME)
        return XTRA_E_AB3_LONG;
    if (!number(value, &v) || v > 0xFFFF)
        return XTRA_E_AB3_NUMBER;
    zero(n, sizeof(n));
    for (i = 0; i < name.n; i++)
        n[i] = name.p[i] == '-' ? ' ' : name.p[i];
    for (k = 0; k < x->nmax; k++) {
        for (i = 0; i < XTRA_MAXNAME && x->max[k].name[i] == n[i]; i++)
            if (!n[i])
                break;
        if ((x->max[k].flags & XTRA_MAX_NAME) && i < XTRA_MAXNAME && x->max[k].name[i] == n[i]) {
            x->max[k].max = (uint16_t)v;
            return XTRA_OK;
        }
    }
    return xtra_add_max(x, -1, n, (uint16_t)v);
}

/* one "KEY = VALUE" line */
static int keyvalue(struct xtra *x, struct span key, struct span value, uint32_t line)
{
    uint32_t v = 0;
    int k;

    trim(&key);
    trim(&value);
    if (key.n >= 3 && same((struct span){key.p, 3}, "max") && (key.n == 3 || blank(key.p[3]))) {
        x->ab3.seen |= 1ul << XTRA_K_MAX;
        return maximum(x, (struct span){key.p + 3, key.n - 3}, value);
    }
    for (k = 0; k < XTRA_K_MAX; k++)
        if (same(key, keyname(k)))
            break;
    if (k == XTRA_K_MAX) {
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
    case XTRA_K_EXEPATH:
        return put(x->exe_path, XTRA_PATHLEN, value) ? XTRA_OK : XTRA_E_AB3_LONG;
    case XTRA_K_EXEFILE:
        return put(x->exe_name, XTRA_PATHLEN, value) ? XTRA_OK : XTRA_E_AB3_LONG;
    case XTRA_K_RESIDENTS:
        return residents(x, value);
    case XTRA_K_CLIARGS:
        return put(x->cli, XTRA_CLILEN, value) ? XTRA_OK : XTRA_E_AB3_LONG;
    case XTRA_K_LABELSEARCH:
        return put(x->ab3.label_search, XTRA_LABELLEN, value) ? XTRA_OK : XTRA_E_AB3_LONG;
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

int xtra_read_ab3(struct xtra *x, const uint8_t *buf, size_t size, size_t *end,
                  struct xtra_error *err)
{
    static const uint8_t ram[] = "RAM:", prog[] = "Prog.exe";
    struct span s, key, value;
    size_t pos = 0, i;
    uint32_t line = 1;
    int rc;

    xtra_init(x);
    if (err)
        zero(err, sizeof(*err));
    *end = 0;
    /* ParseAB3Header's defaults for what the header can set */
    x->source = XTRA_SRC_AB3_TEXT;
    for (i = 0; i < sizeof(ram); i++)
        x->exe_path[i] = ram[i];
    for (i = 0; i < sizeof(prog); i++)
        x->exe_name[i] = prog[i];
    x->icon = 0;
    x->sw[XTRA_SW_MAKE_SMALLEST] = 0;
    x->sw[XTRA_SW_RUNTIME_ERRORS] = XTRA_ON;
    x->sw[XTRA_SW_DEBUG_INFO] = 0;
    x->sw[XTRA_SW_OVERFLOW_CHECK] = XTRA_ON;
    x->sw[XTRA_SW_ASSEMBLER_CHECK] = 0;
    x->sw[XTRA_SW_INTERRUPT_CHECK] = XTRA_ON;
    x->sw[XTRA_SW_AUTO_RUN] = XTRA_ON;
    x->buf[XTRA_BUF_STRING] = 10240;            /* #DEFAULT_STRINGBUFFERSIZE */

    nextline(buf, size, &pos, &s);
    if (!marker(s, "XTRA"))
        return XTRA_OK;                         /* no header */
    x->ab3.header = 1;
    for (;;) {
        if (pos >= size)
            return fail(err, XTRA_E_AB3_END, line + 1);
        line++;
        nextline(buf, size, &pos, &s);
        trim(&s);
        if (!s.n)
            continue;
        if (s.p[0] != ';')
            return fail(err, XTRA_E_AB3_LINE, line);
        if (marker(s, "/XTRA"))
            break;
        for (i = 1; i < s.n && s.p[i] != '='; i++)
            ;
        if (i == s.n)
            continue;                           /* title, ruler, empty */
        key.p = s.p + 1;
        key.n = i - 1;
        value.p = s.p + i + 1;
        value.n = s.n - i - 1;
        rc = keyvalue(x, key, value, line);
        if (rc)
            return fail(err, rc, line);
    }
    *end = pos;
    return XTRA_OK;
}

/* ------------------------------------------------------------ writing */

struct out {
    char *p;
    size_t n, cap;
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

/* a field: the bytes up to its 0 */
static void field(struct out *o, const void *f, size_t size)
{
    const char *s = (const char *)f;
    size_t i;

    for (i = 0; i < size && s[i]; i++)
        ch(o, s[i]);
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

/* "; KEY" padded to 18 columns, "= " (AmiBlitz3's layout) */
static void key(struct out *o, int k)
{
    const char *s = keyname(k);
    size_t n = 2;

    str(o, "; ");
    for (; *s; s++, n++)
        ch(o, *s);
    for (; n < 18; n++)
        ch(o, ' ');
    str(o, "= ");
}

/* a word value as a long: sign-extended */
static uint32_t word(uint16_t v)
{
    return (v & 0x8000u) ? 0xFFFF0000u | v : v;
}

/* a long value as AmiBlitz3's Str$ shows it: signed */
static void numline(struct out *o, int k, uint32_t v)
{
    key(o, k);
    if (v & 0x80000000u) {
        ch(o, '-');
        v = 0u - v;
    }
    num(o, v);
    ch(o, '\n');
}

static void textline(struct out *o, int k, const void *f, size_t size)
{
    key(o, k);
    ch(o, '"');
    field(o, f, size);
    str(o, "\"\n");
}

static void section(struct out *o, const char *title)
{
    str(o, "; \n; ");
    str(o, title);
    str(o, "\n; -------------------------------------------------------\n");
}

int xtra_write_ab3(const struct xtra *x, char *outbuf, size_t cap, size_t *len, unsigned *skipped)
{
    struct out o = {outbuf, 0, cap};
    unsigned i, skip = 0;
    size_t n;
    int k, comma;

    str(&o, "; XTRA\n; Embedded .xtra Header\n");
    section(&o, "General Info");
    textline(&o, XTRA_K_EXEPATH, x->exe_path, XTRA_PATHLEN);
    textline(&o, XTRA_K_EXEFILE, x->exe_name, XTRA_PATHLEN);
    numline(&o, XTRA_K_CREATEICON, (x->icon & XTRA_ON) != 0);
    /* AmiBlitz3's joining: a ',' before each further entry unless the
       text so far ends with one, one trailing ',' removed */
    key(&o, XTRA_K_RESIDENTS);
    ch(&o, '"');
    comma = 0;                          /* the text so far ends with ',' */
    for (i = 0; i < XTRA_NRES; i++) {
        if (i && !comma) {
            ch(&o, ',');
            comma = 1;
        }
        for (k = 0; k < XTRA_RESNAME && x->res[i].name[k]; k++) {
            ch(&o, (char)x->res[i].name[k]);
            comma = x->res[i].name[k] == ',';
        }
    }
    if (comma)
        o.n--;
    str(&o, "\"\n");
    section(&o, "Compiler");
    numline(&o, XTRA_K_STRINGBUFFER, x->buf[XTRA_BUF_STRING]);
    numline(&o, XTRA_K_MAKESMALLEST, (x->sw[XTRA_SW_MAKE_SMALLEST] & XTRA_ON) != 0);
    numline(&o, XTRA_K_FUNCOPTIMIZE, x->ab3.func_optimize != 0);
    key(&o, XTRA_K_VERSION);
    num(&o, x->ab3.version[0]);
    ch(&o, '.');
    num(&o, x->ab3.version[1]);
    ch(&o, '.');
    num(&o, x->ab3.version[2]);
    ch(&o, '\n');
    numline(&o, XTRA_K_NUMBEROFBUILDS, x->ab3.builds);
    section(&o, "Debugger");
    textline(&o, XTRA_K_CLIARGS, x->cli, XTRA_CLILEN);
    numline(&o, XTRA_K_STACKSIZE, x->ab3.stack);
    numline(&o, XTRA_K_RUNTIMEDEBUG, (x->sw[XTRA_SW_RUNTIME_ERRORS] & XTRA_ON) != 0);
    numline(&o, XTRA_K_DEBUGINFO, (x->sw[XTRA_SW_DEBUG_INFO] & XTRA_ON) != 0);
    numline(&o, XTRA_K_CREATEDBGFILE, x->ab3.create_dbgfile != 0);
    numline(&o, XTRA_K_OVERFLOWCHECK, (x->sw[XTRA_SW_OVERFLOW_CHECK] & XTRA_ON) != 0);
    numline(&o, XTRA_K_ASSEMBLERCHECK, (x->sw[XTRA_SW_ASSEMBLER_CHECK] & XTRA_ON) != 0);
    numline(&o, XTRA_K_INTERRUPTCHECK, (x->sw[XTRA_SW_INTERRUPT_CHECK] & XTRA_ON) != 0);
    numline(&o, XTRA_K_AUTORUN, (x->sw[XTRA_SW_AUTO_RUN] & XTRA_ON) != 0);
    section(&o, "Editor");
    numline(&o, XTRA_K_CURSORLINE, (x->editor.flags & XTRA_ED_CURSOR) ? x->editor.line + 1 : 1);
    numline(&o, XTRA_K_CURSORCOLUMN, (x->editor.flags & XTRA_ED_CURSOR) ? x->editor.column + 1 : 1);
    textline(&o, XTRA_K_LABELSEARCH, x->ab3.label_search, XTRA_LABELLEN);
    numline(&o, XTRA_K_LABELREMARK, word(x->ab3.label_remark));
    numline(&o, XTRA_K_LABELALL, word(x->ab3.label_all));
    numline(&o, XTRA_K_LABELCASE, word(x->ab3.label_case));
    numline(&o, XTRA_K_LABELPOSITION, x->ab3.label_position);
    section(&o, "Blitz Objects");
    for (i = 0; i < x->nmax; i++) {
        if (!(x->max[i].flags & XTRA_MAX_NAME)) {
            skip++;
            continue;
        }
        str(&o, "; Max ");
        n = 6;
        for (k = 0; k < XTRA_MAXNAME && x->max[i].name[k]; k++, n++)
            ch(&o, x->max[i].name[k] == ' ' ? '-' : x->max[i].name[k]);
        for (; n < 18; n++)
            ch(&o, ' ');
        str(&o, "= ");
        num(&o, x->max[i].max);
        ch(&o, '\n');
    }
    str(&o, "; /XTRA\n");
    if (skipped)
        *skipped = skip;
    if (len)
        *len = o.n;
    return o.n <= cap ? XTRA_OK : XTRA_E_SPACE;
}
