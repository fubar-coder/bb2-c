/*
 * exprmatrix expect - the committed tests of the matrix (exprmatrix.h,
 * em_expect): written into the source tree ROOT (the target
 * expr-matrix-update), or with --check compared with it (the host test
 * exprmatrix_committed).
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include "cli.hpp"
extern "C" {
#include "exprmatrix.h"
}
#include "cmds.h"

namespace fs = std::filesystem;

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

// the directories whose files em_expect may own
static const char *const dirs[] = {
    "tests/corpus/tests-exprmatrix", "tests/bug-orig/exprmatrix",
    "tests/bug-orig/compiler", "tests/bug-orig/regression",
    "tests/ab3/model/exprmatrix",
};

static int run(const std::string &me, const std::string &desc, const std::string &measured,
               const std::string &root, bool check)
{
    std::string dtext, mtext;
    struct em_matrix m;
    struct em_table t;
    struct em_file *files = nullptr;
    size_t nfiles = 0;
    char *errors = nullptr, *report = nullptr;
    int r;
    std::error_code ec;

    if (!read_file(desc, dtext)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), desc.c_str(), std::strerror(errno));
        return 1;
    }
    if (!read_file(measured, mtext)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), measured.c_str(), std::strerror(errno));
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
    r = em_table_parse(mtext.data(), mtext.size(), measured.c_str(), &t, &errors);
    if (r) {
        std::fprintf(stderr, "%s", r < 0 ? "" : errors);
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        std::free(errors);
        em_free(&m);
        return 1;
    }
    // the description as the headers name it: relative to the tree
    fs::path rel = fs::proximate(fs::absolute(desc), fs::absolute(root), ec);
    std::string dname = ec || rel.empty() || *rel.begin() == ".." ? fs::path(desc).filename().string()
                                                                    : rel.generic_string();
    r = em_expect(&m, dname.c_str(), &t, &files, &nfiles, &report, &errors);
    em_table_free(&t);
    em_free(&m);
    if (r) {
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        else
            std::fprintf(stderr, "%s: %s", me.c_str(), errors);
        std::free(errors);
        return 1;
    }
    std::set<std::string> made;
    int bad = 0, written = 0, removed = 0;
    for (size_t i = 0; i < nfiles; i++) {
        std::string path = (fs::path(root) / files[i].path).string(), old;
        bool same = read_file(path, old) && old == files[i].text;

        made.insert(files[i].path);
        if (check) {
            if (!same) {
                std::printf("%s: %s\n", files[i].path, fs::exists(path) ? "differs from the generator's output"
                                                                        : "missing");
                bad++;
            }
            continue;
        }
        if (same)
            continue;
        fs::create_directories(fs::path(path).parent_path(), ec);
        if (!write_file(path, files[i].text)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), path.c_str(), std::strerror(errno));
            bad++;
        } else
            written++;
    }
    // the files of its own the generator did not write: their directories
    // and the data directories ID.test/ in them
    std::set<std::string> stale;
    for (const char *d : dirs) {
        fs::path dir = fs::path(root) / d;

        if (!fs::is_directory(dir, ec))
            continue;
        for (const auto &e : fs::recursive_directory_iterator(dir, ec)) {
            std::string relp = std::string(d) + "/" + fs::relative(e.path(), dir, ec).generic_string();

            if (e.is_regular_file() && em_expect_owns(relp.c_str()) && !made.count(relp))
                stale.insert(relp);
        }
    }
    for (const std::string &relp : stale) {
        fs::path path = fs::path(root) / relp;

        if (check) {
            std::printf("%s: not written by the generator\n", relp.c_str());
            bad++;
        } else if (fs::remove(path, ec)) {
            removed++;
            // a data directory left empty goes with its last file
            if (path.parent_path().extension() == ".test" && fs::is_empty(path.parent_path(), ec))
                fs::remove(path.parent_path(), ec);
        }
    }
    if (check)
        std::printf("%zu files, %d differ\n", nfiles, bad);
    else
        std::printf("%s%zu files, %d written, %d removed\n", report, nfiles, written, removed);
    std::free(report);
    em_files_free(files, nfiles);
    return bad ? 1 : 0;
}

void cmd_expect(CLI::App &app, int &status)
{
    struct options {
        std::string desc, measured, root;
        bool check = false;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("expect", "the committed tests of the matrix, written or checked");

    sub->add_option("DESC", o->desc, "the description of the matrix")->required()->type_name("FILE");
    sub->add_option("MEASURED", o->measured, "the measured values the model does not know (classify -m)")
        ->required()
        ->type_name("FILE");
    sub->add_option("-o,--root", o->root, "the root of the source tree")->required()->type_name("ROOT");
    sub->add_flag("--check", o->check, "compare the files under ROOT with the generator's output, write nothing");
    sub->footer("Writes the files under ROOT (and removes those of its own the generator no longer "
                "writes) and prints the counts; with --check prints every file that differs, is missing "
                "or is not the generator's. Exit status 0, 1 a file cannot be read or written, the input "
                "is malformed or (--check) a file differs, 2 usage error.");
    sub->callback([sub, o, &status] { status = run(cli::name(*sub), o->desc, o->measured, o->root, o->check); });
}
