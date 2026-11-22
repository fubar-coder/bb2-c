/*
 * exprmatrix generate - the Blitz sources of the matrix's cells from its
 * description (exprmatrix.h): OUTDIR/NAME.bas per source, one line
 * "NAME CELLS" per source and the totals on stdout.
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "exprmatrix.h"
}
#include "cmds.h"

static bool read_file(const std::string &path, std::string &text)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    char buf[8192];
    size_t n;

    if (!f)
        return false;
    text.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        text.append(buf, n);
    bool ok = !std::ferror(f);
    std::fclose(f);
    return ok;
}

static bool write_file(const std::string &path, const char *text)
{
    FILE *f = std::fopen(path.c_str(), "wb");
    size_t n = std::strlen(text);

    if (!f)
        return false;
    bool ok = std::fwrite(text, 1, n, f) == n;
    return std::fclose(f) == 0 && ok;
}

/* the name of the description in the headers: its last path component */
static std::string base_name(const std::string &path)
{
    size_t slash = path.find_last_of('/');

    return slash == std::string::npos ? path : path.substr(slash + 1);
}

static int run(const std::string &me, const std::string &desc, const std::string &outdir, long cells)
{
    std::string text;
    struct em_matrix m;
    struct em_source *src = nullptr;
    size_t nsrc = 0, total = 0;
    char *errors = nullptr;
    int r;

    if (!read_file(desc, text)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), desc.c_str(), std::strerror(errno));
        return 1;
    }
    r = em_parse(text.data(), text.size(), desc.c_str(), &m, &errors);
    if (r) {
        std::fprintf(stderr, "%s", r < 0 ? "" : errors);
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        std::free(errors);
        return 1;
    }
    r = em_generate(&m, base_name(desc).c_str(), cells, &src, &nsrc, &errors);
    em_free(&m);
    if (r) {
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        else
            std::fprintf(stderr, "%s", errors);
        std::free(errors);
        return 1;
    }
    for (size_t i = 0; i < nsrc; i++) {
        std::string path = outdir + "/" + src[i].name + ".bas";

        if (!write_file(path, src[i].text)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), path.c_str(), std::strerror(errno));
            em_sources_free(src, nsrc);
            return 1;
        }
        std::printf("%s %zu\n", src[i].name, src[i].ncells);
        total += src[i].ncells;
    }
    std::printf("%zu cells in %zu sources\n", total, nsrc);
    em_sources_free(src, nsrc);
    return 0;
}

void cmd_generate(CLI::App &app, int &status)
{
    struct options {
        std::string desc, outdir;
        long cells = 0;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("generate", "the Blitz sources of the cells from the description");

    sub->add_option("DESC", o->desc, "the description of the matrix")->required()->type_name("FILE");
    sub->add_option("-o,--out", o->outdir, "the directory the sources are written to (it exists)")
        ->required()
        ->type_name("OUTDIR");
    sub->add_option("--cells", o->cells, "at most N cells per source, 1 to 100000 (default: cells-per-source)")
        ->type_name("N");
    sub->footer("Writes OUTDIR/NAME.bas for every source (NAME is BLOCK-CTX-SET-NN) and prints \"NAME "
                "CELLS\" per source and the totals. Exit status 0, 1 the description is malformed (one "
                "message per error, FILE:LINE: ...) or a file cannot be read or written, 2 usage "
                "error.");
    sub->callback([sub, o, &status] {
        if (sub->count("--cells") && (o->cells < 1 || o->cells > 100000)) {
            std::fprintf(stderr, "%s: --cells %ld: not from 1 to 100000\n", cli::name(*sub).c_str(), o->cells);
            status = 2;
            return;
        }
        status = run(cli::name(*sub), o->desc, o->outdir, o->cells);
    });
}
