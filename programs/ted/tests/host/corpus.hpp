// corpus.hpp - the environment of the corpus tests (test_detok, test_tokit,
// test_loadsave): the core with the oracle's token list, the oracle's
// detok text built with the core.
#ifndef TED_CORPUS_HPP
#define TED_CORPUS_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "loadsave.h"
#include "tokit.h"
}
#include "oracle.hpp"
#include "tempdir.hpp"

namespace corpus {

using ::tempdir;

constexpr size_t bufsize = 32768;       // line buffers, as oracle.c

// set up once per program (the core keeps its globals), tokenise = 1
inline void env()
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 25);
        done = true;
    }
    tokenise = 1;
}

// detok text of a tokenised source as the oracle builds it: per line p
// (p += strlen(p) + 1) detok + LF; false if the file is missing
inline bool detok_text(const oracle::source &s, std::vector<unsigned char> &text)
{
    std::vector<unsigned char> data, out(bufsize);
    size_t size;

    if (!oracle::readfile(s.path, data, size))
        return false;
    for (const UBYTE *p = data.data(); p < data.data() + size;
         p += std::strlen(reinterpret_cast<const char *>(p)) + 1) {
        UBYTE *e = detok(p, out.data());

        text.insert(text.end(), out.data(), e);
        text.push_back('\n');
    }
    return true;
}

// with TED_TEST_DUMP=DIR: a result that differs from the oracle goes to
// DIR/KIND/PATH (PATH with / -> _), to compare with the outputs of a
// ted-oracle run (build/<preset>/programs/ted/tests/oracle/run/vol/out)
inline void dump(const char *kind, const oracle::source &s, const std::vector<unsigned char> &data)
{
    const char *dir = std::getenv("TED_TEST_DUMP");

    if (!dir || !*dir)
        return;
    std::string d = std::string(dir) + "/" + kind, name = s.rel;

    mkdir(dir, 0777);
    mkdir(d.c_str(), 0777);
    for (auto &c : name)
        if (c == '/')
            c = '_';
    if (FILE *f = std::fopen((d + "/" + name).c_str(), "wb")) {
        std::fwrite(data.data(), 1, data.size(), f);
        std::fclose(f);
    }
}

// the result against the oracle (dumped if different)
inline bool matches(const char *kind, const oracle::source &s, const std::vector<unsigned char> &data)
{
    bool ok = oracle::sha256(data) == oracle::expected(kind, s);

    if (!ok)
        dump(kind, s, data);
    return ok;
}

} // namespace corpus

#endif
