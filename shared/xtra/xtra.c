/*
 * xtra.c - the common part of the .xtra code (xtra.h): defaults, the
 * Blitz 2 .xtra (read, write) and the BB22 header.
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
#include "xtra.h"

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

/* 1 if p[0..n) has a 0 byte */
static int terminated(const uint8_t *p, size_t n)
{
    while (n--)
        if (!*p++)
            return 1;
    return 0;
}

static int fail(struct xtra_error *err, int code, size_t offset)
{
    if (err) {
        err->code = code;
        err->offset = offset;
        err->line = 0;
    }
    return code;
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
    case XTRA_E_TOO_MANY: return "too many object maxima";
    case XTRA_E_SPACE: return "output buffer too small";
    case XTRA_E_BB22: return "not a BB22 header (\"BB22\", version 8 or 10, word +10 = 0, +14 = 2)";
    case XTRA_E_AB3_LINE: return "header line does not start with ';'";
    case XTRA_E_AB3_END: return "header without \"; /XTRA\"";
    case XTRA_E_AB3_NUMBER: return "value is not a number in range";
    case XTRA_E_AB3_QUOTE: return "string without its closing '\"'";
    case XTRA_E_AB3_LONG: return "text longer than its field";
    case XTRA_E_AB3_RESIDENTS: return "more than 8 residents";
    case XTRA_E_AB3_VERSION: return "Version is not MAJOR.MINOR.PATCH (0..255 each)";
    case XTRA_E_AB3_NAME: return "\"Max\" without object name";
    }
    return "unknown error";
}

void xtra_init(struct xtra *x)
{
    zero(x, sizeof(*x));
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
    x->sw[XTRA_SW_OPTREQ2GA5] = XTRA_ON;
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

int xtra_add_max(struct xtra *x, int lib, const char *name, uint16_t max)
{
    struct xtra_max *m;
    size_t i;

    if (x->nmax >= XTRA_MAXMAXIMA)
        return XTRA_E_TOO_MANY;
    m = &x->max[x->nmax++];
    zero(m, sizeof(*m));
    m->max = max;
    if (lib >= 0) {
        m->lib = (uint16_t)lib;
        m->flags |= XTRA_MAX_LIB;
    }
    if (name) {
        for (i = 0; i + 1 < XTRA_MAXNAME && name[i]; i++)
            m->name[i] = name[i];
        m->flags |= XTRA_MAX_NAME;
    }
    return XTRA_OK;
}

/* ------------------------------------------------------------ Blitz 2 */

int xtra_read_blitz2(struct xtra *x, const uint8_t *buf, size_t size, struct xtra_error *err)
{
    size_t fixed, n, i;
    const uint8_t *p;

    xtra_init(x);
    if (err)
        zero(err, sizeof(*err));
    if (size < 1)
        return fail(err, XTRA_E_SHORT, 0);
    if (buf[0] > 1)
        return fail(err, XTRA_E_VERS, 0);
    fixed = buf[0] ? XTRA_FIXED_V1 : XTRA_FIXED_V0;
    if (size < fixed)
        return fail(err, XTRA_E_SHORT, size);
    if ((size - fixed) & 3)
        return fail(err, XTRA_E_SIZE, size);
    n = (size - fixed) >> 2;
    if (n > XTRA_MAXMAXIMA)
        return fail(err, XTRA_E_TOO_MANY, fixed + 4 * (size_t)XTRA_MAXMAXIMA);

    for (i = 0; i < XTRA_NRES; i++)
        if (!terminated(buf + OFF_RES + i * RES_SIZE, XTRA_RESNAME))
            return fail(err, XTRA_E_UNTERMINATED, OFF_RES + i * RES_SIZE);
    if (!terminated(buf + OFF_PATH, XTRA_PATHLEN))
        return fail(err, XTRA_E_UNTERMINATED, OFF_PATH);
    if (!terminated(buf + OFF_NAME, XTRA_PATHLEN))
        return fail(err, XTRA_E_UNTERMINATED, OFF_NAME);
    if (buf[0] && !terminated(buf + OFF_CLI, XTRA_CLILEN))
        return fail(err, XTRA_E_UNTERMINATED, OFF_CLI);

    x->source = XTRA_SRC_BLITZ2;
    x->vers = buf[0];
    x->icon = buf[OFF_ICON];
    for (i = 0; i < XTRA_NSWITCH; i++)
        x->sw[i] = get16(buf + OFF_SW + 2 * i);
    for (i = 0; i < XTRA_NBUFFER; i++)
        x->buf[i] = get32(buf + OFF_BUF + 4 * i);
    for (i = 0; i < XTRA_NRES; i++) {
        p = buf + OFF_RES + i * RES_SIZE;
        copy(x->res[i].name, p, XTRA_RESNAME);
        x->res[i].ptr = get32(p + XTRA_RESNAME);
    }
    copy(x->exe_path, buf + OFF_PATH, XTRA_PATHLEN);
    copy(x->exe_name, buf + OFF_NAME, XTRA_PATHLEN);
    if (x->vers)
        copy(x->cli, buf + OFF_CLI, XTRA_CLILEN);
    for (i = 0, p = buf + fixed; i < n; i++, p += 4) {
        x->max[i].lib = get16(p);
        x->max[i].max = get16(p + 2);
        x->max[i].flags = XTRA_MAX_LIB;
    }
    x->nmax = (uint16_t)n;
    return XTRA_OK;
}

size_t xtra_blitz2_size(const struct xtra *x)
{
    size_t size = x->vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0;
    unsigned i;

    for (i = 0; i < x->nmax; i++)
        if (x->max[i].flags & XTRA_MAX_LIB)
            size += 4;
    return size;
}

int xtra_write_blitz2(const struct xtra *x, uint8_t *out, size_t cap, size_t *len,
                      unsigned *skipped, struct xtra_error *err)
{
    size_t size, i;
    unsigned skip = 0;
    uint8_t *p;

    if (err)
        zero(err, sizeof(*err));
    if (len)
        *len = 0;
    if (x->vers > 1)
        return fail(err, XTRA_E_VERS, 0);
    size = xtra_blitz2_size(x);
    if (cap < size)
        return fail(err, XTRA_E_SPACE, cap);

    out[0] = x->vers;
    out[OFF_ICON] = x->icon;
    for (i = 0; i < XTRA_NSWITCH; i++)
        put16(out + OFF_SW + 2 * i, x->sw[i]);
    for (i = 0; i < XTRA_NBUFFER; i++)
        put32(out + OFF_BUF + 4 * i, x->buf[i]);
    for (i = 0; i < XTRA_NRES; i++) {
        p = out + OFF_RES + i * RES_SIZE;
        copy(p, x->res[i].name, XTRA_RESNAME);
        put32(p + XTRA_RESNAME, x->res[i].ptr);
    }
    copy(out + OFF_PATH, x->exe_path, XTRA_PATHLEN);
    copy(out + OFF_NAME, x->exe_name, XTRA_PATHLEN);
    p = out + XTRA_FIXED_V0;
    if (x->vers) {
        copy(p, x->cli, XTRA_CLILEN);
        p += XTRA_CLILEN;
    }
    for (i = 0; i < x->nmax; i++) {
        if (!(x->max[i].flags & XTRA_MAX_LIB)) {
            skip++;
            continue;
        }
        put16(p, x->max[i].lib);
        put16(p + 2, x->max[i].max);
        p += 4;
    }
    if (skipped)
        *skipped = skip;
    if (len)
        *len = size;
    return XTRA_OK;
}

/* ------------------------------------------------------------ BB22 */

int xtra_read_bb22(struct xtra_editor *ed, const uint8_t *buf, size_t size, struct xtra_error *err)
{
    unsigned version, header;

    zero(ed, sizeof(*ed));
    if (err)
        zero(err, sizeof(*err));
    if (size < 6)
        return fail(err, XTRA_E_SHORT, size);
    if (buf[0] != 'B' || buf[1] != 'B' || buf[2] != '2' || buf[3] != '2')
        return fail(err, XTRA_E_BB22, 0);
    version = get16(buf + 4);
    if (version != 8 && version != 10)
        return fail(err, XTRA_E_BB22, 4);
    header = version == 10 ? 16 : 14;
    if (size < header)
        return fail(err, XTRA_E_SHORT, size);
    if (get16(buf + 10) != 0)
        return fail(err, XTRA_E_BB22, 10);
    if (version == 10 && get16(buf + 14) != 2)
        return fail(err, XTRA_E_BB22, 14);
    ed->column = get16(buf + 6);
    ed->top = get16(buf + 12);
    ed->line = ed->top + get16(buf + 8);
    ed->flags = XTRA_ED_CURSOR | XTRA_ED_TOP;
    return XTRA_OK;
}
