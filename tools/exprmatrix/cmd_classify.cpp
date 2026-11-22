/*
 * exprmatrix classify - the class of every measured cell (exprmatrix.h,
 * em_classify): the cell table with a class per cell, the questions file,
 * and on stdout the counts and the model's mismatches.
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

static int run(const std::string &me, const std::string &desc, const std::string &table, const std::string &out,
               const std::string &questions)
{
    std::string dtext, ttext;
    struct em_matrix m;
    struct em_table t;
    struct em_classified c;
    char *errors = nullptr;
    int r;

    if (!read_file(desc, dtext)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), desc.c_str(), std::strerror(errno));
        return 1;
    }
    if (!read_file(table, ttext)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), table.c_str(), std::strerror(errno));
        return 1;
    }
    r = em_parse(dtext.data(), dtext.size(), desc.c_str(), &m, &errors);
    if (r) {
        std::fprintf(stderr, "%s", r < 0 ? "" : errors);
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        std::free(errors);
        return 1;
    }
    r = em_table_parse(ttext.data(), ttext.size(), table.c_str(), &t, &errors);
    if (r) {
        std::fprintf(stderr, "%s", r < 0 ? "" : errors);
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        std::free(errors);
        em_free(&m);
        return 1;
    }
    r = em_classify(&m, &t, &c, &errors);
    em_table_free(&t);
    em_free(&m);
    if (r) {
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        else
            std::fprintf(stderr, "%s: %s: %s", me.c_str(), table.c_str(), errors);
        std::free(errors);
        return 1;
    }
    bool ok = true;
    if (!write_file(out, c.table)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        ok = false;
    } else if (!questions.empty() && !write_file(questions, c.questions)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), questions.c_str(), std::strerror(errno));
        ok = false;
    }
    if (ok)
        std::fputs(c.report, stdout);
    em_classified_free(&c);
    return ok ? 0 : 1;
}

void cmd_classify(CLI::App &app, int &status)
{
    struct options {
        std::string desc, table, out, questions;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("classify", "the class of every measured cell, and the questions");

    sub->add_option("DESC", o->desc, "the description of the matrix")->required()->type_name("FILE");
    sub->add_option("TABLE", o->table, "the measured cell table")->required()->type_name("FILE");
    sub->add_option("-o,--out", o->out, "the cell table with a class per cell (written)")
        ->required()
        ->type_name("CELLS");
    sub->add_option("-q,--questions", o->questions, "the questions, one per class (written)")
        ->type_name("FILE");
    sub->footer("Prints the counts per class and question class and the cells where the model differs "
                "from the measurement (the original's rules against 1.95, the port's against the port). "
                "Exit status 0, 1 a file cannot be read or written or is malformed (one message per "
                "error), 2 usage error.");
    sub->callback([sub, o, &status] { status = run(cli::name(*sub), o->desc, o->table, o->out, o->questions); });
}
