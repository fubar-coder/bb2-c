/*
 * toklist - the compiler's own token list (programs/blitz2/src/toklist.h,
 * tools/lib/toklist/toklist.h): its BASIC keywords as JSON for the website
 * (site/data/compiler/blitz2.json), written or checked.  README.md here.
 *
 *   toklist json [-o FILE] [--check FILE] TOKLIST
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "cli.hpp"
extern "C" {
#include "toklist.h"
}

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

// the fields of the export that do not come from the list
static const char *const about =
    "by tools/toklist (toklist json) from the TK entries of blitz2's token list; do not edit: "
    "after a change of toklist.h build the target blitz2_keywords_update";
static const char *const source = "programs/blitz2/src/toklist.h";

static int run(const std::string &me, const std::string &in, const std::string &out, const std::string &check)
{
    std::string text;
    struct tl_list l = {};
    char err[400];
    int status = 0;

    if (!read_file(in, text)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), in.c_str(), std::strerror(errno));
        return 1;
    }
    if (tl_parse(&l, text.c_str(), err, sizeof err)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), in.c_str(), err);
        return 1;
    }
    char *json = tl_json(&l, about, source);
    tl_free(&l);
    if (!json) {
        std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        return 1;
    }
    if (!check.empty()) {
        std::string want;
        char *report = nullptr;
        int rc;

        if (!read_file(check, want)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), check.c_str(), std::strerror(errno));
            status = 1;
        } else if ((rc = tl_compare(want.data(), want.size(), json, check.c_str(), &report))) {
            if (rc < 0)
                std::fprintf(stderr, "%s: out of memory\n", me.c_str());
            else
                std::fprintf(stderr, "%s%s: if the change of %s is intended, build the target "
                             "blitz2_keywords_update and commit the file\n",
                             report, check.c_str(), in.c_str());
            std::free(report);
            status = 1;
        }
        if (status && !out.empty())
            std::remove(out.c_str());   // the result of a failed check is not built
    }
    if (!status && !out.empty() && !write_file(out, json)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        status = 1;
    } else if (!status && out.empty() && check.empty())
        std::fputs(json, stdout);
    std::free(json);
    return status;
}

int main(int argc, char **argv)
{
    CLI::App app{"The compiler's own token list (programs/blitz2/src/toklist.h): its BASIC keywords as JSON "
                 "for the website.",
                 "toklist"};
    std::string in, out, check;
    int status = 0;
    CLI::App *json = app.add_subcommand("json", "blitz2's keywords as JSON, written or checked");

    app.require_subcommand(1);
    json->add_option("TOKLIST", in, "the token list (programs/blitz2/src/toklist.h)")->required();
    json->add_option("-o,--out", out, "write the JSON to FILE (default: stdout)")->type_name("FILE");
    json->add_option("--check", check, "compare the committed file FILE with the generated JSON")
        ->type_name("FILE");
    json->footer("Without --out and --check the JSON goes to stdout. With --check it is written only to --out, "
                 "and only if FILE equals it (a file at --out is removed otherwise). Exit status 0, 1 the "
                 "files differ (one message per differing line, FILE:LINE: ..., at most ten, and their "
                 "number), a malformed token list (TOKLIST: line N: ...), or a file that cannot be read or "
                 "written, 2 usage error.");
    json->callback([&] { status = run(cli::name(*json), in, out, check); });
    return cli::run(app, argc, argv, status);
}
