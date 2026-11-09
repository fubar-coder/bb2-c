/*
 * blitzres create, dump, source - Blitz Basic 2 resident files with
 * constants, written directly (the format blitz2's "Create Resident"
 * writes, see lib/blitzres/blitzres.h), and the contents of any resident.
 *
 *   blitzres create -o OUT ITEM...
 *   blitzres dump RES
 *   blitzres source RES OUT
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
 * source: the constants of RES (a resident with constants only) as a
 * Blitz source text OUT, one #NAME=VALUE line each (blitzres_source):
 * blitz2's Create Resident makes the same resident of it.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "blitzres.h"
}
#include "cmds.h"

static const char *me; /* "blitzres SUB", the prefix of the messages */

static char *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n) {
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
    char *d;
    int rc;

    if (!(d = read_all(path, &len))) {
        fprintf(stderr, "%s: %s: %s\n", me, path, strerror(errno));
        return 1;
    }
    rc = blitzres_dump((const uint8_t *)d, len, stdout, err, sizeof err);
    if (rc)
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
    free(d);
    return rc ? 1 : 0;
}

/* source: the constants of the resident at path as Blitz source to out */
static int source(const char *path, const char *out)
{
    struct blitzres_list l = {};
    char err[300];
    size_t len;
    char *d;
    FILE *f = NULL;
    int rc = 1;

    if (!(d = read_all(path, &len))) {
        fprintf(stderr, "%s: %s: %s\n", me, path, strerror(errno));
        return 1;
    }
    if (blitzres_read_consts((const uint8_t *)d, len, 0, &l, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        goto done;
    }
    if (!(f = fopen(out, "w"))) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (blitzres_source(&l, f) | fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        remove(out);
        goto done;
    }
    rc = 0;
done:
    free(d);
    blitzres_free(&l);
    return rc;
}

/* create: the items in the order given (cli::occurrences of SUB) */
static int create(CLI::App &sub, const char *out)
{
    struct blitzres_list l = {}, syms = {}, back = {};
    char err[300];
    uint8_t *res = NULL;
    size_t reslen, i;
    int rc = 1;
    FILE *f;

    for (const cli::occurrence &oc : cli::occurrences(sub)) {
        const std::string name = oc.option->get_name();
        const char *opt = name.c_str();
        char *arg = const_cast<char *>(oc.values[0].c_str());

        if (!strcmp(opt, "-o")) {
            continue;
        } else if (!strcmp(opt, "--fd") || !strcmp(opt, "--fd-varargs") || !strcmp(opt, "--equ")
            || !strcmp(opt, "--lvo")) {
            size_t len;
            char *text = read_all(arg, &len);
            int r;

            if (!text) {
                fprintf(stderr, "%s: %s: %s\n", me, arg, strerror(errno));
                goto done;
            }
            r = !strcmp(opt, "--equ")   ? blitzres_equ(text, len, &syms, &l, err, sizeof err)
                : !strcmp(opt, "--lvo") ? blitzres_lvo(text, len, &l, err, sizeof err)
                : blitzres_fd(text, len, !strcmp(opt, "--fd-varargs"), &l, err, sizeof err);
            free(text);
            if (r) {
                fprintf(stderr, "%s: %s: %s\n", me, arg, err);
                goto done;
            }
        } else if (!strcmp(opt, "--const") || !strcmp(opt, "--sym")) {
            char *cname;
            int32_t v;

            if (name_value(arg, &cname, &v)) {
                fprintf(stderr, "%s: %s %s: expected NAME=VALUE\n", me, opt, arg);
                rc = 2;
                goto done;
            }
            if (blitzres_add(&syms, cname, v, err, sizeof err) < 0
                || (!strcmp(opt, "--const") && blitzres_add(&l, cname, v, err, sizeof err) < 0)) {
                fprintf(stderr, "%s: %s: %s\n", me, opt, err);
                goto done;
            }
        } else if (!strcmp(opt, "--skip")) {
            if (blitzres_skip(&l, arg, err, sizeof err)) {
                fprintf(stderr, "%s: --skip: %s\n", me, err);
                goto done;
            }
        }
    }
    if (blitzres_write(&l, &res, &reslen, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", me, err);
        goto done;
    }
    /* the check: read back, exactly these constants */
    if (blitzres_read_consts(res, reslen, 1, &back, err, sizeof err)) {
        fprintf(stderr, "%s: %s read back: %s\n", me, out, err);
        goto done;
    }
    if (back.n != l.n) {
        fprintf(stderr, "%s: %s read back: %zu constants, not %zu\n", me, out, back.n, l.n);
        goto done;
    }
    for (i = 0; i < l.n; i++)
        if (strcmp(back.c[i].name, l.c[i].name) || back.c[i].value != l.c[i].value) {
            fprintf(stderr, "%s: %s read back: constant %zu is %s=%ld, not %s=%ld\n", me,
                    out, i, back.c[i].name, (long)back.c[i].value, l.c[i].name,
                    (long)l.c[i].value);
            goto done;
        }
    if (!(f = fopen(out, "wb")) || fwrite(res, 1, reslen, f) != reslen || fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
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

void cmd_create(CLI::App &app, int &status)
{
    struct options {
        std::string out;
        std::vector<std::string> items[7];
    };
    static const char *const items[7][3] = {
        { "--fd", "FILE", "the public functions of an FD file as _NAME=-offset" },
        { "--fd-varargs", "FILE", "the same, and _NAME for a NAMEA without NAME" },
        { "--equ", "FILE", "the NAME EQU/SET EXPR lines of an assembler include" },
        { "--lvo", "FILE", "the _LVONAME EQU -offset lines of an LVO include as NAME" },
        { "--const", "NAME=VALUE", "a constant (also a symbol for --equ)" },
        { "--sym", "NAME=VALUE", "a symbol for the expressions of --equ only" },
        { "--skip", "NAME", "never NAME as constant" },
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("create", "a Blitz Basic 2 resident with constants, as blitz2's Create Resident writes it");
    sub->add_option("-o", o->out, "the resident written")->type_name("OUT")->required();
    for (int k = 0; k < 7; k++)
        cli::add_repeated(*sub, items[k][0], o->items[k], 1, items[k][2])->type_name(items[k][1]);
    sub->footer("The constants of OUT in the order of the options (the items). A name again with "
                "the same value is left out (blitz2 keeps the first); with another value it is an "
                "error. OUT is read back and must hold exactly these constants. Exit status 0 OUT "
                "written, 1 an input or OUT is wrong (OUT is not left), 2 usage error or a "
                "--const/--sym that is no NAME=VALUE.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = create(*sub, o->out.c_str());
    });
}

void cmd_dump(CLI::App &app, int &status)
{
    auto path = std::make_shared<std::string>();
    CLI::App *sub = app.add_subcommand("dump", "the types, macros and constants of a resident");
    sub->add_option("RES", *path, "the resident")->required();
    sub->footer("Exit status 0, 1 RES cannot be read or is no resident, 2 usage error.");
    sub->callback([sub, path, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = dump(path->c_str());
    });
}

void cmd_source(CLI::App &app, int &status)
{
    auto paths = std::make_shared<std::array<std::string, 2>>();
    CLI::App *sub = app.add_subcommand("source", "the constants of a resident as Blitz source, #NAME=VALUE lines");
    sub->add_option("RES", (*paths)[0], "a resident with constants only")->required();
    sub->add_option("OUT", (*paths)[1], "the source written")->required();
    sub->footer("blitz2's Create Resident makes the same resident of OUT. Exit status 0, 1 RES "
                "cannot be read or holds more than constants, or OUT cannot be written, 2 usage "
                "error.");
    sub->callback([sub, paths, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = source((*paths)[0].c_str(), (*paths)[1].c_str());
    });
}
