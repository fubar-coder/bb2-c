/*
 * rilvcallhooklib - RWE userlib 27 (Red When Excited): RetrieveCallHook,
 * a render hook for GadTools listviews (GTLV_CallBack) that draws an item
 * as fields of text or bitmaps, with an optional tree indent.
 *
 * C translation of rwelibsrc/lvcallhook/LVCallHook3.bb; reference: the
 * reference build of the source (newer than the only binary, AmiBlitz3's
 * RILVCallHook.obj, which lacks the indent, the ghosting of disabled
 * items and has 8 byte field entries; docs/rwelibs/README.md 3.2).
 * Notes: docs/rwelibs/port/rilvcallhooklib.md.
 *
 *   RetrieveCallHook   the address of the hook function (for h_Entry);
 *                      keeps graphics.library's base (a6 of the call,
 *                      !libs #graphicslib,#la6) for the hook
 *
 * The hook (a0 = the Hook, a1 = the LVDrawMsg, a2 = the node): for other
 * messages than LV_DRAW LVCB_UNKNOWN, else LVCB_OK after drawing.
 * h_Data points to the field table: a first entry of 14 bytes (the
 * offset in the node of the indent word, 0 for none; the offset of the
 * colour table pointer), then entries of 14 bytes (the width, the offset
 * of the text / BitMap pointer in the node, the alignment (1 right, else
 * left), the isbitmap flag, 6 bytes unused), ended by width 0.  The
 * colour table: the pens of the normal (and normal disabled) state, then
 * those of the others (apen, bpen each).  The item is drawn in its
 * bounds: the indent (cleared, with dotted tree lines every 16 pixels),
 * then field by field (text with TextFit to the field, or a BitMap cut to
 * it, left or right aligned, centred vertically; the rest of the field in
 * bpen), the last field cut at the right edge; disabled items are ghosted
 * with a pattern in apen.
 *
 * init/finit are empty.  The hook is not reentrant (its state is static,
 * as the original's).  No assembler besides the thunks.
 *
 * BUG(orig):
 *   - The test for the tree line of the last indent level reads the
 *     indent of the next node at the offset of the first field entry's
 *     width (dispaddress is already past the first entry), not at the
 *     indent offset (an odd width: an address error on a 68000).
 *   - The number of tree lines, indent / 16 - 1, is taken as a long with
 *     the upper word of the caller's d7 (MOVE.w indent,d7:ASR.w #4:SUBQ.l
 *     #1): a negative d7 draws no lines and leaves the RastPort's line
 *     pattern at $AAAA; an indent below 16 with an upper word other than
 *     0 and $8001..$FFFF draws 65536 lines (DBRA on the low word).
 *   - TextFit gets the width with $FFFF in the upper word (MOVEQ #-3,d2
 *     with ADD.w/SUB.w).
 *   - A right aligned BitMap is drawn ob_MinX too far right (the
 *     alignment adds the text's offset, but te_MinX is already ob_MinX).
 *   - The tree line pattern is written to rp_LinePtrn without the flags
 *     and count SetDrPt sets.
 *   - A text pointer 0 in a text field is read as a string at address 0.
 *   - RetrieveCallHook must be called before the hook is used (the
 *     graphics base is 0 before).
 */
#include "blitzlib.h"
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/gfx.h>
#include <libraries/gadtools.h>
#include <utility/hooks.h>
#define __NOLIBBASE__
#define GRAPHICS_BASE_NAME gfx
#include <proto/graphics.h>
#undef __NOLIBBASE__

/* entry points (thunks, below) */
void _retrievecallhook(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

/* !libheader {#rilvcallhooklib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_rilvcallhooklib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_graphicslib, BL_LA(6));
BLITZ_SUBS(_retrievecallhook, 0, 0);
BLITZ_NAME("RetrieveCallHook", "");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

#define ENTRYBYTESIZE 14                /* size of the field entries */
#define LVR_NORMALDISABLED_ 2
#define LVR_SELECTEDDISABLED_ 8
#define RIGHTALIGN 1

#define WAT(p, off) (*(const WORD *)((const UBYTE *)(p) + (WORD)(off)))
#define LAT(p, off) (*(const ULONG *)((const UBYTE *)(p) + (WORD)(off)))

static struct Library *gfxbase;
static const UWORD ghostpattern[2] = { 0x4444, 0x1111 };
static struct RastPort *msgrastport;
static struct LVDrawMsg *lvdrawmsg;
static ULONG text;                      /* the text or BitMap of the field */
static UWORD strlen_;
static UWORD apen, bpen;
static WORD alignment;
static WORD maxXboundary;
static struct Rectangle ob;             /* oldbounds: the field */
static struct TextExtent extent;
static WORD indent;
static WORD isbitmap;
static const UWORD *dispaddress;
static const UWORD dispend = 0;         /* an empty field entry: the end */

/* init, finit: RTS */
static void BLITZ_ROUTINE libinit_c(void)
{
}

/* the hook function, below */
void hookfunc(void);

/* RetrieveCallHook: a6 = graphics base */
static void *BLITZ_ROUTINE retrievecallhook_c(struct Library *a6 BL_REG(a6))
{
    gfxbase = a6;
    return (void *)hookfunc;
}

/* Erase any part of the old extent (the field) the new one does not
   cover (FillOldExtent; RectFill in the current pen) */
static void filloldextent(struct Library *gfx, struct RastPort *rp, const struct Rectangle *o,
                          const struct Rectangle *n)
{
    if (n->MinX >= o->MinX)
        RectFill(rp, o->MinX, o->MinY, (WORD)(n->MinX - 1), o->MaxY);
    if (n->MaxX <= o->MaxX)
        RectFill(rp, (WORD)(n->MaxX + 1), o->MinY, o->MaxX, o->MaxY);
    if (n->MaxY <= o->MaxY)
        RectFill(rp, o->MinX, (WORD)(n->MaxY + 1), o->MaxX, o->MaxY);
    if (n->MinY >= o->MinY)
        RectFill(rp, o->MinX, o->MinY, n->MaxX, (WORD)(n->MinY - 1));
}

/* the position of the extent in the field ('vertalign): x by the
   alignment, y centred; the extent moved by them (for a text: the pen
   position, the extent being relative to it) */
static void place(WORD *px, WORD *py)
{
    WORD d5, x, y;

    d5 = (WORD)(ob.MaxY - ob.MinY - (extent.te_Extent.MaxY - extent.te_Extent.MinY));
    if (alignment == RIGHTALIGN)
        x = (WORD)(ob.MaxX - (WORD)extent.te_Width);
    else
        x = (WORD)(2 + ob.MinX - extent.te_Extent.MinX);
    y = (WORD)(((WORD)(d5 + 1) >> 1) + ob.MinY - extent.te_Extent.MinY);
    extent.te_Extent.MinX += x;
    extent.te_Extent.MaxX += x;
    extent.te_Extent.MinY += y;
    extent.te_Extent.MaxY += y;
    *px = x;
    *py = y;
}

/* DisplayField: the field's text or BitMap in ob */
static void displayfield(struct Library *gfx)
{
    struct RastPort *rp = msgrastport;
    WORD x, y;

    if (!isbitmap) {
        const char *s = (const char *)text;     /* BUG(orig): 0 not checked */
        ULONG n = 0, w, h;

        /* the constraining width: MOVEQ #-3,d2 + word arithmetic */
        w = 0xffff0000UL | (UWORD)(-3 + ob.MaxX - ob.MinX);    /* BUG(orig) */
        h = (UWORD)(1 + ob.MaxY - ob.MinY);
        while (s[n])
            n++;
        strlen_ = (UWORD)n;
        strlen_ = (UWORD)TextFit(rp, (CONST_STRPTR)s, n, &extent, NULL, 1, w, h);
        place(&x, &y);
        Move(rp, x, y);
        SetAPen(rp, apen);
        Text(rp, (CONST_STRPTR)s, strlen_);
        SetAPen(rp, bpen);
        filloldextent(gfx, rp, &ob, &extent.te_Extent);
        return;
    }
    /* dobitmap */
    if (!text) {
        extent.te_Width = 0;
        extent.te_Height = 0;
        extent.te_Extent.MinX = ob.MinX;
        extent.te_Extent.MinY = ob.MinY;
        extent.te_Extent.MaxX = (WORD)(ob.MinX - 1);
        extent.te_Extent.MaxY = (WORD)(ob.MinY - 1);
    } else {
        const struct BitMap *bm = (const struct BitMap *)text;
        WORD d0, d1;

        d1 = (WORD)(ob.MaxX - ob.MinX + 1);
        d0 = (WORD)(bm->BytesPerRow << 3);
        if (d1 < d0)
            d0 = d1;
        extent.te_Width = (UWORD)d0;
        d1 = (WORD)(ob.MaxY - ob.MinY + 1);
        d0 = (WORD)bm->Rows;
        if (d1 < d0)
            d0 = d1;
        extent.te_Height = (UWORD)d0;
        extent.te_Extent.MinX = ob.MinX;
        extent.te_Extent.MinY = ob.MinY;
        extent.te_Extent.MaxX = (WORD)(ob.MinX + extent.te_Width - 1);
        extent.te_Extent.MaxY = (WORD)(ob.MinY + extent.te_Height - 1);
    }
    place(&x, &y);                      /* BUG(orig): right: + ob.MinX */
    if (extent.te_Width)
        BltBitMapRastPort((const struct BitMap *)text, 0, 0, rp, extent.te_Extent.MinX,
                          extent.te_Extent.MinY, extent.te_Width, extent.te_Height, 0xc0);
    SetAPen(rp, bpen);
    filloldextent(gfx, rp, &ob, &extent.te_Extent);
}

/* ClearIndent: the indent in pen 0, the tree lines in pen 1; d7 = the
   caller's d7 (its upper word, BUG(orig)) */
static void clearindent(struct Library *gfx, struct RastPort *rp, const UBYTE *node, ULONG d7)
{
    WORD x, y;
    UWORD k;

    SetAPen(rp, 0);
    RectFill(rp, ob.MinX, ob.MinY, (WORD)(ob.MinX + indent - 1), ob.MaxY);
    SetAPen(rp, 1);
    rp->LinePtrn = 0xaaaa;              /* BUG(orig): not SetDrPt */
    x = (WORD)(ob.MinX + 8);
    d7 = (d7 & 0xffff0000UL) | (UWORD)(indent >> 4);
    d7--;                               /* SUBQ.l #1,d7:BMI */
    if ((LONG)d7 < 0)
        return;
    k = (UWORD)d7;
    for (;;) {
        Move(rp, x, ob.MinY);
        y = ob.MaxY;
        if (k == 0) {                   /* the last level: short if the last node */
            const UBYTE *next = *(const UBYTE *const *)node;
            if (!*(const ULONG *)next
                || WAT(next, *dispaddress) < indent)   /* BUG(orig): width offset */
                y = (WORD)((WORD)(y + ob.MinY) >> 1);
        }
        Draw(rp, x, y);
        x = (WORD)(x + 16);
        if (k-- == 0)
            break;
    }
    x = (WORD)(x - 15);
    y = (WORD)((WORD)(ob.MinY + ob.MaxY) >> 1);
    Move(rp, x, y);
    Draw(rp, (WORD)(x + 7), y);
    rp->LinePtrn = 0xffff;
}

/* DrawDisabled: the bounds ghosted in apen */
static void drawdisabled(struct Library *gfx, const struct Rectangle *b, struct RastPort *rp,
                         UWORD pen)
{
    SetABPenDrMd(rp, pen, 0, JAM1);
    rp->AreaPtrn = (UWORD *)ghostpattern;
    rp->AreaPtSz = 1;
    RectFill(rp, b->MinX, b->MinY, b->MaxX, b->MaxY);
    rp->AreaPtrn = NULL;
    rp->AreaPtSz = 0;
}

/* the hook: a0 = hook, a1 = message, a2 = node, d7 = the caller's d7 */
static ULONG BLITZ_ROUTINE hookfunc_c(struct Hook *hook BL_REG(a0), struct LVDrawMsg *msg BL_REG(a1),
                                      const UBYTE *node BL_REG(a2), ULONG d7 BL_REG(d7))
{
    struct Library *gfx;
    const UWORD *a2;
    const UWORD *col;
    UWORD d0, d1;
    ULONG state;

    if (msg->lvdm_MethodID != LV_DRAW)
        return LVCB_UNKNOWN;
    dispaddress = hook->h_Data;
    gfx = gfxbase;
    lvdrawmsg = msg;
    msgrastport = msg->lvdm_RastPort;
    state = msg->lvdm_State;
    /* (the original loads lvdm_DrawInfo->dri_Pens and does not use it) */
    d0 = dispaddress[0];                /* the offset of the indent */
    if (d0)
        d0 = (UWORD)WAT(node, d0);
    indent = (WORD)d0;
    col = (const UWORD *)LAT(node, dispaddress[1]);     /* the colour table */
    dispaddress = (const UWORD *)((const UBYTE *)dispaddress + ENTRYBYTESIZE);
    d0 = col[2];
    d1 = col[3];
    if (state == LVR_NORMAL || state == LVR_NORMALDISABLED_) {
        d0 = col[0];
        d1 = col[1];
    }
    apen = d0;
    bpen = d1;
    SetABPenDrMd(msgrastport, d0, d1, JAM2);

    maxXboundary = msg->lvdm_Bounds.MaxX;
    ob = msg->lvdm_Bounds;
    if (indent)
        clearindent(gfx, msgrastport, node, d7);
    ob.MinX += indent;

    a2 = dispaddress;
    for (;;) {                          /* 'renderfields */
        WORD w = (WORD)*a2++;
        WORD off;

        if (!w)
            break;
        off = (WORD)*a2++;
        alignment = (WORD)*a2++;
        isbitmap = (WORD)*a2++;
        a2 += 3;                        /* ADDQ.l #6,a2 */
        w = (WORD)(w + ob.MinX);
        if (w >= maxXboundary) {        /* the last field */
            w = (WORD)(maxXboundary + 1);
            a2 = &dispend;
        }
        ob.MaxX = (WORD)(w - 1);
        text = LAT(node, off);
        displayfield(gfx);
        ob.MinX = (WORD)(ob.MaxX + 1);
    }

    state = lvdrawmsg->lvdm_State;
    if (state == LVR_NORMALDISABLED_ || state == LVR_SELECTEDDISABLED_)
        drawdisabled(gfx, &lvdrawmsg->lvdm_Bounds, msgrastport, apen);
    return LVCB_OK;
}

/* Registers: the original keeps everything in init/finit, changes d0/a0
   in RetrieveCallHook and d0 (LVCB_UNKNOWN) or d0-d1/a0-a1 (LVCB_OK) in
   the hook; the thunks keep d1/a0-a1 (the hook's a3 too: TextFit's
   constrainingExtent is a3), the C code d2-d7/a2/a4-a6. */
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_retrievecallhook, retrievecallhook_c, "d1/a0-a1");
BLITZ_THUNK(hookfunc, hookfunc_c, "d1/a0-a1/a3");
