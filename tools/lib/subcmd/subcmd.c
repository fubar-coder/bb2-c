/*
 * subcmd.c - the subcommand dispatch of the host programs (subcmd.h).
 */
#include <stdlib.h>
#include <string.h>
#include "subcmd.h"

static const char prefix[] = "usage: ";
static const char indent[] = "       "; /* as wide as prefix */

static int is_help(const char *s)
{
    return !strcmp(s, "-h") || !strcmp(s, "--help");
}

static int valid_name(const char *s)
{
    const char *p;

    if (!s || !*s || *s == '-')
        return 0;
    for (p = s; *p; p++)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-'))
            return 0;
    return p[-1] != '-';
}

int subcmd_check(const char *program, const struct subcmd *table, size_t n, FILE *err)
{
    size_t i, j, lp = strlen(program), ln;
    const char *u;

    if (!n) {
        fprintf(err, "%s: subcommand table: no entry\n", program);
        return 1;
    }
    for (i = 0; i < n; i++) {
        if (!valid_name(table[i].name)) {
            fprintf(err, "%s: subcommand table: entry %zu: no valid name\n", program, i + 1);
            return 1;
        }
        if (!table[i].run) {
            fprintf(err, "%s: subcommand table: %s: no function\n", program, table[i].name);
            return 1;
        }
        u = table[i].usage;
        ln = strlen(table[i].name);
        if (!u || strncmp(u, prefix, sizeof prefix - 1) || strncmp(u + sizeof prefix - 1, program, lp)
            || u[sizeof prefix - 1 + lp] != ' '
            || strncmp(u + sizeof prefix + lp, table[i].name, ln)
            || (u[sizeof prefix + lp + ln] != ' ' && u[sizeof prefix + lp + ln] != '\n')) {
            fprintf(err, "%s: subcommand table: %s: the usage does not start with \"usage: %s %s\"\n",
                    program, table[i].name, program, table[i].name);
            return 1;
        }
        if (u[strlen(u) - 1] != '\n' || strstr(u + 1, "\nusage: ")) {
            fprintf(err, "%s: subcommand table: %s: the usage has a second \"usage: \" or no final newline\n",
                    program, table[i].name);
            return 1;
        }
        for (j = 0; j < i; j++)
            if (!strcmp(table[j].name, table[i].name)) {
                fprintf(err, "%s: subcommand table: %s: twice\n", program, table[i].name);
                return 1;
            }
    }
    return 0;
}

/* the synopsis lines at the start of a usage: the "usage: " line and the
   lines after it that start with seven spaces (more synopsis lines, or a
   continued one); its length */
static size_t synopsis(const char *u)
{
    const char *p = strchr(u, '\n');

    while (p && !strncmp(p + 1, indent, sizeof indent - 1))
        p = strchr(p + 1, '\n');
    return p ? (size_t)(p + 1 - u) : strlen(u);
}

void subcmd_usage(const char *program, const struct subcmd *table, size_t n, FILE *f)
{
    size_t i, len;

    for (i = 0; i < n; i++) {
        len = synopsis(table[i].usage);
        if (i)
            fprintf(f, "%s%.*s", indent, (int)(len - (sizeof prefix - 1)),
                    table[i].usage + sizeof prefix - 1);
        else
            fprintf(f, "%.*s", (int)len, table[i].usage);
    }
    fprintf(f, "The description and options of a subcommand: %s SUB -h\n", program);
}

int subcmd_main_to(const char *program, const struct subcmd *table, size_t n, int argc,
                   char **argv, FILE *out, FILE *err)
{
    const struct subcmd *s = NULL;
    char *name, *saved;
    size_t i;
    int r;

    if (subcmd_check(program, table, n, err))
        return 1;
    if (argc < 2) {
        subcmd_usage(program, table, n, err);
        return 2;
    }
    if (argc == 2 && is_help(argv[1])) {
        subcmd_usage(program, table, n, out);
        return 0;
    }
    for (i = 0; i < n && !s; i++)
        if (!strcmp(argv[1], table[i].name))
            s = &table[i];
    if (!s) {
        subcmd_usage(program, table, n, err);
        return 2;
    }
    if (argc == 3 && is_help(argv[2])) {
        fputs(s->usage, out);
        return 0;
    }
    name = malloc(strlen(program) + strlen(s->name) + 2);
    if (!name) {
        fprintf(err, "%s: out of memory\n", program);
        return 1;
    }
    sprintf(name, "%s %s", program, s->name);
    saved = argv[1];
    argv[1] = name;
    r = s->run(argc - 1, argv + 1);
    argv[1] = saved;
    free(name);
    return r;
}

int subcmd_main(const char *program, const struct subcmd *table, size_t n, int argc, char **argv)
{
    int r = subcmd_main_to(program, table, n, argc, argv, stdout, stderr);

    fflush(stdout);
    return r;
}
