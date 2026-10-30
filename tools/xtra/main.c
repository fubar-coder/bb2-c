/*
 * xtra - the companion settings of a Blitz source (shared/xtra, xtra.h):
 * the Blitz 2 .xtra, the editor part of a BB22 source, AmiBlitz3's
 * embedded header (docs/formats.md 1 and 7).
 *
 *   xtra dump FILE.xtra [--bb22 SRC]   all fields; with the cursor of SRC
 *   xtra dump --ab3 SRC.ab3            the header of an AmiBlitz3 source
 *   xtra dump --source SRC             the type of SRC detected, its
 *                                      companion SRC.xtra if it has one
 *   xtra check [--bb22|--ab3] [--exact] FILE...
 *       Blitz 2 .xtra (default): read, written again, the same bytes;
 *       --bb22: the header of BB22 sources; --ab3: the header written
 *       again is byte for byte the one of the file, and a new header
 *       written from it and read again has the same fields (--exact:
 *       the new header is byte for byte the file's - for headers in
 *       AmiBlitz3's current layout)
 *   xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC | --ab3 SRC]
 *       the header lines (--bb22: with the cursor of SRC); --ab3: OUT is
 *       the source SRC with its header updated from IN (a new header if
 *       it has none)
 *   xtra convert --to-xtra IN.ab3 OUT.xtra           a Blitz 2 .xtra
 *
 * Maxima: the Blitz 2 file has library numbers, the AmiBlitz3 header
 * object names; convert leaves out what the other format cannot name
 * (message on stderr).  Exit status 1 if a file cannot be read or is
 * refused (message with offset or line), 2: wrong arguments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xtra.h"

/* a source and/or its .xtra in memory, loaded */
struct loaded {
    struct xtra x;
    uint8_t *src, *xb;
    size_t slen, xlen;
    struct xtra_base b;
};

static struct loaded a, c;

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (d = malloc((size_t)n + 1)) && fread(d, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(d);
        d = NULL;
    }
    fclose(f);
    return d;
}

static int write_file(const char *path, const void *data, size_t len, const void *data2, size_t len2)
{
    FILE *f = fopen(path, "wb");
    int ok;

    if (!f)
        return 0;
    ok = fwrite(data, 1, len, f) == len && fwrite(data2, 1, len2, f) == len2;
    ok &= fclose(f) == 0;
    if (!ok)
        remove(path);
    return ok;
}

static void report(const char *path, const struct xtra_error *err)
{
    if (err->line)
        fprintf(stderr, "%s: line %u: %s\n", path, (unsigned)err->line, xtra_strerror(err->code));
    else
        fprintf(stderr, "%s: offset %zu: %s\n", path, err->offset, xtra_strerror(err->code));
}

static void unload(struct loaded *l)
{
    free(l->src);
    free(l->xb);
    l->src = l->xb = NULL;
}

/* loads the source src (NULL: none) as type with its .xtra xpath (NULL:
   none) into *l: 1 if ok (message if not) */
static int load(struct loaded *l, int type, const char *src, const char *xpath)
{
    struct xtra_error err;

    unload(l);
    l->slen = l->xlen = 0;
    if (src && !(l->src = read_file(src, &l->slen))) {
        fprintf(stderr, "xtra: cannot read %s\n", src);
        return 0;
    }
    if (xpath && !(l->xb = read_file(xpath, &l->xlen))) {
        fprintf(stderr, "xtra: cannot read %s\n", xpath);
        unload(l);
        return 0;
    }
    if (xtra_load(&l->x, type, l->src, l->slen, l->xb, l->xlen, &err)) {
        report(err.in == XTRA_IN_XTRA ? xpath : src, &err);
        unload(l);
        return 0;
    }
    l->b.xtra = l->xb;
    l->b.header = l->src;           /* the header is at the start */
    return 1;
}

/* ------------------------------------------------------------ dump */

/* a text field as "..." with \xNN for other bytes */
static void text(const char *label, const struct loaded *l, int f)
{
    const uint8_t *p, *raw;
    size_t n, i, cap = xtra_text_cap(f);

    p = xtra_text(&l->x, &l->b, f, &n);
    printf("%-17s\"", label);
    for (i = 0; p && i < n; i++)
        if (p[i] >= 0x20 && p[i] < 0x7F && p[i] != '"' && p[i] != '\\')
            putchar(p[i]);
        else
            printf("\\x%02X", p[i]);
    putchar('"');
    /* bytes after the 0 in the .xtra: kept for the round trip */
    if (l->x.text[f].in == XTRA_IN_XTRA) {
        raw = l->b.xtra + l->x.text[f].off;
        for (i = n + 1; i < cap && !raw[i]; i++)
            ;
        if (i < cap)
            printf("  (bytes after the 0 from offset %zu)", i);
    }
    putchar('\n');
}

static const char *const switches[XTRA_NSWITCH] = {
    "runtime errors", "debugger", "long arrays", "expand macros", "quoted macros",
    "make smallest", "debug info", "auto run", "interrupt check", "assembler check",
    "overflow check", "optreq2ga5", "default interrupt", "default overflow",
};
static const char *const buffers[XTRA_NBUFFER] = {"object", "libs", "system", "data", "macro", "string"};
static const char *const types[XTRA_T_COUNT] = {
    "TED tokenised", "BB22", "AmiBlitz3 text", "text without header"
};

static void dump(const struct loaded *l)
{
    const struct xtra *v = &l->x;
    struct xtra_iter it;
    struct xtra_max m;
    unsigned i, k;

    printf("%-17s%s\n", "type", types[v->type]);
    if (v->hdr)
        printf("%-17s%u bytes kept\n", "source header", (unsigned)v->hdr);
    printf("%-17s%s\n", "source",
           v->source == XTRA_SRC_BLITZ2     ? "Blitz 2 .xtra"
           : v->source == XTRA_SRC_AB3_TEXT ? "AmiBlitz3 embedded header"
                                            : "none");
    if (v->source == XTRA_SRC_BLITZ2)
        printf("%-17s%u (%u bytes fixed)\n", "vers", v->vers, v->vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0);
    printf("%-17s%s ($%02X)\n", "icon", v->icon & XTRA_ON ? "on" : "off", v->icon);
    for (i = 0; i < XTRA_NSWITCH; i++)
        printf("%-17s%s ($%04X)\n", switches[i], v->sw[i] & XTRA_ON ? "on" : "off", v->sw[i]);
    for (i = 0; i < XTRA_NBUFFER; i++)
        printf("%-17s%u\n", buffers[i], (unsigned)v->buf[i]);
    for (i = 0; i < XTRA_NRES; i++) {
        char label[20];
        size_t n;
        xtra_text(v, &l->b, XTRA_F_RES0 + (int)i, &n);
        if (!n && !v->res_ptr[i])
            continue;
        snprintf(label, sizeof label, "resident %u", i);
        text(label, l, XTRA_F_RES0 + (int)i);
        if (v->res_ptr[i])
            printf("%-17s$%08X (runtime value)\n", "  pointer", (unsigned)v->res_ptr[i]);
    }
    text("exe path", l, XTRA_F_EXE_PATH);
    text("exe name", l, XTRA_F_EXE_NAME);
    if (v->vers || v->source != XTRA_SRC_BLITZ2)
        text("cli argument", l, XTRA_F_CLI);
    printf("%-17s%u\n", "maxima", (unsigned)v->max.n);
    xtra_max_begin(&it, v, &l->b);
    while (xtra_next_max(&it, &m)) {
        printf("  ");
        if (m.flags & XTRA_MAX_LIB)
            printf("lib %u", m.lib);
        if (m.flags & XTRA_MAX_NAME)
            printf("%s\"%.*s\"", m.flags & XTRA_MAX_LIB ? " " : "", (int)m.namelen, (const char *)m.name);
        printf(": %u\n", m.max);
    }
    if (v->editor.flags & XTRA_ED_CURSOR)
        printf("%-17sline %u, column %u (0-based)\n", "cursor", (unsigned)v->editor.line,
               (unsigned)v->editor.column);
    if (v->editor.flags & XTRA_ED_TOP)
        printf("%-17s%u (0-based)\n", "top line", (unsigned)v->editor.top);
    if (v->source != XTRA_SRC_AB3_TEXT)
        return;
    printf("AmiBlitz3 header %s\n", v->ab3.header ? "present" : "absent (defaults)");
    printf("  keys:");
    for (k = 0; k < XTRA_K_COUNT; k++)
        if (v->ab3.seen & (1ul << k))
            printf(" %s", xtra_ab3_keyname((int)k));
    printf("\n  %-15s%u\n", "StackSize", (unsigned)v->ab3.stack);
    printf("  %-15s%u\n", "FuncOptimize", v->ab3.func_optimize);
    printf("  %-15s%u\n", "CreateDbgFile", v->ab3.create_dbgfile);
    printf("  %-15s%u.%u.%u\n", "Version", v->ab3.version[0], v->ab3.version[1], v->ab3.version[2]);
    printf("  %-15s%u\n", "NumberOfBuilds", (unsigned)v->ab3.builds);
    printf("  ");
    text("LabelSearch", l, XTRA_F_LABEL_SEARCH);
    printf("  %-15s%u, all %u, case %u, position %u\n", "LabelRemark", v->ab3.label_remark,
           v->ab3.label_all, v->ab3.label_case, (unsigned)v->ab3.label_position);
    if (v->ab3.unknown)
        printf("  unknown keys:  %u (first in line %u)\n", (unsigned)v->ab3.unknown,
               (unsigned)v->ab3.unknown_line);
}

/* ------------------------------------------------------------ check */

/* the fields of p (bases pb) and q (qb) equal (what the AmiBlitz3 header
   holds); a blank in a maximum's name equals '-' */
static int same_ab3(const struct xtra *p, const struct xtra_base *pb, const struct xtra *q,
                    const struct xtra_base *qb)
{
    struct xtra_iter i, j;
    struct xtra_max m, n;
    const uint8_t *s, *t;
    size_t sn, tn, k;
    int f;

    for (f = 0; f < XTRA_F_COUNT; f++) {
        s = xtra_text(p, pb, f, &sn);
        t = xtra_text(q, qb, f, &tn);
        if (!s || !t || sn != tn || memcmp(s, t, sn))
            return 0;
    }
    if (p->icon != q->icon || memcmp(p->sw, q->sw, sizeof(p->sw)) || memcmp(p->buf, q->buf, sizeof(p->buf))
        || memcmp(p->res_ptr, q->res_ptr, sizeof(p->res_ptr)) || p->max.n != q->max.n
        || p->editor.line != q->editor.line || p->editor.column != q->editor.column
        || p->ab3.stack != q->ab3.stack || p->ab3.func_optimize != q->ab3.func_optimize
        || p->ab3.create_dbgfile != q->ab3.create_dbgfile || memcmp(p->ab3.version, q->ab3.version, 3)
        || p->ab3.builds != q->ab3.builds || p->ab3.label_remark != q->ab3.label_remark
        || p->ab3.label_all != q->ab3.label_all || p->ab3.label_case != q->ab3.label_case
        || p->ab3.label_position != q->ab3.label_position)
        return 0;
    xtra_max_begin(&i, p, pb);
    xtra_max_begin(&j, q, qb);
    while (xtra_next_max(&i, &m)) {
        if (!xtra_next_max(&j, &n) || m.max != n.max || m.flags != n.flags || m.namelen != n.namelen)
            return 0;
        for (k = 0; k < m.namelen; k++)
            if (m.name[k] != n.name[k] && !(strchr(" -", m.name[k]) && strchr(" -", n.name[k])))
                return 0;
    }
    return !xtra_next_max(&j, &n);
}

static int check(const char *path, int mode, int exact)
{
    static uint8_t out[65536];
    static char hdr[65536];
    struct xtra_error err;
    struct xtra_base nb = {NULL, (const uint8_t *)hdr};
    size_t olen;
    int ok = 1;

    if (!load(&a, mode, mode == XTRA_T_TED ? NULL : path, mode == XTRA_T_TED ? path : NULL))
        return 0;
    if (mode == XTRA_T_TED) {
        if (xtra_blitz2_size(&a.x) > sizeof out) {
            fprintf(stderr, "%s: too many maxima for this tool\n", path);
            ok = 0;
        } else if (xtra_write_blitz2(&a.x, &a.b, out, sizeof out, &olen, NULL, &err)) {
            report(path, &err);
            ok = 0;
        } else if (olen != a.xlen || memcmp(out, a.xb, olen)) {
            fprintf(stderr, "%s: written again, the bytes differ\n", path);
            ok = 0;
        }
    } else if (mode == XTRA_T_AB3 && a.x.ab3.header) {
        if (xtra_write_ab3(&a.x, &a.b, hdr, sizeof hdr, &olen, NULL)) {
            fprintf(stderr, "%s: header too long\n", path);
            ok = 0;
        } else if (olen != a.x.hdr || memcmp(hdr, a.src, olen)) {
            fprintf(stderr, "%s: written again, the header differs\n", path);
            ok = 0;
        } else {
            /* a new header from the fields, read again */
            c.x = a.x;
            c.x.ab3.header = 0;
            if (xtra_write_ab3(&c.x, &a.b, hdr, sizeof hdr, &olen, NULL)) {
                fprintf(stderr, "%s: new header too long\n", path);
                ok = 0;
            } else if (xtra_load(&c.x, XTRA_T_AB3, (const uint8_t *)hdr, olen, NULL, 0, &err)) {
                fprintf(stderr, "%s: the new header: ", path);
                report("", &err);
                ok = 0;
            } else if (!same_ab3(&a.x, &a.b, &c.x, &nb) || c.x.hdr != olen) {
                fprintf(stderr, "%s: written as new header and read again, the fields differ\n", path);
                ok = 0;
            } else if (exact && (olen != a.x.hdr || memcmp(hdr, a.src, olen))) {
                fprintf(stderr, "%s: written as new header, the header differs\n", path);
                ok = 0;
            }
        }
    }
    unload(&a);
    return ok;
}

/* ------------------------------------------------------------ main */

static const char usage[] =
    "usage: xtra dump FILE.xtra [--bb22 SRC]\n"
    "       xtra dump --ab3 SRC.ab3\n"
    "       xtra dump --source SRC\n"
    "       xtra check [--bb22|--ab3] [--exact] FILE...\n"
    "       xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC | --ab3 SRC]\n"
    "       xtra convert --to-xtra IN.ab3 OUT.xtra\n"
    "The companion settings of a Blitz source: the Blitz 2 .xtra (vers 0\n"
    "and 1), the cursor in the header of a BB22 source, AmiBlitz3's\n"
    "embedded header (docs/formats.md 1 and 7).\n"
    "dump      all fields (with --bb22: the cursor of the BB22 source SRC;\n"
    "          --source: SRC's type detected, with SRC.xtra if it has one)\n"
    "check     every FILE read and written again: a Blitz 2 .xtra gives the\n"
    "          same bytes; --bb22: BB22 headers; --ab3: the header gives the\n"
    "          same bytes, and as a new header the same fields (--exact:\n"
    "          the same bytes)\n"
    "convert   a Blitz 2 .xtra into header lines (--ab3 SRC: SRC with its\n"
    "          header updated), a header into a .xtra; maxima the other\n"
    "          format cannot name are left out (stderr)\n";

static int bad_args(void)
{
    fputs(usage, stderr);
    return 2;
}

/* the source path with its companion: 1 if ok */
static int load_source(const char *path)
{
    char name[4096];
    uint8_t *buf;
    size_t len;
    FILE *f;
    int type;

    if (!(buf = read_file(path, &len))) {
        fprintf(stderr, "xtra: cannot read %s\n", path);
        return 0;
    }
    type = xtra_detect(buf, len, path);
    free(buf);
    if (!xtra_has_companion(type))
        return load(&a, type, path, NULL);
    if (xtra_companion_name(path, name, sizeof name)) {
        fprintf(stderr, "xtra: %s: name too long\n", path);
        return 0;
    }
    if (!(f = fopen(name, "rb"))) {
        printf("%-17s%s (missing)\n", "companion", name);
        return load(&a, type, path, NULL);
    }
    fclose(f);
    printf("%-17s%s\n", "companion", name);
    return load(&a, type, path, name);
}

int main(int argc, char **argv)
{
    static uint8_t out[65536];
    static char hdr[65536];
    struct xtra_error err;
    struct xtra_iter it;
    struct xtra_max m;
    size_t olen;
    unsigned skipped;
    int i, rc;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        fputs(usage, stdout);
        return 0;
    }
    if (argc < 3)
        return bad_args();

    if (!strcmp(argv[1], "dump")) {
        if (argc == 4 && !strcmp(argv[2], "--ab3")) {
            if (!load(&a, XTRA_T_AB3, argv[3], NULL))
                return 1;
        } else if (argc == 4 && !strcmp(argv[2], "--source")) {
            if (!load_source(argv[3]))
                return 1;
        } else if (argc == 3 || (argc == 5 && !strcmp(argv[3], "--bb22"))) {
            if (!load(&a, argc == 5 ? XTRA_T_BB22 : XTRA_T_TED, argc == 5 ? argv[4] : NULL, argv[2]))
                return 1;
        } else {
            return bad_args();
        }
        dump(&a);
        unload(&a);
        return 0;
    }

    if (!strcmp(argv[1], "check")) {
        int mode = XTRA_T_TED, exact = 0, files = 0;

        rc = 0;
        for (i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--bb22") && !files)
                mode = XTRA_T_BB22;
            else if (!strcmp(argv[i], "--ab3") && !files)
                mode = XTRA_T_AB3;
            else if (!strcmp(argv[i], "--exact") && !files)
                exact = 1;
            else {
                files++;
                if (!check(argv[i], mode, exact))
                    rc = 1;
            }
        }
        if (!files)
            return bad_args();
        if (!rc)
            printf("%d file%s ok\n", files, files == 1 ? "" : "s");
        return rc;
    }

    if (!strcmp(argv[1], "convert") && argc >= 5) {
        if (!strcmp(argv[2], "--to-ab3")
            && (argc == 5 || (argc == 7 && (!strcmp(argv[5], "--bb22") || !strcmp(argv[5], "--ab3"))))) {
            const uint8_t *rest = NULL;
            size_t restlen = 0;
            struct loaded *w = &a;
            struct xtra_base b;

            if (!load(&a, XTRA_T_TED, NULL, argv[3]))
                return 1;
            b = a.b;
            if (argc == 7 && !strcmp(argv[5], "--bb22")) {
                if (!load(&c, XTRA_T_BB22, argv[6], NULL))
                    return 1;
                a.x.editor = c.x.editor;
            } else if (argc == 7) {
                /* the source's header updated with the .xtra's values */
                if (!load(&c, XTRA_T_AB3, argv[6], NULL))
                    return 1;
                xtra_take_blitz2(&c.x, &a.x);
                b.header = c.src;
                w = &c;
                rest = c.src + c.x.hdr;
                restlen = c.slen - c.x.hdr;
                if (a.x.max.n)
                    fprintf(stderr, "xtra: %u maxima by library number left out (the header names objects)\n",
                            (unsigned)a.x.max.n);
            }
            if (xtra_write_ab3(&w->x, &b, hdr, sizeof hdr, &olen, &skipped)) {
                fprintf(stderr, "xtra: header too long\n");
                return 1;
            }
            if (skipped && w == &a)
                fprintf(stderr, "xtra: %u maxima by library number left out (the header names objects)\n",
                        skipped);
            if (!write_file(argv[4], hdr, olen, rest, restlen)) {
                fprintf(stderr, "xtra: cannot write %s\n", argv[4]);
                return 1;
            }
            return 0;
        }
        if (!strcmp(argv[2], "--to-xtra") && argc == 5) {
            if (!load(&a, XTRA_T_AB3, argv[3], NULL))
                return 1;
            if (!a.x.ab3.header)
                fprintf(stderr, "xtra: %s has no header: AmiBlitz3's defaults\n", argv[3]);
            if (xtra_write_blitz2(&a.x, &a.b, out, sizeof out, &olen, &skipped, &err)) {
                report(argv[4], &err);
                return 1;
            }
            xtra_max_begin(&it, &a.x, &a.b);
            while (xtra_next_max(&it, &m))
                if (!(m.flags & XTRA_MAX_LIB))
                    fprintf(stderr, "xtra: maximum \"%.*s\" = %u left out (the .xtra needs the library number)\n",
                            (int)m.namelen, (const char *)m.name, m.max);
            if (!write_file(argv[4], out, olen, NULL, 0)) {
                fprintf(stderr, "xtra: cannot write %s\n", argv[4]);
                return 1;
            }
            return 0;
        }
    }
    return bad_args();
}
