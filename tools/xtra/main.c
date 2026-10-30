/*
 * xtra - the companion settings of a Blitz source (shared/xtra, xtra.h):
 * the Blitz 2 .xtra, the editor part of a BB22 source, AmiBlitz3's
 * embedded header (docs/formats.md 1 and 7).
 *
 *   xtra dump FILE.xtra [--bb22 SRC]   all fields; with the cursor of SRC
 *   xtra dump --ab3 SRC.ab3            the header of an AmiBlitz3 source
 *   xtra check [--bb22|--ab3] [--exact] FILE...
 *       Blitz 2 .xtra (default): read, written again, the same bytes;
 *       --bb22: the header of BB22 sources; --ab3: the header read,
 *       written, read again, the same fields (--exact: the written header
 *       is byte for byte the one of the file)
 *   xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC]   the header lines
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
#include "xtra_ab3.h"

static struct xtra x, y;

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

static int write_file(const char *path, const void *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    int ok;

    if (!f)
        return 0;
    ok = fwrite(data, 1, len, f) == len;
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

/* reads FILE in the format of mode into *to: 1 if ok (message if not) */
enum mode { M_BLITZ2, M_BB22, M_AB3 };

static int load(const char *path, enum mode mode, struct xtra *to, uint8_t **data, size_t *len,
                size_t *end)
{
    struct xtra_error err;
    int rc;

    if (!(*data = read_file(path, len))) {
        fprintf(stderr, "xtra: cannot read %s\n", path);
        return 0;
    }
    if (mode == M_BLITZ2)
        rc = xtra_read_blitz2(to, *data, *len, &err);
    else if (mode == M_BB22)
        rc = xtra_read_bb22(&to->editor, *data, *len, &err);
    else
        rc = xtra_read_ab3(to, *data, *len, end, &err);
    if (rc) {
        report(path, &err);
        free(*data);
        *data = NULL;
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------ dump */

/* a text field as "..." with \xNN for other bytes */
static void text(const char *label, const void *field, size_t size)
{
    const uint8_t *p = field;
    size_t i;

    printf("%-17s\"", label);
    for (i = 0; i < size && p[i]; i++)
        if (p[i] >= 0x20 && p[i] < 0x7F && p[i] != '"' && p[i] != '\\')
            putchar(p[i]);
        else
            printf("\\x%02X", p[i]);
    putchar('"');
    /* bytes after the 0: kept for the round trip */
    for (i++; i < size && !p[i]; i++)
        ;
    if (i < size)
        printf("  (bytes after the 0 from offset %zu)", i);
    putchar('\n');
}

static const char *const switches[XTRA_NSWITCH] = {
    "runtime errors", "debugger", "long arrays", "expand macros", "quoted macros",
    "make smallest", "debug info", "auto run", "interrupt check", "assembler check",
    "overflow check", "optreq2ga5", "default interrupt", "default overflow",
};
static const char *const buffers[XTRA_NBUFFER] = {"object", "libs", "system", "data", "macro", "string"};
static const char *const keys[XTRA_K_COUNT] = {
    "ExePath", "ExeFile", "CreateIcon", "Residents", "StringBuffer", "MakeSmallest",
    "FuncOptimize", "Version", "NumberOfBuilds", "CliArgs", "StackSize", "RuntimeDebug",
    "DebugInfo", "CreateDbgFile", "OverflowCheck", "AssemblerCheck", "InterruptCheck",
    "AutoRun", "CursorLine", "CursorColumn", "LabelSearch", "LabelRemark", "LabelAll",
    "LabelCase", "LabelPosition", "Max",
};

static void dump(const struct xtra *v)
{
    unsigned i, k;

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
        if (!v->res[i].name[0] && !v->res[i].ptr)
            continue;
        snprintf(label, sizeof label, "resident %u", i);
        text(label, v->res[i].name, XTRA_RESNAME);
        if (v->res[i].ptr)
            printf("%-17s$%08X (runtime value)\n", "  pointer", (unsigned)v->res[i].ptr);
    }
    text("exe path", v->exe_path, XTRA_PATHLEN);
    text("exe name", v->exe_name, XTRA_PATHLEN);
    if (v->vers || v->source != XTRA_SRC_BLITZ2)
        text("cli argument", v->cli, XTRA_CLILEN);
    printf("%-17s%u\n", "maxima", v->nmax);
    for (i = 0; i < v->nmax; i++) {
        const struct xtra_max *m = &v->max[i];
        printf("  ");
        if (m->flags & XTRA_MAX_LIB)
            printf("lib %u", m->lib);
        if (m->flags & XTRA_MAX_NAME)
            printf("%s\"%s\"", m->flags & XTRA_MAX_LIB ? " " : "", m->name);
        printf(": %u\n", m->max);
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
            printf(" %s", keys[k]);
    printf("\n  %-15s%u\n", "StackSize", (unsigned)v->ab3.stack);
    printf("  %-15s%u\n", "FuncOptimize", v->ab3.func_optimize);
    printf("  %-15s%u\n", "CreateDbgFile", v->ab3.create_dbgfile);
    printf("  %-15s%u.%u.%u\n", "Version", v->ab3.version[0], v->ab3.version[1], v->ab3.version[2]);
    printf("  %-15s%u\n", "NumberOfBuilds", (unsigned)v->ab3.builds);
    printf("  ");
    text("LabelSearch", v->ab3.label_search, XTRA_LABELLEN);
    printf("  %-15s%u, all %u, case %u, position %u\n", "LabelRemark", v->ab3.label_remark,
           v->ab3.label_all, v->ab3.label_case, (unsigned)v->ab3.label_position);
    if (v->ab3.unknown)
        printf("  unknown keys:  %u (first in line %u)\n", (unsigned)v->ab3.unknown,
               (unsigned)v->ab3.unknown_line);
}

/* ------------------------------------------------------------ check */

/* the fields of a and b equal (what the AmiBlitz3 header holds) */
static int same_ab3(const struct xtra *a, const struct xtra *b)
{
    return !memcmp(a->exe_path, b->exe_path, XTRA_PATHLEN) && !memcmp(a->exe_name, b->exe_name, XTRA_PATHLEN)
        && a->icon == b->icon && !memcmp(a->res, b->res, sizeof(a->res)) && !memcmp(a->sw, b->sw, sizeof(a->sw))
        && !memcmp(a->buf, b->buf, sizeof(a->buf)) && !memcmp(a->cli, b->cli, XTRA_CLILEN)
        && a->nmax == b->nmax && !memcmp(a->max, b->max, a->nmax * sizeof(a->max[0]))
        && a->editor.line == b->editor.line && a->editor.column == b->editor.column
        && a->ab3.stack == b->ab3.stack && a->ab3.func_optimize == b->ab3.func_optimize
        && a->ab3.create_dbgfile == b->ab3.create_dbgfile && !memcmp(a->ab3.version, b->ab3.version, 3)
        && a->ab3.builds == b->ab3.builds && !memcmp(a->ab3.label_search, b->ab3.label_search, XTRA_LABELLEN)
        && a->ab3.label_remark == b->ab3.label_remark && a->ab3.label_all == b->ab3.label_all
        && a->ab3.label_case == b->ab3.label_case && a->ab3.label_position == b->ab3.label_position;
}

static int check(const char *path, enum mode mode, int exact)
{
    static uint8_t out[XTRA_BLITZ2_MAXSIZE];
    static char text[65536];
    struct xtra_error err;
    uint8_t *data;
    size_t len, olen, end = 0, e2;
    int ok = 1;

    if (!load(path, mode, &x, &data, &len, &end))
        return 0;
    if (mode == M_BLITZ2) {
        if (xtra_write_blitz2(&x, out, sizeof out, &olen, NULL, &err)) {
            report(path, &err);
            ok = 0;
        } else if (olen != len || memcmp(out, data, len)) {
            fprintf(stderr, "%s: written again, the bytes differ\n", path);
            ok = 0;
        }
    } else if (mode == M_AB3 && x.ab3.header) {
        if (xtra_write_ab3(&x, text, sizeof text, &olen, NULL)) {
            fprintf(stderr, "%s: header too long\n", path);
            ok = 0;
        } else if (xtra_read_ab3(&y, (uint8_t *)text, olen, &e2, &err)) {
            fprintf(stderr, "%s: the written header: ", path);
            report("", &err);
            ok = 0;
        } else if (!same_ab3(&x, &y) || e2 != olen) {
            fprintf(stderr, "%s: written and read again, the fields differ\n", path);
            ok = 0;
        } else if (exact && (olen != end || memcmp(text, data, end))) {
            fprintf(stderr, "%s: written again, the header differs\n", path);
            ok = 0;
        }
    }
    free(data);
    return ok;
}

/* ------------------------------------------------------------ main */

static const char usage[] =
    "usage: xtra dump FILE.xtra [--bb22 SRC]\n"
    "       xtra dump --ab3 SRC.ab3\n"
    "       xtra check [--bb22|--ab3] [--exact] FILE...\n"
    "       xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC]\n"
    "       xtra convert --to-xtra IN.ab3 OUT.xtra\n"
    "The companion settings of a Blitz source: the Blitz 2 .xtra (vers 0\n"
    "and 1), the cursor in the header of a BB22 source, AmiBlitz3's\n"
    "embedded header (docs/formats.md 1 and 7).\n"
    "dump      all fields (with --bb22: the cursor of the BB22 source SRC)\n"
    "check     every FILE read and written again: a Blitz 2 .xtra gives the\n"
    "          same bytes; --bb22: BB22 headers; --ab3: the header gives\n"
    "          the same fields (--exact: the same bytes)\n"
    "convert   a Blitz 2 .xtra into header lines, a header into a .xtra;\n"
    "          maxima the other format cannot name are left out (stderr)\n";

static int bad_args(void)
{
    fputs(usage, stderr);
    return 2;
}

int main(int argc, char **argv)
{
    static uint8_t out[XTRA_BLITZ2_MAXSIZE];
    static char text[65536];
    uint8_t *data, *src;
    size_t len, slen, end, olen;
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
            if (!load(argv[3], M_AB3, &x, &data, &len, &end))
                return 1;
        } else if (argc == 3 || (argc == 5 && !strcmp(argv[3], "--bb22"))) {
            if (!load(argv[2], M_BLITZ2, &x, &data, &len, &end))
                return 1;
            if (argc == 5) {
                if (!load(argv[4], M_BB22, &y, &src, &slen, &end))
                    return 1;
                x.editor = y.editor;
                free(src);
            }
        } else {
            return bad_args();
        }
        dump(&x);
        free(data);
        return 0;
    }

    if (!strcmp(argv[1], "check")) {
        enum mode mode = M_BLITZ2;
        int exact = 0, files = 0;

        rc = 0;
        for (i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--bb22") && !files)
                mode = M_BB22;
            else if (!strcmp(argv[i], "--ab3") && !files)
                mode = M_AB3;
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
        if (!strcmp(argv[2], "--to-ab3") && (argc == 5 || (argc == 7 && !strcmp(argv[5], "--bb22")))) {
            if (!load(argv[3], M_BLITZ2, &x, &data, &len, &end))
                return 1;
            free(data);
            if (argc == 7) {
                if (!load(argv[6], M_BB22, &y, &src, &slen, &end))
                    return 1;
                free(src);
                x.editor = y.editor;
            }
            if (xtra_write_ab3(&x, text, sizeof text, &olen, &skipped)) {
                fprintf(stderr, "xtra: header too long\n");
                return 1;
            }
            if (skipped)
                fprintf(stderr, "xtra: %u maxima by library number left out (the header names objects)\n",
                        skipped);
            if (!write_file(argv[4], text, olen)) {
                fprintf(stderr, "xtra: cannot write %s\n", argv[4]);
                return 1;
            }
            return 0;
        }
        if (!strcmp(argv[2], "--to-xtra") && argc == 5) {
            struct xtra_error err;
            if (!load(argv[3], M_AB3, &x, &data, &len, &end))
                return 1;
            free(data);
            if (!x.ab3.header)
                fprintf(stderr, "xtra: %s has no header: AmiBlitz3's defaults\n", argv[3]);
            if (xtra_write_blitz2(&x, out, sizeof out, &olen, &skipped, &err)) {
                report(argv[4], &err);
                return 1;
            }
            for (i = 0; i < x.nmax; i++)
                if (!(x.max[i].flags & XTRA_MAX_LIB))
                    fprintf(stderr, "xtra: maximum \"%s\" = %u left out (the .xtra needs the library number)\n",
                            x.max[i].name, x.max[i].max);
            if (!write_file(argv[4], out, olen)) {
                fprintf(stderr, "xtra: cannot write %s\n", argv[4]);
                return 1;
            }
            return 0;
        }
    }
    return bad_args();
}
