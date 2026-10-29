/*
 * filereq_core.c - the file requester of Blitz Basic 2 (libincs:filereq.s,
 * not on the CD: translated from the disassembly of the freqlib binary).
 * Shared by two programs, as the original source was:
 *   libs-c/libs/freqlib.c   command library 152 (FileRequest$, wrapper
 *                           dofreq/freqchk - BlitzLib specific, stays there)
 *   ted-c/src/filereq.c     TED 1.1 (ted.s embeds the same code as the
 *                           binary hunk `filerequest`, entry code+4; called by
 *                           ezfreq with a0 = reqstruct)
 * Each of them compiles this file as a source of its own with its own
 * options (freqlib: the BlitzLib options, -ffixed-a3/a4/a5, one hunk; TED:
 * -fbaserel, libnix): an object of one side would be wrong in the other
 * (a4 is the base register of TED's globals but belongs to the Blitz
 * program in a library).  Interface: filereq_core.h.
 *
 * The program provides filereq_env.h (in its include path, included first):
 *   - <exec/types.h> and declarations of the exec functions AllocMem,
 *     FreeMem, FindTask, OldOpenLibrary, WaitPort, GetMsg, ReplyMsg
 *     (freqlib: blitzexec.h - SysBase read from address 4; TED:
 *     <proto/exec.h> with its SysBase).
 *   - fq_divu_w(ULONG d, UWORD s): DIVU.w semantics, result
 *     remainder << 16 | quotient, register unchanged on overflow (macro or
 *     inline function; freqlib: bl_divu_w, TED: divu_w).  None of the three
 *     uses here can overflow or divide by 0 (see setpot/propbody/line
 *     click), so a plain C version is equivalent.
 *   - not <proto/dos.h>, <proto/intuition.h> or <proto/graphics.h>: this
 *     file includes them with __NOLIBBASE__ and the library bases
 *     redirected to its own variables in the data block
 *     (fq.dosbase/intbase/gfxbase, opened by fq_request itself).
 * Everything but fq_args and fq_request is static.
 *
 * The data block ($C04-$39D9 of the freqlib binary) is the static variable
 * `fq` (struct fq_data): the Intuition structures of the requester as NDK
 * structures (NewWindow, Gadget, StringInfo, PropInfo, Image, IntuiText,
 * Border), its variables, buffers and strings, in the original order with
 * no gap (m68k: long and pointer members are 2-aligned, the layout is that
 * of the binary - every member offset is checked by a static_assert at the
 * end of this file).  The pointers inside it are static initializers
 * (&fq.member: the same relocations as the original).  It compiles the
 * same way in both environments (.data; with -fbaserel the code addresses
 * it relative to a4 like every other datum).  It is the freqlib version:
 * TED's embedded copy differs in 7 bytes, all padding behind gadget texts
 * or the rest at the end of the hunk (never read).
 *
 * Kept from the original (BUG(orig)):
 *   - dos, intuition and graphics are opened (OldOpenLibrary) at every
 *     call and never closed.
 *   - The volume list (DRIVES) walks the DOS list without locking it
 *     (the labels are right: dl_Type 1 = DLT_DIRECTORY -> $F9 "<ASN>",
 *     2 = DLT_VOLUME -> $F8 "<VOL>"; late/non-binding assigns are not
 *     listed).
 *   - "CD" (the gadget at $FB6) makes the path the current directory of
 *     the process and unlocks the old current directory.
 *   - Building the path from the current directory leaks the ParentDir
 *     locks.
 *   - List entries are 40 byte nodes (AllocMem, not checked); names longer
 *     than 34 characters overrun them.
 *   - There are 64 line gadgets (fq.line[], 144 bytes each, $15AA-$39A9;
 *     behind them only the 48 bytes of arrow image data up to the end of
 *     the data block, $39DA), but the window shows (height - 85) / 8 lines
 *     with no limit: from a window height of 605 (65 lines) the layout
 *     writes line gadgets beyond fq.line[63] - over fq.arrowdata (already
 *     copied to chip memory) and then past the end of fq, into whatever
 *     follows it in the program's data, exactly as the original writes past
 *     its data block (layout and fill step a struct fq_line pointer over
 *     the end of the array).  The first window is screen height - 40 high,
 *     so this happens on every screen of 645 lines or more (MaxHeight is
 *     $FFFF, so also after enlarging the window); details in freqlib.c.
 *   - "CD" clears the words at $111A and $111E: the high word of the path
 *     StringInfo's UndoBuffer and its BufferPos (the start of fq_request
 *     clears BufferPos and DispPos, $111E/$1122).  UndoBuffer then points
 *     to $0000xxxx (the low word of fq.result's address), for this and all
 *     later calls: Intuition copies the path there when the gadget is
 *     activated.
 * DIFF: if the chip memory or a library or the window cannot be opened,
 *   the original returns the first long of its FileInfoBlock buffer on
 *   the stack (uninitialized); here 0.
 */

#include "filereq_env.h"
#include "filereq_core.h"

#define FQ_SWAP(r)  ((ULONG)(r) << 16 | (ULONG)(r) >> 16)       /* SWAP */

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

/* ----------------------------------------------------- the data block */

/* A parameter block like fq_args (path buffer, file buffer, title, screen):
   never used, fq_request takes fq_args - probably for testing filereq.s on
   its own.  Its buffers fq.temp/fq.deffile and title fq.deftitle follow. */
struct fq_params {
    UBYTE *path;
    UBYTE *file;
    UBYTE *title;
    struct Screen *screen;
};

/* A Border with its own 3 points (x/y pairs): the frames are chains of
   them; layout moves the right/bottom ends (xy[0], xy[2] ... xy[5]). */
struct fq_border {
    struct Border b;
    WORD xy[6];
};

/* A line of the list (144 bytes): boolean gadget with its text.  text[]
   has no terminator of its own: layout puts a 0 behind the spaces of the
   visible width. */
struct fq_line {
    struct Gadget gad;          /* +0  Top 8 * (n + 1), Width by layout */
    struct IntuiText itext;     /* +44 */
    UBYTE text[80];             /* +64 */
};

/* The data block, $C04-$39D9 (addresses = offsets in the freqlib binary) */
struct fq_data {
    struct fq_params defparams; /* $C04 never read */
    UBYTE pad_c14[6];           /* $C14 never read */
    UBYTE temp[160];            /* $C1A defparams.path; currentpath scratch */
    UBYTE deffile[64];          /* $CBA defparams.file */
    UBYTE deftitle[21];         /* $CFA defparams.title */
    UBYTE intname[18];          /* $D0F "intuition.library" */
    UBYTE dosname[12];          /* $D21 "dos.library" */
    UBYTE gfxname[17];          /* $D2D "graphics.library" */
    struct DosLibrary *dosbase; /* $D3E */
    struct IntuitionBase *intbase; /* $D42 */
    struct GfxBase *gfxbase;    /* $D46 */
    struct Window *win;         /* $D4A */
    UWORD visible;              /* $D4E list lines */
    UWORD count;                /* $D50 entries */
    UWORD top;                  /* $D52 first shown entry */
    UBYTE *list;                /* $D54 sorted entries (40 byte nodes, next
                                   at +0; &fq.list is the list head node) */
    UWORD propact;              /* $D58 prop gadget held */
    UWORD scroll;               /* $D5A arrow held: -1/1 */
    APTR iaddr;                 /* $D5C IAddress of the last message */
    UWORD drives;               /* $D60 the volume list is shown */
    UWORD keeplock;             /* $D62 never set: the lock is always freed */
    UWORD first;                /* $D64 window height set */
    struct NewWindow nw;        /* $D66 position and size kept between calls */
    struct fq_border frame_ok[2];       /* $D96 button frames: light/dark */
    struct fq_border frame_drives[2];   /* $DCE */
    struct fq_border frame_parent[2];   /* $E06 */
    struct fq_border frame_cd[2];       /* $E3E */
    struct fq_border frame_cancel[2];   /* $E76 */
    struct IntuiText it_ok;     /* $EAE button texts */
    UBYTE txt_ok[3];            /* $EC2 */
    UBYTE pad_ec5;              /* $EC5 (TED: $06) */
    struct IntuiText it_drives; /* $EC6 */
    UBYTE txt_drives[7];        /* $EDA */
    UBYTE pad_ee1;              /* $EE1 */
    struct IntuiText it_parent; /* $EE2 */
    UBYTE txt_parent[7];        /* $EF6 */
    UBYTE pad_efd;              /* $EFD */
    struct IntuiText it_cd;     /* $EFE */
    UBYTE txt_cd[3];            /* $F12 */
    UBYTE pad_f15;              /* $F15 */
    struct IntuiText it_cancel; /* $F16 */
    UBYTE txt_cancel[7];        /* $F2A */
    UBYTE pad_f31;              /* $F31 */
    struct Gadget g_ok;         /* $F32 first gadget of the window */
    struct Gadget g_drives;     /* $F5E */
    struct Gadget g_parent;     /* $F8A */
    struct Gadget g_cd;         /* $FB6 */
    struct Gadget g_cancel;     /* $FE2 */
    struct fq_border strframe[6];  /* $100E path box, file box, outer frame */
    struct IntuiText it_path;   /* $10B6 */
    UBYTE txt_path[6];          /* $10CA */
    struct IntuiText it_file;   /* $10D0 */
    UBYTE txt_file[6];          /* $10E4 */
    struct Gadget g_path;       /* $10EA */
    struct StringInfo si_path;  /* $1116 */
    UBYTE pad_113a[2];          /* $113A "::", never read */
    UBYTE path[160];            /* $113C path string gadget buffer */
    struct Gadget g_file;       /* $11DC */
    struct StringInfo si_file;  /* $1208 */
    UBYTE file[64];             /* $122C file string gadget buffer */
    UBYTE result[448];          /* $126C result / volume name; UndoBuffer
                                   of both string gadgets */
    struct fq_border listframe[6]; /* $142C list box, outer frame, prop box */
    struct Gadget g_prop;       /* $14D4 */
    struct Image propimage;     /* $1500 AUTOKNOB image */
    struct PropInfo propinfo;   /* $1514 */
    struct Gadget g_up;         /* $152A */
    struct Image img_up;        /* $1556 ImageData: 48 bytes chip memory */
    struct Gadget g_down;       /* $156A NextGadget: the first visible line */
    struct Image img_down;      /* $1596 ImageData: img_up's + 24 bytes */
    struct fq_line line[64];    /* $15AA the list lines */
    UWORD arrowdata[24];        /* $39AA arrow images (2 planes each), copied
                                   to the chip memory */
};

static struct fq_data fq;       /* defined (initialized) at the end */

#define __NOLIBBASE__
#define DOS_BASE_NAME       fq.dosbase
#define INTUITION_BASE_NAME fq.intbase
#define GRAPHICS_BASE_NAME  fq.gfxbase
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#undef __NOLIBBASE__

#define W(p, o)  (*(WORD *)((UBYTE *)(p) + (o)))
#define L(p, o)  (*(ULONG *)((UBYTE *)(p) + (o)))
#define B(p, o)  (*(UBYTE *)((UBYTE *)(p) + (o)))
#define P(p, o)  (*(UBYTE **)((UBYTE *)(p) + (o)))

/* variables inside the code of the original */
ULONG fq_args[4];               /* path, file, title, screen */
static APTR oldwinptr;          /* $6F2 */
static struct Process *task;    /* $6F6 */
static UBYTE *firstvis;         /* $7CA: node of the first line */
static UWORD code;              /* $944: Code of the last message */

/* RefreshGList(gadget, window, 0, n) ($B74) */
static void refresh(struct Gadget *gad, LONG n)
{
    RefreshGList(gad, fq.win, NULL, n);
}

/* $B4C - free the entries */
static void freelist(void)
{
    UBYTE *a2 = (UBYTE *)fq.list, *n;

    fq.list = 0;
    while (a2) {
        n = P(a2, 0);
        FreeMem(a2, 40);
        a2 = n;
    }
}

/* $6FA - new entry (type 247..251, name) sorted into the list (by type,
   then name, letters compared AND $DF); returns its position */
static UWORD addentry(UBYTE type, const UBYTE *name)
{
    UBYTE *node = AllocMem(40, MEMF_PUBLIC), *a1, *a3, *a2, *d2;
    UWORD d3 = 0;

    a1 = node + 4;
    *a1++ = type;
    while ((*a1++ = *name++) != 0)
        ;
    a2 = (UBYTE *)&fq.list;             /* the head: next at +0 */
    for (;;) {
        UBYTE c0, c1;
        d2 = P(a2, 0);
        if (!d2)
            break;
        a1 = d2 + 4;
        a3 = node + 4;
        for (;;) {
            c0 = *a3++;
            if (!c0)
                goto insert;
            c1 = *a1++;
            if (!c1)
                break;          /* next */
            c0 &= 0xdf;
            c1 &= 0xdf;
            if (c0 < c1)
                goto insert;
            if (c0 != c1)
                break;
        }
        d3++;
        a2 = d2;
    }
insert:
    P(node, 0) = d2;
    P(a2, 0) = node;
    fq.count++;
    return d3;
}

/* $7CE - the texts of the visible lines, line gadgets chained */
static void fill(void)
{
    UBYTE *a0 = (UBYTE *)fq.list;
    struct fq_line *a1 = fq.line;
    struct Gadget *slot = &fq.g_down;
    UWORD d1 = fq.top, d0 = 0;

    if (d1) {
        UWORD n = d1 - 1;
        do
            a0 = P(a0, 0);
        while (n-- != 0);
    }
    firstvis = a0;
    while (d1 < fq.count && d0 < fq.visible) {
        UBYTE *a2 = a0 + 4, *a3 = a1->text;
        UBYTE t;

        slot->NextGadget = &a1->gad;
        slot = &a1->gad;
        t = *a2++;
        if (t != 0xfb) {
            const char *pre = t == 0xfa ? "<DIR" : t == 0xf9 ? "<ASN"
                            : t == 0xf8 ? "<VOL" : "<DEV";
            int i;
            for (i = 0; i < 4; i++)
                *a3++ = pre[i];
            *a3++ = '>';
            *a3++ = ' ';
        }
        while (*a3) {
            if ((*a3++ = *a2++) == 0) {
                a3--;
                do
                    *a3++ = ' ';
                while (*a3);
                break;
            }
        }
        d0++;
        d1++;
        a0 = P(a0, 0);
        a1++;                           /* BUG(orig): past fq.line[63], */
    }                                   /* as in layout */
    slot->NextGadget = NULL;
}

/* $778 - refresh the visible lines */
static void refreshlist(void)
{
    UWORD n = fq.count;

    if (!n)
        return;
    if (n >= fq.visible)
        n = fq.visible;
    refresh(&fq.line[0].gad, n);
}

/* $B42 - refresh the prop gadget */
static void refreshprop(void)
{
    refresh(&fq.g_prop, 1);
}

/* $798 - VertPot from fq.top */
static void setpot(void)
{
    ULONG d1 = (ULONG)-1, d3 = fq.top;
    UWORD d0 = fq.count - fq.visible;

    if ((WORD)d3 < (WORD)d0) {
        d3 = fq_divu_w(FQ_SWAP(d3), d0);
        d1 = (d1 & 0xffff0000UL) | (UWORD)d3;
    }
    fq.propinfo.VertPot = (UWORD)d1;
    refreshprop();
}

/* $B1E - VertBody */
static void propbody(void)
{
    ULONG d1 = (ULONG)-1;
    UWORD d0 = fq.count, d2 = fq.visible;

    if (d0 > d2)
        d1 = fq_divu_w((ULONG)d2 << 16, d0);
    fq.propinfo.VertBody = (UWORD)d1;
}

/* $76C - after adding a visible entry */
static void listrefresh(void)
{
    fill();
    propbody();
    setpot();
    refreshlist();
}

/* $75E */
static void addvisible(UBYTE type, const UBYTE *name)
{
    if (addentry(type, name) < fq.visible)
        listrefresh();
}

/* $7BE */
static void redraw2(void)
{
    setpot();
    fill();
    refreshlist();
}

/* $AF8 - fq.top from VertPot; returns old (high word) and new (low word) */
static ULONG calctop(void)
{
    UWORD d2 = fq.top, d1 = fq.count - fq.visible + 1;

    if ((WORD)d1 < 0)
        return ((ULONG)d2 << 16) | d2;
    d1 = (UWORD)(((ULONG)d1 * fq.propinfo.VertPot) >> 16);
    fq.top = d1;
    return ((ULONG)d2 << 16) | d1;
}

/* $AE4 */
static void scrollpot(void)
{
    ULONG r = calctop();

    if ((UWORD)r != (UWORD)(r >> 16)) {
        fill();
        refreshlist();
    }
}

/* $952 - layout for a window of w x h: gadget positions/sizes in the
   data block, number of lines, line texts (spaces) */
static void layout(UWORD d1, UWORD d2)
{
    UBYTE *scr = (UBYTE *)fq_args[3];
    struct fq_line *a1;
    UWORD d0, d3, d4, n;

    if (!(B(scr, 76) & 0x80))           /* not HIRES */
        d1 += 5;
    fq.g_drives.LeftEdge = (UWORD)(((ULONG)d1 * 0x2600) >> 16);
    fq.g_parent.LeftEdge = (UWORD)(((ULONG)d1 * 0x6000) >> 16);
    fq.g_cd.LeftEdge = (UWORD)(((ULONG)d1 * 0x9b00) >> 16);
    d0 = d1 - 32;                       /* right edges: string frames */
    fq.strframe[4].xy[0] = d0;
    fq.strframe[5].xy[0] = d0;
    fq.strframe[5].xy[2] = d0;
    fq.strframe[0].xy[0] = d0;
    fq.strframe[1].xy[0] = d0;
    fq.strframe[2].xy[0] = d0;
    fq.strframe[3].xy[0] = d0;
    fq.strframe[3].xy[2] = d0;
    fq.strframe[1].xy[2] = d0;
    d0 -= 64;
    fq.g_path.Width = d0;
    fq.g_file.Width = d0;
    d0 = d1 - 32;                       /* list frames: outer frame */
    fq.listframe[2].xy[0] = d0;
    fq.listframe[3].xy[0] = d0;
    fq.listframe[3].xy[2] = d0;
    d0 -= 4;                            /* prop box */
    fq.listframe[4].xy[0] = d0;
    fq.listframe[5].xy[0] = d0;
    fq.listframe[5].xy[2] = d0;
    d0 -= 20;                           /* list box */
    fq.listframe[0].xy[0] = d0;
    fq.listframe[1].xy[0] = d0;
    fq.listframe[1].xy[2] = d0;
    d0 += 4;                            /* prop box, left */
    fq.listframe[4].xy[2] = d0;
    fq.listframe[4].xy[4] = d0;
    d0 += 1;
    fq.listframe[5].xy[4] = d0;
    d0 = d2 - 84;                       /* bottom edges */
    fq.listframe[4].xy[5] = d0;
    fq.listframe[5].xy[3] = d0;
    fq.listframe[5].xy[5] = d0;
    d0 = d2 - 78;
    fq.listframe[0].xy[5] = d0;
    fq.listframe[1].xy[3] = d0;
    fq.listframe[1].xy[5] = d0;
    d0 += 4;
    fq.listframe[3].xy[3] = d0;
    fq.listframe[3].xy[5] = d0;
    fq.listframe[2].xy[5] = d0;
    d0 -= 24;
    fq.g_prop.Height = d0;
    fq.visible = (UWORD)(d2 - 85) >> 3;
    n = fq.visible - 1;
    a1 = fq.line;
    d3 = d1 - 70;
    d4 = (UWORD)(d3 >> 3) - 1;
    do {                                /* DBF: 0 lines = 65536 */
        UBYTE *a2 = a1->text;           /* BUG(orig): no limit of 64 lines, */
        UWORD k = d4;                   /* a1 runs past fq.line[63] */
        a1->gad.Width = d3;
        do
            *a2++ = ' ';
        while (k-- != 0);
        *a2 = 0;
        a1++;
    } while (n-- != 0);
    d0 = fq.top + fq.visible;
    if (d0 >= fq.count) {
        d0 = fq.count - fq.visible;
        if ((WORD)d0 < 0)
            d0 = 0;
        fq.top = d0;
    }
}

/* $87E - clear the window, gadgets, lines, border */
static void redraw(void)
{
    struct RastPort *rp = fq.win->RPort;

    SetAPen(rp, 0);
    RectFill(rp, 0, 0, (UWORD)(fq.win->Width - 1), (UWORD)(fq.win->Height - 1));
    refresh(&fq.g_ok, 10);
    calctop();
    fill();
    refreshlist();
    DrawBorder(fq.win->RPort, &fq.listframe[0].b, 8, 2);
}

/* $8DA / $90E - message: class (0 = none), code and IAddress kept */
static ULONG takemsg(struct IntuiMessage *m)
{
    ULONG class;

    code = m->Code;
    fq.iaddr = m->IAddress;
    class = m->Class;
    ReplyMsg(&m->ExecMessage);
    return class;
}

static ULONG waitmsg(void)
{
    struct MsgPort *port = fq.win->UserPort;

    WaitPort(port);
    return takemsg((struct IntuiMessage *)GetMsg(port));
}

static ULONG pollmsg(void)
{
    struct IntuiMessage *m = (struct IntuiMessage *)GetMsg(fq.win->UserPort);

    return m ? takemsg(m) : 0;
}

/* $B82 - the path of the current directory into fq.path (the ParentDir
   locks are never unlocked) */
static void currentpath(struct FileInfoBlock *fib)
{
    BPTR d7 = task->pr_CurrentDir;
    UBYTE *a0, *a1;

    fq.path[0] = 0;
    do {
        Examine(d7, fib);
        d7 = ParentDir(d7);
        a0 = (UBYTE *)fib->fib_FileName;
        a1 = fq.temp;
        while ((*a1++ = *a0++) != 0)
            ;
        a1--;
        *a1 = d7 ? '/' : ':';
        a1++;
        a0 = fq.path;
        while ((*a1++ = *a0++) != 0)
            ;
        a0 = fq.temp;
        a1 = fq.path;
        while ((*a1++ = *a0++) != 0)
            ;
    } while (d7);
    a1 -= 2;
    if (*a1 != ':')
        *a1 = 0;
}

/* $BF0 - first call: window height = screen height - 40 */
static void setheight(void)
{
    fq.nw.Height = W(fq_args[3], 14) - 40;
}

/* $CA - the requester; returns fq.result or 0 */
ULONG fq_request(void)
{
    UBYTE fibbuf[264];                  /* LEA -264(sp): long aligned */
    struct FileInfoBlock *fib =
        (struct FileInfoBlock *)(((ULONG)fibbuf + 3) & ~3UL);
    ULONG *fibmem = (ULONG *)fib;
    UBYTE *a0, *a1, *a2;
    ULONG d7, result;
    BPTR d6 = 0;

    fq.si_path.BufferPos = 0;
    fq.si_path.DispPos = 0;
    fq.si_file.BufferPos = 0;
    fq.si_file.DispPos = 0;
    a0 = fq.path; a1 = (UBYTE *)fq_args[0];
    while ((*a0++ = *a1++) != 0)
        ;
    a0 = fq.file; a1 = (UBYTE *)fq_args[1];
    while ((*a0++ = *a1++) != 0)
        ;
    fq.img_up.ImageData = AllocMem(48, MEMF_CHIP);
    if (!fq.img_up.ImageData)
        return 0;                       /* DIFF: the original: garbage */
    fq.img_down.ImageData = fq.img_up.ImageData + 12;
    {                                   /* both arrows into the chip memory */
        ULONG *s = (ULONG *)fq.arrowdata, *d = (ULONG *)fq.img_up.ImageData;
        while (s < (ULONG *)&fq.arrowdata[24])
            *d++ = *s++;
    }
    result = 0;
    fq.dosbase = (struct DosLibrary *)OldOpenLibrary((CONST_STRPTR)fq.dosname);
    if (!fq.dosbase)
        goto fail;
    fq.intbase = (struct IntuitionBase *)OldOpenLibrary((CONST_STRPTR)fq.intname);
    if (!fq.intbase)
        goto fail;
    fq.gfxbase = (struct GfxBase *)OldOpenLibrary((CONST_STRPTR)fq.gfxname);
    if (!fq.gfxbase)
        goto fail;
    if (!fq.first) {
        setheight();
        fq.first = ~fq.first;
    }
    layout(fq.nw.Width, fq.nw.Height);
    fq.nw.Title = (UBYTE *)fq_args[2];
    fq.nw.Type = WBENCHSCREEN;
    if (fq_args[3]) {
        fq.nw.Type = CUSTOMSCREEN;
        fq.nw.Screen = (struct Screen *)fq_args[3];
    }
    fq.win = OpenWindow(&fq.nw);
    if (!fq.win)
        goto fail;
    task = (struct Process *)FindTask(NULL);
    oldwinptr = task->pr_WindowPtr;
    task->pr_WindowPtr = fq.win;
    d6 = 0;

reread:                                 /* $1FC */
    if (d6 && !fq.keeplock) {
        UnLock(d6);
        d6 = 0;
    }
    freelist();
    fq.count = 0;
    fq.top = 0;
    fq.propinfo.VertPot = 0;
    fq.propinfo.VertBody = 0xffff;
    fq.propact = 0;
    fq.scroll = 0;
    fq.drives = 0;
    fq.keeplock = 0;
    redraw();
    d6 = Lock((CONST_STRPTR)fq.path, -2);
    if (d6 && !Examine(d6, fib) && !fq.keeplock) {
        UnLock(d6);
        d6 = 0;
    }

activate:                               /* $284 */
    ActivateGadget(&fq.g_file, fq.win, NULL);
loop:                                   /* $296 */
    if (fq.propact) {
        scrollpot();
        if (!d6)
            goto poll;
    } else if (fq.scroll) {
        UWORD d0 = fq.scroll, d1 = fq.top + d0;
        if ((WORD)d1 >= 0 &&
            ((WORD)d0 < 0 || (UWORD)(d1 + fq.visible - 1) < fq.count)) {
            fq.top = d1;
            redraw2();
        }
        if (!d6)
            goto poll;
    } else if (!d6) {
        d7 = waitmsg();
        goto handle;
    }
    /* $2EE: the next directory entry */
    if (!ExNext(d6, fib)) {
        if (!fq.keeplock) {
            UnLock(d6);
            d6 = 0;
        }
    } else
        addvisible(fib->fib_DirEntryType < 0 ? 0xfb : 0xfa,
                   (const UBYTE *)fib->fib_FileName);
poll:                                   /* $32A */
    d7 = pollmsg();
    if (!d7)
        goto loop;

handle:                                 /* $33A */
    if (d7 == IDCMP_CLOSEWINDOW)
        goto cancel;
    if (d7 == IDCMP_NEWSIZE) {
        layout(fq.win->Width, fq.win->Height);
        propbody();
        setpot();
        goto activate;
    }
    if (d7 == IDCMP_REFRESHWINDOW) {
        redraw();
        goto activate;
    }
    a0 = (UBYTE *)fq.iaddr;
    if (d7 == IDCMP_GADGETDOWN) {
        if (code == 9)
            goto loop;
        if (a0 == (UBYTE *)&fq.g_prop)
            fq.propact = (UWORD)-1;
        else if (a0 == (UBYTE *)&fq.g_up)
            fq.scroll = (UWORD)-1;
        else
            fq.scroll = 1;
        goto loop;
    }
    if (d7 != IDCMP_GADGETUP)
        goto loop;
    if (code == 9)
        goto loop;
    fq.propact = 0;
    fq.scroll = 0;
    if (a0 == (UBYTE *)&fq.g_prop) {
        scrollpot();
        goto activate;
    }
    if (a0 == (UBYTE *)&fq.g_drives) {
        UBYTE *dl;
        fq.propinfo.VertPot = 0;
        scrollpot();
        if (fq.drives)
            goto activate;
        fq.drives = ~fq.drives;
        /* the DOS list: volumes, assigns, devices with a handler */
        dl = BADDR(((struct RootNode *)fq.dosbase->dl_Root)->rn_Info);
        dl = BADDR(L(dl, 4));
        for (;;) {
            ULONG next = L(dl, 0), type = L(dl, 4);
            UBYTE t = 0;
            if (type == 1)
                t = 0xf9;
            else if (type == 2)
                t = 0xf8;
            else if (type == 0 && L(dl, 8))
                t = 0xf7;
            if (t) {
                const UBYTE *s = BADDR(L(dl, 40));
                UBYTE *d = fq.result;
                UWORD n = (UWORD)(*s++ - 1);    /* DBF */
                do
                    *d++ = *s++;
                while (n-- != 0);
                *d++ = ':';
                *d = 0;
                addentry(t, fq.result);
            }
            if (!next)
                break;
            dl = BADDR(next);
        }
        listrefresh();
        goto activate;
    }
    if (a0 == (UBYTE *)&fq.g_path)
        goto reread;
    if (a0 == (UBYTE *)&fq.g_parent) {
        UBYTE *start;
        a0 = fq.path;
        if (!*a0) {
            currentpath(fib);
            a0 = fq.path;
        }
        start = a0;
        while (*a0++)
            ;
        a0 -= 2;
        if (*a0 == '/') {
            a0[1] = '/';
            a0[2] = 0;
            goto reread;
        }
        if (*a0 == ':')
            goto activate;
        for (;;) {
            if (*--a0 == '/') {
                if (a0 == fq.path)
                    a0++;
                *a0 = 0;
                goto reread;
            }
            if (*a0 == ':') {
                a0[1] = 0;
                goto reread;
            }
            if (!(a0 > start)) {
                *start = 0;
                goto reread;
            }
        }
    }
    if (a0 >= (UBYTE *)fq.line) {       /* a line: the n-th visible entry */
        ULONG q = fq_divu_w((ULONG)(a0 - (UBYTE *)fq.line),
                            sizeof(struct fq_line));
        UWORD n = (UWORD)q - 1;
        UBYTE t;
        a0 = firstvis;
        if ((WORD)n >= 0)
            do
                a0 = P(a0, 0);
            while (n-- != 0);
        a1 = fq.path;
        a0 += 4;
        t = *a0++;
        if (t < 0xfa)
            goto copyname;
        if (t == 0xfa) {
            if (*a1) {
                while (*a1++)
                    ;
                a1--;
                if (a1[-1] != ':' && a1[-1] != '/')
                    *a1++ = '/';
            }
            goto copyname;
        }
        {                               /* a file: twice = OK */
            WORD same = 0;
            UBYTE c;
            a1 = fq.file;
            do {
                c = *a0++;
                if (c != *a1)
                    same = -1;
                *a1++ = c;
            } while (c);
            refresh(&fq.g_file, 1);
            if (same)
                goto activate;
            goto ok;
        }
copyname:
        while ((*a1++ = *a0++) != 0)
            ;
        goto reread;
    }
    if (a0 == (UBYTE *)&fq.g_cd) {
        BPTR lock = Lock((CONST_STRPTR)fq.path, -2);
        ULONG s0, s1, s2;
        if (!lock)
            goto activate;
        UnLock(CurrentDir(lock));
        s0 = fibmem[0];
        s1 = fibmem[1];
        s2 = fibmem[30];
        currentpath(fib);
        fibmem[30] = s2;
        fibmem[1] = s1;
        fibmem[0] = s0;
        /* BUG(orig): $111A/$111E - the high word of UndoBuffer and
           BufferPos, not BufferPos and DispPos (see the top) */
        *(UWORD *)&fq.si_path.UndoBuffer = 0;
        fq.si_path.BufferPos = 0;
        refresh(&fq.g_path, 1);
        goto activate;
    }
    if (a0 == (UBYTE *)&fq.g_ok || a0 == (UBYTE *)&fq.g_file)
        goto ok;
    if (a0 == (UBYTE *)&fq.g_cancel)
        goto cancel;
    goto activate;

ok:                                     /* $646 */
    /* remember position and size (two longs: LeftEdge/TopEdge and
       Width/Height, as the original) */
    *(ULONG *)&fq.nw.LeftEdge = *(ULONG *)&fq.win->LeftEdge;
    *(ULONG *)&fq.nw.Width = *(ULONG *)&fq.win->Width;
    a0 = (UBYTE *)fq_args[0];
    a1 = fq.path;
    a2 = fq.result;
    do
        *a0++ = *a1;
    while ((*a2++ = *a1++) != 0);
    a1 -= 2;
    a2--;
    if (*a1 != ':' && *a1 != '/')
        *a2++ = '/';
    a0 = (UBYTE *)fq_args[1];
    a1 = fq.file;
    do
        *a0++ = *a1;
    while ((*a2++ = *a1++) != 0);
    result = (ULONG)fq.result;
    goto close;
cancel:                                 /* $69E */
    result = 0;
close:
    if (d6 && !fq.keeplock)
        UnLock(d6);
    freelist();
    task->pr_WindowPtr = oldwinptr;
    CloseWindow(fq.win);
fail:                                   /* $6D2 */
    FreeMem(fq.img_up.ImageData, 48);
    return result;
}

/* ------------------------------------------ the data block: contents */

/* button (boolean gadget, relative to the bottom edge; frame + text) */
#define FQ_BUTTON(next, x, w, flags, frame, text, id)                   \
    { (next), (x), -16, (w), 12, GFLG_RELBOTTOM | (flags), GACT_RELVERIFY, \
      GTYP_BOOLGADGET, &(frame)[0].b, NULL, &(text), 0, NULL, (id), NULL }

/* Border n of the chain fq.f[] (pens fp/bp, JAM2, 3 points) */
#define FQ_BORDER(f, n, l, t, fp, bp, next, ...)                        \
    { { (l), (t), (fp), (bp), JAM2, 3, fq.f[n].xy, (next) },             \
      { __VA_ARGS__ } }

/* button frame w x 11: light top/left, dark bottom/right */
#define FQ_BUTTONFRAME(f, w)                                            \
    { FQ_BORDER(f, 0, 0, 0, 1, 2, &fq.f[1].b, (w), 0, 0, 0, 0, 11),     \
      FQ_BORDER(f, 1, 0, 0, 2, 1, NULL, (w), 1, (w), 11, 1, 11) }

#define FQ_SPC10 "          "
#define FQ_SPC80 FQ_SPC10 FQ_SPC10 FQ_SPC10 FQ_SPC10 \
                 FQ_SPC10 FQ_SPC10 FQ_SPC10 FQ_SPC10

/* list line n: at y = 8 * (n + 1), 80 spaces */
#define FQ_LINE(n)                                                      \
    { { NULL, 20, 8 * ((n) + 1), 256, 8, 0, GACT_RELVERIFY,            \
        GTYP_BOOLGADGET, NULL, NULL, &fq.line[n].itext, 0, NULL, 0, NULL }, \
      { 1, 0, JAM2, 0, 0, NULL, fq.line[n].text, NULL },                \
      FQ_SPC80 }
#define FQ_LINES8(n)                                                    \
    FQ_LINE((n) + 0), FQ_LINE((n) + 1), FQ_LINE((n) + 2), FQ_LINE((n) + 3), \
    FQ_LINE((n) + 4), FQ_LINE((n) + 5), FQ_LINE((n) + 6), FQ_LINE((n) + 7)

static struct fq_data fq = {
    .defparams = { fq.temp, fq.deffile, fq.deftitle, NULL },
    .pad_c14 = { 0, 0, 0, 0, 1, 2 },
    .deftitle = "Name of File to Load",
    .intname = "intuition.library",
    .dosname = "dos.library",
    .gfxname = "graphics.library",
    .nw = {
        16, 16, 288, 149, 2, 1,
        IDCMP_NEWSIZE | IDCMP_REFRESHWINDOW | IDCMP_GADGETDOWN
            | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW,
        WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET
            | WFLG_SIMPLE_REFRESH | WFLG_GIMMEZEROZERO | WFLG_ACTIVATE,
        &fq.g_ok, NULL, NULL, NULL, NULL,
        264, 104, 640, 0xffff, WBENCHSCREEN
    },
    .frame_ok = FQ_BUTTONFRAME(frame_ok, 23),
    .frame_drives = FQ_BUTTONFRAME(frame_drives, 55),
    .frame_parent = FQ_BUTTONFRAME(frame_parent, 55),
    .frame_cd = FQ_BUTTONFRAME(frame_cd, 23),
    .frame_cancel = FQ_BUTTONFRAME(frame_cancel, 55),
    .it_ok = { 1, 0, JAM2, 4, 2, NULL, fq.txt_ok, NULL },
    .txt_ok = "OK",
    .it_drives = { 1, 0, JAM2, 4, 2, NULL, fq.txt_drives, NULL },
    .txt_drives = "DRIVES",
    .it_parent = { 1, 0, JAM2, 4, 2, NULL, fq.txt_parent, NULL },
    .txt_parent = "PARENT",
    .it_cd = { 1, 0, JAM2, 4, 2, NULL, fq.txt_cd, NULL },
    .txt_cd = "CD",
    .it_cancel = { 1, 0, JAM2, 4, 2, NULL, fq.txt_cancel, NULL },
    .txt_cancel = "CANCEL",
    .g_ok = FQ_BUTTON(&fq.g_drives, 8, 24, 0, fq.frame_ok, fq.it_ok, 1),
    .g_drives = FQ_BUTTON(&fq.g_parent, 40, 56, 0, fq.frame_drives,
                          fq.it_drives, 2),
    .g_parent = FQ_BUTTON(&fq.g_cd, 104, 56, 0, fq.frame_parent,
                          fq.it_parent, 3),
    .g_cd = FQ_BUTTON(&fq.g_cancel, 168, 24, 0, fq.frame_cd, fq.it_cd, 4),
    .g_cancel = FQ_BUTTON(&fq.g_path, -58, 56, GFLG_RELRIGHT, fq.frame_cancel,
                          fq.it_cancel, 5),
    .strframe = {               /* rendering of g_path: both boxes, frame */
        FQ_BORDER(strframe, 0, -60, -6, 2, 1, &fq.strframe[1].b,
                  207, 4, 56, 4, 56, 15),
        FQ_BORDER(strframe, 1, -60, -6, 1, 2, &fq.strframe[2].b,
                  207, 5, 207, 15, 57, 15),
        FQ_BORDER(strframe, 2, -60, -6, 2, 1, &fq.strframe[3].b,
                  207, 20, 56, 20, 56, 31),
        FQ_BORDER(strframe, 3, -60, -6, 1, 2, &fq.strframe[4].b,
                  207, 21, 207, 31, 57, 31),
        FQ_BORDER(strframe, 4, -52, -6, 1, 2, &fq.strframe[5].b,
                  215, 0, 0, 0, 0, 35),
        FQ_BORDER(strframe, 5, -52, -6, 2, 1, NULL,
                  215, 1, 215, 35, 1, 35)
    },
    .it_path = { 1, 0, JAM2, -46, 0, NULL, fq.txt_path, &fq.it_file },
    .txt_path = "PATH:",
    .it_file = { 1, 0, JAM2, -46, 16, NULL, fq.txt_file, NULL },
    .txt_file = "FILE:",
    .g_path = {
        &fq.g_file, 60, -49, 144, 8, GFLG_RELBOTTOM | GFLG_TABCYCLE,
        GACT_RELVERIFY, GTYP_STRGADGET, &fq.strframe[0].b, NULL,
        &fq.it_path, 0, &fq.si_path, 1, NULL
    },
    .si_path = { .Buffer = fq.path, .UndoBuffer = fq.result, .MaxChars = 160 },
    .pad_113a = { ':', ':' },
    .g_file = {
        &fq.g_prop, 60, -33, 144, 8, GFLG_RELBOTTOM | GFLG_TABCYCLE,
        GACT_RELVERIFY, GTYP_STRGADGET, NULL, NULL,
        NULL, 0, &fq.si_file, 2, NULL
    },
    .si_file = { .Buffer = fq.file, .UndoBuffer = fq.result, .MaxChars = 64 },
    .listframe = {              /* drawn by redraw: list box, frame, prop box */
        FQ_BORDER(listframe, 0, 0, 0, 2, 1, &fq.listframe[1].b,
                  151, 4, 8, 4, 8, 55),
        FQ_BORDER(listframe, 1, 0, 0, 1, 2, &fq.listframe[2].b,
                  151, 5, 151, 55, 9, 55),
        FQ_BORDER(listframe, 2, 0, 0, 1, 2, &fq.listframe[3].b,
                  175, 0, 0, 0, 0, 59),
        FQ_BORDER(listframe, 3, 0, 0, 2, 1, &fq.listframe[4].b,
                  175, 1, 175, 59, 1, 59),
        FQ_BORDER(listframe, 4, 0, 0, 2, 1, &fq.listframe[5].b,
                  171, 10, 156, 10, 156, 53),
        FQ_BORDER(listframe, 5, 0, 0, 1, 2, NULL,
                  171, 11, 171, 53, 157, 53)
    },
    .g_prop = {
        &fq.g_up, -18, 14, 10, 44, GFLG_RELRIGHT,
        GACT_RELVERIFY | GACT_IMMEDIATE, GTYP_PROPGADGET, &fq.propimage, NULL,
        NULL, 0, &fq.propinfo, 1, NULL
    },
    .propimage = { .PlanePick = 1 },
    .propinfo = { AUTOKNOB | FREEVERT | PROPBORDERLESS, 0xffff, 0, 0xffff,
                  0x8000 },
    .g_up = {
        &fq.g_down, -18, 5, 12, 6, GFLG_RELRIGHT | GFLG_GADGIMAGE,
        GACT_RELVERIFY | GACT_IMMEDIATE, GTYP_BOOLGADGET, &fq.img_up, NULL,
        NULL, 0, NULL, 1, NULL
    },
    .img_up = { 0, 0, 12, 6, 2, NULL, 3, 0, NULL },
    .g_down = {
        NULL, -18, -66, 12, 6, GFLG_RELBOTTOM | GFLG_RELRIGHT | GFLG_GADGIMAGE,
        GACT_RELVERIFY | GACT_IMMEDIATE, GTYP_BOOLGADGET, &fq.img_down, NULL,
        NULL, 0, NULL, 1, NULL
    },
    .img_down = { 0, 0, 12, 6, 2, NULL, 3, 0, NULL },
    .line = {
        FQ_LINES8(0), FQ_LINES8(8), FQ_LINES8(16), FQ_LINES8(24),
        FQ_LINES8(32), FQ_LINES8(40), FQ_LINES8(48), FQ_LINES8(56)
    },
    .arrowdata = {
        0x0600, 0x0f00, 0x1f80, 0x3fc0, 0x7fe0, 0xfff0,     /* up */
        0x0600, 0x0f00, 0x1f80, 0x3fc0, 0x7fe0, 0xfff0,
        0xfff0, 0x7fe0, 0x3fc0, 0x1f80, 0x0f00, 0x0600,     /* down */
        0xfff0, 0x7fe0, 0x3fc0, 0x1f80, 0x0f00, 0x0600
    }
};
#undef FQ_BUTTON
#undef FQ_BORDER
#undef FQ_BUTTONFRAME
#undef FQ_SPC10
#undef FQ_SPC80
#undef FQ_LINE
#undef FQ_LINES8

#if defined(__mc68000__)
/* the layout of the original (m68k only: elsewhere pointers are wider) -
   every member at its address in the freqlib binary, no padding */
#include <stddef.h>
#define FQ_AT(m, a) static_assert(offsetof(struct fq_data, m) == (a) - 0xc04, \
                                  #m " not at " #a)
FQ_AT(defparams, 0xc04);    FQ_AT(pad_c14, 0xc14);
FQ_AT(temp, 0xc1a);         FQ_AT(deffile, 0xcba);
FQ_AT(deftitle, 0xcfa);     FQ_AT(intname, 0xd0f);
FQ_AT(dosname, 0xd21);      FQ_AT(gfxname, 0xd2d);
FQ_AT(dosbase, 0xd3e);      FQ_AT(intbase, 0xd42);
FQ_AT(gfxbase, 0xd46);      FQ_AT(win, 0xd4a);
FQ_AT(visible, 0xd4e);      FQ_AT(count, 0xd50);
FQ_AT(top, 0xd52);          FQ_AT(list, 0xd54);
FQ_AT(propact, 0xd58);      FQ_AT(scroll, 0xd5a);
FQ_AT(iaddr, 0xd5c);        FQ_AT(drives, 0xd60);
FQ_AT(keeplock, 0xd62);     FQ_AT(first, 0xd64);
FQ_AT(nw, 0xd66);           FQ_AT(frame_ok, 0xd96);
FQ_AT(frame_drives, 0xdce); FQ_AT(frame_parent, 0xe06);
FQ_AT(frame_cd, 0xe3e);     FQ_AT(frame_cancel, 0xe76);
FQ_AT(it_ok, 0xeae);        FQ_AT(txt_ok, 0xec2);
FQ_AT(pad_ec5, 0xec5);      FQ_AT(it_drives, 0xec6);
FQ_AT(txt_drives, 0xeda);   FQ_AT(pad_ee1, 0xee1);
FQ_AT(it_parent, 0xee2);    FQ_AT(txt_parent, 0xef6);
FQ_AT(pad_efd, 0xefd);      FQ_AT(it_cd, 0xefe);
FQ_AT(txt_cd, 0xf12);       FQ_AT(pad_f15, 0xf15);
FQ_AT(it_cancel, 0xf16);    FQ_AT(txt_cancel, 0xf2a);
FQ_AT(pad_f31, 0xf31);      FQ_AT(g_ok, 0xf32);
FQ_AT(g_drives, 0xf5e);     FQ_AT(g_parent, 0xf8a);
FQ_AT(g_cd, 0xfb6);         FQ_AT(g_cancel, 0xfe2);
FQ_AT(strframe, 0x100e);    FQ_AT(it_path, 0x10b6);
FQ_AT(txt_path, 0x10ca);    FQ_AT(it_file, 0x10d0);
FQ_AT(txt_file, 0x10e4);    FQ_AT(g_path, 0x10ea);
FQ_AT(si_path, 0x1116);     FQ_AT(pad_113a, 0x113a);
FQ_AT(path, 0x113c);        FQ_AT(g_file, 0x11dc);
FQ_AT(si_file, 0x1208);     FQ_AT(file, 0x122c);
FQ_AT(result, 0x126c);      FQ_AT(listframe, 0x142c);
FQ_AT(g_prop, 0x14d4);      FQ_AT(propimage, 0x1500);
FQ_AT(propinfo, 0x1514);    FQ_AT(g_up, 0x152a);
FQ_AT(img_up, 0x1556);      FQ_AT(g_down, 0x156a);
FQ_AT(img_down, 0x1596);    FQ_AT(line, 0x15aa);
FQ_AT(arrowdata, 0x39aa);
/* the fields the code addresses inside the NDK structures */
FQ_AT(nw.Width, 0xd6a);     FQ_AT(nw.Height, 0xd6c);
FQ_AT(nw.Title, 0xd80);     FQ_AT(nw.Screen, 0xd84);
FQ_AT(nw.Type, 0xd94);
FQ_AT(si_path.UndoBuffer, 0x111a);
FQ_AT(si_path.BufferPos, 0x111e);
FQ_AT(si_path.DispPos, 0x1122);
FQ_AT(si_file.BufferPos, 0x1210);
FQ_AT(si_file.DispPos, 0x1214);
FQ_AT(propinfo.VertPot, 0x1518);
FQ_AT(propinfo.VertBody, 0x151c);
FQ_AT(img_up.ImageData, 0x1560);
FQ_AT(img_down.ImageData, 0x15a0);
FQ_AT(g_prop.Height, 0x14de);
FQ_AT(line[0].itext, 0x15d6);
FQ_AT(line[0].text, 0x15ea);
FQ_AT(line[1], 0x163a);
static_assert(sizeof(struct fq_border) == 28, "struct fq_border");
static_assert(sizeof(struct fq_line) == 144, "struct fq_line");
static_assert(sizeof(struct fq_data) == 0x39da - 0xc04, "struct fq_data");
#undef FQ_AT
#endif
