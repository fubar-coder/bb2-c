// test_lineedit_keys - the line editor getline (chunk 06, ted.s
// 5416-5740) on the host: key sequences -> chmap row, cursor, block,
// anyalt.  Plus the rules of horizpos/cursleft/cursrite and of the block
// edges (inbloks/inblokf/blokfix/blokfixb), hand cases from survey-06 A/I.
//
// getline itself (src/input.c) draws, so it does not run on the host.
// feed() below repeats its dispatch (getline3 ... glinedone) with the
// drawing left out and print2's chmap write done by hand; everything else
// is the real core (gl_*, horizpos, cursleft, cursrite, blokfix ...).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <snitch/snitch.hpp>
#include "tempdir.hpp"
extern "C" {
#include "hostenv.h"
#include "lineedit.h"
#include "fileio.h"
}

namespace {

constexpr UWORD SHIFT = 1;              // qualifier & 3
constexpr UWORD NOSHIFT = 0;
constexpr UWORD NOBLOCK = static_cast<UWORD>(-1);

void env()
{
    static bool done;

    if (!done) {
        hostenv_init(nullptr, 10);
        done = true;
    }
    tabstop = 2;
    hscmarg = 12;
    edcols = 80;
}

// getline's dispatch for one event (d7; id = d6), without drawing.
// Returns event if getline would leave (glinedone), else 0.
LONG feed(LONG event, UWORD q, UWORD id = 0)
{
    WORD col;

    qualifier = q;
    if (event < 0)
        goto charhi;
    if (event & (1L << 30)) {
        event &= ~(1L << 30);
        goto isasc;
    }
    if (static_cast<UWORD>(event) < 32)
        goto charlow;
    if (static_cast<UWORD>(event) >= 127)
        goto charhi;
isasc:
    gl_isasc(static_cast<UBYTE>(event));
    // print event -> print2: chmap[chloc + cursoff + cursx] (+$80 if color
    // != normcol)
    chmap[chloc + cursoff + cursx] = static_cast<UBYTE>(event) | (color != normcol ? 0x80 : 0);
    cursrite();
    return 0;
charlow:
    if (event == 31) {
        if ((qualifier & 3) == 0) {
            cursleft();
            return 0;
        }
        if (static_cast<UWORD>(cursx + cursoff) != 0) {
            horizpos(0);
            return 0;
        }
        col = gl_tababove();
        if (col >= 0)
            horizpos(col);
        return 0;
    }
    if (event == 30) {
        if ((qualifier & 3) == 0)
            cursrite();
        else
            horizpos(gl_lastcol());
        return 0;
    }
    if (event == 8) {
        if ((cursx | cursoff) == 0)
            return 0;
        cursleft();
        event = 8;
        goto char127;
    }
    if (event == 9) {
        col = (qualifier & 3) ? gl_untab() : gl_tab();
        if (col >= 0)
            horizpos(col);              // reline -> curspos
        return 0;
    }
    return event;                       // charnot9 -> glinedone
charhi:
    if (static_cast<UWORD>(event) != 127)
        goto charnot127;
char127:
    gl_char127(static_cast<UBYTE>(event));
    return 0;
charnot127:
    if (static_cast<UWORD>(event) == 0xFFFF && id == 1)
        return 0;                       // hprop: needs prop2sinfo (UI)
    if (static_cast<UWORD>(event) == 0xFFFE && id == 0xf8e1) {
        horizpos(gl_delright());
        return 0;
    }
    return event;                       // glinedone
}

void typestr(const char *s)
{
    while (*s)
        feed(static_cast<UBYTE>(*s++), NOSHIFT);
}

// row 0 = text, rest spaces; cursor at col (no scrolling)
void setrow(const char *text, UWORD col)
{
    std::memset(chmap, ' ', static_cast<size_t>(edrows) * chcols);
    std::memcpy(chmap, text, std::strlen(text));
    cursy = 0;
    curstop = 0;
    chloc = 0;
    scloc = 0;
    cursoff = 0;
    cursx = col;
    anyalt = 0;
    bboty = NOBLOCK;
    color = normcol;
    host_resetcalls();
}

// chmap row 0 equals text followed by spaces up to chcols
bool rowis(const char *text)
{
    size_t n = std::strlen(text);

    if (std::memcmp(chmap, text, n) != 0)
        return false;
    for (size_t i = n; i < chcols; i++)
        if (chmap[i] != ' ')
            return false;
    return true;
}

void block(UWORD ty, WORD tx, UWORD by, WORD bx)
{
    btopy = ty;
    btopx = tx;
    bboty = by;
    bbotx = bx;
}

} // namespace

TEST_CASE("typing", "[lineedit]")
{
    env();
    setrow("", 0);
    typestr("abc");
    CHECK(rowis("abc"));
    CHECK(cursx == 3);
    CHECK(cursoff == 0);
    CHECK(anyalt == 'c');

    // insert in the middle
    setrow("acd", 1);
    feed('b', NOSHIFT);
    CHECK(rowis("abcd"));
    CHECK(cursx == 2);

    // the byte at chcols-1 is lost, even if it is no space (I.2)
    setrow("ab", 0);
    chmap[chcols - 1] = 'Z';
    feed('x', NOSHIFT);
    CHECK(chmap[0] == 'x');
    CHECK(chmap[1] == 'a');
    CHECK(chmap[chcols - 1] == ' ');

    // germs replacement (bit 30): inserted as its charset code 26..31
    setrow("", 0);
    feed((1L << 30) | 26, NOSHIFT);
    CHECK(chmap[0] == 26);
    CHECK(cursx == 1);
    CHECK(anyalt == 26);
    // without bit 30, 26..29 are control codes: 28/29 leave getline
    setrow("", 0);
    CHECK(feed(28, NOSHIFT) == 28);
    CHECK(chmap[0] == ' ');
    CHECK(feed(13, NOSHIFT) == 13);
    CHECK(anyalt == 0);
    // Latin-1 >= 127 (except Del) and Help/F-keys leave getline
    CHECK(feed(0xFF, NOSHIFT) == 0xFF);
    CHECK(feed(0x8B, NOSHIFT) == 0x8B);
    CHECK(feed(-3, NOSHIFT) == -3);
    CHECK(feed(-2, NOSHIFT, 0x1234) == -2);

    // token colour: print2 sets bit 7 if color != normcol
    setrow("", 0);
    color = tokcol;
    feed('a', NOSHIFT);
    CHECK(chmap[0] == ('a' | 0x80));
}

TEST_CASE("Del and Backspace", "[lineedit]")
{
    env();
    setrow("abcd", 1);
    feed(127, NOSHIFT);
    CHECK(rowis("acd"));
    CHECK(cursx == 1);
    CHECK(anyalt == 127);

    // Del at chcols-1
    setrow("", 0);
    std::memset(chmap, 'q', chcols);
    cursx = 79;
    feed(127, NOSHIFT);
    CHECK(chmap[79] == 'q');
    CHECK(chmap[chcols - 1] == ' ');
    CHECK(chmap[chcols - 2] == 'q');

    // Backspace
    setrow("abcd", 2);
    feed(8, NOSHIFT);
    CHECK(rowis("acd"));
    CHECK(cursx == 1);
    CHECK(anyalt == 8);
    // Backspace at column 0: nothing, anyalt unchanged
    setrow("abcd", 0);
    feed(8, NOSHIFT);
    CHECK(rowis("abcd"));
    CHECK(cursx == 0);
    CHECK(anyalt == 0);
}

TEST_CASE("Tab and Shift-Tab", "[lineedit][tabs]")
{
    env();
    tabstop = 2;
    setrow("abcdef", 3);
    feed(9, NOSHIFT);
    CHECK(rowis("abc def"));
    CHECK(cursx == 4);
    CHECK(anyalt == 9);

    tabstop = 4;
    setrow("ab", 0);
    feed(9, NOSHIFT);
    CHECK(rowis("    ab"));
    CHECK(cursx == 4);

    // tab past the end of the map: nothing, anyalt unchanged
    setrow("", 0);
    cursoff = 48;
    cursx = 79;                         // abs. 127
    feed(9, NOSHIFT);
    CHECK(anyalt == 0);
    CHECK(cursx == 79);
    CHECK(cursoff == 48);

    // overflow of the inserted spaces is lost
    setrow("", 0);
    std::memset(chmap, 'q', chcols);
    cursx = 1;
    feed(9, NOSHIFT);                   // tabstop 4: 3 spaces
    CHECK(chmap[0] == 'q');
    CHECK(chmap[1] == ' ');
    CHECK(chmap[3] == ' ');
    CHECK(chmap[4] == 'q');
    CHECK(chmap[chcols - 1] == 'q');

    // Shift-Tab deletes any characters back to the tab position (I.3)
    setrow("abcdefgh", 6);
    feed(9, SHIFT);
    CHECK(rowis("abcdgh"));
    CHECK(cursx == 4);
    CHECK(anyalt == 9);
    setrow("abcdefgh", 4);
    feed(9, SHIFT);
    CHECK(rowis("efgh"));
    CHECK(cursx == 0);
    // at column 0: no change, but anyalt is set
    setrow("abc", 0);
    feed(9, SHIFT);
    CHECK(rowis("abc"));
    CHECK(cursx == 0);
    CHECK(anyalt == 9);
}

TEST_CASE("the tab size of the options file is clamped to 1..79", "[lineedit][tabs]")
{
    env();
    tempdir dir;
    REQUIRE(!dir.path.empty());
    char cwd[4096];
    REQUIRE(getcwd(cwd, sizeof(cwd)) != nullptr);
    REQUIRE(chdir(dir.path.c_str()) == 0);

    // l:blitzeditor.opts (defname, relative on the host) with tab size VALUE
    auto loadtab = [](UWORD value) {
        struct defblock d = defstart;
        UBYTE img[sizeof(struct defblock)];

        d.d_tabstop = value;
        defs_tobytes(img, &d);
        FILE *f = std::fopen(defname, "wb");
        REQUIRE(f != nullptr);
        std::fwrite(img, 1, sizeof(img), f);
        std::fclose(f);
        loaddefs();
        std::remove(defname);
    };

    // fixed #385: the original took 0 over (every TAB a division by zero)
    loadtab(0);
    CHECK(tabstop == 1);
    setrow("abc", 1);
    feed(9, NOSHIFT);                   // one column to the next tab position
    CHECK(rowis("a bc"));
    CHECK(cursx == 2);
    loadtab(300);
    CHECK(tabstop == 79);
    loadtab(80);
    CHECK(tabstop == 79);
    loadtab(1);
    CHECK(tabstop == 1);
    loadtab(79);
    CHECK(tabstop == 79);
    loadtab(8);
    CHECK(tabstop == 8);
    REQUIRE(chdir(cwd) == 0);
    tabstop = 2;
}

TEST_CASE("Shift-cursor left and right", "[lineedit]")
{
    env();
    // Shift-right: behind the last non-space
    setrow("abc  x", 0);
    feed(30, SHIFT);
    CHECK(cursx == 6);
    setrow("", 5);
    feed(30, SHIFT);
    CHECK(cursx == 0);
    setrow("", 0);
    std::memset(chmap, 'q', chcols);
    feed(30, SHIFT);                    // max chcols-1
    CHECK(cursx + cursoff == chcols - 1);
    // token colour space ($A0) counts as a character
    setrow("ab", 0);
    chmap[10] = 0xA0;
    feed(30, SHIFT);
    CHECK(cursx == 11);

    // Shift-left: column != 0 -> 0
    setrow("abc", 5);
    feed(31, SHIFT);
    CHECK(cursx == 0);

    // Shift-left at column 0: indentation of the previous line
    auto *p = static_cast<struct item *>(std::calloc(1, sizeof(struct item) + 16));
    auto *c = static_cast<struct item *>(std::calloc(1, sizeof(struct item) + 16));
    REQUIRE(p != nullptr);
    REQUIRE(c != nullptr);
    p->nextitem = c;
    p->lastitem = nullptr;
    std::strcpy(reinterpret_cast<char *>(p->chars), "   foo");
    p->numchars = 7;
    c->nextitem = nullptr;
    c->lastitem = p;
    std::strcpy(reinterpret_cast<char *>(c->chars), "x");
    c->numchars = 2;
    thisitem = c;
    setrow("x", 0);
    cursy = 1;
    feed(31, SHIFT);
    CHECK(cursx == 3);
    // previous line empty / only spaces: no move
    std::strcpy(reinterpret_cast<char *>(p->chars), "   ");
    cursx = 0;
    feed(31, SHIFT);
    CHECK(cursx == 0);
    // line 0: no move
    cursy = 0;
    std::strcpy(reinterpret_cast<char *>(p->chars), "   foo");
    feed(31, SHIFT);
    CHECK(cursx == 0);
    thisitem = nullptr;
    std::free(p);
    std::free(c);
}

TEST_CASE("delete to the end of the line", "[lineedit]")
{
    env();
    setrow("abcdef", 2);
    feed(-2, NOSHIFT, 0xf8e1);
    CHECK(rowis("ab"));
    CHECK(cursx == 2);
    CHECK(anyalt == 0xf8e1);
    // block is not adjusted (I.4)
    setrow("abcdef", 2);
    block(0, 1, 0, 5);
    feed(-2, NOSHIFT, 0xf8e1);
    CHECK(btopx == 1);
    CHECK(bbotx == 5);
    CHECK(bboty == 0);
    // other menu codes leave getline
    setrow("abcdef", 2);
    CHECK(feed(-2, NOSHIFT, 0xf8e0) == -2);
    CHECK(rowis("abcdef"));
}

TEST_CASE("horizpos, cursrite, cursleft", "[lineedit][cursor]")
{
    env();
    setrow("", 0);
    horizpos(10);
    CHECK(cursoff == 0);
    CHECK(cursx == 10);
    CHECK(host_calls[HC_REFRESH] == 0);
    horizpos(70);                       // >= edcols-hscmarg = 68
    CHECK(cursoff == 30);
    CHECK(cursx == 40);
    CHECK(host_calls[HC_REFRESH] == 1);
    horizpos(71);                       // odd -> rounded up
    CHECK(cursoff == 32);
    CHECK(cursx == 39);
    CHECK(host_calls[HC_REFRESH] == 2);
    horizpos(71);                       // cursoff unchanged: no refresh
    CHECK(host_calls[HC_REFRESH] == 2);
    horizpos(100);                      // > chcols-edcols
    CHECK(cursoff == 48);
    CHECK(cursx == 52);
    horizpos(127);
    CHECK(cursoff == 48);
    CHECK(cursx == 79);
    horizpos(0);
    CHECK(cursoff == 0);
    CHECK(cursx == 0);
    // I.5: odd cursoff for an odd chcols-edcols
    edcols = 79;
    horizpos(127);
    CHECK(cursoff == 49);
    CHECK(cursx == 78);
    edcols = 80;
    // I.5: hscmarg > edcols/2 -> negative cursoff
    hscmarg = 50;
    horizpos(35);
    CHECK(cursoff == -4);
    CHECK(cursx == 39);
    hscmarg = 12;

    // cursrite
    setrow("", 67);
    cursrite();
    CHECK(cursx == 68);
    CHECK(cursoff == 0);
    cursrite();                         // at edcols-hscmarg: scroll
    CHECK(cursx == 67);
    CHECK(cursoff == 2);
    CHECK(host_calls[HC_SCROLLLEFT] == 1);
    cursoff = 48;
    cursx = 78;
    cursrite();
    CHECK(cursx == 79);
    cursrite();                         // right edge of the map
    CHECK(cursx == 79);
    CHECK(cursoff == 48);
    // cursleft
    setrow("", 12);
    cursleft();
    CHECK(cursx == 11);
    cursoff = 2;
    cursleft();                         // below hscmarg with cursoff
    CHECK(cursx == 12);
    CHECK(cursoff == 0);
    CHECK(host_calls[HC_SCROLLRITE] == 1);
    cursx = 1;
    cursleft();
    cursleft();
    CHECK(cursx == 0);
    CHECK(cursoff == 0);
    // typing scrolls at the margin
    setrow("", 66);
    typestr("abc");
    CHECK(cursoff == 2);
    CHECK(cursx == 67);
    CHECK(chmap[66] == 'a');
    CHECK(chmap[68] == 'c');
}

TEST_CASE("block edges while editing", "[lineedit][blocks]")
{
    env();
    // inbloks/inblokf
    setrow("", 3);
    block(0, 3, 0, 5);
    CHECK(inbloks());
    CHECK(inblokf());
    cursx = 4;
    CHECK(!inbloks());
    CHECK(inblokf());
    cursx = 6;
    CHECK(!inbloks());
    CHECK(!inblokf());
    cursy = 1;
    cursx = 0;
    CHECK(!inbloks());
    CHECK(!inblokf());

    // insert before / inside / behind a one-line block
    setrow("0123456789", 1);
    block(0, 2, 0, 5);
    feed('x', NOSHIFT);
    CHECK(btopx == 3);
    CHECK(bbotx == 6);
    setrow("0123456789", 5);
    block(0, 2, 0, 5);
    feed('x', NOSHIFT);
    CHECK(btopx == 2);
    CHECK(bbotx == 6);
    setrow("0123456789", 6);
    block(0, 2, 0, 5);
    feed('x', NOSHIFT);
    CHECK(btopx == 2);
    CHECK(bbotx == 5);

    // delete before / at the start
    setrow("0123456789", 1);
    block(0, 2, 0, 5);
    feed(127, NOSHIFT);
    CHECK(btopx == 1);
    CHECK(bbotx == 4);
    setrow("0123456789", 2);
    block(0, 2, 0, 5);
    feed(127, NOSHIFT);
    CHECK(btopx == 2);
    CHECK(bbotx == 4);

    // tab: +count on both edges; Shift-Tab: -count
    tabstop = 4;
    setrow("0123456789", 1);
    block(0, 2, 0, 5);
    feed(9, NOSHIFT);
    CHECK(btopx == 5);
    CHECK(bbotx == 8);
    setrow("0123456789", 6);
    block(0, 6, 0, 9);
    feed(9, SHIFT);                     // back to 4: d4 = 2
    CHECK(btopx == 4);
    CHECK(bbotx == 7);
    tabstop = 2;
}

TEST_CASE("blokfix and blokfixb", "[lineedit][blocks]")
{
    env();
    // blokfix: btopx >= chcols -> next line
    block(0, 128, 2, 10);
    blokfix();
    CHECK(btopx == 0);
    CHECK(btopy == 1);
    CHECK(bboty == 2);
    CHECK(bbotx == 10);
    // blokfixb: bbotx < 0 -> end of the previous line
    block(0, 0, 2, -1);
    blokfixb();
    CHECK(bbotx == 127);
    CHECK(bboty == 1);
    // Fixed (#349) I.1: multi-line block, bbotx >= chcols corrected
    block(0, 0, 1, 200);
    blokfixb();
    CHECK(bbotx == 127);
    CHECK(bboty == 1);
    // ... and a block from line >= chcols keeps its end column
    block(130, 0, 131, 5);
    blokfixb();
    CHECK(bbotx == 5);
    CHECK(bboty == 131);
    block(130, 0, 131, 140);
    blokfixb();
    CHECK(bbotx == 127);
    // single line: bbotx >= chcols corrected
    block(3, 0, 3, 140);
    blokfixb();
    CHECK(bbotx == 127);

    // blokfix: btopy beyond bboty -> blokdis
    block(2, 128, 2, 10);
    blokfix();
    CHECK(bboty == NOBLOCK);
    // blokfixb: single line, bbotx < btopx -> blokdis
    block(0, 5, 0, 4);
    blokfixb();
    CHECK(bboty == NOBLOCK);
    // bbotx < 0 on the first line -> blokdis
    block(1, 0, 1, -1);
    blokfixb();
    CHECK(bboty == NOBLOCK);
    // delete the only character of a block -> blokdis via blokfixb
    setrow("0123456789", 3);
    block(0, 3, 0, 3);
    feed(127, NOSHIFT);
    CHECK(bboty == NOBLOCK);
}
