/*
 * fake_subcmd - a program with two subcommands, alpha and beta, for the
 * tests of cmake/check-help.cmake and cmake/check-readme.cmake
 * (tools/tests/CMakeLists.txt).  Built twice: fake_subcmd uses the
 * dispatcher (lib/subcmd); fake_subcmd_broken (FAKE_BROKEN) writes a
 * message to stderr with the help of beta.
 */
#include <stdio.h>
#include <string.h>
#include "subcmd.h"

static int run(int argc, char **argv)
{
    (void)argc;
    printf("%s\n", argv[0]);
    return 0;
}

static const struct subcmd table[] = {
    { "alpha", run, "usage: fake_subcmd alpha [-x] FILE\n  the first subcommand\n" },
    { "beta", run, "usage: fake_subcmd beta\n       fake_subcmd beta --all\n  the second subcommand\n" },
};

int main(int argc, char **argv)
{
#ifdef FAKE_BROKEN
    if (argc == 3 && !strcmp(argv[1], "beta") && (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))) {
        fputs(table[1].usage, stdout);
        fputs("fake_subcmd beta: a message\n", stderr);
        return 0;
    }
#endif
    return subcmd_main("fake_subcmd", table, sizeof table / sizeof table[0], argc, argv);
}
