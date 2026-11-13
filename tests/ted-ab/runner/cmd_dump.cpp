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
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "teddump.h"
}
#include "cmds.h"

#ifndef TED_FONTS_DIR
#define TED_FONTS_DIR "."
#endif

/* info, text, cmp: CMD is the subcommand, FILES its NFILES dumps; FH the
 * height of the font of DIR (text, cmp) */
static int dump(const char *me, const char *cmd, const char *const files[2], int nfiles,
                const char *fontdir, int fh, int no_mask)
{
    struct teddump d[2] = {};
    struct teddump_fonts fonts;
    int i, rc = 2;
    char err[300];

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

/* the options of info, text and cmp */
struct dump_options {
    std::string file[2], font = "8", fonts = TED_FONTS_DIR;
    bool no_mask = false;
};

/* --font and --fonts of text and cmp */
static void font_options(CLI::App &sub, dump_options &o)
{
    sub.add_option("--font", o.font, "the height of TED's font the dump is decoded with: 8 (default) or 6")
        ->type_name("8|6");
    sub.add_option("--fonts", o.fonts, "ted_font8.b and ted_font6.b of TED-C (default: the host "
                                       "side's of this build)")
        ->type_name("DIR");
}

/* the subcommand CMD with its dumps */
static void callback(CLI::App *sub, std::shared_ptr<dump_options> o, int &status, const char *cmd, int nfiles)
{
    sub->callback([sub, o, cmd, nfiles, &status] {
        const std::string name = cli::name(*sub);
        const char *files[2] = { o->file[0].c_str(), o->file[1].c_str() };
        int fh = atoi(o->font.c_str());

        if (fh != 8 && fh != 6) {
            fprintf(stderr, "%s: --font: 8 or 6, not %s\nRun with --help for more information.\n",
                    name.c_str(), o->font.c_str());
            status = 2;
            return;
        }
        status = dump(name.c_str(), cmd, files, nfiles, o->fonts.c_str(), fh, o->no_mask);
    });
}

void cmd_info(CLI::App &app, int &status)
{
    auto o = std::make_shared<dump_options>();
    CLI::App *sub = app.add_subcommand("info", "the header of a screen dump of teddrive and the SHA-256 of its bit planes");
    sub->add_option("FILE.scr", o->file[0], "the screen dump")->required();
    sub->footer("Size, depth, mode, mouse position, windows. Exit status 0, 2 usage error or a "
                "file that cannot be read.");
    callback(sub, o, status, "info", 1);
}

void cmd_text(CLI::App &app, int &status)
{
    auto o = std::make_shared<dump_options>();
    CLI::App *sub = app.add_subcommand("text", "a screen dump of teddrive as text");
    sub->add_option("FILE.scr", o->file[0], "the screen dump")->required();
    font_options(*sub, *o);
    sub->footer("Exit status 0, 2 usage error, a file that cannot be read or no TED screen.");
    callback(sub, o, status, "text", 1);
}

void cmd_cmp(CLI::App &app, int &status)
{
    auto o = std::make_shared<dump_options>();
    CLI::App *sub = app.add_subcommand("cmp", "two screen dumps of teddrive compared as text");
    sub->add_option("A.scr", o->file[0], "the one dump")->required();
    sub->add_option("B.scr", o->file[1], "the other dump")->required();
    font_options(*sub, *o);
    sub->add_flag("--no-mask", o->no_mask, "compare the status field \"Free\" too (masked by default)");
    sub->footer("Prints SAME, or DIFF and the differences. Exit status 0 the same, 1 different, 2 "
                "usage error or a file that cannot be read.");
    callback(sub, o, status, "cmp", 2);
}
