// test_xtra_bb22 - the editor part from a BB22 header (xtra_read_bb22 of
// shared/xtra) with headers built in memory: both versions, the merge
// with a Blitz 2 .xtra, and broken headers (each refused with its code
// and offset; the rules of tools/lib/bb22).
#include <cstdint>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "xtra.h"
}

namespace {

using bytes = std::vector<uint8_t>;

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

void refused(const bytes &b, int code, size_t offset)
{
    struct xtra_editor ed;
    struct xtra_error err;

    CHECK(xtra_read_bb22(&ed, b.data(), b.size(), &err) == code);
    CHECK(err.code == code);
    CHECK(err.offset == offset);
    CHECK(ed.flags == 0);
}

struct xtra x;

} // namespace

TEST_CASE("version 10: cursor line = top + row (applib3.bb)", "[xtra][bb22]")
{
    struct xtra_editor ed;
    bytes b = header(10, 13, 25, 441);

    b.insert(b.end(), {0, 0, 'A', 0});      // a line: the header is enough
    REQUIRE(xtra_read_bb22(&ed, b.data(), b.size(), nullptr) == XTRA_OK);
    CHECK(ed.flags == (XTRA_ED_CURSOR | XTRA_ED_TOP));
    CHECK(ed.line == 466);
    CHECK(ed.column == 13);
    CHECK(ed.top == 441);
}

TEST_CASE("version 8: 14 bytes (Anim-Library3.bb)", "[xtra][bb22]")
{
    struct xtra_editor ed;
    bytes b = header(8, 19, 1, 96);

    REQUIRE(b.size() == 14);
    REQUIRE(xtra_read_bb22(&ed, b.data(), b.size(), nullptr) == XTRA_OK);
    CHECK(ed.line == 97);
    CHECK(ed.column == 19);
    CHECK(ed.top == 96);
}

TEST_CASE("merge: the .xtra and the editor part of its BB22 source", "[xtra][bb22]")
{
    std::vector<uint8_t> f(1110, 0);
    bytes b = header(10, 0, 54, 571);

    f[0] = 1;
    REQUIRE(xtra_read_blitz2(&x, f.data(), f.size(), nullptr) == XTRA_OK);
    CHECK(x.editor.flags == 0);
    REQUIRE(xtra_read_bb22(&x.editor, b.data(), b.size(), nullptr) == XTRA_OK);
    CHECK(x.editor.line == 625);
    CHECK(x.source == XTRA_SRC_BLITZ2);
    // the editor part is not in the Blitz 2 file
    CHECK(xtra_blitz2_size(&x) == 1110);
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
}
