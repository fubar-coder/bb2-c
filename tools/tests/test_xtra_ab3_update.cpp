// test_xtra_ab3_update - xtra_write_ab3 on a kept AmiBlitz3 header
// (shared/xtra): unchanged settings give the same bytes whatever the
// layout; a changed value rewrites only its line (the last one of a key
// given twice, the rest of the line kept); missing keys with a value other
// than AmiBlitz3's default are added in front of "; /XTRA"; the caller's
// texts, maxima and the settings of a Blitz 2 .xtra (xtra_take_blitz2).
#include <cstdint>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}
#include "xtra_ab3_full.h"

namespace {

struct xtra x, y;
std::string src;
struct xtra_base base;

void read(const std::string &s)
{
    src = s;
    base = {nullptr, reinterpret_cast<const uint8_t *>(src.data())};
    REQUIRE(xtra_load(&x, XTRA_T_AB3, base.header, src.size(), nullptr, 0, nullptr) == XTRA_OK);
    REQUIRE(x.ab3.header == 1);
}

std::string write(unsigned *skipped = nullptr)
{
    static char out[16384];
    size_t len = 0;

    REQUIRE(xtra_write_ab3(&x, &base, out, sizeof(out), &len, skipped) == XTRA_OK);
    REQUIRE(xtra_ab3_size(&x, &base) == len);
    return std::string(out, len);
}

// the header with line from replaced by to
std::string replaced(const std::string &h, const std::string &from, const std::string &to)
{
    std::string r = h;
    size_t at = r.find(from);

    REQUIRE(at != std::string::npos);
    r.replace(at, from.size(), to);
    return r;
}

// an older layout (AmiBlitz3's own sources: no LabelCase, no
// FuncOptimize/CreateDbgFile, CreateDbgFile 65535, other spacing)
const std::string older =
    "; XTRA\n"
    "; Embedded .xtra Header\n"
    "; \n"
    "; General Info\n"
    "; -------------------------------------------------------\n"
    "; ExePath         = \"Ram Disk:\"\n"
    "; ExeFile         = \"Test\"\n"
    "; CreateIcon      = 65535\n"
    "; Residents       = \"all.res\"\n"
    "; \n"
    "; Compiler\n"
    "; -------------------------------------------------------\n"
    "; StringBuffer    = 10240\n"
    "; MakeSmallest    = 0\n"
    "; Version         = 0.0.0\n"
    "; NumberOfBuilds  = 0\n"
    "; \n"
    "; Debugger\n"
    "; -------------------------------------------------------\n"
    "; CliArgs         = \"\"\n"
    "; StackSize       = 8191\n"
    "; RuntimeDebug    = -1\n"
    "; DebugInfo       = 0\n"
    "; CreateDbgFile   = 65535\n"
    "; OverflowCheck   = 0\n"
    "; AssemblerCheck  = 0\n"
    "; InterruptCheck  = 1\n"
    "; AutoRun         = 1\n"
    "; \n"
    "; Editor\n"
    "; -------------------------------------------------------\n"
    "; CursorLine      = 1\n"
    "; CursorColumn    = 1\n"
    "; LabelSearch     = \"\"\n"
    "; LabelRemark     = 0\n"
    "; LabelAll        = 0\n"
    "; LabelPosition   = 0\n"
    "; \n"
    "; Blitz Objects\n"
    "; -------------------------------------------------------\n"
    "; Max File        = 5\n"
    "; Max Screen      = 5\n"
    "; /XTRA\n";

} // namespace

TEST_CASE("unchanged: the same bytes, current and older layouts", "[xtra][ab3]")
{
    read(full + "End\n");
    CHECK(write() == full);
    read(older + "End\n");
    CHECK(write() == older);
    // a header with odd lines: everything kept
    std::string odd = ";XTRA\r\n;comment without key\r\n  ;  Foo=Bar  \r\n;StackSize=$100 \r\n\r\n"
                      "; Max  Some Thing =  3\r\n;/xtra";
    read(odd);
    CHECK(x.hdr == odd.size());
    CHECK(write() == odd);
}

TEST_CASE("a changed number rewrites its line, nothing else", "[xtra][ab3]")
{
    read(full);
    x.ab3.stack = 8000;
    CHECK(write() == replaced(full, "; StackSize       = 64000\n", "; StackSize       = 8000\n"));
    x.ab3.stack = 64000;
    x.buf[XTRA_BUF_STRING] = 0xFFFFFFFF;
    CHECK(write() == replaced(full, "= 20000\n", "= -1\n"));
    x.buf[XTRA_BUF_STRING] = 20000;
    x.ab3.version[1] = 10;
    CHECK(write() == replaced(full, "= 3.9.11\n", "= 3.10.11\n"));
    x.ab3.version[1] = 9;
    x.editor.line = 0;
    CHECK(write() == replaced(full, "; CursorLine      = 145\n", "; CursorLine      = 1\n"));
    x.editor.flags = 0;                     // no cursor: 1, 1
    CHECK(write() == replaced(replaced(full, "= 145\n", "= 1\n"), "; CursorColumn    = 12\n",
                              "; CursorColumn    = 1\n"));
}

TEST_CASE("booleans: on/off as the line says, changed as 0/1", "[xtra][ab3]")
{
    read(older);
    CHECK(x.icon == XTRA_ON);               // 65535
    x.icon = 0x7F;                          // still off for the header? no: bit 7 clear
    CHECK(write() == replaced(older, "; CreateIcon      = 65535\n", "; CreateIcon      = 0\n"));
    x.icon = 0xFF;                          // on, as 65535 says
    CHECK(write() == older);
    x.sw[XTRA_SW_RUNTIME_ERRORS] = 0x1234 | XTRA_ON;   // -1: on
    x.ab3.create_dbgfile = 1;               // 65535: on
    CHECK(write() == older);
    x.ab3.create_dbgfile = 0;
    CHECK(write() == replaced(older, "= 65535\n; OverflowCheck", "= 0\n; OverflowCheck"));
}

TEST_CASE("texts: the caller's bytes, quoted; residents joined", "[xtra][ab3]")
{
    read(full);
    REQUIRE(xtra_set_text(&x, XTRA_F_EXE_NAME, "Prog", 4) == XTRA_OK);
    CHECK(write() == replaced(full, "; ExeFile         = \"\"\n", "; ExeFile         = \"Prog\"\n"));
    REQUIRE(xtra_set_text(&x, XTRA_F_EXE_NAME, "", 0) == XTRA_OK);   // the same as the line
    CHECK(write() == full);
    REQUIRE(xtra_set_text(&x, XTRA_F_RES0 + 2, "blitzlibs:x.res", 15) == XTRA_OK);
    CHECK(write() == replaced(full, "\",all.res\"", "\",all.res,blitzlibs:x.res\""));
    REQUIRE(xtra_set_text(&x, XTRA_F_RES0 + 2, "", 0) == XTRA_OK);
    REQUIRE(xtra_set_text(&x, XTRA_F_LABEL_SEARCH, "a\"b", 3) == XTRA_OK);
    CHECK(write() == replaced(full, "\"errors\"", "\"a\"b\""));
}

TEST_CASE("a key given twice: the last line is the one rewritten", "[xtra][ab3]")
{
    std::string h = "; XTRA\n; StackSize = 1\n; Foo = 2\n; StackSize = 2\n; /XTRA\n";

    read(h);
    CHECK(x.ab3.stack == 2);
    x.ab3.stack = 1;
    CHECK(write() == "; XTRA\n; StackSize = 1\n; Foo = 2\n; StackSize = 1\n; /XTRA\n");
}

TEST_CASE("the rest of the line kept: spacing, CR LF, no blank after '='", "[xtra][ab3]")
{
    read(";XTRA\r\n;StackSize=100  \r\n;  ExePath\t=\t\"a\"\t\r\n; CliArgs =\r\n;/XTRA\r\nEnd\r\n");
    x.ab3.stack = 7;
    REQUIRE(xtra_set_text(&x, XTRA_F_EXE_PATH, "bb", 2) == XTRA_OK);
    REQUIRE(xtra_set_text(&x, XTRA_F_CLI, "c", 1) == XTRA_OK);
    // AmiBlitz3 drops the first character after '=': a blank put in
    CHECK(write() == ";XTRA\r\n;StackSize= 7  \r\n;  ExePath\t=\t\"bb\"\t\r\n; CliArgs = \"c\"\r\n;/XTRA\r\n");
}

TEST_CASE("missing keys: added in front of \"; /XTRA\" if not the default", "[xtra][ab3]")
{
    read(older);
    x.ab3.label_case = 0;                   // the default: nothing added
    x.ab3.func_optimize = 0;
    CHECK(write() == older);
    x.ab3.label_case = 1;
    x.ab3.func_optimize = 1;
    CHECK(write() == replaced(older, "; /XTRA\n",
                              "; FuncOptimize    = 1\n; LabelCase       = 1\n; /XTRA\n"));

    // the line end of the "; /XTRA" line
    read("; XTRA\r\n; /XTRA\r\n");
    x.ab3.stack = 100;
    REQUIRE(xtra_set_text(&x, XTRA_F_EXE_PATH, "RAM:", 4) == XTRA_OK);   // the default
    CHECK(write() == "; XTRA\r\n; StackSize       = 100\r\n; /XTRA\r\n");
    // without one at the end of the file
    read("; XTRA\n; /XTRA");
    x.icon = XTRA_ON;
    CHECK(write() == "; XTRA\n; CreateIcon      = 1\n; /XTRA");
}

TEST_CASE("maxima of the caller replace the \"Max\" lines", "[xtra][ab3]")
{
    static const struct xtra_max mine[] = {
        {reinterpret_cast<const uint8_t *>("Window"), 6, 0, 30, XTRA_MAX_NAME},
        {nullptr, 0, 12, 4, XTRA_MAX_LIB},
        {reinterpret_cast<const uint8_t *>("Req Lib"), 7, 7, 2, XTRA_MAX_NAME | XTRA_MAX_LIB},
    };
    unsigned skipped = 9;

    read(older);
    xtra_set_maxima(&x, mine, 3);
    CHECK(write(&skipped) == replaced(older, "; Max File        = 5\n; Max Screen      = 5\n",
                                      "; Max Window      = 30\n; Max Req-Lib     = 2\n"));
    CHECK(skipped == 1);
    xtra_set_maxima(&x, nullptr, 0);
    CHECK(write() == replaced(older, "; Max File        = 5\n; Max Screen      = 5\n", ""));
    // a header without maxima keeps none
    read("; XTRA\n; /XTRA\n");
    xtra_set_maxima(&x, mine, 1);
    CHECK(write() == "; XTRA\n; Max Window      = 30\n; /XTRA\n");
}

TEST_CASE("the settings of a .xtra into the kept header", "[xtra][ab3]")
{
    std::vector<uint8_t> f(1110 + 4, 0);
    const char *path = "Work:Tests";

    f[0] = 1;
    f[1] = 0;                               // no icon
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        f[2 + 2 * i + 1] = 0x80;
    f[30 + 4 * XTRA_BUF_STRING + 2] = 0x4E;  // 20000 = $4E20
    f[30 + 4 * XTRA_BUF_STRING + 3] = 0x20;
    std::string("all.res").copy(reinterpret_cast<char *>(&f[54 + 68]), 7);
    std::string(path).copy(reinterpret_cast<char *>(&f[598]), 10);
    std::string("PATH=music:sounds").copy(reinterpret_cast<char *>(&f[982]), 17);
    f[1111] = 5;                            // a pair: not taken
    REQUIRE(xtra_load(&y, XTRA_T_TED, nullptr, 0, f.data(), f.size(), nullptr) == XTRA_OK);

    read(full);
    xtra_take_blitz2(&x, &y);
    base.xtra = f.data();                   // the views of both
    std::string w = full;
    w = replaced(w, "\"Development:AmiBlitz3/Tests\"", "\"Work:Tests\"");
    w = replaced(w, "; CreateIcon      = 1\n", "; CreateIcon      = 0\n");
    w = replaced(w, "; RuntimeDebug    = 0\n", "; RuntimeDebug    = 1\n");
    w = replaced(w, "; OverflowCheck   = 0\n", "; OverflowCheck   = 1\n");
    w = replaced(w, "; InterruptCheck  = 0\n", "; InterruptCheck  = 1\n");
    w = replaced(w, "; AutoRun         = 0\n", "; AutoRun         = 1\n");
    CHECK(write() == w);
    CHECK(x.max.in == XTRA_IN_HEADER);      // the maxima stay
    CHECK(x.ab3.stack == 64000);            // AmiBlitz3's own fields stay
    base.xtra = nullptr;
    char out[4096];
    size_t len;
    CHECK(xtra_write_ab3(&x, &base, out, sizeof(out), &len, nullptr) == XTRA_E_BASE);
}
