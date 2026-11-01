/*
 * blitzres - Blitz Basic 2 resident files with constants, written
 * directly (the format blitz2's "Create Resident" writes, see
 * lib/blitzres/blitzres.h), and the contents of any resident.
 *
 *   blitzres -o OUT ITEM...
 *   blitzres --dump RES
 *   blitzres --source RES OUT
 *
 * The constants of OUT in the order of the ITEMs:
 *   --fd FILE          the public functions of an FD file: _NAME=-offset
 *   --fd-varargs FILE  the same, then _NAME for a NAMEA without NAME
 *   --equ FILE         the NAME EQU/SET EXPR of an assembler include
 *   --lvo FILE         the _LVONAME EQU -offset of an LVO include: NAME=-offset
 *   --const NAME=VALUE a constant (also a symbol for --equ)
 *   --sym NAME=VALUE   only a symbol for the expressions of --equ
 *   --skip NAME        NAME is never a constant (before the items)
 * A name again with the same value is left out (blitz2 keeps the first);
 * with another value it is an error.  OUT is read back and must hold
 * exactly these constants (else it is removed, exit status 1).
 * --source: the constants of RES (a resident with constants only) as a
 * Blitz source text OUT, one #NAME=VALUE line each (blitzres_source):
 * blitz2's Create Resident makes the same resident of it.
 */
#include <stdlib.h>
#include <string.h>
#include "blitzres.h"

static void usage(FILE *f)
{
    fprintf(f, "usage: blitzres -o OUT ITEM...\n"
               "       blitzres --dump RES\n"
               "       blitzres --source RES OUT\n"
               "A Blitz Basic 2 resident with constants, as blitz2's Create Resident\n"
               "writes it; ITEMs in order:\n"
               "  --fd FILE          public functions of an FD file as _NAME=-offset\n"
               "  --fd-varargs FILE  the same, and _NAME for a NAMEA without NAME\n"
               "  --equ FILE         NAME EQU/SET EXPR lines of an assembler include\n"
               "  --lvo FILE         _LVONAME EQU -offset lines of an LVO include as NAME\n"
               "  --const NAME=VALUE a constant (also a symbol for --equ)\n"
               "  --sym NAME=VALUE   a symbol for the expressions of --equ only\n"
               "  --skip NAME        never NAME as constant\n"
               "--dump: the types, macros and constants of a resident.\n"
               "--source: the constants of RES as Blitz source, #NAME=VALUE lines.\n");
}

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
        *len = (size_t)n;
        s[n] = 0;
    } else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* NAME=VALUE, VALUE decimal, 0x or $ hex, maybe negative */
static int name_value(char *arg, char **name, int32_t *value)
{
    char *eq = strchr(arg, '='), *v, *e;
    long long n;
    int neg;

    if (!eq || eq == arg)
        return -1;
    *eq = 0;
    *name = arg;
    v = eq + 1;
    neg = *v == '-';
    v += neg;
    if (*v == '$')
        n = strtoll(v + 1, &e, 16);
    else
        n = strtoll(v, &e, 0);
    if (e == v || *e || n > 0xFFFFFFFFll)
        return -1;
    if (neg)
        n = -n;
    if (n < -0x80000000ll)
        return -1;
    *value = (int32_t)(uint32_t)n;
    return 0;
}

static int dump(const char *path)
{
    char err[300];
    size_t len;
    char *d = read_all(path, &len);
    int rc;

    if (!d) {
        perror(path);
        return 1;
    }
    rc = blitzres_dump((const uint8_t *)d, len, stdout, err, sizeof err);
    if (rc)
        fprintf(stderr, "blitzres: %s: %s\n", path, err);
    free(d);
    return rc ? 1 : 0;
}

/* --source: the constants of the resident at path as Blitz source to out */
static int source(const char *path, const char *out)
{
    struct blitzres_list l = { 0 };
    char err[300];
    size_t len;
    char *d = read_all(path, &len);
    FILE *f = NULL;
    int rc = 1;

    if (!d) {
        perror(path);
        return 1;
    }
    if (blitzres_read_consts((const uint8_t *)d, len, 0, &l, err, sizeof err)) {
        fprintf(stderr, "blitzres: %s: %s\n", path, err);
        goto done;
    }
    if (!(f = fopen(out, "w"))) {
        perror(out);
        goto done;
    }
    if (blitzres_source(&l, f) | fclose(f)) {
        perror(out);
        remove(out);
        goto done;
    }
    rc = 0;
done:
    free(d);
    blitzres_free(&l);
    return rc;
}

int main(int argc, char **argv)
{
    struct blitzres_list l = { 0 }, syms = { 0 }, back = { 0 };
    const char *out = NULL;
    char err[300];
    uint8_t *res = NULL;
    size_t reslen, i;
    int a, rc = 1;
    FILE *f;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--dump"))
        return dump(argv[2]);
    if (argc == 4 && !strcmp(argv[1], "--source"))
        return source(argv[2], argv[3]);
    if (argc < 3 || strcmp(argv[1], "-o")) {
        usage(stderr);
        return 2;
    }
    out = argv[2];
    for (a = 3; a < argc; a++) {
        const char *opt = argv[a];
        char *arg = a + 1 < argc ? argv[a + 1] : NULL;

        if (!arg) {
            fprintf(stderr, "blitzres: %s without its argument\n", opt);
            usage(stderr);
            rc = 2;
            goto done;
        }
        a++;
        if (!strcmp(opt, "--fd") || !strcmp(opt, "--fd-varargs") || !strcmp(opt, "--equ")
            || !strcmp(opt, "--lvo")) {
            size_t len;
            char *text = read_all(arg, &len);
            int r;

            if (!text) {
                perror(arg);
                goto done;
            }
            r = !strcmp(opt, "--equ")   ? blitzres_equ(text, len, &syms, &l, err, sizeof err)
                : !strcmp(opt, "--lvo") ? blitzres_lvo(text, len, &l, err, sizeof err)
                : blitzres_fd(text, len, !strcmp(opt, "--fd-varargs"), &l, err, sizeof err);
            free(text);
            if (r) {
                fprintf(stderr, "blitzres: %s: %s\n", arg, err);
                goto done;
            }
        } else if (!strcmp(opt, "--const") || !strcmp(opt, "--sym")) {
            char *name;
            int32_t v;

            if (name_value(arg, &name, &v)) {
                fprintf(stderr, "blitzres: %s %s: expected NAME=VALUE\n", opt, arg);
                rc = 2;
                goto done;
            }
            if (blitzres_add(&syms, name, v, err, sizeof err) < 0
                || (!strcmp(opt, "--const") && blitzres_add(&l, name, v, err, sizeof err) < 0)) {
                fprintf(stderr, "blitzres: %s: %s\n", opt, err);
                goto done;
            }
        } else if (!strcmp(opt, "--skip")) {
            if (blitzres_skip(&l, arg, err, sizeof err)) {
                fprintf(stderr, "blitzres: --skip: %s\n", err);
                goto done;
            }
        } else {
            fprintf(stderr, "blitzres: unknown option %s\n", opt);
            usage(stderr);
            rc = 2;
            goto done;
        }
    }
    if (blitzres_write(&l, &res, &reslen, err, sizeof err)) {
        fprintf(stderr, "blitzres: %s\n", err);
        goto done;
    }
    /* the check: read back, exactly these constants */
    if (blitzres_read_consts(res, reslen, 1, &back, err, sizeof err)) {
        fprintf(stderr, "blitzres: %s read back: %s\n", out, err);
        goto done;
    }
    if (back.n != l.n) {
        fprintf(stderr, "blitzres: %s read back: %zu constants, not %zu\n", out, back.n, l.n);
        goto done;
    }
    for (i = 0; i < l.n; i++)
        if (strcmp(back.c[i].name, l.c[i].name) || back.c[i].value != l.c[i].value) {
            fprintf(stderr, "blitzres: %s read back: constant %zu is %s=%ld, not %s=%ld\n",
                    out, i, back.c[i].name, (long)back.c[i].value, l.c[i].name,
                    (long)l.c[i].value);
            goto done;
        }
    if (!(f = fopen(out, "wb")) || fwrite(res, 1, reslen, f) != reslen || fclose(f)) {
        perror(out);
        remove(out);
        goto done;
    }
    rc = 0;
done:
    free(res);
    blitzres_free(&l);
    blitzres_free(&syms);
    blitzres_free(&back);
    return rc;
}
