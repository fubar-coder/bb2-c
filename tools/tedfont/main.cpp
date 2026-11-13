/*
 * tedfont - TED-C's 256-glyph screen fonts from a glyph text source and
 * TED 1.1's font file (tools/lib/tedfont/tedfont.h; README.md here).
 *
 *   tedfont build HEIGHT ARCHIVE SOURCE OUT    write the font OUT
 *   tedfont check HEIGHT ARCHIVE SOURCE OUT    check OUT, remove it if wrong
 *
 * build writes OUT only when SOURCE and ARCHIVE are valid; check is the
 * second command of the same build step: a font that fails it is removed,
 * so nothing that depends on it is built.
 */
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "tedfont.h"
}

namespace {

const char *const footer =
    "Exit status: 0 the font is written (build) or right (check), 1 an input is wrong or "
    "cannot be read, the font cannot be written or is wrong (a message on stderr; OUT does "
    "not exist afterwards), 2 usage error.";

struct options {
    int height = 0;
    std::string archive, source, out;
};

/* the file PATH into data; false with a message if it cannot be read */
bool read_file(const std::string &me, const std::string &path, std::vector<uint8_t> &data)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    uint8_t buf[4096];
    size_t n;

    if (!f) {
        std::fprintf(stderr, "%s: cannot read %s\n", me.c_str(), path.c_str());
        return false;
    }
    data.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        data.insert(data.end(), buf, buf + n);
    bool bad = std::ferror(f);
    std::fclose(f);
    if (bad)
        std::fprintf(stderr, "%s: cannot read %s\n", me.c_str(), path.c_str());
    return !bad;
}

/* the inputs: the parsed source of HEIGHT and the archive file */
bool read_inputs(const std::string &me, const options &o, struct tedfont_src &src,
                 std::vector<uint8_t> &archive)
{
    std::vector<uint8_t> text;
    char err[300];

    if (!read_file(me, o.source, text) || !read_file(me, o.archive, archive))
        return false;
    if (tedfont_parse(reinterpret_cast<const char *>(text.data()), text.size(), o.source.c_str(),
                      &src, err, sizeof err)) {
        std::fprintf(stderr, "%s\n", err);
        return false;
    }
    if (src.height != o.height) {
        std::fprintf(stderr, "%s: h=%d, but HEIGHT is %d\n", o.source.c_str(), src.height,
                     o.height);
        return false;
    }
    return true;
}

void add_args(CLI::App *sub, options &o, const char *out_help)
{
    sub->add_option("HEIGHT", o.height, "the font height: 8 or 6")
        ->required()
        ->check(CLI::IsMember({8, 6}));
    sub->add_option("ARCHIVE", o.archive, "TED 1.1's font file of that height (chars8.b, chars.b)")
        ->required();
    sub->add_option("SOURCE", o.source, "the glyph text source (glyphs8.txt, glyphs6.txt)")
        ->required();
    sub->add_option("OUT", o.out, out_help)->required();
    sub->footer(footer);
}

void cmd_build(CLI::App &app, int &status)
{
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("build", "write the 256-glyph font from the archive font and the source");
    add_args(sub, *o, "the font to write");
    sub->callback([sub, o, &status] {
        const std::string me = cli::name(*sub);
        struct tedfont_src src;
        std::vector<uint8_t> archive, font;
        char err[300];

        std::remove(o->out.c_str());
        status = 1;
        if (!read_inputs(me, *o, src, archive))
            return;
        font.resize(tedfont_size(src.height));
        if (tedfont_build(&src, archive.data(), archive.size(), font.data(), err, sizeof err)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), o->archive.c_str(), err);
            return;
        }
        FILE *f = std::fopen(o->out.c_str(), "wb");
        bool bad = !f || std::fwrite(font.data(), 1, font.size(), f) != font.size();
        if (f && std::fclose(f))
            bad = true;
        if (bad) {
            std::fprintf(stderr, "%s: cannot write %s\n", me.c_str(), o->out.c_str());
            std::remove(o->out.c_str());
            return;
        }
        status = 0;
    });
}

void cmd_check(CLI::App &app, int &status)
{
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("check", "check a font against the archive font and the source; remove it if wrong");
    add_args(sub, *o, "the font to check");
    sub->callback([sub, o, &status] {
        const std::string me = cli::name(*sub);
        struct tedfont_src src;
        std::vector<uint8_t> archive, font;
        char err[300];

        status = 1;
        if (read_inputs(me, *o, src, archive) && read_file(me, o->out, font)) {
            if (!tedfont_check(&src, archive.data(), archive.size(), font.data(), font.size(), err,
                               sizeof err)) {
                status = 0;
                return;
            }
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), o->out.c_str(), err);
        }
        std::remove(o->out.c_str());
    });
}

} // namespace

int main(int argc, char **argv)
{
    CLI::App app{"TED-C's 256-glyph screen fonts from a glyph text source and TED 1.1's font file.",
                 "tedfont"};
    int status = 0;

    app.require_subcommand(1);
    cmd_build(app, status);
    cmd_check(app, status);
    return cli::run(app, argc, argv, status);
}
