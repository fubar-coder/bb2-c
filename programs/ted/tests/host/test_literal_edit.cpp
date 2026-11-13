// test_literal_edit - literal characters in the editor (spec
// editor/literal-characters: "A literal character is the token pair $90
// cc", "Which characters become a literal character" (typing), "Display,
// help and colour", "Find, blocks and print work on the character"):
// typing through the keymap path and the line editor (cursor, delete,
// backspace, insert in the middle), the chmap row and its colour, help,
// print, search and replace, block copy/cut/save/load, join and split at
// the line limit.  With tokenise on and off (design D6: one stored form).
//
// getline (src/input.c) draws, so it does not run on the host; type()
// does its literal branch (litkey -> gl_islit, print2's chmap write,
// cursrite) as test_lineedit_keys does its other branches.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
#include "lines_helpers.hpp"
#include "tempdir.hpp"
extern "C" {
#include "hostenv.h"
#include "fileio.h"
#include "lineedit.h"
#include "lines.h"
#include "loadsave.h"
#include "services.h"
#include "keymap.h"
#include "literal.h"
}
#include "oracle.hpp"

namespace {

const std::string P = std::string("\x90", 1);

void env(UWORD tok)
{
    static bool done;

    if (!done) {
        hostenv_init(oracle::tokens_file().c_str(), 20);
        done = true;
    }
    REQUIRE(tokens != nullptr);
    tokenise = tok;
    tabstop = 8;
    hscmarg = 12;
    edcols = 80;
    bboty = static_cast<UWORD>(-1);
    qualifier = 0;
}

std::string z(const std::string &s)
{
    return s + std::string(1, '\0');
}

// one list of the given stored lines (bytes, without their 0)
void lines(const std::vector<std::string> &l)
{
    std::vector<const char *> p;

    for (auto &s : l)
        p.push_back(s.c_str());
    mklist(static_cast<int>(l.size()), p.data());
}

std::string bytes(int i)
{
    const struct item *it = firstitem;

    while (i-- > 0 && it)
        it = it->nextitem;
    REQUIRE(it != nullptr);
    return std::string(reinterpret_cast<const char *>(it->chars), it->numchars);
}

// the cursor on line y, column x, the row shown as showa2item shows it
// and its literal characters known (getline's entry: litrow_sync)
void show(UWORD y, UWORD x)
{
    curstop = 0;
    cursy = y;
    cursx = x;
    cursoff = 0;
    calcpos();
    struct item *it = calcitem();
    std::memset(chmap + chloc, ' ', chcols);
    chmap_showa2item(it, nullptr);
    litrow_sync();
}

// getline's branch for a literal character (input.c islit), or for a
// plain character / an umlaut (.isasc), without drawing
void type(UBYTE key)
{
    LONG event = static_cast<LONG>(germs_map(key));
    UBYTE cc = litkey(event);

    anyalt = 0;
    if (cc) {
        gl_islit(cc);
        chmap[chloc + cursoff + cursx] = TED_LITGLYPH;  // print2
    } else {
        event &= ~(1L << 30);
        gl_isasc(static_cast<UBYTE>(event));
        chmap[chloc + cursoff + cursx] = static_cast<UBYTE>(event);
    }
    cursrite();
}

// Del at the cursor (char127) and Backspace (cursleft, char127)
void del()
{
    gl_char127(127);
}

void backspace()
{
    cursleft();
    gl_char127(8);
}

// mainedit's commit of the edited row: charstobuff, makeline
std::string commit()
{
    UWORD len = charstobuff(chmap + chloc);

    thisitem = makeline(thisitem, len);
    return std::string(reinterpret_cast<const char *>(thisitem->chars), thisitem->numchars);
}

// the shown text of the cursor row (chmap without the token bit), with
// the literal characters as their bytes
std::string row()
{
    std::string r;

    for (UWORD i = 0; i < chcols; i++) {
        UBYTE l = litat(chmap + chloc + i);

        r += static_cast<char>(l ? l : (chmap[chloc + i] & 0x7F));
    }
    while (!r.empty() && r.back() == ' ')
        r.pop_back();
    return r;
}

std::vector<std::pair<UBYTE, UWORD>> drawn;     // character, colour
void draw(UBYTE c)
{
    drawn.emplace_back(c, color);
}

std::string readfile(const std::string &path)
{
    long len = 0;
    UBYTE *b = host_readfile(path.c_str(), &len);

    REQUIRE(b != nullptr);
    std::string s(reinterpret_cast<const char *>(b), static_cast<size_t>(len));
    std::free(b);
    return s;
}

} // namespace

TEST_CASE("typing a literal character, with tokenise on and off", "[literal][edit]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        lines({ "a$=\"\"" });
        show(0, 4);                             // between the quotes
        type(0xDF);                             // ß
        CHECK(anyalt != 0);                     // mainedit commits the line
        CHECK(cursx == 5);                      // one step
        CHECK(chmap[chloc + 4] == TED_LITGLYPH);
        CHECK(litat(chmap + chloc + 4) == 0xDF);
        CHECK(row() == "a$=\"\xdf\"");
        type(0xE4);                             // ä: the code 26 as before
        CHECK(row() == "a$=\"\xdf\x1a\"");
        type(0xE9);                             // é
        CHECK(commit() == z("a$=\"" + P + "\xdf\x1a" + P + "\xe9\""));
        // F1-F10 and Help ($81-$8B) are no characters
        CHECK(litkey(0x81) == 0);
        CHECK(litkey(0x8B) == 0);
        CHECK(litkey(static_cast<LONG>(germs_map(0xE4))) == 0);
        CHECK(litkey(0xA0) == 0xA0);
        CHECK(litkey(0xFF) == 0xFF);
        CHECK(litkey(-1) == 0);
    }
}

TEST_CASE("the line editor moves, deletes and inserts around a literal character", "[literal][edit]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        // a, é, b: b is in column 2
        lines({ std::string("a") + P + "\xe9" "b" });
        show(0, 0);
        CHECK(row() == "a\xe9" "b");
        CHECK(chmap[chloc + 2] == 'b');
        // insert in front of the literal character: it moves with the row
        show(0, 1);
        type('x');
        CHECK(row() == "ax\xe9" "b");
        CHECK(litat(chmap + chloc + 2) == 0xE9);
        CHECK(litat(chmap + chloc + 1) == 0);
        CHECK(commit() == z(std::string("ax") + P + "\xe9" "b"));
        // Del on it: gone, whole
        show(0, 2);
        del();
        CHECK(row() == "axb");
        CHECK(commit() == z("axb"));
        // Backspace behind a literal character
        lines({ std::string("a") + P + "\xe9" "b" });
        show(0, 2);
        backspace();
        CHECK(cursx == 1);
        CHECK(row() == "ab");
        CHECK(commit() == z("ab"));
        // a literal character typed in the middle of two others
        lines({ P + "\xe0" + P + "\xe8" });
        show(0, 1);
        type(0xEC);
        CHECK(row() == "\xe0\xec\xe8");
        CHECK(commit() == z(P + "\xe0" + P + "\xec" + P + "\xe8"));
        // TAB and untab move the characters too
        lines({ P + "\xe0" "x" + P + "\xe8" });
        show(0, 1);
        CHECK(gl_tab() == 8);
        CHECK(row() == "\xe0       x\xe8");
        CHECK(litat(chmap + chloc + 9) == 0xE8);
        cursx = 8;
        qualifier = 1;
        CHECK(gl_untab() == 0);
        qualifier = 0;
        CHECK(row() == "x\xe8");
        CHECK(commit() == z("x" + P + "\xe8"));
        // delete to the end of the line
        lines({ "a" + P + "\xe8" + P + "\xe9" });
        show(0, 1);
        gl_delright();
        CHECK(row() == "a");
        CHECK(commit() == z("a"));
    }
}

TEST_CASE("display: one cell, its glyph in the colour of the text, no help, print sends the byte", "[literal][display]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        lines({ "a$=\"" + P + "\xe9\"" });
        normcol = 1;
        tokcol = 3;
        color = normcol;
        curstop = 0;
        cursy = 0;
        cursx = 0;
        cursoff = 0;
        calcpos();
        std::memset(chmap + chloc, ' ', chcols);
        drawn.clear();
        chmap_showa2item(firstitem, draw);
        REQUIRE(drawn.size() == 6);
        CHECK(drawn[4].first == 0xE9);          // the glyph of the character
        CHECK(drawn[4].second == normcol);      // not the token colour
        CHECK(chmap[chloc + 4] == TED_LITGLYPH); // no token bit
        CHECK(litat(chmap + chloc + 4) == 0xE9); // the literal map
        CHECK(chmap[chloc + 5] == '"');
        // help on the literal character: nothing; the column behind it is 5
        UWORD num = 0;
        CHECK(help_core(firstitem, 4, &num) == nullptr);
        lines({ "NPrint " + P + "\xe9" });
        // print: the byte, no ESC sequence
        UWORD n = prtfile_line(firstitem, charbuff);
        std::string printed(reinterpret_cast<const char *>(charbuff), n);
        CHECK(printed.find(P) == std::string::npos);
        CHECK(printed.substr(printed.size() - 2) == "\xe9\n");
        lines({ "a$=\"" + P + "\xe9\"" });
        n = prtfile_line(firstitem, charbuff);
        CHECK(std::string(reinterpret_cast<const char *>(charbuff), n) == "a$=\"\xe9\"\n");
    }
}

TEST_CASE("help counts a literal character as one column", "[literal][display]")
{
    env(1);
    // a token behind two literal characters: help on column 2 finds it
    lines({ P + "\xe9" + P + "\xe8" + "NPrint" });
    show(0, 0);
    UWORD num = 0;
    std::string nprint;
    {
        // the token word of NPrint as the tokeniser writes it
        std::memset(charbuff2, ' ', chcols);
        std::memcpy(charbuff2, "NPrint", 6);
        litclear();
        UWORD n = charstobuff(charbuff2);
        nprint = std::string(reinterpret_cast<const char *>(charbuff), n - 1);
    }
    REQUIRE(nprint.size() == 2);
    lines({ P + "\xe9" + P + "\xe8" + nprint });
    const char *name = help_core(firstitem, 2, &num);
    REQUIRE(name != nullptr);
    CHECK(std::string(name) == "NPrint");
    CHECK(help_core(firstitem, 1, &num) == nullptr);
}

TEST_CASE("search and replace compare the character", "[literal][search]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        host_findcase = 0;
        // Größe: Gr, the code 27, $90 DF, e
        lines({ "a$=\"Gr\x1b" + P + "\xdf" "e\"" });
        std::strcpy(reinterpret_cast<char *>(findreqla16), "Gr\xf6\xdf" "e");
        CHECK(isithere(firstitem, nullptr) == 4);
        std::strcpy(reinterpret_cast<char *>(findreqla16), "\xdf" "e");
        CHECK(isithere(firstitem, nullptr) == 7);
        std::strcpy(reinterpret_cast<char *>(findreqla16), "\xe9");
        CHECK(isithere(firstitem, nullptr) == -1);
        // replace ß by é: the pair is replaced whole
        std::strcpy(reinterpret_cast<char *>(findreqla16), "\xdf");
        std::strcpy(reinterpret_cast<char *>(findreqla17), "\xe9");
        thisitem = firstitem;
        struct item *out = nullptr;
        CHECK(replace2(firstitem, &out));
        CHECK(bytes(0) == z("a$=\"Gr\x1b" + P + "\xe9" "e\""));
        // replace e by ä: the code 26
        std::strcpy(reinterpret_cast<char *>(findreqla16), "\xe9" "e");
        std::strcpy(reinterpret_cast<char *>(findreqla17), "\xe4\xdf");
        thisitem = firstitem;
        CHECK(replace2(firstitem, &out));
        CHECK(bytes(0) == z("a$=\"Gr\x1b\x1a" + P + "\xdf\""));
        findreqla16[0] = 0;
        findreqla17[0] = 0;
    }
}

TEST_CASE("blocks keep a literal character at the first and the last place", "[literal][blocks]")
{
    tempdir dir;

    REQUIRE(!dir.path.empty());
    const std::string path = dir.path + "/test_literal_edit.blk";
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        host_bname = reinterpret_cast<UBYTE *>(const_cast<char *>(path.c_str()));
        const std::string l0 = P + "\xe9" "abc" + P + "\xdf";     // é a b c ß

        // block copy of the whole first line's characters to line 2
        lines({ l0, "x", "y" });
        btopx = 0;
        btopy = 0;
        bbotx = 4;
        bboty = 0;
        show(2, 0);
        blokcopy();
        CHECK(bytes(2) == z(l0 + "y"));
        // the middle part, ending in the literal character at the end
        lines({ l0, "x", "y" });
        btopx = 1;
        btopy = 0;
        bbotx = 4;
        bboty = 0;
        show(1, 1);
        blokcopy();
        CHECK(bytes(1) == z("xabc" + P + "\xdf"));
        // a block over two lines
        lines({ l0, "q" + P + "\xe8" "r", "y" });
        btopx = 3;
        btopy = 0;
        bbotx = 1;
        bboty = 1;
        show(2, 0);
        blokcopy();
        CHECK(numlines == 4);
        CHECK(bytes(2) == z("c" + P + "\xdf"));
        CHECK(bytes(3) == z("q" + P + "\xe8" "y"));
        // block save: the characters as bytes, the umlauts as Latin-1
        // without tokenise (a text save, design D6), as stored with it
        lines({ l0, "\x1a" + P + "\xe8" });
        btopx = 0;
        btopy = 0;
        bbotx = static_cast<WORD>(chcols - 1);
        bboty = 1;
        show(0, 0);
        bloksave();
        CHECK(readfile(path) == std::string("\xe9" "abc\xdf\n")
                                + (tok ? "\x1a\xe8\n" : "\xe4\xe8\n"));
        // block load of the file: the pairs again (the umlaut: 26)
        lines({ "", "end" });
        bboty = static_cast<UWORD>(-1);
        show(0, 0);
        blokload();
        CHECK(numlines == 4);
        CHECK(bytes(0).find(P + "\xe9") == 0);
        CHECK(bytes(0).find(P + "\xdf") != std::string::npos);
        CHECK(bytes(1).find("\x1a" + P + "\xe8") == 0);
        // block cut (blokkill) of the middle of a line
        lines({ l0 });
        btopx = 1;
        btopy = 0;
        bbotx = 3;
        bboty = 0;
        show(0, 0);
        blokkill();
        CHECK(bytes(0) == z(P + "\xe9" + P + "\xdf"));
        std::remove(path.c_str());
    }
    host_bname = nullptr;
}

TEST_CASE("join and split keep the pair, and the line limit in bytes", "[literal][lines]")
{
    for (UWORD tok = 0; tok < 2; tok++) {
        CAPTURE(tok);
        env(tok);
        lines({ "a" + P + "\xe9", P + "\xdf" "b" });
        show(0, 0);
        join();
        CHECK(numlines == 1);
        CHECK(bytes(0) == z("a" + P + "\xe9" + P + "\xdf" "b"));
        // split in front of the second literal character
        show(0, 2);
        qualifier = 1;
        split();
        qualifier = 0;
        CHECK(numlines == 2);
        CHECK(bytes(0) == z("a" + P + "\xe9"));
        CHECK(bytes(1) == z(P + "\xdf" "b"));
        // 64 literal characters are a full line in bytes: a join with one
        // more is refused with the beep, nothing changes (as #1005 for a
        // line wider than the row)
        std::string p64;
        for (int i = 0; i < 64; i++)
            p64 += P + "\xdf";
        lines({ p64, P + "\xe9" });
        show(0, 0);
        host_resetcalls();
        join();
        CHECK(host_calls[HC_BEEPEM] == 1);
        CHECK(numlines == 2);
        CHECK(bytes(0) == z(p64));
        CHECK(bytes(1) == z(P + "\xe9"));
        // 63 and one: 128 bytes, joined
        lines({ p64.substr(2), P + "\xe9" });
        show(0, 0);
        join();
        CHECK(numlines == 1);
        CHECK(bytes(0) == z(p64.substr(2) + P + "\xe9"));
    }
}
