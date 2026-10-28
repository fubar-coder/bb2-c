// oracle.hpp - the oracle data of the TED tests (programs/ted/tests/oracle,
// target ted-oracle): the original TED's results for every source of the
// CD as SHA-256 in tests/data/ted-oracle.sha256, the token list in
// tests/data/ted-tokens.txt.  The sources themselves come from the CD
// (archive/cd, fetch-originals); without it the corpus tests skip.
//
// What the oracle did, the tests repeat with the core:
//   detok/PATH   per line p (0-terminated, p += strlen(p) + 1): detok, LF
//   tokall/PATH  every LF line of the detok text through tokit2, d5 bytes
//   save/PATH    fileopen2, realsave (tokenise = 1)
#ifndef TED_ORACLE_HPP
#define TED_ORACLE_HPP

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <sys/stat.h>
#include "sha256.h"

namespace oracle {

inline const std::string data_dir = TED_TEST_DATA_DIR;     // tests/data
inline const std::string cd_dir = TED_CD_DIR;              // archive/cd/...

inline std::string tokens_file()
{
    return data_dir + "/ted-tokens.txt";
}

struct source {
    std::string rel;            // path on the CD, e.g. BUI5/X/Y.BB2
    std::string path;           // on the host
    bool tok;                   // tokenised (has detok/ and tokall/)
};

// "SHA  KIND/PATH" lines -> {KIND/PATH: SHA}
inline const std::map<std::string, std::string> &manifest()
{
    static const std::map<std::string, std::string> m = [] {
        std::map<std::string, std::string> r;
        std::ifstream f(data_dir + "/ted-oracle.sha256");
        std::string line;

        while (std::getline(f, line))
            if (line.size() > 66 && line[64] == ' ' && line[65] == ' ')
                r[line.substr(66)] = line.substr(0, 64);
        return r;
    }();
    return m;
}

// every source of the manifest, in its order; empty without the CD
inline const std::vector<source> &sources()
{
    static const std::vector<source> v = [] {
        std::vector<source> r;
        struct stat st;

        if (stat((cd_dir + "/BLITZ2").c_str(), &st) || !S_ISDIR(st.st_mode))
            return r;
        for (const auto &[name, sha] : manifest())
            if (name.compare(0, 5, "save/") == 0) {
                std::string rel = name.substr(5);

                r.push_back({ rel, cd_dir + "/" + rel, manifest().count("detok/" + rel) != 0 });
            }
        return r;
    }();
    return v;
}

inline std::string expected(const std::string &kind, const source &s)
{
    auto it = manifest().find(kind + "/" + s.rel);

    return it == manifest().end() ? std::string() : it->second;
}

inline std::string sha256(const std::vector<unsigned char> &data)
{
    char hex[65];

    sha256_hex_of(data.data(), data.size(), hex);
    return hex;
}

// the whole file, 16 zero bytes behind it (detok may read past a line's 0
// like on the Amiga, where oracle.c pads the same way); false if missing
inline bool readfile(const std::string &path, std::vector<unsigned char> &data, size_t &size)
{
    std::ifstream f(path, std::ios::binary);

    if (!f)
        return false;
    data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    size = data.size();
    data.resize(size + 16, 0);
    return true;
}

} // namespace oracle

#endif
