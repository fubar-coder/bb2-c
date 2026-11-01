// test_tokenizer - the tokeniser component (shared/tokenizer) with
// AmiBlitz3's rules: the scenarios of the spec ab3-compat/tokenizer (case,
// whole words after a separator, name$ first, the first entry wins,
// strings and comments, the 250-character lines) and every rule of
// preproc_Tokenize the tokeniser follows, the embedded headers, and
// broken input (bad set-up, a full list, an output buffer too small).
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "tokenizer.h"
}

namespace {

using bytes = std::vector<uint8_t>;

// a token as it appears in a tokenised source
std::string tk(uint16_t num)
{
    return std::string{ char(num >> 8), char(num & 0xFF) };
}

const uint16_t T_NPRINT = 0x8020, T_PRINT = 0x8021, T_STR = 0x8030, T_STRS = 0x8031,
               T_EXECBASE = 0xDE26, T_SHOW = 0x8040, T_SUB = 0x8041, T_END = 0x8003,
               T_LEFT = 0x8050, T_WIDTH = 0x8060, T_IF = 0x8004, T_A = 0x8070;

struct fixture {
    tokenizer t{};
    std::vector<tokenizer_entry> entries;
    std::vector<uint16_t> buckets;
    std::vector<std::string> names;     // the bytes the entries point to

    explicit fixture(uint32_t cap = 64, uint32_t nbucket = 16)
        : entries(cap), buckets(nbucket)
    {
        names.reserve(cap + 8);
        REQUIRE(tokenizer_init(&t, TOKENIZER_RULES_AB3, entries.data(), cap, buckets.data(),
                               nbucket) == TOKENIZER_OK);
    }

    int add(const std::string &name, uint16_t num)
    {
        names.push_back(name);
        const std::string &n = names.back();
        return tokenizer_add(&t, reinterpret_cast<const uint8_t *>(n.data()), n.size(), num);
    }

    // the usual list: compiler tokens, then a "library" with a lower-case clash
    static fixture standard()
    {
        fixture f;
        f.add("NPrint", T_NPRINT);
        f.add("Print", T_PRINT);
        f.add("Str", T_STR);
        f.add("Str$", T_STRS);
        f.add("ExecBase", T_EXECBASE);
        f.add("Show", T_SHOW);
        f.add("Sub", T_SUB);
        f.add("End", T_END);
        f.add("Left", T_LEFT);
        f.add("Width", T_WIDTH);
        f.add("If", T_IF);
        f.add("a", T_A);
        return f;
    }

    std::string text(const std::string &s, int *rc = nullptr) const
    {
        bytes out(TOKENIZER_BOUND(s.size()));
        size_t n = 99;
        int r = tokenizer_text(&t, reinterpret_cast<const uint8_t *>(s.data()), s.size(),
                               out.data(), out.size(), &n);
        if (rc)
            *rc = r;
        else
            REQUIRE(r == TOKENIZER_OK);
        return std::string(out.begin(), out.begin() + long(n));
    }

    std::string line(const std::string &s) const
    {
        bytes out(TOKENIZER_BOUND(s.size()));
        size_t n = 99;
        REQUIRE(tokenizer_line(&t, reinterpret_cast<const uint8_t *>(s.data()), s.size(),
                               out.data(), out.size(), &n) == TOKENIZER_OK);
        return std::string(out.begin(), out.begin() + long(n));
    }
};

const std::string Z(1, '\0');

} // namespace

TEST_CASE("names match case-sensitively", "[tokenizer]")
{
    fixture f = fixture::standard();

    // spec: *exec.ExecBase = execbase - the variable stays a name, the
    // type .ExecBase is behind a '.'
    CHECK(f.line("*exec.ExecBase = execbase") == "*exec.ExecBase = execbase" + Z);
    CHECK(f.line("x = ExecBase") == "x = " + tk(T_EXECBASE) + Z);
    CHECK(f.line("NPrint \"x\"") == tk(T_NPRINT) + " \"x\"" + Z);
    CHECK(f.line("nprint \"x\"") == "nprint \"x\"" + Z);
    CHECK(f.line("NPRINT 1") == "NPRINT 1" + Z);
}

TEST_CASE("only whole words after a separator", "[tokenizer]")
{
    fixture f = fixture::standard();

    // spec: show{1} and sub1 contain no token (TED would split them)
    CHECK(f.line("Show{1}") == tk(T_SHOW) + "{1}" + Z);
    CHECK(f.line("show2{1} : sub1 = 3") == "show2{1} : sub1 = 3" + Z);
    CHECK(f.line("Showx{1}") == "Showx{1}" + Z);
    CHECK(f.line("Sub_a") == "Sub_a" + Z);
    // spec: the field after '\' stays a name; also after . # ' @ _ and a digit
    CHECK(f.line("*w.Window\\Width") == "*w.Window\\Width" + Z);
    CHECK(f.line("#Print 'Print @Print _Print 2Print .Print")
          == "#Print 'Print @Print _Print 2Print .Print" + Z);
    // after the operators, brackets, ':' and a space a token starts
    CHECK(f.line("(Print)+Print:Print,Print") == "(" + tk(T_PRINT) + ")+" + tk(T_PRINT) + ":"
                                                     + tk(T_PRINT) + "," + tk(T_PRINT) + Z);
    // behind a token another one needs a separator first
    CHECK(f.line("Ifa") == "Ifa" + Z);
    CHECK(f.line("If a") == tk(T_IF) + " " + tk(T_A) + Z);
    // a word ending in '#' is looked up there, the '#' stays
    CHECK(f.line("Print#") == tk(T_PRINT) + "#" + Z);
}

TEST_CASE("name$ before name, the first entry wins", "[tokenizer]")
{
    fixture f = fixture::standard();

    // spec: Str$ is the string function's token
    CHECK(f.line("b$ = Str$(5)") == "b$ = " + tk(T_STRS) + "(5)" + Z);
    CHECK(f.line("x = Str(5)") == "x = " + tk(T_STR) + "(5)" + Z);
    // only "Left" is a name: its token, then the '$'
    CHECK(f.line("Left$(b$,1)") == tk(T_LEFT) + "$(b$,1)" + Z);
    CHECK(f.line("a$=1") == tk(T_A) + "$=1" + Z);
    // neither: the '$' belongs to the word, which goes on
    CHECK(f.line("Foo$x") == "Foo$x" + Z);
    // a name given twice keeps its first token
    CHECK(f.add("Print", 0x9999) == TOKENIZER_DUPLICATE);
    CHECK(f.add("print", 0x9998) == TOKENIZER_OK);  // another case: another name
    uint16_t n = 0;
    CHECK(tokenizer_find(&f.t, reinterpret_cast<const uint8_t *>("Print"), 5, &n) == 1);
    CHECK(n == T_PRINT);
    CHECK(f.line("Print print") == tk(T_PRINT) + " " + tk(0x9998) + Z);
    CHECK(tokenizer_find(&f.t, reinterpret_cast<const uint8_t *>("Prin"), 4, &n) == 0);
    CHECK(tokenizer_find(&f.t, reinterpret_cast<const uint8_t *>("x"), 0, &n) == 0);
}

TEST_CASE("strings and comments are not tokenised", "[tokenizer]")
{
    fixture f = fixture::standard();

    // spec: NPrint "Print"
    CHECK(f.line("NPrint \"Print\"") == tk(T_NPRINT) + " \"Print\"" + Z);
    CHECK(f.line("NPrint \"a\":Print") == tk(T_NPRINT) + " \"a\":" + tk(T_PRINT) + Z);
    CHECK(f.line("Print ; NPrint here") == tk(T_PRINT) + " ; NPrint here" + Z);
    // inside: a TAB becomes a space, every other byte stays (also CR, 128+)
    CHECK(f.line("\"a\tb\xe4\r\"") == "\"a b\xe4\r\"" + Z);
    CHECK(f.line(";a\tb\xe4\r") == ";a b\xe4\r" + Z);
    // a string ends at the line's end
    CHECK(f.text("\"Print\nPrint\n") == "\"Print" + Z + tk(T_PRINT) + Z);
}

TEST_CASE("characters outside strings and comments", "[tokenizer]")
{
    fixture f = fixture::standard();

    CHECK(f.line("Print\t1") == tk(T_PRINT) + " 1" + Z);           // TAB: a space
    CHECK(f.text("Print 1\r\nEnd\r\n") == tk(T_PRINT) + " 1" + Z + tk(T_END) + Z);  // CR dropped
    CHECK(f.line("a\x7f\x01 b") == tk(T_A) + " b" + Z);              // DEL, controls dropped
    CHECK(f.line("x=\xe4") == "x=?" + Z);                              // 128+: '?'
    CHECK(f.text(std::string("Print\0End\n", 10)) == tk(T_PRINT) + Z + tk(T_END) + Z);
}

TEST_CASE("the end of a text", "[tokenizer]")
{
    fixture f = fixture::standard();

    // a word without a character behind it is not looked up (AmiBlitz3's
    // loop ends first); a string or comment there gets its 0
    CHECK(f.text("Print 1\nEnd") == tk(T_PRINT) + " 1" + Z + "End");
    CHECK(f.text("End\n") == tk(T_END) + Z);
    CHECK(f.text("Print \"x") == tk(T_PRINT) + " \"x" + Z);
    CHECK(f.text("Print ;x") == tk(T_PRINT) + " ;x" + Z);
    CHECK(f.text("") == "");
    CHECK(f.line("") == Z);
}

TEST_CASE("the output buffer", "[tokenizer]")
{
    fixture f = fixture::standard();
    // the largest growth: one-letter tokens, and a comment at the end
    std::string s = "a a a ;";
    bytes out(TOKENIZER_BOUND(s.size()));
    size_t n = 0;

    CHECK(tokenizer_text(&f.t, reinterpret_cast<const uint8_t *>(s.data()), s.size(),
                         out.data(), out.size(), &n) == TOKENIZER_OK);
    CHECK(std::string(out.begin(), out.begin() + long(n))
          == tk(T_A) + " " + tk(T_A) + " " + tk(T_A) + " ;" + Z);
    // too small: refused, at every size
    for (size_t cap = 0; cap < n; cap++) {
        size_t m = 99;
        CHECK(tokenizer_text(&f.t, reinterpret_cast<const uint8_t *>(s.data()), s.size(),
                             out.data(), cap, &m) == TOKENIZER_E_SPACE);
        CHECK(m <= cap);
    }
    size_t m = 0;
    CHECK(tokenizer_line(&f.t, reinterpret_cast<const uint8_t *>("Str$"), 4, out.data(), 2, &m)
          == TOKENIZER_E_SPACE);
}

TEST_CASE("broken set-up", "[tokenizer]")
{
    tokenizer t{};
    tokenizer_entry e[2];
    uint16_t bk[8];

    CHECK(tokenizer_init(&t, 0, e, 2, bk, 8) == TOKENIZER_E_RULES);
    CHECK(tokenizer_init(&t, 2, e, 2, bk, 8) == TOKENIZER_E_RULES);
    CHECK(tokenizer_init(&t, TOKENIZER_RULES_AB3, e, 2, bk, 0) == TOKENIZER_E_BUCKETS);
    CHECK(tokenizer_init(&t, TOKENIZER_RULES_AB3, e, 2, bk, 6) == TOKENIZER_E_BUCKETS);
    CHECK(tokenizer_init(&t, TOKENIZER_RULES_AB3, e, 2, bk, 65537) == TOKENIZER_E_BUCKETS);
    REQUIRE(tokenizer_init(&t, TOKENIZER_RULES_AB3, e, 2, bk, 1) == TOKENIZER_OK);
    const uint8_t *n = reinterpret_cast<const uint8_t *>("PrintEnd");
    CHECK(tokenizer_add(&t, n, 0, 1) == TOKENIZER_E_NAME);
    CHECK(tokenizer_add(&t, n, 5, 1) == TOKENIZER_OK);
    CHECK(tokenizer_add(&t, n + 5, 3, 2) == TOKENIZER_OK);
    CHECK(tokenizer_add(&t, n, 8, 3) == TOKENIZER_E_FULL);
    CHECK(tokenizer_add(&t, n, 5, 4) == TOKENIZER_DUPLICATE);
    // one bucket: both names are still found
    uint16_t num = 0;
    CHECK(tokenizer_find(&t, n + 5, 3, &num) == 1);
    CHECK(num == 2);
    CHECK(tokenizer_find(&t, n, 5, &num) == 1);
    CHECK(num == 1);
    // an object with another rule set does not tokenise
    t.rules = 7;
    uint8_t out[8];
    size_t m = 99;
    CHECK(tokenizer_text(&t, n, 5, out, sizeof out, &m) == TOKENIZER_E_RULES);
    CHECK(m == 0);
}

TEST_CASE("the header of a main source", "[tokenizer]")
{
    auto hdr = [](const std::string &s) {
        return tokenizer_ab3_header(reinterpret_cast<const uint8_t *>(s.data()), s.size());
    };
    std::string h = "; XTRA-Embedded header\n; Project = \"x\"\n; /XTRA\n";

    CHECK(hdr(h + "NPrint 1\n") == h.size());
    CHECK(hdr(";/xtra\nx") == 7);
    CHECK(hdr("; / X-T R A\n") == 0);              // '-' is a space
    CHECK(hdr("a=1\n;x=/XTRA\nx") == 0);           // the token is before '='
    CHECK(hdr("; /XTRA = 5\nx") == 12);
    CHECK(hdr("NPrint 1\n;/Xtra\nEnd\n") == 16);   // any line before it
    CHECK(hdr("; XTRA\n; /XTRA") == 14);           // last line: to the end
    CHECK(hdr("; XTRA\nNPrint 1\n") == 0);         // none
    CHECK(hdr("; /XTRAS\n") == 0);
    CHECK(hdr("") == 0);
    CHECK(tokenizer_lines(reinterpret_cast<const uint8_t *>(h.data()), h.size()) == 3);
    CHECK(tokenizer_lines(reinterpret_cast<const uint8_t *>("a\nb"), 3) == 2);
    CHECK(tokenizer_lines(reinterpret_cast<const uint8_t *>(""), 0) == 0);
}

TEST_CASE("the header of an include", "[tokenizer]")
{
    auto hdr = [](const std::string &s) {
        return tokenizer_ab3_incheader(reinterpret_cast<const uint8_t *>(s.data()), s.size());
    };
    std::string h = "; XTRA-Embedded header\n; Project = \"x\"\n; /XTRA\n";

    CHECK(hdr(h + "NPrint 1\n") == h.size());
    CHECK(hdr("; a\n;/Xtra x\nEnd") == 13);
    CHECK(hdr("; a\n;/xTRA\nEnd\n") == 0);         // other spellings: none
    CHECK(hdr("NPrint 1\n; /XTRA\n") == 0);        // a line without ';' first
    CHECK(hdr("; a\n\n; /XTRA\n") == 0);           // also an empty one
    CHECK(hdr("; a\n; b\n") == 0);                 // the end reached
    CHECK(hdr("; a\n; /XTRA") == 0);               // ... also behind /XTRA
    CHECK(hdr("x/XTRA\nEnd\n") == 7);              // '/' needs no ';' before
    CHECK(hdr("") == 0);
}

TEST_CASE("the lines of a main source", "[tokenizer]")
{
    // spec: a line longer than 250 characters is cut there
    std::string long300(300, 'x');
    std::string tok = "a" + Z + Z + long300 + Z + std::string(250, 'y') + Z + "End";
    const uint8_t *p = reinterpret_cast<const uint8_t *>(tok.data());
    size_t keep = 0, pos = 0;

    pos = tokenizer_ab3_mainline(p, tok.size(), pos, &keep);
    CHECK(pos == 2);
    CHECK(keep == 1);
    pos = tokenizer_ab3_mainline(p, tok.size(), pos, &keep);
    CHECK(pos == 3);
    CHECK(keep == 0);
    pos = tokenizer_ab3_mainline(p, tok.size(), pos, &keep);
    CHECK(pos == 304);
    CHECK(keep == TOKENIZER_AB3_COLUMNS);
    pos = tokenizer_ab3_mainline(p, tok.size(), pos, &keep);
    CHECK(pos == 555);
    CHECK(keep == 250);
    pos = tokenizer_ab3_mainline(p, tok.size(), pos, &keep);     // without its 0
    CHECK(pos == tok.size());
    CHECK(keep == 3);
}

TEST_CASE("file names", "[tokenizer]")
{
    auto ext = [](const std::string &s) {
        return s.substr(tokenizer_ab3_getext(s.data(), s.size()));
    };
    auto src = [](const std::string &s) { return tokenizer_ab3_source(s.data(), s.size()); };

    CHECK(ext("TEST:src/x.ab3") == "ab3");
    CHECK(ext("dos.include.ab3") == "ab3");
    CHECK(ext("dos.include") == "include");
    CHECK(ext("mylib") == "");
    CHECK(ext(".ab3") == "");                  // a '.' at the start: none
    CHECK(ext("x..ab3") == "ab3");
    CHECK(ext("a.123456789") == "123456789");  // 9 characters behind the '.'
    CHECK(ext("a.1234567890") == "");          // 10: too far back
    CHECK(ext("dir.x/name") == "x/name");      // not stopped by a '/'
    CHECK(ext("") == "");
    CHECK(src("x.ab3") == (TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER));
    CHECK(src("x.AB3") == (TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER));
    CHECK(src("x.txt") == (TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER));
    CHECK(src("x") == (TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER));
    CHECK(src("x.ab2") == TOKENIZER_AB3_TEXT);
    CHECK(src("x.Ab2") == TOKENIZER_AB3_TEXT);
    CHECK(src("x.bb2") == 0);
    CHECK(src("x.BB") == 0);
    CHECK(src("x.bb22") == (TOKENIZER_AB3_TEXT | TOKENIZER_AB3_HEADER));
}
