// test_xtra_bb22 - a BB22 source loaded (xtra_load of shared/xtra) with
// headers built in memory: both versions, the header size to keep, the
// companion .xtra, and broken headers (each refused with its code and
// offset; the rules of tools/lib/bb22).
#include <cstdint>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}

namespace {

using bytes = std::vector<uint8_t>;

struct xtra x;

void put16(bytes &b, unsigned v)
{
    b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v));
}

// "BB22", version, column, row, w10, top (and w14 for version 10)
bytes header(unsigned version, unsigned column, unsigned row, unsigned top, unsigned w10 = 0,
             unsigned w14 = 2)
{
    bytes b{'B', 'B', '2', '2'};

    put16(b, version);
    put16(b, column);
    put16(b, row);
    put16(b, w10);
    put16(b, top);
    if (version == 10)
        put16(b, w14);
    return b;
}

int load(const bytes &b, struct xtra_error *err = nullptr, const bytes *xtra = nullptr)
{
    return xtra_load(&x, XTRA_T_BB22, b.data(), b.size(), xtra ? xtra->data() : nullptr,
                     xtra ? xtra->size() : 0, err);
}

void refused(const bytes &b, int code, size_t offset)
{
    struct xtra_error err;

    CHECK(load(b, &err) == code);
    CHECK(err.code == code);
    CHECK(err.in == XTRA_IN_HEADER);
    CHECK(err.offset == offset);
    CHECK(x.editor.flags == 0);
    CHECK(x.hdr == 0);
}

} // namespace

TEST_CASE("version 10: cursor line = top + row (applib3.bb), 16 bytes kept", "[xtra][bb22]")
{
    bytes b = header(10, 13, 25, 441);

    b.insert(b.end(), {0, 0, 'A', 0});      // a line: the header is enough
    CHECK(xtra_detect(b.data(), b.size(), nullptr) == XTRA_T_BB22);
    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.type == XTRA_T_BB22);
    CHECK(x.hdr == 16);
    CHECK(x.source == XTRA_SRC_NONE);       // no .xtra: the defaults
    CHECK(x.editor.flags == (XTRA_ED_CURSOR | XTRA_ED_TOP));
    CHECK(x.editor.line == 466);
    CHECK(x.editor.column == 13);
    CHECK(x.editor.top == 441);
}

TEST_CASE("version 8: 14 bytes (Anim-Library3.bb)", "[xtra][bb22]")
{
    bytes b = header(8, 19, 1, 96);

    REQUIRE(b.size() == 14);
    REQUIRE(load(b) == XTRA_OK);
    CHECK(x.hdr == 14);
    CHECK(x.editor.line == 97);
    CHECK(x.editor.column == 19);
    CHECK(x.editor.top == 96);
}

TEST_CASE("with its .xtra: the settings and the editor part", "[xtra][bb22]")
{
    bytes f(1110, 0);
    bytes b = header(10, 0, 54, 571);

    f[0] = 1;
    f[1] = 0x80;
    REQUIRE(load(b, nullptr, &f) == XTRA_OK);
    CHECK(x.editor.line == 625);
    CHECK(x.source == XTRA_SRC_BLITZ2);
    CHECK(x.icon == 0x80);
    CHECK(x.hdr == 16);
    // the editor part is not in the Blitz 2 file
    CHECK(xtra_blitz2_size(&x) == 1110);
    bytes out(1110);
    struct xtra_base base = {f.data(), b.data()};
    REQUIRE(xtra_write_blitz2(&x, &base, out.data(), out.size(), nullptr, nullptr, nullptr) == XTRA_OK);
    CHECK(out == f);
}

TEST_CASE("broken .xtra of a BB22 source: the error is in the .xtra", "[xtra][bb22][broken]")
{
    struct xtra_error err;
    bytes f(1111, 0);

    f[0] = 1;
    CHECK(load(header(8, 1, 2, 3), &err, &f) == XTRA_E_SIZE);
    CHECK(err.in == XTRA_IN_XTRA);
    CHECK(err.offset == 1111);
    CHECK(x.hdr == 0);
    CHECK(x.editor.flags == 0);
}

TEST_CASE("broken BB22 headers", "[xtra][bb22][broken]")
{
    refused(bytes{}, XTRA_E_SHORT, 0);
    refused(bytes{'B', 'B', '2', '2', 0}, XTRA_E_SHORT, 5);
    refused(bytes{'B', 'B', '2', '1', 0, 10}, XTRA_E_BB22, 0);
    bytes b = header(10, 1, 2, 3);
    b[0] = 'b';
    refused(b, XTRA_E_BB22, 0);
    refused(header(9, 1, 2, 3), XTRA_E_BB22, 4);
    refused(header(12, 1, 2, 3), XTRA_E_BB22, 4);
    b = header(10, 1, 2, 3);
    b.resize(15);
    refused(b, XTRA_E_SHORT, 15);
    b = header(8, 1, 2, 3);
    b.resize(13);
    refused(b, XTRA_E_SHORT, 13);
    refused(header(10, 1, 2, 3, 1), XTRA_E_BB22, 10);
    refused(header(8, 1, 2, 3, 0x100), XTRA_E_BB22, 10);
    refused(header(10, 1, 2, 3, 0, 0), XTRA_E_BB22, 14);
    refused(header(10, 1, 2, 3, 0, 3), XTRA_E_BB22, 14);
    struct xtra_error err;
    CHECK(xtra_load(&x, XTRA_T_BB22, nullptr, 0, nullptr, 0, &err) == XTRA_E_SHORT);
}
