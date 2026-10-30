// test_xtra_ab3 - AmiBlitz3's embedded .xtra header (xtra_ab3.h of
// shared/xtra) with sources in memory: a header as AmiBlitz3 writes it,
// read field by field and written back byte for byte, the defaults, a
// source without header, AmiBlitz3's residents joining, conversion to the
// Blitz 2 .xtra and back, unknown keys, and broken headers (each refused
// with its code and line).
#include <cstdint>
#include <cstring>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra_ab3.h"
}

namespace {

struct xtra x, y;

// a header in the layout of AmiBlitz3's write_AB3Header
// (CLIModeSupport.ab3, commit b1a9dee150ef)
const std::string full =
    "; XTRA\n"
    "; Embedded .xtra Header\n"
    "; \n"
    "; General Info\n"
    "; -------------------------------------------------------\n"
    "; ExePath         = \"Development:AmiBlitz3/Tests\"\n"
    "; ExeFile         = \"\"\n"
    "; CreateIcon      = 1\n"
    "; Residents       = \",all.res\"\n"
    "; \n"
    "; Compiler\n"
    "; -------------------------------------------------------\n"
    "; StringBuffer    = 20000\n"
    "; MakeSmallest    = 1\n"
    "; FuncOptimize    = 1\n"
    "; Version         = 3.9.11\n"
    "; NumberOfBuilds  = 537\n"
    "; \n"
    "; Debugger\n"
    "; -------------------------------------------------------\n"
    "; CliArgs         = \"PATH=music:sounds\"\n"
    "; StackSize       = 64000\n"
    "; RuntimeDebug    = 0\n"
    "; DebugInfo       = 1\n"
    "; CreateDbgFile   = 1\n"
    "; OverflowCheck   = 0\n"
    "; AssemblerCheck  = 1\n"
    "; InterruptCheck  = 0\n"
    "; AutoRun         = 0\n"
    "; \n"
    "; Editor\n"
    "; -------------------------------------------------------\n"
    "; CursorLine      = 145\n"
    "; CursorColumn    = 12\n"
    "; LabelSearch     = \"errors\"\n"
    "; LabelRemark     = 1\n"
    "; LabelAll        = 1\n"
    "; LabelCase       = 1\n"
    "; LabelPosition   = 7\n"
    "; \n"
    "; Blitz Objects\n"
    "; -------------------------------------------------------\n"
    "; Max Screen      = 5\n"
    "; Max Req-Lib     = 5\n"
    "; Max ChunkyBuffer= 2\n"
    "; /XTRA\n";

int read(const std::string &s, size_t *end = nullptr, struct xtra_error *err = nullptr,
         struct xtra *to = &x)
{
    size_t e;

    return xtra_read_ab3(to, reinterpret_cast<const uint8_t *>(s.data()), s.size(), end ? end : &e,
                         err);
}

std::string write(const struct xtra *from = &x, unsigned *skipped = nullptr)
{
    static char out[16384];
    size_t len = 0;

    REQUIRE(xtra_write_ab3(from, out, sizeof(out), &len, skipped) == XTRA_OK);
    return std::string(out, len);
}

std::string str(const void *p)
{
    return std::string(static_cast<const char *>(p));
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
    size_t end = 99;

    CHECK(read(s, &end, &err) == code);
    CHECK(err.code == code);
    CHECK(err.line == line);
    CHECK(end == 0);
}

} // namespace

TEST_CASE("a full header: every field", "[xtra][ab3]")
{
    size_t end = 0;
    std::string src = full + "NPrint \"x\"\n";

    REQUIRE(read(src, &end) == XTRA_OK);
    CHECK(end == full.size());
    CHECK(x.source == XTRA_SRC_AB3_TEXT);
    CHECK(x.ab3.header == 1);
    CHECK(str(x.exe_path) == "Development:AmiBlitz3/Tests");
    CHECK(str(x.exe_name) == "");
    CHECK(x.icon == XTRA_ON);
    CHECK(x.res[0].name[0] == 0);
    CHECK(str(x.res[1].name) == "all.res");
    CHECK(x.res[2].name[0] == 0);
    CHECK(x.buf[XTRA_BUF_STRING] == 20000);
    CHECK(x.buf[XTRA_BUF_OBJECT] == 16384);     // not in the header: Blitz 2's
    CHECK(x.sw[XTRA_SW_MAKE_SMALLEST] == XTRA_ON);
    CHECK(x.ab3.func_optimize == 1);
    CHECK(x.ab3.version[0] == 3);
    CHECK(x.ab3.version[1] == 9);
    CHECK(x.ab3.version[2] == 11);
    CHECK(x.ab3.builds == 537);
    CHECK(str(x.cli) == "PATH=music:sounds");
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
    CHECK(str(x.ab3.label_search) == "errors");
    CHECK(x.ab3.label_remark == 1);
    CHECK(x.ab3.label_all == 1);
    CHECK(x.ab3.label_case == 1);
    CHECK(x.ab3.label_position == 7);
    REQUIRE(x.nmax == 3);
    CHECK(str(x.max[0].name) == "Screen");
    CHECK(x.max[0].max == 5);
    CHECK(x.max[0].flags == XTRA_MAX_NAME);
    CHECK(str(x.max[1].name) == "Req Lib");
    CHECK(str(x.max[2].name) == "ChunkyBuffer");
    CHECK(x.max[2].max == 2);
    CHECK(x.ab3.seen == ((1ul << XTRA_K_COUNT) - 1));
    CHECK(x.ab3.unknown == 0);
}

TEST_CASE("written back: the same bytes", "[xtra][ab3]")
{
    REQUIRE(read(full) == XTRA_OK);
    CHECK(write() == full);
}

TEST_CASE("no header: AmiBlitz3's defaults, end 0", "[xtra][ab3]")
{
    size_t end = 99;

    REQUIRE(read("NPrint \"hello\"\n; XTRA\n", &end) == XTRA_OK);
    CHECK(end == 0);
    CHECK(x.ab3.header == 0);
    CHECK(x.ab3.seen == 0);
    CHECK(str(x.exe_path) == "RAM:");
    CHECK(str(x.exe_name) == "Prog.exe");
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
    REQUIRE(read("", &end) == XTRA_OK);
    CHECK(end == 0);
}

TEST_CASE("a short header: only what it has; case, blanks, CR LF", "[xtra][ab3]")
{
    size_t end = 0;
    std::string s = ";xtra\r\n;   version=1.2.3  \r\n\n\t; CURSORLINE =\t0\r\n;stacksize= $1F40\r\n; /xtra \r\nEnd\n";

    REQUIRE(read(s, &end) == XTRA_OK);
    CHECK(s.substr(end) == "End\n");
    CHECK(x.ab3.seen == ((1ul << XTRA_K_VERSION) | (1ul << XTRA_K_CURSORLINE) | (1ul << XTRA_K_STACKSIZE)));
    CHECK(x.ab3.version[0] == 1);
    CHECK(x.ab3.version[2] == 3);
    CHECK(x.editor.line == 0);              // 0: as 1
    CHECK(x.ab3.stack == 8000);
    CHECK(str(x.exe_name) == "Prog.exe");   // default
}

TEST_CASE("unknown keys and other lines are ignored, unknown keys counted", "[xtra][ab3]")
{
    REQUIRE(read(with("; Embedded .xtra Header\n; some text\n; Colour = 3\n; Foo = \"bar\n"
                      "; ExeFile = \"a\"\n")) == XTRA_OK);
    CHECK(x.ab3.unknown == 2);
    CHECK(x.ab3.unknown_line == 4);
    CHECK(str(x.exe_name) == "a");
}

TEST_CASE("values: bare text, '=' in text, last key wins", "[xtra][ab3]")
{
    REQUIRE(read(with("; ExePath = ram:t\n; CliArgs = \"a=b\"\n; StackSize = 1\n; StackSize = 2\n"
                      "; Max Screen = 1\n; Max Screen = 9\n; Residents = \"a\"\n; Residents = \"b,c\"\n")) ==
            XTRA_OK);
    CHECK(str(x.exe_path) == "ram:t");
    CHECK(str(x.cli) == "a=b");
    CHECK(x.ab3.stack == 2);
    REQUIRE(x.nmax == 1);
    CHECK(x.max[0].max == 9);
    CHECK(str(x.res[0].name) == "b");
    CHECK(str(x.res[1].name) == "c");
}

TEST_CASE("negative values (Blitz's True), the label words, AmiBlitz3's Str$", "[xtra][ab3]")
{
    REQUIRE(read(with("; LabelAll = -1\n; AssemblerCheck = -1\n; LabelPosition = -2147483648\n"
                      "; CursorLine = -5\n; StackSize = -$10\n")) == XTRA_OK);
    CHECK(x.ab3.label_all == 0xFFFF);       // a word as written
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == XTRA_ON);
    CHECK(x.ab3.label_position == 0x80000000u);
    CHECK(x.editor.line == 0);
    CHECK(x.ab3.stack == 0xFFFFFFF0u);
    std::string w = write();
    CHECK(w.find("; LabelPosition   = -2147483648\n") != std::string::npos);
    CHECK(w.find("; StackSize       = -16\n") != std::string::npos);
    CHECK(w.find("; LabelAll        = -1\n") != std::string::npos);
    CHECK(w.find("; AssemblerCheck  = 1\n") != std::string::npos);
    REQUIRE(read(with("; LabelRemark = 25\n; LabelCase = 65535\n")) == XTRA_OK);
    CHECK(x.ab3.label_remark == 25);
    w = write();
    CHECK(w.find("; LabelRemark     = 25\n") != std::string::npos);
    CHECK(w.find("; LabelCase       = -1\n") != std::string::npos);
}

TEST_CASE("residents: AmiBlitz3's joining when writing", "[xtra][ab3]")
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
        std::string w = write();
        std::string want = std::string("; Residents       = \"") + c.out + "\"\n";
        CHECK(w.find(want) != std::string::npos);
    }
}

TEST_CASE("to Blitz 2 and back: what both have", "[xtra][ab3]")
{
    static uint8_t bin[XTRA_BLITZ2_MAXSIZE];
    size_t len = 0;
    unsigned skipped = 0;

    REQUIRE(read(full) == XTRA_OK);
    REQUIRE(xtra_add_max(&x, 12, nullptr, 30) == XTRA_OK);
    REQUIRE(xtra_write_blitz2(&x, bin, sizeof(bin), &len, &skipped, nullptr) == XTRA_OK);
    CHECK(skipped == 3);                    // the named maxima
    CHECK(len == 1110 + 4);
    REQUIRE(xtra_read_blitz2(&y, bin, len, nullptr) == XTRA_OK);
    CHECK(str(y.exe_path) == "Development:AmiBlitz3/Tests");
    CHECK(str(y.res[1].name) == "all.res");
    CHECK(str(y.cli) == "PATH=music:sounds");
    CHECK(y.buf[XTRA_BUF_STRING] == 20000);
    CHECK(y.sw[XTRA_SW_ASSEMBLER_CHECK] == XTRA_ON);
    CHECK(y.icon == XTRA_ON);
    REQUIRE(y.nmax == 1);
    CHECK(y.max[0].lib == 12);

    skipped = 0;
    std::string w = write(&y, &skipped);
    CHECK(skipped == 1);                    // the numbered maximum
    CHECK(w.find("; CliArgs         = \"PATH=music:sounds\"\n") != std::string::npos);
    CHECK(w.find("; StackSize       = 32768\n") != std::string::npos);  // AB3 only: default
    CHECK(w.find("; CursorLine      = 1\n") != std::string::npos);      // no editor part
    CHECK(w.find("; Max") == std::string::npos);
    REQUIRE(read(w, nullptr, nullptr, &x) == XTRA_OK);
    CHECK(x.ab3.seen == ((1ul << XTRA_K_COUNT) - 1 - (1ul << XTRA_K_MAX)));
}

TEST_CASE("writer: too small buffer, the size needed", "[xtra][ab3][broken]")
{
    char out[100];
    size_t len = 0;

    REQUIRE(read(full) == XTRA_OK);
    CHECK(xtra_write_ab3(&x, out, sizeof(out), &len, nullptr) == XTRA_E_SPACE);
    CHECK(len == full.size());
    CHECK(xtra_write_ab3(&x, nullptr, 0, &len, nullptr) == XTRA_E_SPACE);
    CHECK(len == full.size());
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
    refused(with("; ExePath = \"" + std::string(192, 'p') + "\"\n"), XTRA_E_AB3_LONG, 2);
    CHECK(read(with("; ExePath = \"" + std::string(191, 'p') + "\"\n")) == XTRA_OK);
    refused(with("; CliArgs = \"" + std::string(128, 'c') + "\"\n"), XTRA_E_AB3_LONG, 2);
    CHECK(read(with("; CliArgs = \"" + std::string(127, 'c') + "\"\n")) == XTRA_OK);
    refused(with("; LabelSearch = \"" + std::string(32, 'l') + "\"\n"), XTRA_E_AB3_LONG, 2);
    refused(with("; Residents = \"" + std::string(64, 'r') + "\"\n"), XTRA_E_AB3_LONG, 2);
}

TEST_CASE("broken: residents, version, maxima", "[xtra][ab3][broken]")
{
    refused(with("; Residents = \"1,2,3,4,5,6,7,8,9\"\n"), XTRA_E_AB3_RESIDENTS, 2);
    refused(with("; Residents = \"1,2,3,4,5,6,7,8,\"\n"), XTRA_E_AB3_RESIDENTS, 2);
    for (const char *v : {"1.2", "1.2.3.4", "1..3", "256.0.0", "a.b.c", "$1.2.3", "1.2.", ""})
        refused(with(std::string("; Version = ") + v + "\n"), XTRA_E_AB3_VERSION, 2);
    refused(with("; Max = 5\n"), XTRA_E_AB3_NAME, 2);
    refused(with("; Max   = 5\n"), XTRA_E_AB3_NAME, 2);
    refused(with("; Max " + std::string(32, 'n') + " = 5\n"), XTRA_E_AB3_LONG, 2);
    CHECK(read(with("; Maximum = 5\n")) == XTRA_OK);    // another key, not "Max"
    CHECK(x.ab3.unknown == 1);
    std::string many;
    for (unsigned i = 0; i <= XTRA_MAXMAXIMA; i++)
        many += "; Max O" + std::to_string(i) + " = 1\n";
    refused(with(many), XTRA_E_TOO_MANY, 2 + XTRA_MAXMAXIMA);
}
