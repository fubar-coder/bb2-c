/*
 * mktestdata - binary test data (8SVX, ILBM, MED and tracker modules) from
 * a text description (tools/lib/mktestdata/mktestdata.h).
 *
 *   mktestdata SPEC OUT
 *
 * Writes OUT only when the description is valid; exit status 1 on an
 * error (message with the line of SPEC).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "cli.hpp"
extern "C" {
#include "mktestdata.h"
}

static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (char *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        s[n] = 0;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* the data of the description SPEC to OUT */
static int build(const char *spec_path, const char *out)
{
    char err[200], *spec;
    uint8_t *data;
    size_t len;
    FILE *f;
    int bad;

    if (!(spec = read_text(spec_path))) {
        fprintf(stderr, "mktestdata: cannot read %s\n", spec_path);
        return 1;
    }
    if (mktestdata_build(spec, &data, &len, err, sizeof err)) {
        fprintf(stderr, "%s: %s\n", spec_path, err);
        free(spec);
        return 1;
    }
    free(spec);
    f = fopen(out, "wb");
    bad = !f || fwrite(data, 1, len, f) != len;
    if (f && fclose(f))
        bad = 1;
    free(data);
    if (bad) {
        fprintf(stderr, "mktestdata: cannot write %s\n", out);
        remove(out);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    CLI::App app{"Binary test data (8SVX, ILBM, MED, tracker modules) from a text description.", "mktestdata"};
    std::string spec, out;
    int status = 0;

    app.add_option("SPEC", spec, "the description (tools/lib/mktestdata/mktestdata.h)")->required();
    app.add_option("OUT", out, "the data written, only when the description is valid")->required();
    app.footer("Exit status 0 written, 1 SPEC cannot be read, the description has an error (OUT is "
               "not written) or OUT cannot be written, 2 usage error.");
    app.callback([&] { status = build(spec.c_str(), out.c_str()); });
    return cli::run(app, argc, argv, status);
}
