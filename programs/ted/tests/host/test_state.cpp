// test_state - the globals of the TED core have their original initial
// values and the fixed layouts fit (a smoke test of the host build).
#include <cstring>
#include <snitch/snitch.hpp>
extern "C" {
#include "hostenv.h"
#include "keymap.h"
}

TEST_CASE("initial values of the globals", "[state]")
{
    static const UWORD pal[12] = { 8, 8, 8, 15, 15, 15, 0, 0, 0, 15, 13, 0 };

    CHECK(sizeof(struct defblock) == 50u);
    CHECK(fonthite == 8);
    CHECK(fonthite2 == 7);
    CHECK(fontmul == 0);
    CHECK(tokenise == 0);
    CHECK(makeicons == 1);
    CHECK(normcol == 1);
    CHECK(labcol == 2);
    CHECK(tokcol == 3);
    CHECK(propcol == 0);
    CHECK(hscmarg == 12);
    CHECK(vscmarg == 6);
    CHECK(tabstop == 2);
    CHECK(std::memcmp(palit, pal, sizeof(pal)) == 0);
    CHECK(chcols == 128);
    CHECK(totsccols == 80);
    CHECK(color == 1);
    CHECK(colon == 255);
    CHECK(quote == 255);
    CHECK(label == 255);
    CHECK(remark == 255);
    CHECK(bstx == -1);
    CHECK(bboty == 0);
    CHECK(std::strcmp(defname, "l:blitzeditor.opts") == 0);
    CHECK(std::strcmp(tokeerr + 6, "Token Unavailable") == 0);
    CHECK(germs[0] == 228);
    CHECK(germs[11] == 31);
}

TEST_CASE("embedded resources", "[state]")
{
    CHECK(sizeof(font6) == 768u);
    CHECK(sizeof(font8) == 1024u);
    CHECK(srcinfo[0] == 0xE3);          // DiskObject magic
    CHECK(srcinfo[1] == 0x10);
}
