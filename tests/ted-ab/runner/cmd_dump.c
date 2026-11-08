/*
 * tedab info, tedab text, tedab cmp - screen dumps of teddrive
 * (teddump.h) by hand:
 *
 *   tedab info FILE.scr
 *   tedab text FILE.scr [--font 8|6] [--fonts DIR]
 *   tedab cmp A.scr B.scr [--font 8|6] [--fonts DIR] [--no-mask]
 *
 * DIR: chars8.b / chars.b of TED (default: archive/sources/ted-1.1 of this
 * repository).  cmp prints SAME or DIFF (with details); exit status 0 /
 * 1, 2 on errors.
 */
#define _XOPEN_SOURCE 700       /* open_memstream */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds.h"
#include "teddump.h"

#ifndef TED_FONTS_DIR
#define TED_FONTS_DIR "."
#endif

const char cmd_info_usage[] =
    "usage: tedab info FILE.scr\n"
    "Print the header of the screen dump FILE.scr of teddrive (size, depth,\n"
    "mode, mouse position, windows) and the SHA-256 of its bit planes.\n";

const char cmd_text_usage[] =
    "usage: tedab text FILE.scr [--font 8|6] [--fonts DIR]\n"
    "Print the screen dump FILE.scr as text, decoded with TED's font of\n"
    "height 8 (default) or 6.  DIR: chars8.b and chars.b of TED (default:\n"
    "archive/sources/ted-1.1 of this repository).\n";

const char cmd_cmp_usage[] =
    "usage: tedab cmp A.scr B.scr [--font 8|6] [--fonts DIR] [--no-mask]\n"
    "Compare two screen dumps as text; print SAME, or DIFF and the\n"
    "differences.  The status field \"Free\" is masked unless --no-mask.\n"
    "--font, --fonts as for text.\n";

/* info, text, cmp: CMD is the subcommand, USAGE its text */
static int dump(int argc, char **argv, const char *cmd, const char *usage)
{
    const char *files[2] = { NULL, NULL }, *fontdir = TED_FONTS_DIR, *me = argv[0];
    struct teddump d[2] = { { 0 } };
    struct teddump_fonts fonts;
    int i, nfiles = 0, fh = 8, no_mask = 0, rc = 2;
    char err[300];

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc)
            fh = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fonts") && i + 1 < argc)
            fontdir = argv[++i];
        else if (!strcmp(argv[i], "--no-mask"))
            no_mask = 1;
        else if (argv[i][0] != '-' && nfiles < 2)
            files[nfiles++] = argv[i];
        else
            break;
    }
    if (i < argc || (fh != 8 && fh != 6) || nfiles != (!strcmp(cmd, "cmp") ? 2 : 1)) {
        fputs(usage, stderr);
        return 2;
    }
    for (i = 0; i < nfiles; i++)
        if (teddump_load(files[i], &d[i], err, sizeof err)) {
            fprintf(stderr, "%s: %s: %s\n", me, files[i], err);
            goto out;
        }
    if (strcmp(cmd, "info") && teddump_load_fonts(fontdir, &fonts)) {
        fprintf(stderr, "%s: no fonts in %s\n", me, fontdir);
        goto out;
    }
    if (!strcmp(cmd, "info")) {
        char hex[65];
        size_t k;

        printf("width %ld\nheight %ld\ndepth %ld\nbytesperrow %ld\nrows %ld\nrowbytes %ld\n"
               "interleaved %ld\nmodeid $%08lX\nmousex %ld\nmousey %ld\nmemtype $%08lX\n"
               "title %s\n", d[0].width, d[0].height, d[0].depth, d[0].bytesperrow, d[0].rows,
               d[0].rowbytes, d[0].interleaved, (unsigned long)d[0].modeid & 0xFFFFFFFFul,
               d[0].mousex, d[0].mousey, (unsigned long)d[0].memtype & 0xFFFFFFFFul, d[0].title);
        for (k = 0; k < d[0].nwindows; k++)
            printf("window %ld %ld %ld %ld $%08lX %s\n", d[0].windows[k].left,
                   d[0].windows[k].top, d[0].windows[k].width, d[0].windows[k].height,
                   (unsigned long)d[0].windows[k].flags & 0xFFFFFFFFul, d[0].windows[k].title);
        teddump_digest(&d[0], hex);
        printf("sha256 %s\n", hex);
        rc = 0;
    } else if (!strcmp(cmd, "text")) {
        char *t = teddump_text(&d[0], &fonts, fh);

        if (!t) {
            fprintf(stderr, "%s: %s: not a TED screen (depth < 2)\n", me, files[0]);
            goto out;
        }
        fputs(t, stdout);
        free(t);
        rc = 0;
    } else {
        char *detail = NULL;
        size_t len = 0;
        FILE *m = open_memstream(&detail, &len);

        if (!m)
            goto out;
        rc = teddump_compare(&d[0], &d[1], &fonts, fh, no_mask, m);
        fclose(m);
        printf("%s\n%s", rc ? "DIFF" : "SAME", detail ? detail : "");
        free(detail);
    }
out:
    teddump_free(&d[0]);
    teddump_free(&d[1]);
    return rc;
}

int cmd_info(int argc, char **argv)
{
    return dump(argc, argv, "info", cmd_info_usage);
}

int cmd_text(int argc, char **argv)
{
    return dump(argc, argv, "text", cmd_text_usage);
}

int cmd_cmp(int argc, char **argv)
{
    return dump(argc, argv, "cmp", cmd_cmp_usage);
}
