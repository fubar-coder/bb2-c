/*
 * test_subcmd - the subcommand dispatch (lib/subcmd/subcmd.h): help of the
 * program and of a subcommand, usage errors, the call of a subcommand and
 * its argv[0], the check of the table.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "subcmd.h"

static int failures;

#define CHECK(cond) do { \
        if (!(cond)) { \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++; \
        } \
    } while (0)

static char seen_name[64];
static int seen_argc;
static char seen_arg[64];

static int run_alpha(int argc, char **argv)
{
    snprintf(seen_name, sizeof seen_name, "%s", argv[0]);
    seen_argc = argc;
    snprintf(seen_arg, sizeof seen_arg, "%s", argc > 1 ? argv[1] : "");
    return 0;
}

static int run_fails(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return 1;
}

static const struct subcmd table[] = {
    { "alpha", run_alpha, "usage: prog alpha [-x] FILE\n"
                          "       prog alpha --list\n"
                          "  the first one\n" },
    { "beta-two", run_fails, "usage: prog beta-two\n  the second one, it fails\n"
                             "       not a synopsis line after the description\n" },
};
#define N (sizeof table / sizeof table[0])

static const char program_usage[] =
    "usage: prog alpha [-x] FILE\n"
    "       prog alpha --list\n"
    "       prog beta-two\n"
    "The description and options of a subcommand: prog SUB -h\n";

struct result {
    int status;
    char out[1024], err[1024];
};

/* one dispatch with the arguments (NULL-terminated), stdout and stderr caught */
static struct result run(const struct subcmd *t, size_t n, ...)
{
    struct result r;
    char *argv[16];
    int argc = 0;
    va_list ap;
    FILE *out = tmpfile(), *err = tmpfile();
    size_t k;

    argv[argc++] = "prog";
    va_start(ap, n);
    while ((argv[argc] = va_arg(ap, char *)))
        argc++;
    va_end(ap);
    if (!out || !err)
        abort();
    r.status = subcmd_main_to("prog", t, n, argc, argv, out, err);
    rewind(out);
    rewind(err);
    k = fread(r.out, 1, sizeof r.out - 1, out);
    r.out[k] = 0;
    k = fread(r.err, 1, sizeof r.err - 1, err);
    r.err[k] = 0;
    fclose(out);
    fclose(err);
    CHECK(!strcmp(argv[0], "prog"));
    return r;
}

static void test_help(void)
{
    struct result r;

    r = run(table, N, "-h", NULL);
    CHECK(r.status == 0);
    CHECK(!strcmp(r.out, program_usage));
    CHECK(!*r.err);
    r = run(table, N, "--help", NULL);
    CHECK(r.status == 0);
    CHECK(!strcmp(r.out, program_usage));
    CHECK(!*r.err);
    r = run(table, N, "alpha", "-h", NULL);
    CHECK(r.status == 0);
    CHECK(!strcmp(r.out, table[0].usage));
    CHECK(!*r.err);
    r = run(table, N, "beta-two", "--help", NULL);
    CHECK(r.status == 0);
    CHECK(!strcmp(r.out, table[1].usage));
    CHECK(!*r.err);
    /* -h with further arguments is the subcommand's */
    seen_argc = 0;
    r = run(table, N, "alpha", "-h", "x", NULL);
    CHECK(r.status == 0 && seen_argc == 3 && !strcmp(seen_arg, "-h"));
    CHECK(!*r.out && !*r.err);
}

static void test_usage_errors(void)
{
    struct result r;

    r = run(table, N, NULL);
    CHECK(r.status == 2);
    CHECK(!*r.out);
    CHECK(!strcmp(r.err, program_usage));
    r = run(table, N, "gamma", NULL);
    CHECK(r.status == 2);
    CHECK(!*r.out);
    CHECK(!strcmp(r.err, program_usage));
    /* no prefix match, no option in place of the name */
    r = run(table, N, "alph", NULL);
    CHECK(r.status == 2);
    r = run(table, N, "-x", "alpha", NULL);
    CHECK(r.status == 2);
}

static void test_call(void)
{
    struct result r;

    seen_name[0] = 0;
    r = run(table, N, "alpha", "file.txt", NULL);
    CHECK(r.status == 0);
    CHECK(!strcmp(seen_name, "prog alpha"));
    CHECK(seen_argc == 2);
    CHECK(!strcmp(seen_arg, "file.txt"));
    CHECK(!*r.out && !*r.err);
    seen_argc = -1;
    r = run(table, N, "alpha", NULL);
    CHECK(r.status == 0 && seen_argc == 1);
    r = run(table, N, "beta-two", "x", NULL);
    CHECK(r.status == 1);
}

static void bad_table(const struct subcmd *t, size_t n, const char *message)
{
    struct result r = run(t, n, "-h", NULL);
    FILE *null;

    CHECK(r.status == 1);
    CHECK(!*r.out);
    CHECK(strstr(r.err, message) != NULL);
    if (!strstr(r.err, message))
        fprintf(stderr, "  stderr: %s", r.err);
    null = fopen("/dev/null", "w");
    CHECK(null && subcmd_check("prog", t, n, null) == 1);
    if (null)
        fclose(null);
}

static void test_table(void)
{
    static const struct subcmd no_usage[] = { { "alpha", run_alpha, NULL } };
    static const struct subcmd wrong_name[] = { { "alpha", run_alpha, "usage: prog beta X\n" } };
    static const struct subcmd longer_name[] = { { "alpha", run_alpha, "usage: prog alphabet X\n" } };
    static const struct subcmd wrong_program[] = { { "alpha", run_alpha, "usage: other alpha X\n" } };
    static const struct subcmd no_newline[] = { { "alpha", run_alpha, "usage: prog alpha X" } };
    static const struct subcmd two_usages[] = {
        { "alpha", run_alpha, "usage: prog alpha X\nusage: prog alpha Y\n" } };
    static const struct subcmd no_run[] = { { "alpha", NULL, "usage: prog alpha X\n" } };
    static const struct subcmd upper[] = { { "Alpha", run_alpha, "usage: prog Alpha X\n" } };
    static const struct subcmd twice[] = { { "alpha", run_alpha, "usage: prog alpha X\n" },
                                           { "alpha", run_alpha, "usage: prog alpha Y\n" } };

    CHECK(subcmd_check("prog", table, N, stderr) == 0);
    bad_table(no_usage, 1, "alpha: the usage does not start with");
    bad_table(wrong_name, 1, "alpha: the usage does not start with");
    bad_table(longer_name, 1, "alpha: the usage does not start with");
    bad_table(wrong_program, 1, "alpha: the usage does not start with");
    bad_table(no_newline, 1, "no final newline");
    bad_table(two_usages, 1, "a second \"usage: \"");
    bad_table(no_run, 1, "alpha: no function");
    bad_table(upper, 1, "entry 1: no valid name");
    bad_table(twice, 2, "alpha: twice");
    bad_table(table, 0, "no entry");
}

int main(void)
{
    test_help();
    test_usage_errors();
    test_call();
    test_table();
    if (failures)
        fprintf(stderr, "%d failures\n", failures);
    return failures != 0;
}
