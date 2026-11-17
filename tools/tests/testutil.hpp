// testutil.hpp - byte buffer and load file builder for the snitch tests of
// the host tools (the C++ form of the helpers of testutil.h).
#ifndef TESTUTIL_HPP
#define TESTUTIL_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
extern "C" {
#include "hunk.h"
}

// growing byte buffer
using buf = std::vector<uint8_t>;

inline void put8(buf &b, uint8_t v)
{
    b.push_back(v);
}

inline void put16(buf &b, uint16_t v)
{
    put8(b, uint8_t(v >> 8));
    put8(b, uint8_t(v & 0xFF));
}

inline void put32(buf &b, uint32_t v)
{
    put16(b, uint16_t(v >> 16));
    put16(b, uint16_t(v & 0xFFFF));
}

inline void putbytes(buf &b, const void *p, size_t n)
{
    for (size_t i = 0; i < n; i++)
        put8(b, static_cast<const uint8_t *>(p)[i]);
}

// a load file: table_size in the header, one hunk of type `type` with the
// (long padded) contents and RELOC32 longs at `relocs` pointing into hunk
// `target` (nrelocs may be 0: no reloc hunk), then HUNK_END
inline buf loadfile1(uint32_t table_size, uint32_t type, const buf &contents,
                     const uint32_t *relocs, uint32_t nrelocs, uint32_t target)
{
    buf b;
    uint32_t longs = uint32_t((contents.size() + 3) / 4);

    put32(b, HUNK_HEADER);
    put32(b, 0);                        // no resident names
    put32(b, table_size);
    put32(b, 0);
    put32(b, table_size - 1);
    for (uint32_t i = 0; i < table_size; i++)
        put32(b, longs);
    put32(b, type);
    put32(b, longs);
    putbytes(b, contents.data(), contents.size());
    while (b.size() % 4)
        put8(b, 0);
    if (nrelocs) {
        put32(b, HUNK_RELOC32);
        put32(b, nrelocs);
        put32(b, target);
        for (uint32_t i = 0; i < nrelocs; i++)
            put32(b, relocs[i]);
        put32(b, 0);
    }
    put32(b, HUNK_END);
    return b;
}

#endif
