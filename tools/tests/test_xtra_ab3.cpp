// test_xtra_ab3 - AmiBlitz3's embedded .xtra header (xtra_load with
// XTRA_T_AB3, xtra_write_ab3 of shared/xtra) with sources in memory: a
// header as AmiBlitz3 writes it, loaded field by field (texts and maxima
// as views into the header) and written back byte for byte, the
// defaults, a source without header, a new header in AmiBlitz3's layout
// (residents joining, numbers like Str$), conversion to the Blitz 2 .xtra
// and back, unknown keys, and broken headers (each refused with its code
// and line).  Updating a kept header: test_xtra_ab3_update.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}
#include "xtra_ab3_full.h"

namespace {

struct xtra x, y;
std::string src;                // the loaded source (the header's bytes)
struct xtra_base base;

int read(const std::string &s, struct xtra_error *err = nullptr)
{
    src = s;
    base = {nullptr, reinterpret_cast<const uint8_t *>(src.data())};
    return xtra_load(&x, XTRA_T_AB3, base.header, src.size(), nullptr, 0, err);
}

std::string text(int f, const struct xtra *v = &x, const struct xtra_base *b = &base)
{
    size_t n = 99;
    const uint8_t *p = xtra_text(v, b, f, &n);

    REQUIRE(p != nullptr);
    return std::string(reinterpret_cast<const char *>(p), n);
}

struct max {
    std::string name;
    unsigned lib, max, flags;
};

std::vector<max> maxima(const struct xtra *v = &x, const struct xtra_base *b = &base)
{
    std::vector<max> r;
    struct xtra_iter it;
    struct xtra_max m;

    xtra_max_begin(&it, v, b);
    while (xtra_next_max(&it, &m))
        r.push_back({m.name ? std::string(reinterpret_cast<const char *>(m.name), m.namelen) : "", m.lib,
                     m.max, m.flags});
    return r;
}

// written as a header: update of the kept header (if any)
std::string write(const struct xtra *v = &x, const struct xtra_base *b = &base, unsigned *skipped = nullptr)
{
    static char out[16384];
    size_t len = 0;

    REQUIRE(xtra_write_ab3(v, b, out, sizeof(out), &len, skipped) == XTRA_OK);
    REQUIRE(xtra_ab3_size(v, b) == len);
    return std::string(out, len);
}

// written as a new header (whatever the source had)
std::string fresh(const struct xtra *v = &x, unsigned *skipped = nullptr)
{
    static struct xtra c;

    c = *v;
    c.ab3.header = 0;
    return write(&c, &base, skipped);
}

// a header with these lines between "; XTRA" and "; /XTRA"
std::string with(const std::string &lines)
{
    return "; XTRA\n" + lines + "; /XTRA\nNPrint \"x\"\n";
}

// refused with code in line
void refused(const std::string &s, int code, uint32_t line)
{
    struct xtra_error err;

    CHECK(read(s, &err) == code);
    CHECK(err.code == code);
    CHECK(err.in == XTRA_IN_HEADER);
    CHECK(err.line == line);
    CHECK(x.hdr == 0);
    CHECK(x.type == XTRA_T_AB3);
}

} // namespace

TEST_CASE("a full header: every field", "[xtra][ab3]")
{
    REQUIRE(read(full + "NPrint \"x\"\n") == XTRA_OK);
    CHECK(x.hdr == full.size());
    CHECK(x.type == XTRA_T_AB3);
    CHECK(x.source == XTRA_SRC_AB3_TEXT);
    CHECK(x.ab3.header == 1);
    CHECK(text(XTRA_F_EXE_PATH) == "Development:AmiBlitz3/Tests");
    CHECK(x.text[XTRA_F_EXE_PATH].in == XTRA_IN_HEADER);
    CHECK(src.compare(x.text[XTRA_F_EXE_PATH].off, 27, "Development:AmiBlitz3/Tests") == 0);
    CHECK(text(XTRA_F_EXE_NAME) == "");
    CHECK(x.icon == XTRA_ON);
    CHECK(text(XTRA_F_RES0) == "");
    CHECK(text(XTRA_F_RES0 + 1) == "all.res");
    CHECK(text(XTRA_F_RES0 + 2) == "");
    CHECK(x.buf[XTRA_BUF_STRING] == 20000);
    CHECK(x.buf[XTRA_BUF_OBJECT] == 16384);     // not in the header: Blitz 2's
    CHECK(x.sw[XTRA_SW_MAKE_SMALLEST] == XTRA_ON);
    CHECK(x.ab3.func_optimize == 1);
    CHECK(x.ab3.version[0] == 3);
    CHECK(x.ab3.version[1] == 9);
    CHECK(x.ab3.version[2] == 11);
    CHECK(x.ab3.builds == 537);
    CHECK(text(XTRA_F_CLI) == "PATH=music:sounds");
    CHECK(x.ab3.stack == 64000);
    CHECK(x.sw[XTRA_SW_RUNTIME_ERRORS] == 0);
    CHECK(x.sw[XTRA_SW_DEBUG_INFO] == XTRA_ON);
    CHECK(x.ab3.create_dbgfile == 1);
    CHECK(x.sw[XTRA_SW_OVERFLOW_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == XTRA_ON);
    CHECK(x.sw[XTRA_SW_INTERRUPT_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_AUTO_RUN] == 0);
    CHECK(x.editor.flags == XTRA_ED_CURSOR);
    CHECK(x.editor.line == 144);
    CHECK(x.editor.column == 11);
    CHECK(text(XTRA_F_LABEL_SEARCH) == "errors");
    CHECK(x.ab3.label_remark == 1);
    CHECK(x.ab3.label_all == 1);
    CHECK(x.ab3.label_case == 1);
    CHECK(x.ab3.label_position == 7);
    auto m = maxima();
    REQUIRE(m.size() == 3);
    CHECK(x.max.n == 3);
    CHECK(x.max.in == XTRA_IN_HEADER);
    CHECK(m[0].name == "Screen");
    CHECK(m[0].max == 5);
    CHECK(m[0].flags == XTRA_MAX_NAME);
    CHECK(m[1].name == "Req-Lib");              // as written: '-' for a blank
    CHECK(m[2].name == "ChunkyBuffer");
    CHECK(m[2].max == 2);
    CHECK(x.ab3.seen == ((1ul << XTRA_K_COUNT) - 1));
    CHECK(x.ab3.unknown == 0);
}

TEST_CASE("written back: the same bytes, kept or new", "[xtra][ab3]")
{
    REQUIRE(read(full + "End\n") == XTRA_OK);
    CHECK(write() == full);
    CHECK(fresh() == full);
}

TEST_CASE("the header may move: the views are offsets", "[xtra][ab3]")
{
    REQUIRE(read(full + "End\n") == XTRA_OK);
    std::string kept = src.substr(0, x.hdr);    // stored elsewhere
    src.assign(src.size(), '#');
    struct xtra_base kb = {nullptr, reinterpret_cast<const uint8_t *>(kept.data())};
    CHECK(text(XTRA_F_CLI, &x, &kb) == "PATH=music:sounds");
    CHECK(maxima(&x, &kb).size() == 3);
    CHECK(write(&x, &kb) == full);
}

TEST_CASE("no header: AmiBlitz3's defaults, nothing kept", "[xtra][ab3]")
{
    REQUIRE(read("NPrint \"hello\"\n; XTRA\n") == XTRA_OK);
    CHECK(x.hdr == 0);
    CHECK(x.ab3.header == 0);
    CHECK(x.ab3.seen == 0);
    CHECK(x.source == XTRA_SRC_AB3_TEXT);
    CHECK(text(XTRA_F_EXE_PATH, &x, nullptr) == "RAM:");
    CHECK(text(XTRA_F_EXE_NAME, &x, nullptr) == "Prog.exe");
    CHECK(x.icon == 0);
    CHECK(x.sw[XTRA_SW_RUNTIME_ERRORS] == XTRA_ON);
    CHECK(x.sw[XTRA_SW_MAKE_SMALLEST] == 0);
    CHECK(x.sw[XTRA_SW_OVERFLOW_CHECK] == XTRA_ON);
    CHECK(x.sw[XTRA_SW_INTERRUPT_CHECK] == XTRA_ON);
    CHECK(x.sw[XTRA_SW_AUTO_RUN] == XTRA_ON);
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_DEBUG_INFO] == 0);
    CHECK(x.buf[XTRA_BUF_STRING] == 10240);
    CHECK(x.ab3.stack == 32768);
    CHECK(x.editor.flags == 0);
    CHECK(maxima().empty());
    // the writer: a new header, no base needed
    std::string w = write(&x, nullptr);
    CHECK(w.find("; ExeFile         = \"Prog.exe\"\n") != std::string::npos);
    REQUIRE(read("") == XTRA_OK);
    CHECK(x.hdr == 0);
    CHECK(xtra_load(&x, XTRA_T_AB3, nullptr, 0, nullptr, 0, nullptr) == XTRA_OK);
    CHECK(x.ab3.header == 0);
}

TEST_CASE("an AB3 source has no .xtra: one given is ignored", "[xtra][ab3]")
{
    std::vector<uint8_t> f(1110, 0);

    f[0] = 1;
    f[1] = 0x80;
    src = full;
    REQUIRE(xtra_load(&x, XTRA_T_AB3, reinterpret_cast<const uint8_t *>(src.data()), src.size(), f.data(),
                      f.size(), nullptr) == XTRA_OK);
    CHECK(x.source == XTRA_SRC_AB3_TEXT);
    CHECK(x.ab3.stack == 64000);
}

TEST_CASE("a short header: only what it has; case, blanks, CR LF", "[xtra][ab3]")
{
    std::string s = ";xtra\r\n;   version=1.2.3  \r\n\n\t; CURSORLINE =\t0\r\n;stacksize= $1F40\r\n; /xtra \r\nEnd\n";

    REQUIRE(read(s) == XTRA_OK);
    CHECK(s.substr(x.hdr) == "End\n");
    CHECK(x.ab3.seen == ((1ul << XTRA_K_VERSION) | (1ul << XTRA_K_CURSORLINE) | (1ul << XTRA_K_STACKSIZE)));
    CHECK(x.ab3.version[0] == 1);
    CHECK(x.ab3.version[2] == 3);
    CHECK(x.editor.line == 0);              // 0: as 1
    CHECK(x.ab3.stack == 8000);
    CHECK(text(XTRA_F_EXE_NAME) == "Prog.exe");   // default
    CHECK(write() == s.substr(0, x.hdr));
}

TEST_CASE("unknown keys and other lines are kept, unknown keys counted", "[xtra][ab3]")
{
    std::string s = with("; Embedded .xtra Header\n; some text\n; Colour = 3\n; Foo = \"bar\n"
                         "; ExeFile = \"a\"\n");

    REQUIRE(read(s) == XTRA_OK);
    CHECK(x.ab3.unknown == 2);
    CHECK(x.ab3.unknown_line == 4);
    CHECK(text(XTRA_F_EXE_NAME) == "a");
    CHECK(write() == s.substr(0, x.hdr));
}

TEST_CASE("values: bare text, '=' in text, last key wins", "[xtra][ab3]")
{
    REQUIRE(read(with("; ExePath = ram:t\n; CliArgs = \"a=b\"\n; StackSize = 1\n; StackSize = 2\n"
                      "; Max Screen = 1\n; Max Screen = 9\n; Residents = \"a\"\n; Residents = \"b,c\"\n")) ==
            XTRA_OK);
    CHECK(text(XTRA_F_EXE_PATH) == "ram:t");
    CHECK(text(XTRA_F_CLI) == "a=b");
    CHECK(x.ab3.stack == 2);
    auto m = maxima();
    REQUIRE(m.size() == 2);                 // both lines; the last one counts
    CHECK(m[0].max == 1);
    CHECK(m[1].max == 9);
    CHECK(text(XTRA_F_RES0) == "b");
    CHECK(text(XTRA_F_RES0 + 1) == "c");
    CHECK(text(XTRA_F_RES0 + 2) == "");
}

TEST_CASE("negative values (Blitz's True), the label words, AmiBlitz3's Str$", "[xtra][ab3]")
{
    std::string s = with("; LabelAll = -1\n; AssemblerCheck = -1\n; LabelPosition = -2147483648\n"
                         "; CursorLine = -5\n; StackSize = -$10\n");

    REQUIRE(read(s) == XTRA_OK);
    CHECK(x.ab3.label_all == 0xFFFF);       // a word as written
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == XTRA_ON);
    CHECK(x.ab3.label_position == 0x80000000u);
    CHECK(x.editor.line == 0);
    CHECK(x.ab3.stack == 0xFFFFFFF0u);
    CHECK(write() == s.substr(0, x.hdr));   // kept as written
    std::string w = fresh();
    CHECK(w.find("; LabelPosition   = -2147483648\n") != std::string::npos);
    CHECK(w.find("; StackSize       = -16\n") != std::string::npos);
    CHECK(w.find("; LabelAll        = -1\n") != std::string::npos);
    CHECK(w.find("; AssemblerCheck  = 1\n") != std::string::npos);
    REQUIRE(read(with("; LabelRemark = 25\n; LabelCase = 65535\n")) == XTRA_OK);
    CHECK(x.ab3.label_remark == 25);
    w = fresh();
    CHECK(w.find("; LabelRemark     = 25\n") != std::string::npos);
    CHECK(w.find("; LabelCase       = -1\n") != std::string::npos);
}

TEST_CASE("residents: AmiBlitz3's joining in a new header", "[xtra][ab3]")
{
    struct {
        const char *in, *out;
    } cases[] = {
        {"", ""},
        {"a", "a"},
        {"a,b", "a,b"},
        {",b", ",b"},
        {"a,,b", "a,b"},            // empty entries vanish (except a first one)
        {"1,2,3,4,5,6,7,8", "1,2,3,4,5,6,7,8"},
    };
    for (auto &c : cases) {
        REQUIRE(read(with(std::string("; Residents = \"") + c.in + "\"\n")) == XTRA_OK);
        std::string w = fresh();
        std::string want = std::string("; Residents       = \"") + c.out + "\"\n";
        CHECK(w.find(want) != std::string::npos);
    }
}

TEST_CASE("to Blitz 2 and back: what both have", "[xtra][ab3]")
{
    static uint8_t bin[2000];
    static const struct xtra_max numbered[] = {{nullptr, 0, 12, 30, XTRA_MAX_LIB}};
    size_t len = 0;
    unsigned skipped = 0;

    REQUIRE(read(full) == XTRA_OK);
    REQUIRE(xtra_write_blitz2(&x, &base, bin, sizeof(bin), &len, &skipped, nullptr) == XTRA_OK);
    CHECK(skipped == 3);                    // the named maxima
    CHECK(len == 1110);
    xtra_set_maxima(&x, numbered, 1);
    REQUIRE(xtra_write_blitz2(&x, &base, bin, sizeof(bin), &len, &skipped, nullptr) == XTRA_OK);
    CHECK(skipped == 0);
    CHECK(len == 1110 + 4);
    struct xtra_base yb = {bin, nullptr};
    REQUIRE(xtra_load(&y, XTRA_T_TEXT, nullptr, 0, bin, len, nullptr) == XTRA_OK);
    CHECK(text(XTRA_F_EXE_PATH, &y, &yb) == "Development:AmiBlitz3/Tests");
    CHECK(text(XTRA_F_RES0 + 1, &y, &yb) == "all.res");
    CHECK(text(XTRA_F_CLI, &y, &yb) == "PATH=music:sounds");
    CHECK(y.buf[XTRA_BUF_STRING] == 20000);
    CHECK(y.sw[XTRA_SW_ASSEMBLER_CHECK] == XTRA_ON);
    CHECK(y.icon == XTRA_ON);
    auto m = maxima(&y, &yb);
    REQUIRE(m.size() == 1);
    CHECK(m[0].lib == 12);

    skipped = 0;
    std::string w = write(&y, &yb, &skipped);
    CHECK(skipped == 1);                    // the numbered maximum
    CHECK(w.find("; CliArgs         = \"PATH=music:sounds\"\n") != std::string::npos);
    CHECK(w.find("; StackSize       = 32768\n") != std::string::npos);  // AB3 only: default
    CHECK(w.find("; CursorLine      = 1\n") != std::string::npos);      // no editor part
    CHECK(w.find("; Max") == std::string::npos);
    REQUIRE(read(w) == XTRA_OK);
    CHECK(x.ab3.seen == ((1ul << XTRA_K_COUNT) - 1 - (1ul << XTRA_K_MAX)));
    CHECK(x.hdr == w.size());
}

TEST_CASE("writer: too small buffer, the size needed, missing base", "[xtra][ab3][broken]")
{
    char out[100];
    size_t len = 0;

    REQUIRE(read(full) == XTRA_OK);
    CHECK(xtra_write_ab3(&x, &base, out, sizeof(out), &len, nullptr) == XTRA_E_SPACE);
    CHECK(len == full.size());
    CHECK(xtra_write_ab3(&x, &base, nullptr, 0, &len, nullptr) == XTRA_E_SPACE);
    CHECK(len == full.size());
    CHECK(xtra_ab3_size(&x, &base) == full.size());
    // the kept header is not given
    CHECK(xtra_write_ab3(&x, nullptr, out, sizeof(out), &len, nullptr) == XTRA_E_BASE);
    struct xtra_base none = {nullptr, nullptr};
    CHECK(xtra_write_ab3(&x, &none, out, sizeof(out), &len, nullptr) == XTRA_E_BASE);
    CHECK(xtra_ab3_size(&x, &none) == 0);
    // a new header from views without their base
    x.ab3.header = 0;
    CHECK(xtra_write_ab3(&x, &none, out, sizeof(out), &len, nullptr) == XTRA_E_BASE);
    CHECK(maxima(&x, &none).empty());
}

TEST_CASE("broken: no end, line without ';'", "[xtra][ab3][broken]")
{
    refused("; XTRA\n; ExeFile = \"a\"\n", XTRA_E_AB3_END, 3);
    refused("; XTRA\n", XTRA_E_AB3_END, 2);
    refused("; XTRA", XTRA_E_AB3_END, 2);
    refused("; XTRA\n; /XTRACNIF 1\nEnd\n", XTRA_E_AB3_LINE, 3);
    refused(with("; ExeFile = \"a\"\nNPrint 1\n"), XTRA_E_AB3_LINE, 3);
}

TEST_CASE("broken: numbers", "[xtra][ab3][broken]")
{
    refused(with("; StackSize = abc\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; StackSize =\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; CreateIcon = --1\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; CreateIcon = -\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; LabelPosition = -2147483649\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; Max Screen = -1\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; CreateIcon = 1 2\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; StackSize = 4294967296\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; StackSize = $\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; StackSize = $123456789\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; StackSize = $12G\n"), XTRA_E_AB3_NUMBER, 2);
    refused(with("; \n; Max Screen = 65536\n"), XTRA_E_AB3_NUMBER, 3);
    refused(with("; Max Screen = x\n"), XTRA_E_AB3_NUMBER, 2);
    CHECK(read(with("; StackSize = 4294967295\n")) == XTRA_OK);
    CHECK(x.ab3.stack == 4294967295u);
}

TEST_CASE("broken: strings", "[xtra][ab3][broken]")
{
    refused(with("; ExeFile = \"a\n"), XTRA_E_AB3_QUOTE, 2);
    refused(with("; ExeFile = \"\n"), XTRA_E_AB3_QUOTE, 2);
    refused(with("; ExePath = \"" + std::string(192, 'p') + "\"\n"), XTRA_E_LONG, 2);
    CHECK(read(with("; ExePath = \"" + std::string(191, 'p') + "\"\n")) == XTRA_OK);
    refused(with("; CliArgs = \"" + std::string(128, 'c') + "\"\n"), XTRA_E_LONG, 2);
    CHECK(read(with("; CliArgs = \"" + std::string(127, 'c') + "\"\n")) == XTRA_OK);
    refused(with("; LabelSearch = \"" + std::string(32, 'l') + "\"\n"), XTRA_E_LONG, 2);
    refused(with("; Residents = \"" + std::string(64, 'r') + "\"\n"), XTRA_E_LONG, 2);
    refused(with("; Residents = \"a," + std::string(64, 'r') + "\"\n"), XTRA_E_LONG, 2);
}

TEST_CASE("broken: residents, version, maxima", "[xtra][ab3][broken]")
{
    refused(with("; Residents = \"1,2,3,4,5,6,7,8,9\"\n"), XTRA_E_AB3_RESIDENTS, 2);
    refused(with("; Residents = \"1,2,3,4,5,6,7,8,\"\n"), XTRA_E_AB3_RESIDENTS, 2);
    for (const char *v : {"1.2", "1.2.3.4", "1..3", "256.0.0", "a.b.c", "$1.2.3", "1.2.", ""})
        refused(with(std::string("; Version = ") + v + "\n"), XTRA_E_AB3_VERSION, 2);
    refused(with("; Max = 5\n"), XTRA_E_AB3_NAME, 2);
    refused(with("; Max   = 5\n"), XTRA_E_AB3_NAME, 2);
    refused(with("; Max " + std::string(65536, 'n') + " = 5\n"), XTRA_E_LONG, 2);
    CHECK(read(with("; Max " + std::string(200, 'n') + " = 5\n")) == XTRA_OK);
    CHECK(maxima()[0].name.size() == 200);
    CHECK(read(with("; Maximum = 5\n")) == XTRA_OK);    // another key, not "Max"
    CHECK(x.ab3.unknown == 1);
    CHECK(maxima().empty());
    // no upper limit for the maxima
    std::string many;
    for (unsigned i = 0; i < 1000; i++)
        many += "; Max O" + std::to_string(i) + " = 1\n";
    REQUIRE(read(with(many)) == XTRA_OK);
    auto m = maxima();
    REQUIRE(m.size() == 1000);
    CHECK(m[999].name == "O999");
}
