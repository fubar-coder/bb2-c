/*
 * bbsrc tokens - the token list of Blitz Basic 2 as the compiler hands it
 * to the editor, from the files: the compiler and its libraries
 * (format: lib/blitztokens/blitztokens.h, as tests/data/ted-tokens.txt).
 *
 *   bbsrc tokens [-o OUT] [--last XXXX] BLITZ2 [LIBS...]
 *
 * BLITZ2: the compiler (blitz2:blitz2); LIBS: library files or BigLib
 * files in the order the compiler links them - blitz2:acidlibs,
 * blitz2:deflibs, then the files of blitz2:userlibs.  Without -o the list
 * goes to stdout.  --last: the compiler has several lists (blitz2-ab3),
 * take the one whose last token is $XXXX (hex; AmiBlitz3's: 80DB).
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "blitztokens.h"
}
#include "bbsrc.h"

static const char *me;          /* "bbsrc tokens", the prefix of the messages */

static uint8_t *read_all(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    uint8_t *s = NULL;
    long n;

    if (!f)
        return NULL;
    if (!fseek(f, 0, SEEK_END) && (n = ftell(f)) >= 0 && !fseek(f, 0, SEEK_SET)
        && (s = (uint8_t *)malloc((size_t)n + 1)) && fread(s, 1, (size_t)n, f) == (size_t)n)
        *len = (size_t)n;
    else {
        free(s);
        s = NULL;
    }
    fclose(f);
    return s;
}

/* the token list of the compiler FILES[0] (its list ending with LAST, 0:
 * the only one) and the libraries FILES[1..] to OUT (NULL: stdout) */
static int tokens(const char *out, unsigned long last, const std::vector<std::string> &files)
{
    struct blitztokens list = {};
    std::vector<uint8_t *> bufs(files.size(), nullptr);
    size_t i;
    int rc = 1;
    FILE *f;

    for (i = 0; i < files.size(); i++) {
        const char *path = files[i].c_str();
        char err[300];
        size_t len;
        int r;

        if (!(bufs[i] = read_all(path, &len))) {
            fprintf(stderr, "%s: %s: %s\n", me, path, strerror(errno));
            goto done;
        }
        r = i == 0 ? blitztokens_compiler_last(bufs[i], len, (uint16_t)last, &list, err, sizeof err)
                   : blitztokens_libraries(bufs[i], len, &list, err, sizeof err);
        if (r) {
            fprintf(stderr, "%s: %s: %s\n", me, path, err);
            goto done;
        }
    }
    f = out ? fopen(out, "w") : stdout;
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", me, out, strerror(errno));
        goto done;
    }
    if (blitztokens_write(f, &list) || (out ? fclose(f) : fflush(f))) {
        fprintf(stderr, "%s: %s: %s\n", me, out ? out : "stdout", strerror(errno));
        if (out)
            remove(out);
        goto done;
    }
    rc = 0;
done:
    for (uint8_t *buf : bufs)
        free(buf);
    blitztokens_free(&list);
    return rc;
}

void cmd_tokens(CLI::App &app, int &status)
{
    struct options {
        std::string out, last, blitz2;
        std::vector<std::string> libs;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("tokens", "the token list of Blitz Basic 2 as the compiler hands it to the editor");
    sub->add_option("-o", o->out, "the list written (default: stdout)")->type_name("OUT");
    sub->add_option("--last", o->last, "of several lists in BLITZ2 (blitz2-ab3) the one whose last token is $XXXX (hex)")
        ->type_name("XXXX");
    sub->add_option("BLITZ2", o->blitz2, "the compiler (blitz2:blitz2)")->required();
    sub->add_option("LIBS", o->libs, "library or BigLib files in the order the compiler links them");
    sub->footer("The compiler's own tokens, then the tokens of LIBS (in the compiler's order: "
                "acidlibs, deflibs, userlibs), one \"XXXX name\" per line like "
                "tests/data/ted-tokens.txt. Exit status 0, 1 a file cannot be read or holds no "
                "token list, or OUT cannot be written, 2 usage error.");
    sub->callback([sub, o, &status] {
        const std::string name = cli::name(*sub);
        unsigned long last = 0;

        me = name.c_str();
        if (sub->count("--last")) {
            char *end;

            last = strtoul(o->last.c_str(), &end, 16);
            if (*end || !last || last > 0xFFFF) {
                fprintf(stderr, "%s: --last: not a token number: %s\n", me, o->last.c_str());
                status = 2;
                return;
            }
        }
        std::vector<std::string> files{ o->blitz2 };

        files.insert(files.end(), o->libs.begin(), o->libs.end());
        status = tokens(sub->count("-o") ? o->out.c_str() : NULL, last, files);
    });
}
