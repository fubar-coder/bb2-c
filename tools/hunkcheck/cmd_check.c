/*
 * hunkcheck start, layout, stored - checks of the hunks of a load file
 * (see hunkcheck.h).
 *
 *   hunkcheck start FILE OFFSET:HEXBYTES...   e.g. hunkcheck start ted 0:6100 4:48e7
 *   hunkcheck layout LAYOUT FILE              e.g. hunkcheck layout code,bss exe
 *   hunkcheck stored FILE                     every CODE/DATA hunk stored whole
 *
 * Silent if everything is right; exit status 1 otherwise.
 */
#include <stdio.h>
#include "hunkcheck.h"
#include "cmds.h"

const char cmd_start_usage[] =
    "usage: hunkcheck start FILE OFFSET:HEXBYTES...\n"
    "  the bytes at offsets of the first hunk of a load file\n";
const char cmd_layout_usage[] =
    "usage: hunkcheck layout LAYOUT FILE\n"
    "  the hunks of a load file (LAYOUT: code|data|bss[+chip],... e.g.\n"
    "  code,bss,data+chip)\n";
const char cmd_stored_usage[] =
    "usage: hunkcheck stored FILE\n"
    "  no CODE/DATA hunk of a load file is larger than its stored block\n";

/* the load file PATH; 1 with a message if it cannot be read */
static int read_file(const char *me, const char *path, struct loadfile *lf)
{
    char err[200];

    if (load_read(path, lf, err, sizeof err)) {
        fprintf(stderr, "%s: %s: %s\n", me, path, err);
        return 1;
    }
    return 0;
}

int cmd_start(int argc, char **argv)
{
    struct loadfile lf;
    int bad;

    if (argc < 3) {
        fputs(cmd_start_usage, stderr);
        return 2;
    }
    if (read_file(argv[0], argv[1], &lf))
        return 1;
    bad = hunk_check(&lf, argc - 2, argv + 2, argv[1], stderr);
    load_free(&lf);
    return bad;
}

int cmd_layout(int argc, char **argv)
{
    struct loadfile lf;
    int bad;

    if (argc != 3) {
        fputs(cmd_layout_usage, stderr);
        return 2;
    }
    if (read_file(argv[0], argv[2], &lf))
        return 1;
    bad = hunk_layout(&lf, argv[1], argv[2], stderr);
    load_free(&lf);
    return bad;
}

int cmd_stored(int argc, char **argv)
{
    struct loadfile lf;
    int bad;

    if (argc != 2) {
        fputs(cmd_stored_usage, stderr);
        return 2;
    }
    if (read_file(argv[0], argv[1], &lf))
        return 1;
    bad = hunk_stored(&lf, argv[1], stderr);
    load_free(&lf);
    return bad;
}
