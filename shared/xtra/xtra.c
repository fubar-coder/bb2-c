/*
 * xtra.c - the common part of the .xtra code (xtra.h): defaults, file
 * types, loading, the fields, the Blitz 2 .xtra (read, write) and the
 * BB22 header.
 *
 * The Blitz 2 layout (docs/formats.md 1.1; blitz2.s:15290 optspnts, the
 * C port programs/blitz2/src/data.c loadxtra_c/savextra_c):
 *
 *   0    1        vers (0: no CLI block, 1: with)
 *   1    1        icon (low byte of optreqga9's Flags)
 *   2    14 * 2   switches (enum xtra_switch)
 *   30   6 * 4    buffer sizes (enum xtra_buffer)
 *   54   8 * 68   residents: 64 bytes path, 4 bytes pointer (runtime)
 *   598  192      path of the executable
 *   790  192      name of the executable
 *   982  128      CLI argument (vers 1 only)
 *   982/1110      {lib.w, max.w} up to the end of the file
 *
 * No libc, no 32-bit multiply/divide (68000 without libgcc), no writable
 * static data.
 */
#include "xtra_int.h"

#define OFF_ICON   1
#define OFF_SW     2
#define OFF_BUF    30
#define OFF_RES    54
#define RES_SIZE   68
#define OFF_PATH   598
#define OFF_NAME   790
#define OFF_CLI    982

/* ------------------------------------------------------------ helpers */

static void zero(void *p, size_t n)
{
    uint8_t *d = (uint8_t *)p;

    while (n--)
        *d++ = 0;
}

static void copy(void *to, const void *from, size_t n)
{
    uint8_t *d = (uint8_t *)to;
    const uint8_t *s = (const uint8_t *)from;

    while (n--)
        *d++ = *s++;
}

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* the index of the first 0 byte of p[0..n), n if there is none */
static size_t fieldlen(const uint8_t *p, size_t n)
{
    size_t i;

    for (i = 0; i < n && p[i]; i++)
        ;
    return i;
}

int xtra_fail(struct xtra_error *err, int code, int in, size_t offset, uint32_t line)
{
    if (err) {
        err->code = code;
        err->in = in;
        err->offset = offset;
        err->line = line;
    }
    return code;
}

static int fail(struct xtra_error *err, int code, int in, size_t offset)
{
    return xtra_fail(err, code, in, offset, 0);
}

/* ------------------------------------------------------------ common */

const char *xtra_strerror(int code)
{
    switch (code) {
    case XTRA_OK: return "ok";
    case XTRA_E_SHORT: return "file too short";
    case XTRA_E_SIZE: return "size is not the fixed part (982 or 1110 bytes) plus 4 per maximum";
    case XTRA_E_VERS: return "vers is neither 0 nor 1";
    case XTRA_E_UNTERMINATED: return "text field without its 0 byte";
    case XTRA_E_SPACE: return "output buffer too small";
    case XTRA_E_BB22: return "not a BB22 header (\"BB22\", version 8 or 10, word +10 = 0, +14 = 2)";
    case XTRA_E_AB3_LINE: return "header line does not start with ';'";
    case XTRA_E_AB3_END: return "header without \"; /XTRA\"";
    case XTRA_E_AB3_NUMBER: return "value is not a number in range";
    case XTRA_E_AB3_QUOTE: return "string without its closing '\"'";
    case XTRA_E_LONG: return "text longer than its field";
    case XTRA_E_AB3_RESIDENTS: return "more than 8 residents";
    case XTRA_E_AB3_VERSION: return "Version is not MAJOR.MINOR.PATCH (0..255 each)";
    case XTRA_E_AB3_NAME: return "\"Max\" without object name";
    case XTRA_E_TYPE: return "unknown source type";
    case XTRA_E_FIELD: return "unknown text field";
    case XTRA_E_TEXT: return "text with a 0, CR or LF byte";
    case XTRA_E_BASE: return "the bytes of a view are not given";
    }
    return "unknown error";
}

void xtra_init(struct xtra *x)
{
    zero(x, sizeof(*x));
    x->type = XTRA_T_TED;
    x->source = XTRA_SRC_NONE;
    x->vers = 1;
    /* initial Flags of the option gadgets (programs/blitz2/src/data.c,
       options.src/options2.src.new) */
    x->icon = XTRA_ON;                          /* optreqga9 */
    x->sw[XTRA_SW_RUNTIME_ERRORS] = XTRA_ON;
    x->sw[XTRA_SW_DEBUGGER] = XTRA_ON;
    x->sw[XTRA_SW_LONG_ARRAYS] = XTRA_ON;
    x->sw[XTRA_SW_EXPAND_MACROS] = XTRA_ON;
    x->sw[XTRA_SW_AUTO_RUN] = XTRA_ON;
    x->sw[XTRA_SW_INTERRUPT_CHECK] = XTRA_ON;
    x->sw[XTRA_SW_WARNINGS] = XTRA_ON;         /* warnings on */
    x->sw[XTRA_SW_DEFAULT_INTERRUPT] = XTRA_ON;
    x->sw[XTRA_SW_DEFAULT_OVERFLOW] = XTRA_ON;
    /* the LongInt of optreqsg1..6 */
    x->buf[XTRA_BUF_OBJECT] = 16384;
    x->buf[XTRA_BUF_LIBS] = 32768;
    x->buf[XTRA_BUF_SYSTEM] = 4096;
    x->buf[XTRA_BUF_DATA] = 4096;
    x->buf[XTRA_BUF_MACRO] = 8192;
    x->buf[XTRA_BUF_STRING] = 10240;
    /* AmiBlitz3's own fields: ParseAB3Header's defaults */
    x->ab3.stack = 32768;                       /* #DEFAULT_STACKSIZE */
}

void xtra_apply_vers0(struct xtra *x)
{
    x->sw[XTRA_SW_AUTO_RUN] = 128;
    x->sw[XTRA_SW_INTERRUPT_CHECK] = x->sw[XTRA_SW_DEFAULT_INTERRUPT];
    x->sw[XTRA_SW_ASSEMBLER_CHECK] = 0;
    x->sw[XTRA_SW_OVERFLOW_CHECK] = x->sw[XTRA_SW_DEFAULT_OVERFLOW];
}

/* ------------------------------------------------------------ fields */

size_t xtra_text_cap(int field)
{
    if (field >= XTRA_F_RES0 && field < XTRA_F_COUNT)
        return XTRA_RESNAME;
    switch (field) {
    case XTRA_F_EXE_PATH: case XTRA_F_EXE_NAME: return XTRA_PATHLEN;
    case XTRA_F_CLI: return XTRA_CLILEN;
    case XTRA_F_LABEL_SEARCH: return XTRA_LABELLEN;
    }
    return 0;
}

const uint8_t *xtra_text(const struct xtra *x, const struct xtra_base *b, int field, size_t *len)
{
    const struct xtra_text *t;
    const uint8_t *base = NULL;

    *len = 0;
    if (field < 0 || field >= XTRA_F_COUNT)
        return NULL;
    t = &x->text[field];
    switch (t->in) {
    case XTRA_IN_SET:
        *len = t->len;
        return t->set;
    case XTRA_IN_XTRA:
        base = b ? b->xtra : NULL;
        break;
    case XTRA_IN_HEADER:
        base = b ? b->header : NULL;
        break;
    default:
        return (const uint8_t *)"";
    }
    if (!base)
        return NULL;
    *len = t->len;
    return base + t->off;
}

int xtra_set_text(struct xtra *x, int field, const void *p, size_t len)
{
    const uint8_t *s = (const uint8_t *)p;
    size_t i;

    if (field < 0 || field >= XTRA_F_COUNT)
        return XTRA_E_FIELD;
    if (len >= xtra_text_cap(field))
        return XTRA_E_LONG;
    for (i = 0; i < len; i++)
        if (!s[i] || s[i] == '\r' || s[i] == '\n')
            return XTRA_E_TEXT;
    x->text[field].in = XTRA_IN_SET;
    x->text[field].set = s;
    x->text[field].off = 0;
    x->text[field].len = (uint16_t)len;
    return XTRA_OK;
}

void xtra_set_maxima(struct xtra *x, const struct xtra_max *m, uint32_t n)
{
    x->max.in = XTRA_IN_SET;
    x->max.set = m;
    x->max.off = 0;
    x->max.n = n;
}

void xtra_max_begin(struct xtra_iter *it, const struct xtra *x, const struct xtra_base *b)
{
    it->x = x;
    it->pos = x->max.off;
    it->left = x->max.n;
    it->base = NULL;
    it->set = x->max.set;
    if (x->max.in == XTRA_IN_XTRA)
        it->base = b ? b->xtra : NULL;
    else if (x->max.in == XTRA_IN_HEADER)
        it->base = b ? b->header : NULL;
    else if (x->max.in != XTRA_IN_SET)
        it->left = 0;
    if (!it->base && x->max.in != XTRA_IN_SET)
        it->left = 0;
}

int xtra_next_max(struct xtra_iter *it, struct xtra_max *m)
{
    const struct xtra *x = it->x;

    if (!it->left)
        return 0;
    switch (x->max.in) {
    case XTRA_IN_SET:
        *m = *it->set++;
        break;
    case XTRA_IN_XTRA:
        m->name = NULL;
        m->namelen = 0;
        m->lib = get16(it->base + it->pos);
        m->max = get16(it->base + it->pos + 2);
        m->flags = XTRA_MAX_LIB;
        it->pos += 4;
        break;
    default:
        if (!xtra_ab3_next_max(it->base, &it->pos, x->hdr, m)) {
            it->left = 0;
            return 0;
        }
    }
    it->left--;
    return 1;
}

void xtra_take_blitz2(struct xtra *to, const struct xtra *from)
{
    unsigned i;

    to->vers = from->vers;
    to->icon = from->icon;
    for (i = 0; i < XTRA_NSWITCH; i++)
        to->sw[i] = from->sw[i];
    for (i = 0; i < XTRA_NBUFFER; i++)
        to->buf[i] = from->buf[i];
    for (i = 0; i < XTRA_NRES; i++) {
        to->res_ptr[i] = from->res_ptr[i];
        to->text[XTRA_F_RES0 + i] = from->text[XTRA_F_RES0 + i];
    }
    to->text[XTRA_F_EXE_PATH] = from->text[XTRA_F_EXE_PATH];
    to->text[XTRA_F_EXE_NAME] = from->text[XTRA_F_EXE_NAME];
    to->text[XTRA_F_CLI] = from->text[XTRA_F_CLI];
}

/* ------------------------------------------------------------ loading */

int xtra_detect(const uint8_t *buf, size_t len, const char *path)
{
    static const char ext[] = ".ab3";
    size_t i, n;

    if (len >= 4 && buf[0] == 'B' && buf[1] == 'B' && buf[2] == '2' && buf[3] == '2')
        return XTRA_T_BB22;
    for (i = 0; i < len; i++)
        if (!buf[i])
            return XTRA_T_TED;
    if (xtra_ab3_has_header(buf, len))
        return XTRA_T_AB3;
    if (path) {
        for (n = 0; path[n]; n++)
            ;
        if (n >= 4) {
            for (i = 0; i < 4; i++) {
                char c = path[n - 4 + i];
                if (c >= 'A' && c <= 'Z')
                    c = (char)(c + ('a' - 'A'));
                if (c != ext[i])
                    break;
            }
            if (i == 4)
                return XTRA_T_AB3;
        }
    }
    return XTRA_T_TEXT;
}

int xtra_has_companion(int type)
{
    return type == XTRA_T_TED || type == XTRA_T_BB22 || type == XTRA_T_TEXT;
}

int xtra_companion_name(const char *path, char *out, size_t cap)
{
    static const char ext[] = ".xtra";
    size_t n, i;

    for (n = 0; path[n]; n++)
        ;
    if (cap < n + sizeof(ext))
        return XTRA_E_SPACE;
    copy(out, path, n);
    for (i = 0; i < sizeof(ext); i++)
        out[n + i] = ext[i];
    return XTRA_OK;
}

/* AmiBlitz3's binary .xtra since @bdae3f9e (docs/formats.md 1.3; AmiBlitz3.ab3
   ptr_xtradata, write_xtrafile at @58a11658): the 15 switch words move
   everything behind them by 2 bytes */
#define AB3_OFF_BUF   32
#define AB3_OFF_RES   56
#define AB3_OFF_PATH  600
#define AB3_OFF_NAME  792
#define AB3_OFF_CLI   984
#define AB3_CLITEXT   116

int xtra_is_ab3_xtra(const uint8_t *xtra, size_t len)
{
    size_t i;

    /* the fixed part and the 4 bytes write_xtrafile always appends; a
       multiple of 4 (a Blitz 2 file is 982/1110 + 4n: 2 more) */
    if (!xtra || len < XTRA_AB3_FIXED + 4 || (len & 3))
        return 0;
    if (xtra[0] != 1 || (xtra[OFF_ICON] & 0x7f))
        return 0;
    for (i = 0; i < XTRA_AB3_NSWITCH; i++)
        if (get16(xtra + OFF_SW + 2 * i) & ~(unsigned)XTRA_ON)
            return 0;
    /* the buffer sizes: read in the 14-word layout's place, a buffer's
       low word becomes the next one's high word ($4000 -> $40000000) */
    for (i = 0; i < XTRA_NBUFFER; i++)
        if (xtra[AB3_OFF_BUF + 4 * i])
            return 0;
    for (i = 0; i < XTRA_NRES; i++)
        if (fieldlen(xtra + AB3_OFF_RES + RES_SIZE * i, XTRA_RESNAME) == XTRA_RESNAME)
            return 0;
    return fieldlen(xtra + AB3_OFF_PATH, XTRA_PATHLEN) < XTRA_PATHLEN
        && fieldlen(xtra + AB3_OFF_NAME, XTRA_PATHLEN) < XTRA_PATHLEN
        && fieldlen(xtra + AB3_OFF_CLI, AB3_CLITEXT) < AB3_CLITEXT;
}

/* the Blitz 2 .xtra buf[0..size) into x */
static int read_blitz2(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err)
{
    size_t fixed, i;
    const uint8_t *p;

    if (size < 1)
        return fail(err, XTRA_E_SHORT, XTRA_IN_XTRA, 0);
    if (buf[0] > 1)
        return fail(err, XTRA_E_VERS, XTRA_IN_XTRA, 0);
    fixed = buf[0] ? XTRA_FIXED_V1 : XTRA_FIXED_V0;
    if (size < fixed)
        return fail(err, XTRA_E_SHORT, XTRA_IN_XTRA, size);
    if ((size - fixed) & 3)
        return fail(err, XTRA_E_SIZE, XTRA_IN_XTRA, size);
    for (i = 0; i < XTRA_NRES; i++)
        if (fieldlen(buf + OFF_RES + i * RES_SIZE, XTRA_RESNAME) == XTRA_RESNAME)
            return fail(err, XTRA_E_UNTERMINATED, XTRA_IN_XTRA, OFF_RES + i * RES_SIZE);
    if (fieldlen(buf + OFF_PATH, XTRA_PATHLEN) == XTRA_PATHLEN)
        return fail(err, XTRA_E_UNTERMINATED, XTRA_IN_XTRA, OFF_PATH);
    if (fieldlen(buf + OFF_NAME, XTRA_PATHLEN) == XTRA_PATHLEN)
        return fail(err, XTRA_E_UNTERMINATED, XTRA_IN_XTRA, OFF_NAME);
    if (buf[0] && fieldlen(buf + OFF_CLI, XTRA_CLILEN) == XTRA_CLILEN)
        return fail(err, XTRA_E_UNTERMINATED, XTRA_IN_XTRA, OFF_CLI);

    x->source = XTRA_SRC_BLITZ2;
    x->vers = buf[0];
    x->icon = buf[OFF_ICON];
    for (i = 0; i < XTRA_NSWITCH; i++)
        x->sw[i] = get16(buf + OFF_SW + 2 * i);
    for (i = 0; i < XTRA_NBUFFER; i++)
        x->buf[i] = get32(buf + OFF_BUF + 4 * i);
    for (i = 0; i < XTRA_NRES; i++) {
        struct xtra_text *t = &x->text[XTRA_F_RES0 + i];
        p = buf + OFF_RES + i * RES_SIZE;
        t->in = XTRA_IN_XTRA;
        t->off = (uint32_t)(p - buf);
        t->len = (uint16_t)fieldlen(p, XTRA_RESNAME);
        x->res_ptr[i] = get32(p + XTRA_RESNAME);
    }
    x->text[XTRA_F_EXE_PATH].in = XTRA_IN_XTRA;
    x->text[XTRA_F_EXE_PATH].off = OFF_PATH;
    x->text[XTRA_F_EXE_PATH].len = (uint16_t)fieldlen(buf + OFF_PATH, XTRA_PATHLEN);
    x->text[XTRA_F_EXE_NAME].in = XTRA_IN_XTRA;
    x->text[XTRA_F_EXE_NAME].off = OFF_NAME;
    x->text[XTRA_F_EXE_NAME].len = (uint16_t)fieldlen(buf + OFF_NAME, XTRA_PATHLEN);
    if (x->vers) {
        x->text[XTRA_F_CLI].in = XTRA_IN_XTRA;
        x->text[XTRA_F_CLI].off = OFF_CLI;
        x->text[XTRA_F_CLI].len = (uint16_t)fieldlen(buf + OFF_CLI, XTRA_CLILEN);
    }
    if (size > fixed) {
        x->max.in = XTRA_IN_XTRA;
        x->max.off = (uint32_t)fixed;
        x->max.n = (uint32_t)((size - fixed) >> 2);
    }
    return XTRA_OK;
}

/* the editor part from the header of a BB22 source (14 or 16 bytes, the
   rules of tools/lib/bb22): line = top + row, column, top */
static int read_bb22(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err)
{
    unsigned version, header;

    if (size < 6)
        return fail(err, XTRA_E_SHORT, XTRA_IN_HEADER, size);
    if (buf[0] != 'B' || buf[1] != 'B' || buf[2] != '2' || buf[3] != '2')
        return fail(err, XTRA_E_BB22, XTRA_IN_HEADER, 0);
    version = get16(buf + 4);
    if (version != 8 && version != 10)
        return fail(err, XTRA_E_BB22, XTRA_IN_HEADER, 4);
    header = version == 10 ? 16 : 14;
    if (size < header)
        return fail(err, XTRA_E_SHORT, XTRA_IN_HEADER, size);
    if (get16(buf + 10) != 0)
        return fail(err, XTRA_E_BB22, XTRA_IN_HEADER, 10);
    if (version == 10 && get16(buf + 14) != 2)
        return fail(err, XTRA_E_BB22, XTRA_IN_HEADER, 14);
    x->editor.column = get16(buf + 6);
    x->editor.top = get16(buf + 12);
    x->editor.line = x->editor.top + get16(buf + 8);
    x->editor.flags = XTRA_ED_CURSOR | XTRA_ED_TOP;
    x->hdr = header;
    return XTRA_OK;
}

int xtra_load(struct xtra *x, int type, const uint8_t *src, size_t len, const uint8_t *xtra,
              size_t xlen, struct xtra_error *err)
{
    int rc = XTRA_OK;

    xtra_init(x);
    if (err)
        zero(err, sizeof(*err));
    switch (type) {
    case XTRA_T_AB3:
        rc = xtra_ab3_read(x, src, src ? len : 0, err);
        xtra = NULL;
        break;
    case XTRA_T_BB22:
        rc = read_bb22(x, src, src ? len : 0, err);
        break;
    case XTRA_T_TED:
    case XTRA_T_TEXT:
        break;
    default:
        return fail(err, XTRA_E_TYPE, XTRA_IN_NONE, 0);
    }
    if (!rc && xtra)
        rc = read_blitz2(x, xtra, xlen, err);
    if (rc)
        xtra_init(x);
    x->type = (uint8_t)type;
    return rc;
}

/* ------------------------------------------------------------ Blitz 2 */

size_t xtra_blitz2_size(const struct xtra *x)
{
    size_t size = x->vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0;
    const struct xtra_max *m = x->max.set;
    uint32_t n;

    if (x->max.in == XTRA_IN_XTRA)
        size += (size_t)x->max.n << 2;
    else if (x->max.in == XTRA_IN_SET)
        for (n = x->max.n; n; n--, m++)
            if (m->flags & XTRA_MAX_LIB)
                size += 4;
    return size;
}

/* the text field f into out[0..cap) */
static int put_text(const struct xtra *x, const struct xtra_base *b, int f, uint8_t *out, size_t cap)
{
    const uint8_t *s;
    size_t n;

    if (x->text[f].in == XTRA_IN_XTRA) {
        if (!b || !b->xtra)
            return XTRA_E_BASE;
        copy(out, b->xtra + x->text[f].off, cap);
        return XTRA_OK;
    }
    if (!(s = xtra_text(x, b, f, &n)))
        return XTRA_E_BASE;
    if (n >= cap)
        return XTRA_E_LONG;
    zero(out, cap);
    copy(out, s, n);
    return XTRA_OK;
}

int xtra_write_blitz2(const struct xtra *x, const struct xtra_base *b, uint8_t *out, size_t cap,
                      size_t *len, unsigned *skipped, struct xtra_error *err)
{
    struct xtra_iter it;
    struct xtra_max m;
    size_t size, i;
    unsigned skip = 0;
    uint8_t *p;
    int rc;

    if (err)
        zero(err, sizeof(*err));
    if (len)
        *len = 0;
    if (skipped)
        *skipped = 0;
    if (x->vers > 1)
        return fail(err, XTRA_E_VERS, XTRA_IN_NONE, 0);
    if (x->max.in == XTRA_IN_XTRA && (!b || !b->xtra))
        return fail(err, XTRA_E_BASE, XTRA_IN_NONE, 0);
    size = xtra_blitz2_size(x);
    if (cap < size)
        return fail(err, XTRA_E_SPACE, XTRA_IN_NONE, cap);

    out[0] = x->vers;
    out[OFF_ICON] = x->icon;
    for (i = 0; i < XTRA_NSWITCH; i++)
        put16(out + OFF_SW + 2 * i, x->sw[i]);
    for (i = 0; i < XTRA_NBUFFER; i++)
        put32(out + OFF_BUF + 4 * i, x->buf[i]);
    for (i = 0; i < XTRA_NRES; i++) {
        p = out + OFF_RES + i * RES_SIZE;
        if ((rc = put_text(x, b, XTRA_F_RES0 + (int)i, p, XTRA_RESNAME)))
            return fail(err, rc, XTRA_IN_NONE, OFF_RES + i * RES_SIZE);
        put32(p + XTRA_RESNAME, x->res_ptr[i]);
    }
    if ((rc = put_text(x, b, XTRA_F_EXE_PATH, out + OFF_PATH, XTRA_PATHLEN)))
        return fail(err, rc, XTRA_IN_NONE, OFF_PATH);
    if ((rc = put_text(x, b, XTRA_F_EXE_NAME, out + OFF_NAME, XTRA_PATHLEN)))
        return fail(err, rc, XTRA_IN_NONE, OFF_NAME);
    p = out + XTRA_FIXED_V0;
    if (x->vers) {
        if ((rc = put_text(x, b, XTRA_F_CLI, p, XTRA_CLILEN)))
            return fail(err, rc, XTRA_IN_NONE, OFF_CLI);
        p += XTRA_CLILEN;
    }
    xtra_max_begin(&it, x, b);
    while (xtra_next_max(&it, &m)) {
        if (!(m.flags & XTRA_MAX_LIB)) {
            skip++;
            continue;
        }
        put16(p, m.lib);
        put16(p + 2, m.max);
        p += 4;
    }
    if (skipped)
        *skipped = skip;
    if (len)
        *len = size;
    return XTRA_OK;
}
