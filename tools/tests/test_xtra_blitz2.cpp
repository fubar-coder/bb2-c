// test_xtra_blitz2 - the Blitz 2 .xtra of shared/xtra (xtra.h) with files
// built in memory: the defaults, both versions loaded field by field (the
// texts as views into the file), the byte-exact round trip (garbage after
// the 0 bytes and in the resident pointers included), the writer with
// texts and maxima of the caller, and broken files and structures (each
// refused with its code and offset).
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}

namespace {

using bytes = std::vector<uint8_t>;

struct xtra x;
struct xtra_base base;

void put16(bytes &b, size_t at, unsigned v)
{
    b[at] = uint8_t(v >> 8);
    b[at + 1] = uint8_t(v);
}

void put32(bytes &b, size_t at, uint32_t v)
{
    put16(b, at, v >> 16);
    put16(b, at + 2, v & 0xFFFF);
}

void putstr(bytes &b, size_t at, const char *s)
{
    std::memcpy(&b[at], s, std::strlen(s) + 1);
}

// a file: vers, the fixed part with recognisable values, n pairs
bytes file(unsigned vers, unsigned npairs)
{
    bytes b(size_t(vers ? XTRA_FIXED_V1 : XTRA_FIXED_V0) + 4 * npairs, 0);

    b[0] = uint8_t(vers);
    b[1] = 0x80;
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        put16(b, 2 + 2 * i, (i & 1) ? 0x80 : 0);
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        put32(b, 30 + 4 * i, 1000 + i);
    putstr(b, 54, "blitzlibs:amigalibs.res");
    put32(b, 54 + 64, 0x0012ABCD);          // garbage pointer
    putstr(b, 54 + 68, "blitzlibs:libmacs.res");
    putstr(b, 598, "ram:");
    putstr(b, 790, "ram:test");
    b[790 + 100] = 0x5A;                    // garbage behind the 0
    if (vers)
        putstr(b, 982, "setpa c:");
    for (unsigned i = 0; i < npairs; i++) {
        size_t at = b.size() - 4 * (npairs - i);
        put16(b, at, 4 + i * 7);
        put16(b, at + 2, 10 + i);
    }
    return b;
}

// loads b as the .xtra of a TED source (no source in memory); an empty
// file is a buffer of 0 bytes (NULL would be no .xtra)
int load(const bytes &b, struct xtra_error *err = nullptr)
{
    static const uint8_t empty[1] = {0};

    base = {b.empty() ? empty : b.data(), nullptr};
    return xtra_load(&x, XTRA_T_TED, nullptr, 0, base.xtra, b.size(), err);
}

std::string text(int f, const struct xtra_base *b = &base)
{
    size_t n = 99;
    const uint8_t *p = xtra_text(&x, b, f, &n);

    REQUIRE(p != nullptr);
    return std::string(reinterpret_cast<const char *>(p), n);
}

bytes write(const struct xtra_base *b = &base)
{
    bytes out(xtra_blitz2_size(&x));
    size_t len = 99;
    unsigned skipped = 99;

    REQUIRE(xtra_write_blitz2(&x, b, out.data(), out.size(), &len, &skipped, nullptr) == XTRA_OK);
    REQUIRE(len == out.size());
    return out;
}

std::vector<struct xtra_max> maxima(const struct xtra_base *b = &base)
{
    std::vector<struct xtra_max> v;
    struct xtra_iter it;
    struct xtra_max m;

    xtra_max_begin(&it, &x, b);
    while (xtra_next_max(&it, &m))
        v.push_back(m);
    return v;
}

// refused with code at offset in the .xtra
void refused(const bytes &b, int code, size_t offset)
{
    struct xtra_error err;

    CHECK(load(b, &err) == code);
    CHECK(err.code == code);
    CHECK(err.in == XTRA_IN_XTRA);
    CHECK(err.offset == offset);
    CHECK(x.source == XTRA_SRC_NONE);       // the defaults again
    CHECK(x.max.n == 0);
}

} // namespace

TEST_CASE("xtra_init: the defaults of blitz2's options requester", "[xtra]")
{
    xtra_init(&x);
    CHECK(x.source == XTRA_SRC_NONE);
    CHECK(x.vers == 1);
    CHECK(x.icon == XTRA_ON);
    const uint16_t sw[XTRA_NSWITCH] = {0x80, 0x80, 0x80, 0x80, 0, 0, 0, 0x80, 0x80, 0, 0, 0x80, 0x80, 0x80};
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        CHECK(x.sw[i] == sw[i]);
    const uint32_t buf[XTRA_NBUFFER] = {16384, 32768, 4096, 4096, 8192, 10240};
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        CHECK(x.buf[i] == buf[i]);
    CHECK(x.max.n == 0);
    CHECK(x.max.in == XTRA_IN_NONE);
    CHECK(x.editor.flags == 0);
    CHECK(x.ab3.stack == 32768);
    CHECK(x.ab3.seen == 0);
    CHECK(x.hdr == 0);
    for (int f = 0; f < XTRA_F_COUNT; f++)
        CHECK(text(f, nullptr) == "");
}

TEST_CASE("vers 1: every field, texts as views, the pairs in file order", "[xtra]")
{
    bytes b = file(1, 3);

    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.source == XTRA_SRC_BLITZ2);
    CHECK(x.type == XTRA_T_TED);
    CHECK(x.hdr == 0);
    CHECK(x.vers == 1);
    CHECK(x.icon == 0x80);
    for (unsigned i = 0; i < XTRA_NSWITCH; i++)
        CHECK(x.sw[i] == ((i & 1) ? 0x80 : 0));
    for (unsigned i = 0; i < XTRA_NBUFFER; i++)
        CHECK(x.buf[i] == 1000 + i);
    CHECK(text(XTRA_F_RES0) == "blitzlibs:amigalibs.res");
    CHECK(x.res_ptr[0] == 0x0012ABCD);
    CHECK(text(XTRA_F_RES0 + 1) == "blitzlibs:libmacs.res");
    CHECK(x.res_ptr[1] == 0);
    CHECK(text(XTRA_F_RES0 + 2) == "");
    CHECK(text(XTRA_F_EXE_PATH) == "ram:");
    CHECK(text(XTRA_F_EXE_NAME) == "ram:test");
    CHECK(text(XTRA_F_CLI) == "setpa c:");
    CHECK(text(XTRA_F_LABEL_SEARCH) == "");
    // views, no copies: the bytes are the file's
    size_t n;
    CHECK(xtra_text(&x, &base, XTRA_F_EXE_NAME, &n) == b.data() + 790);
    CHECK(x.text[XTRA_F_EXE_NAME].in == XTRA_IN_XTRA);
    CHECK(x.text[XTRA_F_EXE_NAME].off == 790);
    auto m = maxima();
    REQUIRE(m.size() == 3);
    for (unsigned i = 0; i < 3; i++) {
        CHECK(m[i].lib == 4 + i * 7);
        CHECK(m[i].max == 10 + i);
        CHECK(m[i].flags == XTRA_MAX_LIB);
        CHECK(m[i].name == nullptr);
    }
    CHECK(x.editor.flags == 0);
    CHECK(write() == b);
}

TEST_CASE("the views follow the base: the file may move", "[xtra]")
{
    bytes b = file(1, 2);

    REQUIRE(load(b) == XTRA_OK);
    bytes moved = b;
    b.assign(b.size(), 0xEE);               // the old buffer overwritten
    struct xtra_base mb = {moved.data(), nullptr};
    CHECK(text(XTRA_F_EXE_NAME, &mb) == "ram:test");
    CHECK(maxima(&mb).size() == 2);
    CHECK(write(&mb) == moved);
}

TEST_CASE("vers 0: no CLI block; xtra_apply_vers0 as loadxtra", "[xtra]")
{
    bytes b = file(0, 2);

    REQUIRE(b.size() == 982 + 8);
    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.vers == 0);
    CHECK(text(XTRA_F_CLI) == "");
    CHECK(x.text[XTRA_F_CLI].in == XTRA_IN_NONE);
    CHECK(x.max.n == 2);
    CHECK(maxima()[0].lib == 4);
    CHECK(write() == b);

    x.sw[XTRA_SW_AUTO_RUN] = 0;
    x.sw[XTRA_SW_ASSEMBLER_CHECK] = 0x80;
    x.sw[XTRA_SW_DEFAULT_INTERRUPT] = 0;
    x.sw[XTRA_SW_DEFAULT_OVERFLOW] = 0x80;
    xtra_apply_vers0(&x);
    CHECK(x.sw[XTRA_SW_AUTO_RUN] == 0x80);
    CHECK(x.sw[XTRA_SW_INTERRUPT_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_ASSEMBLER_CHECK] == 0);
    CHECK(x.sw[XTRA_SW_OVERFLOW_CHECK] == 0x80);

    // written as vers 1: an empty CLI block
    x.vers = 1;
    bytes w = write();
    CHECK(w.size() == 1110 + 8);
    CHECK(bytes(w.begin() + 982, w.begin() + 1110) == bytes(128, 0));
}

TEST_CASE("round trip: odd values stay as they are", "[xtra]")
{
    bytes b = file(1, 0);

    b[1] = 0x7F;                    // icon byte other than $00/$80
    put16(b, 2, 0x1234);            // switch word other than $00/$80
    put32(b, 30, 0xFFFFFFFF);
    for (unsigned i = 0; i < XTRA_NRES; i++)
        put32(b, 54 + 68 * i + 64, 0xDEAD0000 + i);
    b[982 + 120] = 0x11;            // behind the CLI argument's 0
    b[54 + 68 * 3 + 10] = 0x22;     // behind an empty resident's 0
    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.icon == 0x7F);
    CHECK(x.sw[0] == 0x1234);
    CHECK(x.res_ptr[7] == 0xDEAD0007);
    CHECK(write() == b);
}

TEST_CASE("many maxima: no upper limit", "[xtra]")
{
    bytes b = file(1, 5000);

    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.max.n == 5000);
    CHECK(maxima().size() == 5000);
    CHECK(write() == b);
}

TEST_CASE("writer: texts and maxima of the caller", "[xtra]")
{
    bytes b = file(1, 2);
    static const char path[] = "work:new";
    static const struct xtra_max mine[] = {
        {nullptr, 0, 5, 20, XTRA_MAX_LIB},
        {reinterpret_cast<const uint8_t *>("Screen"), 6, 0, 7, XTRA_MAX_NAME},
        {reinterpret_cast<const uint8_t *>("Window"), 6, 9, 8, XTRA_MAX_LIB | XTRA_MAX_NAME},
    };

    REQUIRE(load(b) == XTRA_OK);
    REQUIRE(xtra_set_text(&x, XTRA_F_EXE_NAME, path, 8) == XTRA_OK);
    REQUIRE(xtra_set_text(&x, XTRA_F_RES0 + 1, "", 0) == XTRA_OK);
    xtra_set_maxima(&x, mine, 3);
    CHECK(text(XTRA_F_EXE_NAME) == "work:new");
    CHECK(xtra_blitz2_size(&x) == 1110 + 8);

    bytes out(2000, 0xEE);
    size_t len = 0;
    unsigned skipped = 0;
    REQUIRE(xtra_write_blitz2(&x, &base, out.data(), out.size(), &len, &skipped, nullptr) == XTRA_OK);
    CHECK(len == 1118);
    CHECK(skipped == 1);
    // the set field: 0-terminated, padded with 0 (the garbage is gone)
    CHECK(std::string(reinterpret_cast<char *>(&out[790])) == "work:new");
    CHECK(bytes(out.begin() + 790 + 8, out.begin() + 790 + 192) == bytes(184, 0));
    CHECK(out[54 + 68] == 0);
    CHECK(bytes(out.begin(), out.begin() + 790) != bytes(b.begin(), b.begin() + 790));
    CHECK(bytes(out.begin() + 598, out.begin() + 790) == bytes(b.begin() + 598, b.begin() + 790));
    CHECK(out[1111] == 5);
    CHECK(out[1113] == 20);
    CHECK(out[1115] == 9);
    CHECK(out[1117] == 8);
    CHECK(out[1118] == 0xEE);       // nothing behind
    out.resize(len);
    REQUIRE(load(out) == XTRA_OK);
    CHECK(x.max.n == 2);
    CHECK(text(XTRA_F_EXE_NAME) == "work:new");

    // no maxima at all
    xtra_set_maxima(&x, nullptr, 0);
    CHECK(xtra_blitz2_size(&x) == 1110);
}

TEST_CASE("from scratch: the defaults as a file", "[xtra]")
{
    bytes out(1110);
    size_t len = 0;

    xtra_init(&x);
    REQUIRE(xtra_write_blitz2(&x, nullptr, out.data(), out.size(), &len, nullptr, nullptr) == XTRA_OK);
    CHECK(len == 1110);
    CHECK(out[0] == 1);
    CHECK(out[1] == 0x80);
    CHECK(out[2 + 2 * XTRA_SW_DEBUGGER + 1] == 0x80);
    CHECK(bytes(out.begin() + 54, out.end()) == bytes(1110 - 54, 0));
}

TEST_CASE("set_text: refused texts", "[xtra][broken]")
{
    xtra_init(&x);
    CHECK(xtra_set_text(&x, XTRA_F_COUNT, "a", 1) == XTRA_E_FIELD);
    CHECK(xtra_set_text(&x, -1, "a", 1) == XTRA_E_FIELD);
    std::string s(192, 'p');
    CHECK(xtra_set_text(&x, XTRA_F_EXE_PATH, s.data(), 192) == XTRA_E_LONG);
    CHECK(xtra_set_text(&x, XTRA_F_EXE_PATH, s.data(), 191) == XTRA_OK);
    CHECK(xtra_set_text(&x, XTRA_F_CLI, s.data(), 128) == XTRA_E_LONG);
    CHECK(xtra_set_text(&x, XTRA_F_RES0 + 7, s.data(), 64) == XTRA_E_LONG);
    CHECK(xtra_set_text(&x, XTRA_F_LABEL_SEARCH, s.data(), 32) == XTRA_E_LONG);
    CHECK(xtra_set_text(&x, XTRA_F_CLI, "a\nb", 3) == XTRA_E_TEXT);
    CHECK(xtra_set_text(&x, XTRA_F_CLI, "a\rb", 3) == XTRA_E_TEXT);
    CHECK(xtra_set_text(&x, XTRA_F_CLI, "a\0b", 3) == XTRA_E_TEXT);
    CHECK(text(XTRA_F_CLI) == "");
    CHECK(xtra_text_cap(XTRA_F_COUNT) == 0);
    size_t n = 99;
    CHECK(xtra_text(&x, nullptr, XTRA_F_COUNT, &n) == nullptr);
    CHECK(n == 0);
}

TEST_CASE("writer: broken structure, missing base, too small buffer", "[xtra][broken]")
{
    struct xtra_error err;
    uint8_t out[1200];
    size_t len = 7;

    xtra_init(&x);
    CHECK(xtra_write_blitz2(&x, nullptr, out, 1109, &len, nullptr, &err) == XTRA_E_SPACE);
    CHECK(err.code == XTRA_E_SPACE);
    CHECK(len == 0);
    x.vers = 2;
    CHECK(xtra_write_blitz2(&x, nullptr, out, sizeof(out), &len, nullptr, &err) == XTRA_E_VERS);
    x.vers = 0;
    CHECK(xtra_write_blitz2(&x, nullptr, out, 982, &len, nullptr, &err) == XTRA_OK);
    CHECK(len == 982);

    // views into a .xtra that is not given
    bytes b = file(1, 0);
    REQUIRE(load(b) == XTRA_OK);
    CHECK(xtra_write_blitz2(&x, nullptr, out, sizeof(out), &len, nullptr, &err) == XTRA_E_BASE);
    struct xtra_base none = {nullptr, nullptr};
    CHECK(xtra_write_blitz2(&x, &none, out, sizeof(out), &len, nullptr, &err) == XTRA_E_BASE);
    size_t n = 99;
    CHECK(xtra_text(&x, &none, XTRA_F_EXE_NAME, &n) == nullptr);
    CHECK(n == 0);
    b = file(1, 1);
    REQUIRE(load(b) == XTRA_OK);
    CHECK(xtra_write_blitz2(&x, &none, out, sizeof(out), &len, nullptr, &err) == XTRA_E_BASE);
    CHECK(maxima(&none).empty());

    // a view into the header of a source for a field too long for the .xtra
    std::string hdr(200, 'h');
    struct xtra_base hb = {nullptr, reinterpret_cast<const uint8_t *>(hdr.data())};
    xtra_init(&x);
    x.text[XTRA_F_EXE_PATH] = {nullptr, 0, 192, XTRA_IN_HEADER};
    CHECK(xtra_write_blitz2(&x, &hb, out, sizeof(out), &len, nullptr, &err) == XTRA_E_LONG);
    CHECK(err.offset == 598);
    CHECK(xtra_write_blitz2(&x, nullptr, out, sizeof(out), &len, nullptr, &err) == XTRA_E_BASE);
}

TEST_CASE("broken: short files", "[xtra][broken]")
{
    refused(bytes{}, XTRA_E_SHORT, 0);
    refused(bytes(981, 0), XTRA_E_SHORT, 981);
    bytes b = file(1, 0);
    b.resize(1109);
    refused(b, XTRA_E_SHORT, 1109);
    b.resize(982);                  // a complete vers 0 size, but vers 1
    refused(b, XTRA_E_SHORT, 982);
}

TEST_CASE("broken: size not fixed part + 4n", "[xtra][broken]")
{
    for (size_t extra = 1; extra < 4; extra++) {
        bytes b = file(1, 1);
        b.resize(b.size() + extra);
        refused(b, XTRA_E_SIZE, b.size());
        bytes c = file(0, 0);
        c.resize(c.size() + extra);
        refused(c, XTRA_E_SIZE, c.size());
    }
}

TEST_CASE("broken: vers other than 0 and 1", "[xtra][broken]")
{
    for (unsigned v : {2u, 0x80u, 0xFFu}) {
        bytes b = file(1, 0);
        b[0] = uint8_t(v);
        refused(b, XTRA_E_VERS, 0);
    }
}

TEST_CASE("broken: text fields without their 0 byte", "[xtra][broken]")
{
    for (unsigned i = 0; i < XTRA_NRES; i++) {
        bytes b = file(1, 0);
        std::memset(&b[54 + 68 * i], 'r', 64);
        refused(b, XTRA_E_UNTERMINATED, 54 + 68 * i);
    }
    bytes b = file(1, 0);
    std::memset(&b[598], 'p', 192);
    refused(b, XTRA_E_UNTERMINATED, 598);
    b = file(1, 0);
    std::memset(&b[790], 'n', 192);
    refused(b, XTRA_E_UNTERMINATED, 790);
    b = file(1, 0);
    std::memset(&b[982], 'c', 128);
    refused(b, XTRA_E_UNTERMINATED, 982);
    // vers 0: the same bytes are the first pairs, not a CLI argument
    b = file(0, 32);
    std::memset(&b[982], 'c', 128);
    CHECK(load(b) == XTRA_OK);
}

TEST_CASE("xtra_strerror: a message for every code", "[xtra]")
{
    for (int c = 0; c < XTRA_E_COUNT; c++)
        CHECK(std::string(xtra_strerror(c)) != "unknown error");
    CHECK(std::string(xtra_strerror(XTRA_E_COUNT)) == "unknown error");
}
