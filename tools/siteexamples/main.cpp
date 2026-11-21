/*
 * siteexamples - the Blitz Basic examples of the website
 * (tools/lib/siteexamples/siteexamples.h, README.md here):
 *
 *   siteexamples check SITE MANIFEST
 *   siteexamples extract SITE MANIFEST OUTDIR
 *
 * check: every ```blitzbasic block of the site has an entry in the
 * manifest and every entry a block; says how many are run and how many
 * only compiled.  extract: the same check, then OUTDIR/NAME.bas for every
 * entry.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "siteexamples.h"
}

namespace fs = std::filesystem;

static bool read_text(const fs::path &p, std::string &out)
{
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;

    if (!f)
        return false;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

/* the blocks of the site: the text files of its content, data, layouts and
   assets, in the order of their paths */
static int scan_site(const fs::path &site, se_blocks *b, std::string &err)
{
    std::vector<fs::path> files;
    char msg[400];

    if (!fs::is_directory(site)) {
        err = site.string() + ": not a directory";
        return -1;
    }
    for (const char *dir : { "content", "data", "layouts", "assets" }) {
        fs::path d = site / dir;

        if (!fs::is_directory(d))
            continue;
        for (const auto &e : fs::recursive_directory_iterator(d))
            if (e.is_regular_file())
                files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path &p : files) {
        std::string text;

        if (!read_text(p, text)) {
            err = p.string() + ": cannot read";
            return -1;
        }
        if (text.find('\0') != std::string::npos || text.find("```blitzbasic") == std::string::npos)
            continue;
        std::string rel = fs::relative(p, site).generic_string();

        if (se_scan_text(b, rel.c_str(), text.c_str(), msg, sizeof msg)) {
            err = msg;
            return -1;
        }
    }
    return 0;
}

/* the checked blocks and the manifest; 0, or an error message on stderr and 1 */
static int load(const std::string &site, const std::string &manifest, se_blocks *b,
                se_manifest *m)
{
    std::string text, err;
    char msg[400];

    if (!read_text(manifest, text)) {
        fprintf(stderr, "siteexamples: cannot read %s\n", manifest.c_str());
        return 1;
    }
    if (se_manifest_parse(m, text.c_str(), msg, sizeof msg)) {
        fprintf(stderr, "siteexamples: %s: %s\n", manifest.c_str(), msg);
        return 1;
    }
    if (scan_site(site, b, err)) {
        fprintf(stderr, "siteexamples: %s\n", err.c_str());
        return 1;
    }
    if (se_check(m, b, msg, sizeof msg)) {
        fprintf(stderr, "siteexamples: %s\n", msg);
        return 1;
    }
    return 0;
}

static void summary(const se_manifest &m)
{
    printf("siteexamples: %zu examples, %zu run, %zu compiled only\n", m.n,
           se_count(&m, SE_RUN), se_count(&m, SE_COMPILE));
}

static int do_check(const std::string &site, const std::string &manifest)
{
    se_blocks b = {};
    se_manifest m = {};
    int rc = load(site, manifest, &b, &m);

    if (!rc)
        summary(m);
    se_blocks_free(&b);
    se_manifest_free(&m);
    return rc;
}

static int do_extract(const std::string &site, const std::string &manifest,
                      const std::string &outdir)
{
    se_blocks b = {};
    se_manifest m = {};
    int rc = load(site, manifest, &b, &m);

    if (!rc) {
        std::error_code ec;

        fs::create_directories(outdir, ec);
        for (size_t i = 0; !rc && i < m.n; i++) {
            const se_block *blk = se_find(&b, &m.v[i]);
            fs::path out = fs::path(outdir) / (std::string(m.v[i].name) + ".bas");
            std::ofstream f(out, std::ios::binary);

            f << "; " << m.v[i].file << "#" << m.v[i].index << " ("
              << (m.v[i].mode == SE_RUN ? "run" : "compiled only") << ")\n" << blk->text;
            f.close();
            if (!f) {
                fprintf(stderr, "siteexamples: cannot write %s\n", out.string().c_str());
                rc = 1;
            }
        }
        if (!rc)
            summary(m);
    }
    se_blocks_free(&b);
    se_manifest_free(&m);
    return rc;
}

int main(int argc, char **argv)
{
    CLI::App app{"The Blitz Basic examples of the website, against their manifest.", "siteexamples"};
    std::string site, manifest, outdir;
    int status = 0;
    CLI::App *check = app.add_subcommand("check", "every example has an entry and every entry an example");
    CLI::App *extract = app.add_subcommand("extract", "the check, then every example as NAME.bas");

    app.require_subcommand(1);
    app.footer("Exit status 0 ok, 1 an example without an entry, an entry without an example, a "
               "malformed manifest or a file that cannot be read or written, 2 usage error.");
    for (CLI::App *c : { check, extract }) {
        c->add_option("SITE", site, "the site directory (site/: content, data, layouts, assets)")
            ->required();
        c->add_option("MANIFEST", manifest, "the manifest (tests/site-examples/examples.txt)")
            ->required();
    }
    extract->add_option("OUTDIR", outdir, "the directory of the sources (made if missing)")
        ->required();
    check->callback([&] { status = do_check(site, manifest); });
    extract->callback([&] { status = do_extract(site, manifest, outdir); });
    return cli::run(app, argc, argv, status);
}
