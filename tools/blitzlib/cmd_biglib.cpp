/*
 * blitzlib join, extract, numbers - join Blitz Basic 2 library files into
 * one BigLib file (blitz2:acidlibs, blitz2:deflibs; format:
 * lib/biglib/biglib.h), take one out of it, check the library numbers.
 *
 *   blitzlib join OUT LIB.obj...
 *   blitzlib extract BIGLIB NUMBER|LIB.obj OUT
 *   blitzlib numbers [--allow-33 FILE]... FILE...
 *
 * join: every input must be a well-formed AmigaDOS load file (lib/hunk).
 * extract: the library with that number (or the number of LIB.obj, e.g.
 * the original of a library built here) as a file of its own.
 * numbers: no two libraries of the FILEs (BigLib or library files, e.g.
 * acidlibs, deflibs and the userlibs of a variant) with the same number;
 * exit status 1 with both named if there are; and none with a reserved
 * number (32, 33 other than literallib unless --allow-33 names its file;
 * #1018).
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "biglib.h"
#include "hunk.h"
}
#include "cmds.h"

static const char *me; /* "blitzlib SUB", the prefix of the messages */

static int join(const char *out, int n, const char *const *files)
{
    struct loadfile *lf = (struct loadfile *)calloc(n ? (size_t)n : 1, sizeof *lf);
    struct biglib_entry *e = (struct biglib_entry *)calloc(n ? (size_t)n : 1, sizeof *e);
    FILE *f;
    int i, rc = 1;

    if (!lf || !e) {
        fprintf(stderr, "%s: out of memory\n", me);
        goto done;
    }
    for (i = 0; i < n; i++) {
        char err[200];

        if (load_read(files[i], &lf[i], err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, files[i], err);
            goto done;
        }
        e[i].data = lf[i].data;
        e[i].size = (uint32_t)lf[i].size;
    }
    f = fopen(out, "wb");
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (biglib_write(f, e, (size_t)n)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        fclose(f);
        remove(out);
        goto done;
    }
    if (fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        remove(out);
        goto done;
    }
    printf("%s: %d libs\n", out, n);
    rc = 0;
done:
    for (i = 0; lf && i < n; i++)
        load_free(&lf[i]);
    free(lf);
    free(e);
    return rc;
}

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (uint8_t *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* the library WHICH (a number, or a library file: its number) of BIGLIB */
static int extract(const char *big, const char *which, const char *out)
{
    size_t size, n, len;
    uint8_t *buf = read_all(big, &size), *lib = NULL;
    struct biglib_entry *e = NULL;
    char err[200], *end;
    unsigned num = (unsigned)strtoul(which, &end, 0);
    long i;
    int rc = 1;
    FILE *f;

    if (!buf || biglib_read(buf, size, &e, &n, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, big, buf ? err : "cannot read");
        goto done;
    }
    if (!*which || *end) {              /* not a number: a library file */
        if (!(lib = read_all(which, &len)) || biglib_number(lib, len, &num)) {
            fprintf(stderr, "%s: %s: no library file\n", me, which);
            goto done;
        }
    }
    if ((i = biglib_find(e, n, num)) < 0) {
        fprintf(stderr, "%s: %s: no library %u\n", me, big, num & 0xFFFF);
        goto done;
    }
    if (!(f = fopen(out, "wb")) || fwrite(e[i].data, 1, e[i].size, f) != e[i].size) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        if (f)
            fclose(f);
        goto done;
    }
    if (fclose(f)) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    rc = 0;
done:
    free(e);
    free(buf);
    free(lib);
    return rc;
}

/* room for n entries in the three arrays of check; 0 = out of memory */
static int grow(struct biglib_entry **all, int **from, int **index, size_t n)
{
    struct biglib_entry *a = (struct biglib_entry *)realloc(*all, (n ? n : 1) * sizeof *a);
    int *f, *x;

    if (a)
        *all = a;
    f = (int *)realloc(*from, (n ? n : 1) * sizeof *f);
    if (f)
        *from = f;
    x = (int *)realloc(*index, (n ? n : 1) * sizeof *x);
    if (x)
        *index = x;
    return a && f && x;
}

/* the libraries of FILES (BigLib or library files) with the same number */
static int check(int n, const char *const *files, const std::vector<std::string> &allow33)
{
    struct biglib_entry *all = NULL, *e;
    size_t nall = 0, ne, k, earlier;
    uint8_t **bufs = (uint8_t **)calloc(n ? (size_t)n : 1, sizeof *bufs);
    int *from = NULL, *index = NULL, i, rc = 1;
    long c;

    if (!bufs)
        goto nomem;
    for (i = 0; i < n; i++) {
        char err[200];
        size_t size;

        if (!(bufs[i] = read_all(files[i], &size))) {
            fprintf(stderr, "%s: %s: cannot read\n", me, files[i]);
            goto done;
        }
        if (size >= 4 && be32(bufs[i]) == HUNK_HEADER) {      /* one library */
            if (!(e = (struct biglib_entry *)malloc(sizeof *e)))
                goto nomem;
            e->data = bufs[i];
            e->size = (uint32_t)size;
            ne = 1;
        } else if (biglib_read(bufs[i], size, &e, &ne, err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, files[i], err);
            goto done;
        }
        if (!grow(&all, &from, &index, nall + ne)) {
            free(e);
            goto nomem;
        }
        for (k = 0; k < ne; k++) {
            all[nall] = e[k];
            from[nall] = i;
            index[nall++] = (int)k + 1;
        }
        free(e);
    }
    /* the reserved numbers 32 and 33 (#1018): a library 33 of a file
       --allow-33 names is allowed */
    for (k = 0; k < nall; k++) {
        char what[600];
        int allowed = 0;

        for (const std::string &a : allow33)
            if (a == files[from[k]])
                allowed = 1;
        snprintf(what, sizeof what, "%s (library %d)", files[from[k]], index[k]);
        if (check_reserved(me, all[k].data, all[k].size, what, allowed))
            goto done;
    }
    if ((c = biglib_clash(all, nall, &earlier)) >= 0) {
        unsigned num = 0;

        biglib_number(all[c].data, all[c].size, &num);
        fprintf(stderr, "%s: library number %u twice: %s (library %d) and %s (library %d)\n", me,
                num, files[from[earlier]], index[earlier], files[from[c]], index[c]);
        goto done;
    }
    rc = 0;
    goto done;
nomem:
    fprintf(stderr, "%s: out of memory\n", me);
done:
    for (i = 0; bufs && i < n; i++)
        free(bufs[i]);
    free(bufs);
    free(all);
    free(from);
    free(index);
    return rc;
}

/* the C strings of ARGS, for the functions above */
static std::vector<const char *> c_strings(const std::vector<std::string> &args)
{
    std::vector<const char *> v;

    for (const std::string &s : args)
        v.push_back(s.c_str());
    v.push_back(NULL);
    return v;
}

void cmd_join(CLI::App &app, int &status)
{
    struct options {
        std::string out;
        std::vector<std::string> libs;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("join", "join Blitz Basic 2 library files into one BigLib file");
    sub->add_option("OUT", o->out, "the BigLib file written")->required();
    sub->add_option("LIB.obj", o->libs, "the library files, in this order");
    sub->footer("Every input must be a well-formed AmigaDOS load file; prints \"OUT: N libs\". Exit "
                "status 0, 1 an input cannot be read or is no load file, or OUT cannot be written, "
                "2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        std::vector<const char *> libs = c_strings(o->libs);

        me = name.c_str();
        status = join(o->out.c_str(), (int)o->libs.size(), libs.data());
    });
}

void cmd_extract(CLI::App &app, int &status)
{
    struct options {
        std::string big, which, out;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand(
        "extract", "take the library with that number (or the number of LIB.obj) out of a BigLib file");
    sub->add_option("BIGLIB", o->big, "the BigLib file")->required();
    sub->add_option("WHICH", o->which, "the library number, or a library file whose number is taken")
        ->type_name("NUMBER|LIB.obj")
        ->required();
    sub->add_option("OUT", o->out, "the library file written")->required();
    sub->footer("Exit status 0, 1 BIGLIB cannot be read or holds no such library, WHICH is no "
                "library file, or OUT cannot be written, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);

        me = name.c_str();
        status = extract(o->big.c_str(), o->which.c_str(), o->out.c_str());
    });
}

void cmd_numbers(CLI::App &app, int &status)
{
    auto files = std::make_shared<std::vector<std::string>>();
    auto allow = std::make_shared<std::vector<std::string>>();
    CLI::App *sub = app.add_subcommand("numbers", "no library number twice in the FILEs (BigLib or library files)");
    sub->add_option("FILE", *files, "BigLib or library files")->required();
    sub->add_option("--allow-33", *allow,
                    "a FILE whose library 33 is allowed although it is not literallib (RIDosLib "
                    "in the RWE variants)")->type_name("FILE")->expected(1)->allow_extra_args(false);
    sub->footer("Library 32 and a library 33 other than literallib (the literal characters, "
                "#1018) are refused, but the library 33 of a FILE named by --allow-33. Exit status "
                "0, 1 a library number is twice (both named) or reserved, or a FILE cannot be read "
                "or is no BigLib or library file, 2 usage error.");
    sub->callback([sub, files, allow, &status] {
        const std::string name = cli::name(*sub);
        std::vector<const char *> v = c_strings(*files);

        me = name.c_str();
        status = check((int)files->size(), v.data(), *allow);
    });
}
