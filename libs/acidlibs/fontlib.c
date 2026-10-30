/*
 * fontlib - command library 142: BlitzFonts (8x8 fonts) and text output
 * to a Blitz bitmap: LoadBlitzFont BitMapOutput Colour Locate CursX CursY
 * BitMapInput, plus the output routines bitout2/bitout3 for other
 * libraries.
 *
 * C translation of bblibsrc/fontlib ("Library 197 - Font Lib"); binary of
 * the CD's acidlibs checked: same as the source.
 *
 * BlitzFont object (4 bytes, maximum 4): the TextFont pointer.  init
 * loads topaz.font (size 8) into object 0 (a3 = object 0, a6 = diskfont).
 * LoadBlitzFont opens the font with OpenDiskFont (TextAttr {name, 8, 0,
 * 0}); only non-proportional 8x8 fonts are accepted ("Can't Open Font",
 * also without the debugger: err1-err3 are in the normal code, their
 * texts in the error area as in the original).
 *
 * Blitz bitmap object: +0 BytesPerRow.w, +2 Rows.w, +4 Depth.w,
 * +8 Planes[8], +40 cursor x (pixels), +42 cursor y (pixels).
 *
 * BitMapOutput n: output handler (inputoutputlib dosetout) bitout:
 *   0 one character, -4 a0/d0, -8 null terminated; characters below
 *   tf_LoChar or above tf_HiChar are ignored, 10 = new line; at the right
 *   edge a new line, below the bitmap nothing is drawn (the cursor still
 *   moves).  Each plane i gets the character bits where bit i of the
 *   Colour is set and the background where bit i of the background colour
 *   is set (8 lines from tf_CharData, tf_CharLoc bit offset / 8).
 * BitMapInput: input handler: single characters from the PREVIOUS input
 *   handler (doinput1 jumps there, assembler), lines with an editor on the
 *   bitmap (doinput2): printable characters are inserted, 31/30 cursor
 *   left/right, 8 backspace, 127 delete, 13 return; the cursor is drawn
 *   with EOR; result = the line without trailing spaces.  It waits while
 *   the previous handler returns -1.
 *
 * BUG(orig), kept:
 *   - Not a bug (pixel cursor by design): Locate: Y is Quick, X a word;
 *     CursX/CursY return the cursor / 8 as Quick (fractions of a character).
 *   - characters with code < tf_LoChar: SUB.b/BMI, so characters 128 or
 *     more above tf_LoChar are dropped as well; with tf_LoChar > 128 codes
 *     more than 128 below it are drawn with index c - LoChar + 256. (#148)
 *   - a bitmap of depth 0 runs the plane loops 65536 times. (#149)
 *   - LoadBlitzFont of a wrong font: the font is not closed again. (#150)
 *   - BitMapInput edits an existing (Edit$ padded) buffer and waits for
 *     return without end: at end of file (-1 from the previous handler)
 *     it loops forever. (#151)
 * DIFF: after a runtime error trap in calcoff (no bitmap / no font, only
 *   reached when the trap returns) the C routine draws nothing; the
 *   original runs into the next trap.  The unreferenced check `bitmapchk`
 *   of the original is left out.
 */
#include "blitzlib.h"

#define LVO_OpenDiskFont (-30)  /* diskfont: a0 = TextAttr */
#define LVO_CloseFont    (-78)  /* graphics: a1 = font */

/* entry points (thunks / C routines, defined below) */
void dobitmapoutput(void), dobitmapinput(void), docolour(void), docolour2(void);
void dolocate(void), docursx(void), docursy(void), codeinit(void);
void dousefont(void), dofreefont(void), bitout2(void), bitout3(void);
void locatechk(void), bitout(void), doinput1(void);
void BLITZ_ROUTINE doloadfont(const UBYTE *name BL_REG(d1), WORD used BL_REG(d5),
                              const void *gfx BL_REG(d6), const void *diskfont BL_REG(a6));

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);

/* !libheader{#fontlib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_fontlib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_fontlib, 0x1380, LIB_diskfontlib, BL_LA(6), LIB_fontlib, 0x0504,
           LIB_graphicslib, 0x0600);
BLITZ_SUBS(BLITZ_AMIGAONLY(doloadfont), 0, 0);
BLITZ_NAME("LoadBlitzFont", "BlitzFont#,Fontname.font$");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_bitmaplib, 0x1080);
BLITZ_SUBS(dobitmapoutput, 0, 0);
BLITZ_NAME("BitMapOutput", "Bitmap#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(docolour2, 0, 0);
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(docolour, 0, 0);
BLITZ_NAME("Colour", "Foreground Colour[,Background Colour]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_QUICK);
BLITZ_LIBS0();
BLITZ_SUBS(dolocate, locatechk, 0);
BLITZ_NAME("Locate", "X,Y");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(docursx, locatechk, 0);
BLITZ_NAME("CursX", "");

BLITZ_AFUNCTION(BL_QUICK);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(docursy, locatechk, 0);
BLITZ_NAME("CursY", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(dobitmapinput, 1, 0);                /* error1 = 1: Blitz mode bit */
BLITZ_NAME("BitMapInput", "");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_fontlib, 0x1301, LIB_fontlib, 0x0504,
                 LIB_diskfontlib, BL_LA(6));
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, dousefont, 0, 0);
BLITZ_NULLSUB_AT(_free, dofreefont, 0, 0, LIB_graphicslib, BL_LA(6));

BLITZ_DUMTOKE(_toke, "BlitzFont", "");

BLITZ_NULLSUB(bitout2, 0, 0);
BLITZ_NULLSUB(bitout3, 0, 0);

BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 4, 2);

/* ------------------------------------------------------------ variables */

/* TextAttr for OpenDiskFont: {name, ysize 8, style 0, flags 0} */
static struct BL_PACKED {
    const UBYTE *name;
    UWORD ysize;
    UBYTE style, flags;
} textattr = { 0, 8, 0, 0 };

static const char deffont[] = "topaz.font";
void *fontlib_oldin;                            /* input handler before BitMapInput */
static UBYTE *bitmapout;                        /* output bitmap object */
static ULONG *fontout;                          /* used font object */
static UBYTE pcolour = 1;                       /* colour for font (plane pick) */
static UBYTE pback = 0;                         /* colour for background */

/* messages in the error area (BLITZ_ERRCHK_MESSAGE at the end), used by
   the normal code like in the original */
extern const char mess1[], mess2[], mess3[];

/* Blitz bitmap object */
#define BM_BPR(b)    (*(UWORD *)((b) + 0))
#define BM_ROWS(b)   (*(UWORD *)((b) + 2))
#define BM_DEPTH(b)  (*(UWORD *)((b) + 4))
#define BM_PLANE(b, i) (((UBYTE **)((b) + 8))[i])
#define BM_CURSX(b)  (*(UWORD *)((b) + 40))
#define BM_CURSY(b)  (*(UWORD *)((b) + 42))

/* TextFont fields */
#define TF_YSIZE(f)    (*(UWORD *)((f) + 20))
#define TF_FLAGS(f)    (*(UBYTE *)((f) + 23))
#define TF_XSIZE(f)    (*(UWORD *)((f) + 24))
#define TF_LOCHAR(f)   (*(UBYTE *)((f) + 32))
#define TF_HICHAR(f)   (*(UBYTE *)((f) + 33))
#define TF_CHARDATA(f) (*(UBYTE **)((f) + 34))
#define TF_MODULO(f)   (*(UWORD *)((f) + 38))
#define TF_CHARLOC(f)  (*(UBYTE **)((f) + 40))

/* -------------------------------------------------------- fonts */

/* dofreefont - a3 = font object, a6 = graphics */
static void freefont(ULONG *obj, const void *gfx)
{
    struct bl_regs r = { 0 };

    if (!*obj)
        return;
    r.a1 = *obj;
    *obj = 0;
    BLITZ_BASEJSR(gfx, LVO_CloseFont, r);
}

static void BLITZ_ROUTINE dofreefont_c(const void *gfx BL_REG(a6))
{
    freefont((ULONG *)blitz_string, gfx);
}
BLITZ_THUNK(dofreefont, dofreefont_c, "d1/a0-a1");

static void BLITZ_ROUTINE dousefont_c(void)
{
    fontout = (ULONG *)blitz_string;            /* MOVE.l a3,fontout */
}
BLITZ_THUNK(dousefont, dousefont_c, "d0-d1/a0-a1");

/* doloadfont - a3 = object, d1 = name, d5 = offset of `used`, d6 =
   graphics (for freeing the old font), a6 = diskfont */
void BLITZ_ROUTINE doloadfont(const UBYTE *name BL_REG(d1), WORD used BL_REG(d5),
                              const void *gfx BL_REG(d6), const void *diskfont BL_REG(a6))
{
    ULONG *obj = (ULONG *)blitz_string;
    struct bl_regs r = { 0 };
    UBYTE *f;

    *(ULONG **)(blitz_globals + used) = obj;    /* set used font... */
    fontout = obj;                              /* and output font. */
    freefont(obj, gfx);
    textattr.name = name;
    r.a0 = (ULONG)&textattr;
    f = (UBYTE *)BLITZ_BASEJSR(diskfont, LVO_OpenDiskFont, r);
    if (!f || TF_XSIZE(f) != 8 || TF_YSIZE(f) != 8 || (TF_FLAGS(f) & 0x20)) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return;
    }
    *obj = (ULONG)f;
}

/* codeinit - no bitmap, no font, colours 1/0, topaz.font into object 0 */
static void BLITZ_ROUTINE codeinit_c(WORD used BL_REG(d5), const void *gfx BL_REG(d6),
                                     const void *diskfont BL_REG(a6))
{
    bitmapout = 0;
    fontout = 0;
    pcolour = 1;                                /* MOVE #$100,pcolour */
    pback = 0;
    doloadfont((const UBYTE *)deffont, used, gfx, diskfont);
}
BLITZ_THUNK(codeinit, codeinit_c, "d1/a0-a1");

/* ----------------------------------------------- Colour, Locate, CursX/Y */

static void BLITZ_ROUTINE docolour_c(UBYTE fg BL_REG(d0), UBYTE bg BL_REG(d1))
{
    pback = bg;
    pcolour = fg;
}
BLITZ_THUNK(docolour, docolour_c, "d0-d1/a0-a1");

static void BLITZ_ROUTINE docolour2_c(UBYTE fg BL_REG(d0))
{
    pcolour = fg;
}
BLITZ_THUNK(docolour2, docolour2_c, "d0-d1/a0-a1");

/* Locate x,y - LSL #3,d0:LSL.l #3,d1:SWAP d1:MOVEM d0-d1,40(a0)
   (y is Quick: the integer part of y*8) */
static void BLITZ_ROUTINE dolocate_c(UWORD x BL_REG(d0), ULONG y BL_REG(d1))
{
    BM_CURSX(bitmapout) = x << 3;
    BM_CURSY(bitmapout) = (y << 3) >> 16;
}
BLITZ_THUNK(dolocate, dolocate_c, "d1/a0-a1");

/* CursX/CursY - (cursor << 16) >> 3 as Quick */
static ULONG BLITZ_ROUTINE docursx_c(void)
{
    return ((ULONG)BM_CURSX(bitmapout) << 16) >> 3;
}
BLITZ_THUNK(docursx, docursx_c, "d1/a0-a1");

static ULONG BLITZ_ROUTINE docursy_c(void)
{
    return ((ULONG)BM_CURSY(bitmapout) << 16) >> 3;
}
BLITZ_THUNK(docursy, docursy_c, "d1/a0-a1");

/* ------------------------------------------------------------- drawing */

/* calcoff - check bitmap and font: bitmap, font, d7 = line offset */
static int calcoff(UBYTE **bm, UBYTE **font, LONG *d7)
{
    if (!bitmapout || !BM_BPR(bitmapout)) {
        BLITZ_RUNERR(mess2);                    /* err2 */
        return 0;
    }
    if (!fontout || !*fontout) {
        BLITZ_RUNERR(mess3);                    /* err3 */
        return 0;
    }
    *bm = bitmapout;
    *font = (UBYTE *)*fontout;
    *d7 = (LONG)(WORD)BM_CURSY(*bm) * (WORD)BM_BPR(*bm);   /* MULS */
    return 1;
}

/* docr - new line: x = 0, y += 8, d7 += bytes per row * 8 */
static void docr(UBYTE *bm, UWORD *d1, UWORD *d2, LONG *d7)
{
    *d1 = 0;
    *d2 += 8;
    *d7 += (WORD)(BM_BPR(bm) << 3);             /* LSL #3:EXT.l */
}

/* bitchar - d0 = character, a0 = bitmap, a1 = font, d7 = offset of the
   line's left edge in the bitmap (changed by a new line) */
static void bitchar(UBYTE c, UBYTE *bm, UBYTE *font, LONG *d7)
{
    UWORD d1, d2 = BM_CURSY(bm), d4, d5, depth, bpr, modulo;
    const UBYTE *src;
    LONG off;
    UWORD i, k;

    if (c == 10) {
        docr(bm, &d1, &d2, d7);
        BM_CURSX(bm) = d1;
        BM_CURSY(bm) = d2;
        return;
    }
    if (c > TF_HICHAR(font))                    /* CMP.b 33(a1),d0:BHI */
        return;
    c -= TF_LOCHAR(font);                       /* SUB.b 32(a1),d0:BMI */
    if ((BYTE)c < 0)
        return;
    d1 = BM_CURSX(bm) >> 3;
    if (d1 >= BM_BPR(bm)) {                     /* CMP (a0),d1:BCS 'skip */
        docr(bm, &d1, &d2, d7);
        BM_CURSY(bm) = d2;
    }
    BM_CURSX(bm) = (d1 + 1) << 3;
    if ((UWORD)(d2 + 8) > BM_ROWS(bm))          /* CMP 2(a0),d3:BHI 'done */
        return;

    off = *d7 + (WORD)d1;                       /* EXT.l d1:ADD.l d1,d7 */
    src = TF_CHARDATA(font)
        + (WORD)(*(UWORD *)(TF_CHARLOC(font) + ((UWORD)c << 2)) >> 3);
    modulo = TF_MODULO(font);
    depth = BM_DEPTH(bm);
    bpr = BM_BPR(bm);
    d5 = pcolour;
    d4 = pback;
    i = 0;
    do {                                        /* 'loop: one bitplane */
        UBYTE *dst = BM_PLANE(bm, i) + off;
        const UBYTE *s = src;
        UBYTE bg = (d4 & 1) ? 0xff : 0;         /* LSR #1,d4:SCS d3 */
        UBYTE fg = (d5 & 1) ? 0xff : 0;         /* LSR #1,d5:SCS d2 */

        d4 >>= 1;
        d5 >>= 1;
        for (k = 0; k < 8; k++) {               /* !oneline x 8 */
            *dst = ~*s & bg;
            *dst |= *s & fg;
            s += (WORD)modulo;
            dst += (WORD)bpr;
        }
        i++;
    } while (--depth);                          /* SUBQ #1,(a7):BNE */
}

/* bitout - one character d0 */
static void BLITZ_ROUTINE bitout_c(UBYTE c BL_REG(d0))
{
    UBYTE *bm, *font;
    LONG d7;

    if (calcoff(&bm, &font, &d7))
        bitchar(c, bm, font, &d7);
}

/* bitout2 - string a0, length d0 */
static void bitout2_(const UBYTE *s, ULONG len)
{
    UBYTE *bm, *font;
    LONG d7;

    if (!len)
        return;
    if (!calcoff(&bm, &font, &d7))
        return;
    do
        bitchar(*s++, bm, font, &d7);
    while (--len);
}

static void BLITZ_ROUTINE bitout2_c(ULONG len BL_REG(d0), const UBYTE *s BL_REG(a0))
{
    bitout2_(s, len);
}

/* bitout3 - null terminated string a0 */
static void BLITZ_ROUTINE bitout3_c(const UBYTE *s BL_REG(a0))
{
    const UBYTE *p = s;
    ULONG n = (ULONG)-1;

    do
        n++;
    while (*p++);
    if (n)
        bitout2_(s, n);
}

/* output handler: -8 bitout3, -4 bitout2, 0 bitout */
__asm__("   .text\n   .even\n"
        "   bra.w   _bitout3\n"
        "   bra.w   _bitout2\n"
        "   .globl  _bitout\n"
        "_bitout:\n"
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _bitout_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");
BLITZ_THUNK(bitout2, bitout2_c, "d1/a0-a1");
BLITZ_THUNK(bitout3, bitout3_c, "d0-d1/a0-a1");

/* BitMapOutput n - a0 = bitmap object; the output goes to bitout */
static void BLITZ_ROUTINE dobitmapoutput_c(UBYTE *bm BL_REG(a0))
{
    struct bl_regs r = { 0 };

    bitmapout = bm;
    r.a0 = (ULONG)bitout;
    BLITZ_ALIBJSR(TOK_setout, r);
}
BLITZ_THUNK(dobitmapoutput, dobitmapoutput_c, "d1/a0-a1");

/* --------------------------------------------------------- BitMapInput */

/* doinput1 - one character from the previous input handler */
static ULONG getchar_old(void)
{
    struct bl_regs r = { 0 };

    return BLITZ_BASEJSR(fontlib_oldin, 0, r);  /* MOVE.l oldin,a6:JMP (a6) */
}

/* docursor - EOR the cursor cell (8 lines, all planes) at x */
static void docursor(UBYTE *bm, UWORD x)
{
    ULONG d0 = (ULONG)BM_CURSY(bm) * BM_BPR(bm)                 /* MULU */
             + (LONG)(WORD)((BM_CURSX(bm) >> 3) + x);           /* EXT.l */
    UWORD line, d2, i;

    for (line = 0; line < 8; line++) {
        d2 = BM_DEPTH(bm) - 1;
        i = 0;
        do
            BM_PLANE(bm, i++)[d0] ^= 0xff;      /* NOT.b (a1) */
        while (d2-- != 0);                      /* DBF d2 */
        d0 += (WORD)BM_BPR(bm);                 /* ADD (a4),a1 (ADDA.w) */
    }
}

/* edstring - show the text from x on */
static void edstring(UBYTE *bm, UBYTE *buf, UWORD max, UWORD x)
{
    ULONG save = *(ULONG *)(bm + 40);

    BM_CURSX(bm) += x << 3;
    bitout2_(buf + (WORD)x, (UWORD)(max - x));
    *(ULONG *)(bm + 40) = save;
}

/* doinput2 - edit a line: a0 = buffer, d0 = length */
static ULONG BLITZ_ROUTINE doinput2_c(ULONG len BL_REG(d0), UBYTE *buf BL_REG(a0))
{
    UBYTE *bm = bitmapout;
    UWORD max = len, x = 0, c, i;

    for (;;) {
        edstring(bm, buf, max, x);              /* 'loop1 */
        for (;;) {
            docursor(bm, x);                    /* 'loop2 */
            do
                c = getchar_old();              /* 'loop3 */
            while (c == 0xffff);
            docursor(bm, x);
            if (c >= 32 && c <= 126) {          /* insert and redisplay */
                for (i = max; --i > x;)         /* 'loop4: BLS 'skip */
                    buf[(WORD)i] = buf[(WORD)i - 1];
                buf[(WORD)x] = c;
                edstring(bm, buf, max, x);
                goto cursrite;
            }
            if (c == 31) {                      /* 'cursleft */
                if ((WORD)--x < 0)
                    x++;
                continue;
            }
            if (c == 30) {
cursrite:
                if (++x >= max)                 /* CMP d6,d7:BCS 'loop2 */
                    x--;
                continue;
            }
            if (c == 13) {                      /* 'doreturn */
                ULONG n = max;

                while (n && buf[(WORD)n - 1] == ' ')
                    n = (UWORD)(n - 1);
                return n;
            }
            if (c == 127)
                break;                          /* 'dodel */
            if (c == 8) {                       /* 'doback */
                if ((WORD)--x >= 0)
                    break;
                x++;
                continue;
            }
        }
        /* 'dodel: delete the character at x */
        for (i = x + 1; i < max; i++)           /* 'loop6: BCC 'skip2 */
            buf[(WORD)i - 1] = buf[(WORD)i];
        buf[(WORD)max - 1] = ' ';
    }
}

/* input handler: -4 doinput2, 0 doinput1 (assembler: jumps on to the
   previous handler with all registers) */
__asm__("   .text\n   .even\n"
        "   bra.w   1f\n"
        "   .globl  _doinput1\n"
        "_doinput1:\n"
        "   move.l  _fontlib_oldin,a6\n"
        "   jmp     (a6)\n"
        "1: movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _doinput2_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");

/* BitMapInput - input handler doinput1, remember the previous one */
static void BLITZ_ROUTINE dobitmapinput_c(void)
{
    struct bl_regs r = { 0 };

    r.a0 = (ULONG)doinput1;
    fontlib_oldin = (void *)BLITZ_ALIBJSR(TOK_setin, r);
}
BLITZ_THUNK(dobitmapinput, dobitmapinput_c, "d1/a0-a1");

/* ------------------------------------------- error check (debugger only) */

/* locatechk - MOVE.l bitmapout(pc),a0:TST (a0):BEQ err2 */
static void BLITZ_ERRCHK locatechk_c(void)
{
    if (!BM_BPR(bitmapout))
        BLITZ_RUNERR(mess2);
}

/* the thunk and the messages last: they leave the assembler in .text
   while GCC still assumes the error check section */
BLITZ_ERRCHK_THUNK(locatechk, locatechk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_MESSAGE(mess1, "Can't Open Font");
BLITZ_ERRCHK_MESSAGE(mess2, "Uninitialized BitMap");
BLITZ_ERRCHK_MESSAGE(mess3, "Uninitialized Font");
