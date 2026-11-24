/*
 * exprtables page - the page docs/blitz2/expression-type-tables.md
 * (exprtype_tables.h, ett_page) by today's rules, from exprtype.c or from
 * the rows of its committed table exprtype.tsv: written to a file or
 * stdout, or checked against the committed page.
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include "cli.hpp"
extern "C" {
#include "exprtype_tables.h"
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

static int run(const std::string &me, const std::string &out, const std::string &check, const std::string &tsv)
{
    struct ett_prims p;
    struct ett_rules r;
    struct ett_tsv *t = nullptr;
    char *error = nullptr, *page;
    int status = 0;

    if (!tsv.empty()) {
        std::string text;
        int rc;

        if (!read_file(tsv, text)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), tsv.c_str(), std::strerror(errno));
            return 1;
        }
        rc = ett_tsv_load(text.data(), text.size(), tsv.c_str(), &t, &error);
        if (rc) {
            if (rc < 0)
                std::fprintf(stderr, "%s: out of memory\n", me.c_str());
            else
                std::fprintf(stderr, "%s", error);
            std::free(error);
            return 1;
        }
        ett_tsv_prims(t, &p);
    } else
        ett_prims_exprtype(&p);
    ett_rules_flow(&r, &p);
    page = ett_page(&r, nullptr, &error);
    if (!page) {
        if (t && ett_tsv_first_missing(t))
            std::fprintf(stderr, "%s: %s: no row %s\n", me.c_str(), tsv.c_str(), ett_tsv_first_missing(t));
        std::fprintf(stderr, "%s: %s\n", me.c_str(), error ? error : "out of memory");
        std::free(error);
        ett_tsv_free(t);
        return 1;
    }
    ett_tsv_free(t);
    if (!check.empty()) {
        std::string want;
        char *report = nullptr;
        int rc;

        if (!read_file(check, want)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), check.c_str(), std::strerror(errno));
            status = 1;
        } else if ((rc = ett_page_compare(want.data(), want.size(), page, std::strlen(page), check.c_str(),
                                          &report))) {
            if (rc < 0)
                std::fprintf(stderr, "%s: out of memory\n", me.c_str());
            else
                std::fprintf(stderr, "%s", report);
            std::free(report);
            status = 1;
        }
        if (status && !out.empty())
            std::remove(out.c_str());   // the result of a failed check is not built
    }
    if (!status && !out.empty() && !write_file(out, page)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        status = 1;
    } else if (!status && out.empty() && check.empty())
        std::fputs(page, stdout);
    std::free(page);
    return status;
}

void cmd_page(CLI::App &app, int &status)
{
    struct options {
        std::string out, check, tsv;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("page", "the page of the tables, written or checked");
    sub->add_option("-o,--out", o->out, "write the page to FILE (default: stdout)")->type_name("FILE");
    sub->add_option("--check", o->check, "compare the committed page FILE with the generated one")
        ->type_name("FILE");
    sub->add_option("--tsv", o->tsv, "take the decisions from the rows of exprtype.tsv FILE, not from exprtype.c")
        ->type_name("FILE");
    sub->footer("Without --out and --check the page goes to stdout. With --check it is written only to --out, "
                "and only if FILE equals it (a file at --out is removed otherwise). Exit status 0, 1 the pages differ (one message "
                "per differing line, FILE:LINE: ..., at most ten, and their number), a file cannot be read or "
                "written, or a decision has no row in the --tsv table, 2 usage error.");
    sub->callback([sub, o, &status] { status = run(cli::name(*sub), o->out, o->check, o->tsv); });
}
