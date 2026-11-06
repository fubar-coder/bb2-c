/*
 * rifnslib - RWE userlib 93 (Reflective Images, FNS Library 1.1):
 * bitmap fonts ("FNS." files) printed into a Blitz bitmap by the CPU.
 *
 * C translation of rwelibsrc/FNSLib/fns-library11.bb; reference BSS2
 * RIFNSLIB.OBJ, which is newer than the source (docs/rwelibs/README.md
 * 3.2): the binary has no `TST.w d5:BEQ exit` behind the width count (the
 * source's guard stops at a line that begins with chr$(10) and returns
 * a stale end position), two short branches (no behaviour), the corrected
 * help text of FNSShadowPrint and its own font data (below).  Notes:
 * docs/rwelibs/port/rifnslib.md.
 *
 * A font: "FNS." +4 height.w, +6 width in pixels (16, 32, 48, 64; any
 * other value is drawn as 16), +8 underline offset, +10 bytes per
 * character; the width table is indexed by the character code from +0
 * (codes 32..159 are used); the glyphs from +256, character c at
 * (c - 32) * bytes per character, one row after the other (1..4 words).
 * 16 font slots; slot 0 is the font built into the library (def_font).
 *
 *   FNSOutput bitmap[,clip]  the bitmap to print into (clip: FNSClipOutput)
 *   FNSClip xmin,ymin,xmax,ymax / FNSClipOutput   the clipping zone (x
 *                            rounded down to 16)
 *   FNSPrint font,x,y,a$[,prefs,colour]  prints; prefs bit 0 centred, 1
 *                            bold, 2 underlined, 3 right aligned; chr$(1)
 *                            + c changes the colour, chr$(9) tab,
 *                            chr$(10) a new line, chr$(0) and codes below
 *                            32 or from 160 on end the text; a0 = the
 *                            address of the end x and y (words)
 *   FNSShadowPrint ...[,prefs,colour[,shadow]]  first in the shadow colour
 *                            at x+1,y+1, then normal
 *   FNSInk, FNSPrefs, FNSOrigin, FNSSetTab   settings
 *   InstallFNS, RemoveFNS, FNSLoad, FNSUnload, FNSSlot   font slots
 *                            (FNSLoad/FNSUnload: RIAmosFuncLib banks 55+n)
 *   FNSHeight, FNSWidth, FNSUnderline, FNSLength, FNSVersion
 *
 * The variables are one block in the original's order (struct fns): the
 * original's overruns stay inside it as there (bitstack written for a
 * depth above 8, font_add read with FNSLength's unmasked number), and
 * FNSPrint's a0 and FNSSlot point into it as in the original.
 *
 * Assembler: the FNSPrint/FNSShadowPrint entries (they share the
 * original's crash, below, which needs a stack imbalance) and
 * FNSVersion (an inline routine); everything else is C behind thunks.
 *
 * BUG(orig):
 *   - FNSPrint before FNSOutput (no bitmap): RTS with 13 registers on the
 *     stack - it returns to the pushed d0, a crash (#665).
 *   - An empty text (or a line that begins with chr$(10), which the
 *     binary prints - the source's guard stopped there) still counts
 *     width + 1 = 1: underlined, it sets 1 pixel; centred, the ASR.l of
 *     $0000FFFF moves x by -32767 (#659).
 *   - FNSLength gives the width - 2 (0 at least) (#662) and does not mask
 *     the font number (ASL.w #2 of the whole word: slots 16 on read the
 *     font data, negative ones the variables before font_add) (#660).
 *     FNSLength without prefs takes the prefs of the last FNSPrint, not
 *     FNSPrefs' (#661).
 *   - A depth of 0 (or more than 8) overruns bitstack: 256 (depth) plane
 *     pointers are written over the variables and the font; the 16 pixel
 *     loop draws bitdepthminusone + 1 planes, the word (Flags<<8|Depth) - 1
 *     of the bitmap - with Flags set or depth 0 hundreds or 65536 planes
 *     (#658).
 *   - Rows above ymax are tested unsigned, rows below ymin signed (#664);
 *     48 and 64 pixel fonts read their third/fourth word only for rows
 *     that are not clipped: after a clipped row the glyph data are
 *     shifted (#656).
 *   - Planes 8 and on take the colour bit of plane & 7 (BTST on memory)
 *     in the 32/48/64 pixel loops, 0 in the 16 pixel loop (#658).
 *   - FNSSetTab 0: chr$(9) divides by 0 (exception) (#666).
 *   - The underline ignores xmin (it is clipped at x = 0 only) (#667).
 *   - A font height of 0 draws 65536 rows (#663).
 *   - Codes from 160 end the text (a sign test after CMPI.b #32) (#657).
 * DIFF: what the original reads behind its variables reads other bytes
 *   here (its code there): FNSLength's font number outside the block, and
 *   the glyphs of codes the font has no data for - the built-in font ends
 *   at 'z' (91 glyphs), codes 123..159 are drawn from the bytes behind it.
 */
#include "blitzlib.h"
#include <graphics/gfx.h>
#include <stddef.h>

/* RIAmosFuncLib (75): index n-1 of its token list */
#define TOK_bload ((LIB_riamosfunclib << 7) + 15)       /* form 1: d0 name, d1 bank */
#define TOK_start ((LIB_riamosfunclib << 7) + 17)       /* d0 bank -> d0 address */
#define TOK_erase ((LIB_riamosfunclib << 7) + 19)       /* d0 bank */

#define FNSID 0x464e532eUL                              /* "FNS." */
#define FONTSIZE 1712

/* entry points (thunks, below) */
void _fnsprint(void), _fnsprint2(void), _installfns(void), _fnsoutput2(void);
void _fnsoutput(void), _fnsink(void), _fnsprefs2(void), _fnsprefs(void);
void _fnsheight(void), _fnsclipset(void), _fnsclipoutputset(void), _removefns(void);
void _fnsunderline(void), _fnslength2(void), _fnslength(void), _fnsversion(void);
void _fnsorigin2(void), _fnsorigin(void), _fnswidth(void), _fnsload(void);
void _fnsslot(void), _fnsunload(void), _fnssettab(void), _fnsshadowprint(void);
void _fnsshadowprint2(void), _fnsshadowprint3(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rifnslib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rifnslib, init, BL_NORET, finit, 0);

#define BMAP0 LIB_bitmaplib, BL_LA(0) | BL_ITEM | BL_PD(0)     /* !bmap {0,0} */

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsprint, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_UNKNOWN, BL_BYTE, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsprint2, 0, 0);
BLITZ_NAME("FNSPrint", "fontnum,x,y,a$/string_address[,prefs,colour]");

BLITZ_ACOMMAND(BL_BYTE);
BLITZ_ARGS(BL_BYTE, BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_installfns, 0, 0);
BLITZ_NAME("InstallFNS", "(fontnum,address)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS(BMAP0);
BLITZ_SUBS(_fnsoutput2, 0, 0);
BLITZ_ARGS(BL_LONG, BL_BYTE);
BLITZ_LIBS(BMAP0);
BLITZ_SUBS(_fnsoutput, 0, 0);
BLITZ_NAME("FNSOutput", "bitmap[,clip_update (On|Off)]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsink, 0, 0);
BLITZ_NAME("FNSInk", "colour");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsprefs2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsprefs, 0, 0);
BLITZ_NAME("FNSPrefs", "prefs[,colour]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsheight, 0, 0);
BLITZ_NAME("FNSHeight", "(fontnum)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsclipset, 0, 0);
BLITZ_NAME("FNSClip", "xmin,ymin,xmax,ymax");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_fnsclipoutputset, 0, 0);
BLITZ_NAME("FNSClipOutput", "- set clipping zone to output bitmap size");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_removefns, 0, 0);
BLITZ_NAME("RemoveFNS", "fontnum");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsunderline, 0, 0);
BLITZ_NAME("FNSUnderline", "(fontnum)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_BYTE, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_fnslength2, 0, 0);
BLITZ_ARGS(BL_BYTE, BL_UNKNOWN, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fnslength, 0, 0);
BLITZ_NAME("FNSLength", "(fontnum,a$[,prefs])");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_fnsversion, 0, 0);
BLITZ_NAME("FNSVersion", "- returns quick, lastest version=1.1");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_fnsorigin2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsorigin, 0, 0);
BLITZ_NAME("FNSOrigin", "[x,y]");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(_fnswidth, 0, 0);
BLITZ_NAME("FNSWidth", "(fontnum)");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(BL_LONG, BL_BYTE);
BLITZ_LIBS0();
BLITZ_SUBS(BLITZ_AMIGAONLY(_fnsload), 0, 0);
BLITZ_NAME("FNSLoad", "(fontname$,fontslot)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_fnsslot, 0, 0);
BLITZ_NAME("FNSSlot", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsunload, 0, 0);
BLITZ_NAME("FNSUnload", "fontslot");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnssettab, 0, 0);
BLITZ_NAME("FNSSetTab", "tab_size - set tab size in pixels");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_UNKNOWN);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsshadowprint, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_UNKNOWN, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsshadowprint2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD, BL_WORD, BL_UNKNOWN, BL_WORD, BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(_fnsshadowprint3, 0, 0);
/* the binary's help text (the source: "...,[prefs,[colour,shadow]]") */
BLITZ_NAME("FNSShadowPrint", "fontnum,x,y,a$/string_address,[prefs,colour[,shadow]]");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ------------------------------------------------------------ the data */

/* the variables of the original, in its order (temp ... def_font) */
struct fns {
    ULONG temp;
    ULONG bitstack[8];                  /* plane pointers + y offset */
    UWORD xcord, ycord;                 /* FNSPrint's a0 points here */
    UWORD underpos, linewidth;
    UBYTE tcolour, tcolour2, textprefs, textprefs2;
    UBYTE bold, nbold, bitdepth, character;
    UWORD bitdepthminusone;
    ULONG scr_width;
    UWORD multi, height, fontwidth;
    UWORD originx, originy;
    UWORD xmin, ymin, xmax, ymax;
    UWORD tabsize;
    struct BitMap *bit_address;         /* FNSOutput's bitmap */
    ULONG font_add[16];                 /* FNSSlot */
    UBYTE def_font[FONTSIZE];           /* IncBin "FNS:Personal.FNS" */
} __attribute__((packed, aligned(2)));

/* the offsets the assembler entries use */
#define OFF_TCOLOUR 44
#define OFF_TCOLOUR2 45
#define OFF_TEXTPREFS 46
#define OFF_TEXTPREFS2 47
#define OFF_BITADDRESS 78
_Static_assert(offsetof(struct fns, xcord) == 0x24, "layout of the original");
_Static_assert(offsetof(struct fns, tcolour) == OFF_TCOLOUR, "layout");
_Static_assert(offsetof(struct fns, tcolour2) == OFF_TCOLOUR2, "layout");
_Static_assert(offsetof(struct fns, textprefs) == OFF_TEXTPREFS, "layout");
_Static_assert(offsetof(struct fns, textprefs2) == OFF_TEXTPREFS2, "layout");
_Static_assert(offsetof(struct fns, bit_address) == OFF_BITADDRESS, "layout");
_Static_assert(offsetof(struct fns, font_add) == 0x52, "layout");
_Static_assert(offsetof(struct fns, def_font) == 0x92, "layout");

/* The built-in font (IncBin "FNS:Personal.FNS"): PERSONAL.FNS as
   AmiBlitz3 publishes it (archive/amiblitz3-github/..., byte-identical to
   the CD's BSS2/REDBLITZLIBS/EXAMPLES/FNS/FONTS/PERSONAL.FNS; the path is
   relative to this file), with the 7 words at font offset $4B0-$4BC in
   the state of the library's binary (BSS2 RIFNSLIB.OBJ = AmiBlitz3
   Rifnslib.obj): an arrow glyph one pixel further left than in the file
   (0800 0C00 0E00 FF00 0E00 0C00 0800).  So the built-in font equals the
   binary's. */
#define FNS_FONT "../../../archive/amiblitz3-github/Sourcecodes/Data/fns/fonts/PERSONAL.FNS"
_Static_assert(sizeof((const UBYTE[]){
#embed FNS_FONT
    }) == FONTSIZE, "PERSONAL.FNS: 1712 bytes");
struct fns fns __asm__("_fns") = {
    .tabsize = 32,
    .def_font = {
#embed FNS_FONT limit(0x4b0)
        ,                               /* $4B0-$4BD: the binary's glyph words */
        0x10, 0x00, 0x18, 0x00, 0x1c, 0x00, 0xfe, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x10, 0x00,
#embed FNS_FONT gnu::offset(0x4be)
    },
};

/* the header of an FNS font; the width table, indexed by the character
   code, starts at +0 too (codes 32..159 are used), the glyphs at +256 */
struct fnshead {
    ULONG id;                           /* FNSID */
    UWORD height, width, underpos;      /* 4 6 8 */
    UWORD multi;                        /* 10: bytes per character */
};
static_assert(sizeof(struct fnshead) == 12);
#define FH(font) ((const struct fnshead *)(font))

#define LONGAT(p) (*(ULONG *)(p))
#define BITSTACK ((ULONG *)((UBYTE *)&fns + offsetof(struct fns, bitstack)))

/* ------------------------------------------------------------ FNSPrint */

static void underline(UWORD x);

/* the width count of FNSPrint/FNSLength: the widths of the codes 32..159
   (+ bold for every one that is not 0), TAB skipped, chr$(1) and its
   colour skipped, everything else ends it */
static UWORD countwidth(const UBYTE *font, const UBYTE *p, UBYTE bold)
{
    UWORD w = 0;
    UBYTE c, cw;

    for (;;) {
        c = *p++;
        if ((BYTE)(c - 32) >= 0) {      /* CMPI.b #32:BPL */
            cw = font[c];
            if (cw)
                w = (UWORD)(w + (UBYTE)(cw + bold));
            continue;
        }
        if (c == 9)
            continue;
        if (c != 1)
            return w;
        p++;
    }
}

/* the glyph rows of one character: rows `rows` + 1 from y, rowoff (d7) =
   the x offset in bytes (+ scr_width per row), s = x & 15, the font
   width `fw` */
static void drawchar(const UWORD *g, UWORD y, ULONG rowoff, UWORD s, UWORD rows, UWORD fw)
{
    ULONG *bs;
    UBYTE *p;

    do {
        UWORD w1 = *g++;
        if (fw == 32 || fw == 48 || fw == 64) {
            UWORD w2 = *g++;
            ULONG b1, b2, m1, m2, b3 = 0, b4 = 0, m3 = 0, m4 = 0;
            UBYTE pl;
            if ((WORD)y < (WORD)fns.ymin || y > fns.ymax)
                goto next;
            b1 = (ULONG)w1 << (16 - s);
            b2 = (ULONG)w2 << (16 - s);
            m1 = ~b1;
            m2 = ~b2;
            bs = BITSTACK;
            pl = 0;
            do {
                p = (UBYTE *)*bs++ + rowoff;
                if (fns.tcolour & (1 << (pl & 7))) {
                    LONGAT(p) |= b1;
                    LONGAT(p + 2) |= b2;
                } else {
                    LONGAT(p) &= m1;
                    LONGAT(p + 2) &= m2;
                }
            } while (++pl != fns.bitdepth);
            if (fw != 32) {             /* the 3rd (and 4th) word at +4 */
                b3 = (ULONG)*g++ << (16 - s);
                m3 = ~b3;
                if (fw == 64) {
                    b4 = (ULONG)*g++ << (16 - s);
                    m4 = ~b4;
                }
                bs = BITSTACK;
                pl = 0;
                do {
                    p = (UBYTE *)*bs++ + rowoff + 4;
                    if (fns.tcolour & (1 << (pl & 7))) {
                        LONGAT(p) |= b3;
                        if (fw == 64)
                            LONGAT(p + 2) |= b4;
                    } else {
                        LONGAT(p) &= m3;
                        if (fw == 64)
                            LONGAT(p + 2) &= m4;
                    }
                } while (++pl != fns.bitdepth);
            }
            rowoff += fns.scr_width;
        } else {                        /* 16 pixels */
            ULONG b, m;
            UWORD col, n;
            if ((WORD)y < (WORD)fns.ymin || y > fns.ymax)
                goto next;
            b = (ULONG)w1 << (16 - s);  /* ROR.l d2,d3:SWAP d3 */
            m = ~b;
            bs = BITSTACK;
            col = fns.tcolour;
            n = fns.bitdepthminusone;
            do {
                p = (UBYTE *)*bs++ + rowoff;
                if (col & 1)
                    LONGAT(p) |= b;
                else
                    LONGAT(p) &= m;
                col >>= 1;
            } while (n-- != 0);
            rowoff += fns.scr_width;
        }
next:
        y++;
    } while (rows-- != 0);
}

/* _FNSPrint2 (behind the bitmap check of the entry): fontnum, x, y,
   string, prefs, colour; the result a0 (`result`, the caller's a0
   without a font) */
static UBYTE *print(UWORD fontnum, UWORD x0, UWORD y0, const UBYTE *str, UBYTE prefs,
                    UBYTE colour, UBYTE *result)
{
    const UBYTE *font, *s;
    UBYTE *planes;                      /* the plane pointers */
    UWORD x, y, n, lw;
    ULONG yoff;
    UBYTE c, i;

restart:
    fns.textprefs = prefs;
    fns.tcolour = colour;
    font = (const UBYTE *)fns.font_add[fontnum & 15];
    if (FH(font)->id != FNSID)
        return result;
    x = (UWORD)(x0 + fns.originx);
    y = (UWORD)(y0 + fns.originy);
    s = str;
    fns.height = (UWORD)(FH(font)->height - 1);
    fns.multi = FH(font)->multi;
    fns.fontwidth = FH(font)->width;
    fns.underpos = FH(font)->underpos;
    planes = (UBYTE *)fns.bit_address->Planes;
    fns.ycord = y;
    if ((WORD)y < (WORD)fns.ymin)
        y = fns.ymin;
    yoff = bl_mulu_w((UWORD)fns.scr_width, y);
    n = (UBYTE)(fns.bitdepth - 1);      /* DBRA on a byte count */
    i = 0;
    do
        BITSTACK[i] = LONGAT(planes + 4 * i) + yoff, i++;
    while (n-- != 0);

    fns.bold = (fns.textprefs & 2) ? 1 : 0;
    /* the binary: no TST.w d5:BEQ exit here (the source's guard) */
    lw = (UWORD)(countwidth(font, s, fns.bold) + 1);
    fns.linewidth = lw;
    if (fns.textprefs & 1) {            /* centred: ASR.l of the long */
        lw = (UWORD)(lw - 2);
        x = (UWORD)(x - (UWORD)((ULONG)lw >> 1));
    } else if (fns.textprefs & 8) {     /* right aligned */
        lw = (UWORD)(lw - 2);
        x = (UWORD)(x - lw);
    }
    fns.xcord = x;
    if (fns.textprefs & 4)
        underline(x);

    for (;;) {
        UWORD xc;
        c = *s++;
        fns.character = c;
        if ((BYTE)(c - 32) < 0) {       /* testchar */
            if (c == 0)
                break;
            if (c == 9) {
                xc = fns.xcord;
                xc = (UWORD)(xc - (UWORD)(bl_divu_w(xc, fns.tabsize) >> 16) + fns.tabsize);
                fns.xcord = xc;
                if ((WORD)xc >= (WORD)fns.xmax)
                    break;
                continue;
            }
            if (c == 10) {              /* a new line: _FNSPrint2 again */
                y0 = (UWORD)(y0 + fns.height + 1);
                str = s;
                goto restart;
            }
            if (c != 1)
                break;
            fns.tcolour = *s++;
            continue;
        }
        fns.nbold = fns.bold;
draw:
        xc = fns.xcord;
        if ((WORD)xc >= (WORD)fns.xmin) {
            UBYTE w = (UBYTE)(font[c] + fns.bold);
            const UWORD *g = (const UWORD *)(font + 256
                                             + bl_mulu_w(fns.multi, (UBYTE)(c - 32)));
            if ((WORD)(UWORD)(xc + w) >= (WORD)fns.xmax)
                break;
            drawchar(g, fns.ycord, (UWORD)((WORD)(xc & 0xfff0) >> 3), xc & 15,
                     fns.height, fns.fontwidth);
        }
        /* add_xcord */
        c = fns.character;
        if (fns.nbold) {
            fns.nbold = 0;
            fns.xcord++;
            goto draw;
        }
        fns.xcord = (UWORD)(fns.xcord + font[c]);
    }
    return (UBYTE *)&fns.xcord;         /* exit: LEA xcord(pc),a0 */
}

/* draw_underline: from x (after centring), linewidth - 1 pixels at
   ycord + underpos, in the planes of the bitmap */
static void underline(UWORD x)
{
    PLANEPTR *planes = fns.bit_address->Planes;
    UWORD *p;
    UWORD xend, y, cnt, left, word;
    ULONG off;
    UBYTE pl, bit, b;

    if ((WORD)x < 0) {                  /* clipped at 0, not at xmin (#667) */
        fns.linewidth = (UWORD)(fns.linewidth + x);
        x = 0;
    }
    xend = (UWORD)(x + fns.linewidth);
    if ((WORD)xend > (WORD)fns.xmax)
        fns.linewidth = (UWORD)(fns.xmax - x);
    y = (UWORD)(fns.ycord + fns.underpos);
    if ((WORD)y < (WORD)fns.ymin || y > fns.ymax)
        return;
    off = bl_mulu_w((UWORD)fns.scr_width, y) + (UWORD)((WORD)(x & 0xfff0) >> 3);
    bit = (UBYTE)(~x & 15);
    cnt = (UWORD)(fns.linewidth - 2);
    pl = 0;
    do {
        int set = fns.tcolour & (1 << (pl & 7));
        p = (UWORD *)(planes[pl] + off);
        left = cnt;
        b = bit;
        word = *p;
        for (;;) {
            if (set)
                word |= (UWORD)(1 << (b & 15));
            else
                word &= (UWORD)~(1 << (b & 15));
            left--;
            if (left & 0x8000)          /* SUBQ.w #1,d2:BMI */
                break;
            if ((BYTE)--b >= 0)
                continue;
            *p++ = word;
            word = *p;
            b = 15;
        }
        *p = word;
    } while (++pl != fns.bitdepth);
}

/**
 * @brief FNSPrint fontnum,x,y,a$,prefs,colour: prints a text into the FNSOutput bitmap.
 *
 * Behind the entries _fnsprint/_fnsprint2 (below), which check the
 * bitmap.  At x,y + the origin, in the font of slot fontnum & 15 (nothing
 * if it is no "FNS." font); prefs bit 0 centred, 1 bold, 2 underlined, 3
 * right aligned; chr$(1) + c changes the colour, chr$(9) a tab
 * (FNSSetTab), chr$(10) a new line, chr$(0), codes below 32 and from 160
 * on (BUG(orig) #657) end the text; clipped by FNSClip.  BUG(orig): an
 * empty text counts width 1 (#659); depth 0 or above 8 overruns the
 * plane stack (#658); rows above ymax are tested unsigned (#664), 48/64
 * pixel glyphs shift after a clipped row (#656); FNSSetTab 0 divides by
 * 0 (#666); the underline ignores xmin (#667); height 0 draws 65536 rows
 * (#663).
 * @param fontnum  d0: the font slot
 * @param x        d1: x
 * @param y        d2: y
 * @param str      d3: the text (a string or an address)
 * @param prefs    d4: the prefs (a byte)
 * @param colour   d5: the colour (a byte)
 * @param result   a0: the caller's a0 (the result without a font)
 * @return a0: the address of the end x and y (words, in the variable block)
 */
static UBYTE *BLITZ_ROUTINE print_c(UWORD fontnum BL_REG(d0), UWORD x BL_REG(d1),
                                    UWORD y BL_REG(d2), const UBYTE *str BL_REG(d3),
                                    UWORD prefs BL_REG(d4), UWORD colour BL_REG(d5),
                                    UBYTE *result BL_REG(a0))
{
    return print(fontnum, x, y, str, (UBYTE)prefs, (UBYTE)colour, result);
}

/**
 * @brief FNSShadowPrint fontnum,x,y,a$,prefs,colour,shadow: prints a text with a shadow.
 *
 * Behind the entries _fnsshadowprint..3 (below).  First the text at
 * x+1,y+1 in the shadow colour, then normally, as FNSPrint.
 * @param fontnum  d0: the font slot
 * @param x        d1: x
 * @param y        d2: y
 * @param str      d3: the text
 * @param prefs    d4: the prefs (a byte)
 * @param colour   d5: the colour (a byte)
 * @param shadow   d6: the shadow colour (a byte)
 * @param result   a0: the caller's a0
 * @return a0: the address of the end x and y of the second print
 */
static UBYTE *BLITZ_ROUTINE shadow_c(UWORD fontnum BL_REG(d0), UWORD x BL_REG(d1),
                                     UWORD y BL_REG(d2), const UBYTE *str BL_REG(d3),
                                     UWORD prefs BL_REG(d4), UWORD colour BL_REG(d5),
                                     UWORD shadow BL_REG(d6), UBYTE *result BL_REG(a0))
{
    result = print(fontnum, (UWORD)(x + 1), (UWORD)(y + 1), str, (UBYTE)prefs, (UBYTE)shadow,
                   result);
    return print(fontnum, x, y, str, (UBYTE)prefs, (UBYTE)colour, result);
}

/* The FNSPrint entries (_fnsprint: FNSPrint fontnum,x,y,a$; _fnsprint2:
   with prefs,colour; _fnsshadowprint, ...2, ...3: FNSShadowPrint without
   prefs, with prefs,colour, with the shadow colour).  _FNSPrint2 pushes d0-d6/a1-a6 and then, without
   a bitmap (bit_address 0), returns with RTS - to the pushed d0
   (BUG(orig) (#665)): a stack imbalance C cannot express, so the entries are
   assembler; the work is print_c/shadow_c.  They keep d0-d6/a1-a6 (the
   original restores them), a0 = the result; the forms without prefs and
   colour take FNSPrefs'/FNSInk's (d4/d5 changed, as in the original),
   the shadow forms without shadow colour take 0 (d6 = 0). */
#define S_(x) BL_STR(x)
BLITZ_ASM_ROUTINE(_fnsprint,
    "   move.b  _fns+" S_(OFF_TCOLOUR2) ",d5\n"
    "   move.b  _fns+" S_(OFF_TEXTPREFS2) ",d4\n"
    "   bra.w   __fnsprint2");
BLITZ_ASM_ROUTINE(_fnsprint2,
    "   tst.l   _fns+" S_(OFF_BITADDRESS) "\n"
    "   bne.s   1f\n"
    "   move.b  d4,_fns+" S_(OFF_TEXTPREFS) "\n"
    "   move.b  d5,_fns+" S_(OFF_TCOLOUR) "\n"
    "   movem.l d0-d6/a1-a6,-(sp)\n"
    "   rts\n"
    "1: movem.l d0-d1/a1,-(sp)\n"
    "   jsr     _print_c\n"
    "   move.l  d0,a0\n"
    "   movem.l (sp)+,d0-d1/a1\n"
    "   rts");
BLITZ_ASM_ROUTINE(_fnsshadowprint,
    "   move.b  _fns+" S_(OFF_TEXTPREFS2) ",d4\n"
    "   move.b  _fns+" S_(OFF_TCOLOUR2) ",d5\n"
    "   bra.w   __fnsshadowprint2");
BLITZ_ASM_ROUTINE(_fnsshadowprint2,
    "   moveq   #0,d6\n"
    "   bra.w   __fnsshadowprint3");
BLITZ_ASM_ROUTINE(_fnsshadowprint3,
    "   tst.l   _fns+" S_(OFF_BITADDRESS) "\n"
    "   bne.s   1f\n"
    "   move.b  d6,d5\n"                /* the shadow print crashes first */
    "   bra.w   __fnsprint2\n"
    "1: movem.l d0-d1/a1,-(sp)\n"
    "   jsr     _shadow_c\n"
    "   move.l  d0,a0\n"
    "   movem.l (sp)+,d0-d1/a1\n"
    "   rts");

/**
 * @brief FNSVersion: the version of the library, 1.1 as a Quick.
 *
 * Inline: MOVE.l #65536+6553,d0.
 * @par Result
 * d0: 1.1 (a Quick)
 */
BLITZ_INLINE(_fnsversion, "move.l #65536+6553,d0");

/* ---------------------------------------------------------- the rest */

/**
 * @brief init: slot 0 = the built-in font, origin 0,0.
 */
static void BLITZ_ROUTINE libinit_c(void)
{
    fns.font_add[0] = (ULONG)fns.def_font;
    fns.originx = 0;
    fns.originy = 0;
}

/**
 * @brief finit: nothing.
 */
void BLITZ_ROUTINE _libfinit(void)
{
}

/**
 * @brief InstallFNS(fontnum,address): puts a font in memory into a slot.
 *
 * Only an "FNS." font.
 * @param n     d0: the slot (& 15)
 * @param font  d1: the address of the font
 * @return d0.b: the slot, 0 if it is no FNS font
 */
static ULONG BLITZ_ROUTINE installfns_c(ULONG n BL_REG(d0), const UBYTE *font BL_REG(d1))
{
    if (FH(font)->id != FNSID)
        return 0;
    n &= 15;
    fns.font_add[n] = (ULONG)font;
    return n;
}

/**
 * @brief RemoveFNS fontnum: empties a font slot.
 * @param n  d0: the slot (& 15)
 */
static void BLITZ_ROUTINE removefns_c(ULONG n BL_REG(d0))
{
    fns.font_add[n & 15] = 0;
}

/**
 * @brief FNSClipOutput: the clipping zone = the whole output bitmap.
 *
 * xmin, ymin 0, xmax = the bytes per row * 8, ymax = the rows - 1.
 */
static void BLITZ_ROUTINE clipoutput_c(void)
{
    const struct BitMap *bm = fns.bit_address;

    fns.xmin = 0;
    fns.ymin = 0;
    fns.ymax = (UWORD)(bm->Rows - 1);
    fns.xmax = (UWORD)(bm->BytesPerRow << 3);
}

/**
 * @brief FNSOutput bitmap,clip_update: the bitmap to print into.
 *
 * Keeps the bitmap, its bytes per row and depth.  BUG(orig) (#658): the
 * 16 pixel loop draws (Flags<<8 | Depth) - 1 + 1 planes - with Flags set
 * or depth 0 hundreds or 65536.
 * @param clip  d1: On: also FNSClipOutput (a byte)
 * @param bm    a0: the bitmap object (!bmap)
 */
static void BLITZ_ROUTINE output_c(UWORD clip BL_REG(d1), struct BitMap *bm BL_REG(a0))
{
    UWORD fd = *(const UWORD *)&bm->Flags;      /* one word: Flags<<8 | Depth */

    fns.bit_address = bm;
    fns.scr_width = bm->BytesPerRow;
    fns.bitdepth = (UBYTE)fd;
    fns.bitdepthminusone = (UWORD)(fd - 1);
    if ((UBYTE)clip)
        clipoutput_c();
}

/**
 * @brief FNSOutput bitmap: as FNSOutput bitmap,Off (the clipping zone stays).
 * @param bm  a0: the bitmap object
 */
static void BLITZ_ROUTINE output2_c(struct BitMap *bm BL_REG(a0))
{
    output_c(0, bm);
}

/**
 * @brief FNSInk colour: the colour of FNSPrint without prefs,colour.
 * @param c  d0: the colour (its low byte)
 */
static void BLITZ_ROUTINE ink_c(UWORD c BL_REG(d0))
{
    fns.tcolour2 = (UBYTE)c;
}

/**
 * @brief FNSPrefs prefs: the prefs of FNSPrint without prefs,colour.
 * @param p  d0: the prefs (its low byte)
 */
static void BLITZ_ROUTINE prefs2_c(UWORD p BL_REG(d0))
{
    fns.textprefs2 = (UBYTE)p;
}

/**
 * @brief FNSPrefs prefs,colour: the prefs and the colour of FNSPrint without them.
 * @param p  d0: the prefs (its low byte)
 * @param c  d1: the colour (its low byte)
 */
static void BLITZ_ROUTINE prefs_c(UWORD p BL_REG(d0), UWORD c BL_REG(d1))
{
    fns.tcolour2 = (UBYTE)c;
    fns.textprefs2 = (UBYTE)p;
}

/* FNSHeight/FNSWidth/FNSUnderline: the word at `off` of the font header,
   -1 if the slot has none */
static ULONG fontword(ULONG n, int off)
{
    const UBYTE *font = (const UBYTE *)fns.font_add[n & 15];

    if (FH(font)->id != FNSID)
        return (ULONG)-1;
    return *(const UWORD *)(font + off);
}

/**
 * @brief FNSHeight(fontnum): the height of a font.
 * @param n  d0: the slot (& 15)
 * @return d0.w: the height, -1 if the slot has no FNS font
 */
static ULONG BLITZ_ROUTINE height_c(ULONG n BL_REG(d0))
{
    return fontword(n, offsetof(struct fnshead, height));
}

/**
 * @brief FNSUnderline(fontnum): the underline offset of a font.
 * @param n  d0: the slot (& 15)
 * @return d0.w: the offset, -1 if the slot has no FNS font
 */
static ULONG BLITZ_ROUTINE underline_c(ULONG n BL_REG(d0))
{
    return fontword(n, offsetof(struct fnshead, underpos));
}

/**
 * @brief FNSWidth(fontnum): the width of a font in pixels (16, 32, 48, 64).
 * @param n  d0: the slot (& 15)
 * @return d0.w: the width, -1 if the slot has no FNS font
 */
static ULONG BLITZ_ROUTINE width_c(ULONG n BL_REG(d0))
{
    return fontword(n, offsetof(struct fnshead, width));
}

/**
 * @brief FNSClip xmin,ymin,xmax,ymax: the clipping zone; x rounded down to 16.
 * @param x0  d0: xmin
 * @param y0  d1: ymin
 * @param x1  d2: xmax
 * @param y1  d3: ymax
 */
static void BLITZ_ROUTINE clipset_c(UWORD x0 BL_REG(d0), UWORD y0 BL_REG(d1),
                                    UWORD x1 BL_REG(d2), UWORD y1 BL_REG(d3))
{
    fns.xmin = x0 & 0xfff0;
    fns.ymin = y0;
    fns.xmax = x1 & 0xfff0;
    fns.ymax = y1;
}

/**
 * @brief FNSLength(fontnum,a$,prefs): the width of a text in pixels.
 *
 * The widths of the characters (+ 1 for bold, prefs bit 1) up to the
 * first code that ends the text.  BUG(orig): the result is the width - 2,
 * 0 at least (#662); the font number is not masked (ASL.w #2 of the whole
 * word: slots 16 on read the font data, negative ones the variables
 * before font_add) (#660; DIFF outside the block).
 * @param n      d0: the slot
 * @param str    d1: the text
 * @param prefs  d2: the prefs (a byte)
 * @return d0.w: the width - 2 (0 at least), -1 if the slot has no FNS font
 */
static ULONG BLITZ_ROUTINE length_c(UWORD n BL_REG(d0), const UBYTE *str BL_REG(d1),
                                    UWORD prefs BL_REG(d2))
{
    const UBYTE *font = (const UBYTE *)LONGAT((UBYTE *)fns.font_add + (WORD)(n << 2));
    UWORD w;

    if (FH(font)->id != FNSID)
        return (ULONG)-1;
    w = (UWORD)(countwidth(font, str, (prefs & 2) ? 1 : 0) - 2);
    if ((WORD)w < 0)
        return 0;
    return w;
}

/**
 * @brief FNSLength(fontnum,a$): as FNSLength with the prefs of the last FNSPrint.
 *
 * BUG(orig) (#661): not FNSPrefs' prefs.
 * @param n    d0: the slot
 * @param str  d1: the text
 * @return d0.w: as FNSLength
 */
static ULONG BLITZ_ROUTINE length2_c(UWORD n BL_REG(d0), const UBYTE *str BL_REG(d1))
{
    return length_c(n, str, fns.textprefs);
}

/**
 * @brief FNSOrigin: the origin of FNSPrint back to 0,0.
 */
static void BLITZ_ROUTINE origin2_c(void)
{
    fns.originx = 0;
    fns.originy = 0;
}

/**
 * @brief FNSOrigin x,y: the origin added to FNSPrint's x,y.
 * @param x  d0: x
 * @param y  d1: y
 */
static void BLITZ_ROUTINE origin_c(UWORD x BL_REG(d0), UWORD y BL_REG(d1))
{
    fns.originx = x;
    fns.originy = y;
}

/**
 * @brief FNSLoad(fontname$,fontslot): loads a font file into a slot.
 *
 * RIAmosFuncLib BLoad into bank 55 + slot, then its Start into the slot
 * (no "FNS." check).  Amiga mode only.
 * @param name  d0: the file name
 * @param slot  d1: the slot (& 15)
 * @return d0.w: the slot, -1 if the load failed
 */
static ULONG BLITZ_ROUTINE load_c(ULONG name BL_REG(d0), ULONG slot BL_REG(d1))
{
    struct bl_regs r;
    ULONG bank, start;

    slot &= 15;
    bank = 55 + slot;
    if (!BL_JSR_D4_(BL_TOKVAL(TOK_bload, 1), name, bank, 0, 0))
        return (ULONG)-1;
    r = (struct bl_regs){ .d0 = bank };
    start = BLITZ_ALIBJSR(TOK_start, r);
    if (!start)
        return (ULONG)-1;
    fns.font_add[slot] = start;
    return slot;
}

/**
 * @brief FNSUnload fontslot: empties the slot and erases bank 55 + slot.
 * @param slot  d0: the slot (& 15)
 */
static void BLITZ_ROUTINE unload_c(ULONG slot BL_REG(d0))
{
    struct bl_regs r;

    slot &= 15;
    fns.font_add[slot] = 0;
    r = (struct bl_regs){ .d0 = 55 + slot };
    BLITZ_ALIBJSR(TOK_erase, r);
}

/**
 * @brief FNSSlot: the address of the 16 font slots (longs).
 * @return d0: the address (in the variable block)
 */
static ULONG BLITZ_ROUTINE slot_c(void)
{
    return (ULONG)fns.font_add;
}

/**
 * @brief FNSSetTab tab_size: the tab size in pixels (32 at first).
 *
 * BUG(orig) (#666): 0 makes chr$(9) divide by 0.
 * @param t  d0: the tab size
 */
static void BLITZ_ROUTINE settab_c(UWORD t BL_REG(d0))
{
    fns.tabsize = t;
}

/* Registers: the original changes d0 (and a0, d1, d2, d7 in places, the
   RIAmosFuncLib calls more); the thunks keep d1/a0/a1, FNSLength all
   but d0 as the original. */
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_installfns, installfns_c, "d1/a0-a1");
BLITZ_THUNK(_removefns, removefns_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsoutput, output_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsoutput2, output2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsclipoutputset, clipoutput_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsink, ink_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsprefs, prefs_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsprefs2, prefs2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsheight, height_c, "d1/a0-a1");
BLITZ_THUNK(_fnsunderline, underline_c, "d1/a0-a1");
BLITZ_THUNK(_fnswidth, width_c, "d1/a0-a1");
BLITZ_THUNK(_fnsclipset, clipset_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnslength, length_c, "d1/a0-a1");
BLITZ_THUNK(_fnslength2, length2_c, "d1/a0-a1");
BLITZ_THUNK(_fnsorigin, origin_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsorigin2, origin2_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsload, load_c, "d1/a0-a1");
BLITZ_THUNK(_fnsunload, unload_c, "d0-d1/a0-a1");
BLITZ_THUNK(_fnsslot, slot_c, "d1/a0-a1");
BLITZ_THUNK(_fnssettab, settab_c, "d0-d1/a0-a1");
