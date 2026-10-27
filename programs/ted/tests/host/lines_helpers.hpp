// lines_helpers.hpp - line list helpers of the TED core host tests:
// building a list, reading a line, checking the links, placing the cursor.
#ifndef TED_LINES_HELPERS_HPP
#define TED_LINES_HELPERS_HPP

#include <cstdio>
#include <cstring>
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "lines.h"
#include "tokit.h"
}

// a list of n lines "line0".."line{n-1}" (or the given texts)
inline void mklist(int n, const char *const *txt)
{
    struct item *last = nullptr, *it;
    char buf[64];

    hostenv_droplist();
    for (int i = 0; i < n; i++) {
        const char *s = txt ? txt[i] : buf;

        if (!txt)
            std::snprintf(buf, sizeof(buf), "line%d", i);
        size_t len = std::strlen(s) + 1;
        it = static_cast<struct item *>(ted_alloc(len + lenitem, TED_MEMF_PUBLIC));
        it->numchars = static_cast<UBYTE>(len);
        std::memcpy(it->chars, s, len);
        it->lastitem = last;
        it->nextitem = nullptr;
        if (last)
            last->nextitem = it;
        else
            firstitem = it;
        last = it;
    }
    enditem = last;
    numlines = static_cast<UWORD>(n);
}

inline const char *chars(const struct item *it)
{
    return reinterpret_cast<const char *>(it->chars);
}

inline const char *linetext(int i)
{
    struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    return it ? chars(it) : "<none>";
}

// the list is consistent: links both ways, numlines, enditem
inline bool listok()
{
    struct item *it = firstitem, *prev = nullptr;
    int n = 0;

    while (it) {
        if (it->lastitem != prev)
            return false;
        prev = it;
        it = it->nextitem;
        n++;
    }
    return n == numlines && prev == enditem;
}

// cursor at top line `top`, row y, column x; the chmap row = line text
inline void place(UWORD top, UWORD y, UWORD x)
{
    curstop = top;
    cursy = y;
    cursx = x;
    cursoff = 0;
    calcpos();
    struct item *it = calcitem();
    std::memset(chmap + chloc, ' ', chcols);
    std::memcpy(chmap + chloc, it->chars, std::strlen(chars(it)));
}

#endif
