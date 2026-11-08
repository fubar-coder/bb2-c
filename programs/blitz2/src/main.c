/*
 * main.c - chunk 01: blitz2.s lines 1-1911 (+ include wbheader, asmmac).
 *
 * Startup (main() replaces wbheader + begin), the token list, edstruct and
 * menus handed to the editor (blitz2:ted), the editor (comdata) wrappers,
 * the object-code templates (`objects`), the menu handlers, running the
 * compiled program in its own process, the libs directory scan and the
 * asmmac checker routines of the inline assembler.
 * See notes/survey-01.md.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include "blitz2.h"

/* library bases: asm `maths` (1265) and `int` (14866, editor's base).
   asm `dos` (wbheader:107) is libnix's DOSBase. */
struct Library *MathBase;
struct IntuitionBase *IntuitionBase;

/* ================================================== wbheader data */

struct Task *us;                        /* wb:108 */
struct WBStartup *wbmess;               /* wb:109 */
char progfile[192];                     /* wb:110 */
static const char wb_infoname[] = ".info";      /* wb:103 */
/* wb:105 wb_dosname "dos.library": DIFF: libnix opens dos.library */

/* libnix: WBStartup message when started from Workbench */
extern struct WBStartup *_WBenchMsg;

/* ================================================== edstruct (274) */


/* ================================================== token list (283-722)
 *
 * One object holding all token nodes in source order, so the memory image
 * is the one of the asm (struct token: next.l, num.w, name\0, help\0,
 * even).  The asm mnemonic tokens (asmmac, fnum..fnum+117) are followed by
 * their struct asment data at the next even address (makeasmtab relies on
 * that).  The nodes are listed in toklist.h; `tokens` (main.h) is the
 * first node of the list of the program's mode (ext.h): this one in BB2
 * mode, in AB3 mode blitz2-ab3's image with AmiBlitz3's names and new
 * tokens (ab3tok.c). */

#include "tokimg.h"

/* the nodes (toklist.h): a struct member per node, then its initialiser;
   IMG is the image being defined, the names are blitz2's (M picks b2) */
#define TK(id, next, num, name, help)       TKMEMBER(id, name, help)
#define TA(id, nx, n, name, ...)            TAMEMBER(id, name)
#define TKLAST(id, num, name, help)         TKMEMBER(id, name, help)
#define TK3(id, next, num, name, help)
#define TK3LAST(id, num, name, help)
#define M(b2, b3) b2

struct tokimage {
#include "toklist.h"
};

#undef TK
#undef TA
#undef TKLAST
#define TK(id, next, num, name, help)       { NX(next), num, name "\0" help },
#define TA(id, next, num, name, ...)        { NX(next), num, name "\0" "", { __VA_ARGS__ } },
#define TKLAST(id, num, name, help)         { NULL, num, name "\0" help },
#define IMG tokimg

struct tokimage tokimg = {              /* asm label: tokens = first node */
#include "toklist.h"
};

#undef TK
#undef TA
#undef TKLAST
#undef TK3
#undef TK3LAST
#undef M
#undef IMG

/* asm label `tokens` = first node: macro in main.h */

/* 274 edstruct: copied (28 bytes) by the editor into its own block */
struct edstruct edstruct = {
    (struct token *)&tokimg,      /* tokens */
    menus,
    (APTR)loadxtra_stub,        /* load .xtra routine */
    (APTR)savextra_stub,        /* save .xtra routine */
    (APTR)menu_stub,            /* Menu Routine */
    NULL,                       /* cleanup (?) */
    { ':', '"', '.', ';' }
};

/* 724 menus: "COMPILER\0", items "TEXT\0" + shortcut char, 0 */
const char menus[] =
    "COMPILER" "\0"
    "COMPILE AND RUN        " "\0" "X"
    "RUN                    " "\0" "M"
    "CREATE EXECUTABLE      " "\0" "E"
    "COMPILER OPTIONS       " "\0" "O"
    "CREATE RESIDENT        " "\0" ";"
    "VIEW NEWTYPES          " "\0" "-"
    "CLI ARGUMENT           " "\0" "="
    "CALCULATOR             " "\0" "H"
    "RELOAD ALL LIBS        " "\0" "\\"
    "LOAD DEBUG MODULE      " "\0" "."
#if !final
    "PRINT TOKENS" "\0" "\0"
#endif
    ;   /* + the implicit terminating 0 = asm `dc.b 0` */

/* 741 menuadds: menu dispatch (index = editor item number) */
static void (*const menuadds[])(void) = {
    menu0, menu10, menu1, menu4, menu5,
    menu6, menu9, menu7, menu8, menu_d,
#if !final
    menu3,
#endif
};

/* 748 */
UBYTE undobuffer[128] __attribute__((aligned(4)));

/* 750 */
char libsdir[] = "blitz2:userlibs/";

/* ================================================== hunk templates (265) */

ULONG tsthead[12] = {
    0x3f3, 0, 1, 0, 0,          /* tsthead: HUNK_HEADER, no names, 1 hunk */
    256, 0x3e9,                 /* tstsize1, HUNK_CODE */
    256,                        /* tstsize2 */
    0x3ec,                      /* tstend: HUNK_RELOC32 */
    0, 0,                       /* tstsize3, hunk 0 */
    0x3f2                       /* tstdone: HUNK_END */
};

/* ================================================== requesters (753-912)
   Templates for the editor's openreq (window size from Width/Height). */

static WORD nomemreqla1[] = { 303,0,0,0,0,67 };
static WORD nomemreqla3[] = { 303,1,303,67,1,67 };
static WORD nomemreqla5[] = { 207,0,0,0,0,11 };
static WORD nomemreqla7[] = { 207,1,207,11,1,11 };
static WORD nomemreqla9[] = { 207,0,0,0,0,11 };
static WORD nomemreqla11[] = { 207,1,207,11,1,11 };
static WORD nomemreqla13[] = { 119,0,0,0,0,11 };
static WORD nomemreqla15[] = { 119,1,119,11,1,11 };
static struct Border nomemreqla2 = { 0,0, 2,1,1,3, nomemreqla3, NULL };
static struct Border nomemreqbo1 = { 0,0, 1,2,1,3, nomemreqla1, &nomemreqla2 };
static struct Border nomemreqla6 = { 0,0, 2,1,1,3, nomemreqla7, NULL };
static struct Border nomemreqbo2 = { 0,0, 1,2,1,3, nomemreqla5, &nomemreqla6 };
static struct Border nomemreqla10 = { 0,0, 2,1,1,3, nomemreqla11, NULL };
static struct Border nomemreqbo3 = { 0,0, 1,2,1,3, nomemreqla9, &nomemreqla10 };
static struct Border nomemreqla14 = { 0,0, 2,1,1,3, nomemreqla15, NULL };
static struct Border nomemreqbo4 = { 0,0, 1,2,1,3, nomemreqla13, &nomemreqla14 };
static struct IntuiText nomemreqin1 = { 1,0,1, 12,6, NULL,
    (UBYTE *)"Not Enough Object Memory Available!", NULL };
static struct IntuiText nomemreqin2 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"INCREASE 150% - RECOMPILE", NULL };
static struct IntuiText nomemreqin3 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"MAKE SMALLEST - RECOMPILE", NULL };
static struct IntuiText nomemreqin4 = { 1,0,1, 4,2, NULL,
    (UBYTE *)"CANCEL COMPILE", NULL };
static struct Gadget nomemreqga3 = { NULL, 96,52,120,12, 0,5,1,
    &nomemreqbo4, NULL, &nomemreqin4, 0, NULL, 3, NULL };
static struct Gadget nomemreqga2 = { &nomemreqga3, 48,36,208,12, 0,5,1,
    &nomemreqbo3, NULL, &nomemreqin3, 0, NULL, 2, NULL };
static struct Gadget nomemreqga1 = { &nomemreqga2, 48,20,208,12, 0,5,1,
    &nomemreqbo2, NULL, &nomemreqin2, 0, NULL, 1, NULL };
struct Requester nomemreq = { NULL, 116,58,304,68, 0,0,
    &nomemreqga1, &nomemreqbo1, &nomemreqin1, 0, 0 };

/* 837 asmpnts: defined in stmts.c (chunk 03) */

static WORD verreqla1[] = { 279,0,0,0,0,59 };
static WORD verreqla3[] = { 279,1,279,59,1,59 };
static WORD verreqla7[] = { 103,0,0,0,0,11 };
static WORD verreqla9[] = { 103,1,103,11,1,11 };
static struct Border verreqla2 = { 0,0, 2,1,1,3, verreqla3, NULL };
static struct Border verreqbo1 = { 0,0, 1,2,1,3, verreqla1, &verreqla2 };
static struct Border verreqla8 = { 0,0, 2,1,1,3, verreqla9, NULL };
static struct Border verreqbo2 = { 0,0, 1,2,1,3, verreqla7, &verreqla8 };
/* 870 verreqla4: digits computed from `version` at assembly time */
static char verreqla4[] = { 'B','l','i','t','z',' ','B','a','s','i','c',' ',
    '2',' ','V','e','r','s','i','o','n',' ',
    '0' + blitz2version / 100, '.', '0' + blitz2version % 100 / 10,
    '0' + blitz2version % 10, 0 };
#if demo
static struct IntuiText verreqin3 = { 1,0,1, 64-8,30, NULL,
    (UBYTE *)"Demonstartion Version", NULL };
#endif
#if release
static struct IntuiText verreqin3 = { 1,0,1, 64+16,30, NULL,
    (UBYTE *)"Release Version", NULL };
#endif
static struct IntuiText verreqin2 = { 1,0,1, 12,18, NULL,
    (UBYTE *)"(C)opyright 1995 - Acid Software", &verreqin3 };
static struct IntuiText verreqin1 = { 1,0,1, 36,6, NULL,
    (UBYTE *)verreqla4, &verreqin2 };
static struct IntuiText verreqin4 = { 1,0,1, 4,2, NULL,
    (UBYTE *)" OKEE DOKEE ", NULL };
static struct Gadget verreqga1 = { NULL, 92,44,104,12, 0,5,1,
    &verreqbo2, NULL, &verreqin4, 0, NULL, 1, NULL };
struct Requester verreq = { NULL, 108,58,280,60, 0,0,
    &verreqga1, &verreqbo1, &verreqin1, 0, 0 };

/* ================================================== objects (914-1254)
 * Some object codes for compile: copied into the generated program or
 * patched in place (see survey-01 D).  Bytes from the vasm listing,
 * verified against the shipped binary. */
/* 548 bytes */
UWORD objects[] = {
    0xE148,                            /* +0   lslimm lsl #8,d0 */
    0x266D, 0x0000,                    /* +2   use move.l 0(a5),a3 */
    0xD6C0,                            /* +6   add d0,a3 (add.l in douse: Fixed (#939)) */
    0x2B4B, 0x0000,                    /* +8   move.l a3,0(a5) */
    0x203C, 0x0000, 0x0000,            /* +12  beginarr move.l #0,d0 */
    0xC0C1,                            /* +18  mularr mulu d1,d0 */
    0x2B40, 0x0000,                    /* +20  putarr move.l d0,0(a5) */
    0x343C, 0x0000,                    /* +24  doarr move #0,d2 */
    0x363C, 0x0000,                    /* +28  move #0,d3 */
    0x4A40,                            /* +32  tstword tst.w d0 */
    0x4A80,                            /* +34  tstlong tst.l d0 */
    0x4E71,                            /* +36  nops8 nop */
    0x4E71,                            /* +38  nop */
    0x4E71,                            /* +40  nop */
    0x4E71,                            /* +42  nop */
    0x4E71,                            /* +44  nops4 nop */
    0x4E71,                            /* +46  nops3 nop */
    0x4E71,                            /* +48  nop */
    0x4E71,                            /* +50  nop */
    0x266D, 0x0000,                    /* +52  dataget move.l 0(a5),a3 */
    0x2B4B, 0x0000,                    /* +56  dataput move.l a3,0(a5) */
    0x2F1B,                            /* +60  stlenget move.l (a3)+,-(a7) */
    0x524B,                            /* +62  stalign addq #1,a3 */
    0xC18B,                            /* +64  exg d0,a3 */
    0x0880, 0x0000,                    /* +66  bclr #0,d0 */
    0xC18B,                            /* +70  exg d0,a3 */
    0x524B,                            /* +72  dataletb addq #1,a3 */
    0x1B5B, 0x0000,                    /* +74  move.b (a3)+,0(a5) */
    0x524B,                            /* +78  dataletbl addq #1,a3 */
    0x195B, 0x0000,                    /* +80  move.b (a3)+,0(a4) */
    0x524B,                            /* +84  dataletb2 addq #1,a3 */
    0x149B,                            /* +86  move.b (a3)+,(a2) */
    0x3B5B, 0x0000,                    /* +88  dataletw move (a3)+,0(a5) */
    0x349B,                            /* +92  move (a3)+,(a2) */
    0x395B, 0x0000,                    /* +94  move (a3)+,0(a4) */
    0x2B5B, 0x0000,                    /* +98  dataletl move.l (a3)+,0(a5) */
    0x249B,                            /* +102 move.l (a3)+,(a2) */
    0x295B, 0x0000,                    /* +104 move.l (a3)+,0(a4) */
    0x504F,                            /* +108 nextfixw addq #8,a7 */
    0x4FEF, 0x000A,                    /* +110 nextfixw2 lea 10(a7),a7 */
    0x4FEF, 0x000C,                    /* +114 nextfixl lea 12(a7),a7 */
    0x4FEF, 0x000E,                    /* +118 nextfixl2 lea 14(a7),a7 */
    0x246F, 0x0004,                    /* +122 move.l 4(a7),a2 */
    0x3017,                            /* +126 move (a7),d0 */
    0xD112,                            /* +128 add.b d0,(a2) */
    0x246F, 0x0004,                    /* +130 move.l 4(a7),a2 */
    0x3017,                            /* +134 move (a7),d0 */
    0xD152,                            /* +136 add d0,(a2) */
    0x246F, 0x0008,                    /* +138 move.l 8(a7),a2 */
    0x2017,                            /* +142 move.l (a7),d0 */
    0xD192,                            /* +144 add.l d0,(a2) */
    0x246F, 0x0008,                    /* +146 move.l 8(a7),a2 */
    0x2012,                            /* +150 move.l (a2),d0 */
    0x2217,                            /* +152 move.l (a7),d1 */
    0x4EAE, 0xFFBE,                    /* +154 jsr -66(a6) */
    0x2480,                            /* +158 move.l d0,(a2) */
    0x246F, 0x0004,                    /* +160 move.l 4(a7),a2 */
    0x1012,                            /* +164 move.b (a2),d0 */
    0x322F, 0x0002,                    /* +166 move 2(a7),d1 */
    0x4A57,                            /* +170 tst (a7) */
    0x6A00, 0x0004,                    /* +172 bpl .skip */
    0xC141,                            /* +176 exg d0,d1 */
    0xB001,                            /* +178 .skip cmp.b d1,d0 */
    0x246F, 0x0004,                    /* +180 move.l 4(a7),a2 */
    0x3012,                            /* +184 move (a2),d0 */
    0x322F, 0x0002,                    /* +186 move 2(a7),d1 */
    0x4A57,                            /* +190 tst (a7) */
    0x6A00, 0x0004,                    /* +192 bpl .skip */
    0xC141,                            /* +196 exg d0,d1 */
    0xB041,                            /* +198 .skip cmp d1,d0 */
    0x246F, 0x0008,                    /* +200 move.l 8(a7),a2 */
    0x2012,                            /* +204 move.l (a2),d0 */
    0x222F, 0x0004,                    /* +206 move.l 4(a7),d1 */
    0x4A57,                            /* +210 tst (a7) */
    0x6A00, 0x0004,                    /* +212 bpl .skip */
    0xC141,                            /* +216 exg d0,d1 */
    0xB081,                            /* +218 .skip cmp.l d1,d0 */
    0x246F, 0x0008,                    /* +220 move.l 8(a7),a2 */
    0x2012,                            /* +224 move.l (a2),d0 */
    0x222F, 0x0004,                    /* +226 move.l 4(a7),d1 */
    0x082F, 0x0007, 0x0003,            /* +230 btst #7,3(a7) */
    0x6700, 0x0004,                    /* +236 beq .skip */
    0xC141,                            /* +240 exg d0,d1 */
    0x4EAE, 0xFFD6,                    /* +242 jsr -42(a6) */
    0x4840,                            /* +246 swapd0 swap d0 */
    0x2F0A,                            /* +248 pushindex move.l a2,-(a7) */
    0x45ED, 0x0000,                    /* +250 leaamp lea 0(a5),a2 */
    0x200A,                            /* +254 leaamp2 move.l a2,d0 */
    0x45EC, 0x0000,                    /* +256 leaampl lea 0(a4),a2 */
    0x2012,                            /* +260 stamp move.l (a2),d0 */
    0x246D, 0x0000,                    /* +262 stamp2 move.l 0(a5),a2 */
    0x584F,                            /* +266 stol addq #4,a7 */
    0x3F00,                            /* +268 pushd0wd move d0,-(a7) */
    0x2F00,                            /* +270 pushd0l move.l d0,-(a7) */
    0x3E3C, 0x0000,                    /* +272 movenumrep move #0,d7 */
    0x45EF, 0x0000,                    /* +276 preplibst lea 0(a7),a2 */
    0x4FEF, 0x0000,                    /* +280 fixlibst lea 0(a7),a7 */
    0x2F00,                            /* +284 pushdn move.l d0,-(a7) */
    0x2F0B,                            /* +286 movea3a7 move.l a3,-(a7) */
    0x202F, 0x0006,                    /* +288 movestst2 move.l 6(a7),d0 */
    0x3F6F, 0x0004, 0x0008,            /* +292 move 4(a7),8(a7) */
    0x2E9F,                            /* +298 move.l (a7)+,(a7) */
    0x202F, 0x0004,                    /* +300 movea7dn move.l 4(a7),d0 */
    0x2E9F,                            /* +304 move.l (a7)+,(a7) */
    0x2640,                            /* +306 moved0a3 move.l d0,a3 */
    0x202D, 0x0000,                    /* +308 movestdn move.l 0(a5),d0 */
    0x200B,                            /* +312 movea3dn move.l a3,d0 */
    0x2F00,                            /* +314 putstlen move.l d0,-(a7) */
    0x201F,                            /* +316 getstlen move.l (a7)+,d0 */
    0x2F57, 0x0000,                    /* +318 moverts move.l (a7),0(a7) */
    0x4FEF, 0x0000,                    /* +322 fixstack lea 0(a7),a7 */
    0x504F,                            /* +326 fixq addq #8,a7 */
    0x2E9F,                            /* +328 fix1 move.l (a7)+,(a7) */
    0x2640,                            /* +330 movedna3 move.l d0,a3 */
    0x45EC, 0x0000,                    /* +332 least lea 0(a4),a2 */
    0x2F2C, 0x0000,                    /* +336 funcst move.l 0(a4),-(a7) */
    0x584A,                            /* +340 addq4a2 addq #4,a2 */
    0x2F0B,                            /* +342 savesbase move.l a3,-(a7) */
    0x2B4B, 0x0000,                    /* +344 savesbase2 move.l a3,0(a5) */
    0x265F,                            /* +348 ressbase move.l (a7)+,a3 */
    0x2B4B, 0x0000,                    /* +350 ressbase2 move.l a3,0(a5) */
    0xCB4C,                            /* +354 exga4a5 exg.l a4,a5 (Devpac encoding; vasm: C94D) */
    0x4E54, 0x0000,                    /* +356 linka4 link a4,#0 */
    0x4E5C,                            /* +360 unlinka4 unlk a4 */
    0x2F0A,                            /* +362 a2toa7 move.l a2,-(a7) */
    0x245F,                            /* +364 a7toa2 move.l (a7)+,a2 */
    0x4EF9, 0x0000, 0x0000,            /* +366 gotocode jmp 0 */
    0x4EB9, 0x0000, 0x0000,            /* +372 gosubcode jsr 0 */
    0x0839, 0x0006, 0x00BF, 0xE001,    /* +378 mwait btst #6,$bfe001 */
    0x6600, 0xFFF6,                    /* +386 bne mwait */
    0x2000,                            /* +390 libreg move.l d0,d0 */
    0x2C6D, 0x0000,                    /* +392 libbase move.l 0(a5),a6 */
    0x4EAE, 0x0000,                    /* +396 libjsr jsr 0(a6) */
    0x202D, 0x0000,                    /* +400 regtransd move.l 0(a5),d0 */
    0x206D, 0x0000,                    /* +404 regtransa move.l 0(a5),a0 */
    0x267C, 0x0000, 0x0000,            /* +408 litget move.l #0,a3 */
    0x3F1B,                            /* +414 litget2 move (a3)+,-(a7) */
    0x2012,                            /* +416 movea2dn move.l (a2),d0 */
    0x202D, 0x0000,                    /* +418 moved3a5dn move.l 0(a5),d0 */
    0x7000,                            /* +422 stvarget2 moveq #0,d0 */
    0x2F00,                            /* +424 move.l d0,-(a7) */
    0x4A92,                            /* +426 tst.l (a2) */
    0x6700, 0x000E,                    /* +428 beq varget2skip */
    0x2052,                            /* +432 move.l (a2),a0 */
    0x5948,                            /* +434 subq #4,a0 */
    0x2E98,                            /* +436 move.l (a0)+,(a7) */
    0x4EB9, 0x0000, 0x0000,            /* +438 jsr 0 */
    0x201F,                            /* +444 addcode move.l (a7)+,d0 */
    0xD197,                            /* +446 add.l d0,(a7) */
    0x2F18,                            /* +448 pushlen move.l (a0)+,-(a7) */
    0x45ED, 0x0000,                    /* +450 leaa5d3a2 lea 0(a5),a2 */
    0x2F0A,                            /* +454 pusha2 move.l a2,-(a7) */
    0x2457,                            /* +456 geta2 move.l (a7),a2 */
    0x245F,                            /* +458 pulla2 move.l (a7)+,a2 */
    0x246F, 0x0004,                    /* +460 pulla2st move.l 4(a7),a2 */
    0x2E9F,                            /* +464 move.l (a7)+,(a7) */
    0x2F2D, 0x0000,                    /* +466 pusha3 move.l 0(a5),-(a7) */
    0x2B4B, 0x0000,                    /* +470 pusha32 move.l a3,0(a5) */
    0x2B5F, 0x0000,                    /* +474 pulla3 move.l (a7)+,0(a5) */
    0x266D, 0x0000,                    /* +478 geta3 move.l 0(a5),a3 */
    0x207C, 0x0000, 0x0000,            /* +482 numtoa0 move.l #0,a0 */
    0x247C, 0x0000, 0x0000,            /* +488 numtoa2 move.l #0,a2 */
    0x383C, 0x0000,                    /* +494 endarray move #0,d4 */
    0x343C, 0x0000,                    /* +498 preparray move #0,d2 */
    0x363C, 0x0000,                    /* +502 move #0,d3 */
    0x203C, 0x0000, 0x0004,            /* +506 dimbegin move.l #4,d0 */
    0x2B40, 0x0000,                    /* +512 dimb4 move.l d0,0(a5) */
    0xC0C1,                            /* +516 dimaf mulu d1,d0 */
    0x2B40, 0x0000,                    /* +518 dimfin move.l d0,0(a5) */
    0x2452,                            /* +522 movea0 move.l (a2),a2 */
    0xD5CA,                            /* +524 adda0a0 add.l a2,a2 */
    0xD5CA,                            /* +526 add.l a2,a2 */
    0x2C78, 0x0004,                    /* +528 getebase move.l 4.w,a6 */
    0x246D, 0x0000,                    /* +532 movea5a0 move.l 0(a5),a2 */
    0x246A, 0x0000,                    /* +536 movea0a0 move.l 0(a2),a2 */
    0x524A,                            /* +540 addqa0 addq #1,a2 */
    0x45EA, 0x0000,                    /* +542 leaa0a0 lea 0(a2),a2 */
    0xD4C0,                            /* +546 addrega2 add d0,a2 */
};

char tstname[] = "ram:test";            /* 1256 (unused) */

/* -----------end of object codes!-------------- */

/* ================================================== globals (1261-1296) */

static ULONG memat __attribute__((unused));                     /* 1261 (unused) */
static UWORD memlen __attribute__((unused));                    /* 1262 (unused); memstackf (1264):
                                           label without storage */
/* 1265 maths -> MathBase (above) */
UWORD regat;                            /* 1266 register being used! */
UWORD typelen;                          /* 1267 */
struct type *prevtype;                  /* 1268 */
UBYTE *prevloc;                         /* 1269 */
UWORD lastchar;                         /* 1270 */
struct type *thistype;                  /* 1271 */
/* 1272 errstack, 1273 errcont: replaced by errjmp/errmsg (parse.h) */
WORD linemode;                          /* 1274 0 if norm, 1 if newtype */
UWORD linesleft;                        /* 1275 */
struct editem *thisitem;                /* 1276 */
struct var *firstvar;                   /* 1277 */
struct alllocal *alllocals;             /* 1278 */
struct var *firstlocal;                 /* 1279 */
struct var *firstglob;                  /* 1280 */
WORD varoff;                            /* 1281 */
WORD locvaroff;                         /* 1282 */
struct editem *firstitem;               /* 1283 */
UWORD numlines;                         /* 1284 */
WORD returncode;                        /* 1285 */
struct ifnode *firstif;                 /* 1286 */

BPTR segadd;                            /* 1288 */
static char segname[] = "blitz2:ted";   /* 1289 */
static char mathsname[] = "mathffp.library";    /* 1291 */
static char debugname[] = "blitz2:defaultdbug"; /* 1483 */

/* 1294 cifbras -> tokens1.c (chunk 08), 1298 atokens -> compile.c (02) */

/* ================================================== begin (1353) */

/* blitz2.s:1353 actualbegin / 1357 begin - OKAY, Here it is - The actual
   compiler!  Returns the asm returncode (see main for what DOS gets). */
static WORD begin(void)
{
    struct m68kregs r;
    struct token *tok;
    APTR edmain;
    WORD gadget;
    UWORD count = 0;

    /* find out how many system tokens exist */
    tok = &tokens;
    for (;;) {
        count++;                        /* count is not used */
        if (!tok->next)
            break;
        tok = tok->next;
    }
    (void)count;
    lasttoken = (ULONG)tok;
    /* the editor gets the list of the program's mode (ext.h: decided
       before begin; blitz2's own in BB2 mode, as initialised) */
    edstruct.toklist = &tokens;
    makeasmtab();
    /* @1369 (set before optreqprep: Fixed (#8), without chip memory for
       the arrow images blitz2 gives up like at the other allocations) */
    returncode = -1;
    if (!optreqprep())
        goto fail0;
    MathBase = OldOpenLibrary((CONST_STRPTR)mathsname);
    if (!MathBase)
        goto fail0;
    /* @1376 */
    constpcat = (ULONG)AllocMem(constmaxpc, MEMF_PUBLIC);
    if (!constpcat)
        goto failme;
    constlibat = constpcat + constmaxpc;
    /* @1384: macrolen bytes (8192 here); fixed (#336): later changes of
       macrolen reallocate it (makemacbuff) */
    if (!makemacbuff())
        goto fail2;
    /* @1392 load the editor and call its init (seg+8 = ted `outside`):
       a1=edstruct in; Z=1 ok, a0=editor main entry, a1=comdata out */
    segadd = LoadSeg((CONST_STRPTR)ext_editor(segname));
    if (!segadd)
        goto fail2a;
    memset(&r, 0, sizeof(r));
    r.a[1] = (ULONG)&edstruct;
    if (!(callregs((UBYTE *)BADDR(segadd) + 8, &r) & CCR_Z))
        goto fail3;
    edmain = (APTR)r.a[0];              /* asm: move.l a0,-(a7) */
    comdata = (APTR *)r.a[1];
    /* @1405 */
    returncode = 0;
    IntuitionBase = (struct IntuitionBase *)CD_L(CD_EXTINT);
    ed_openreq(&verreq);
    /* @1411 */
    sleepon();
    linenumat = -1;
    makelibs();
    loadlibs();
    makeiallox();
    pointeron();
    /* click cancel of version requester... */
    do {
        ;
    } while (!ed_getinput(&gadget) || gadget != 1);
    /* grab a panic bit (failure of AllocSignal not checked, as in the
       original: not observable, #18 closed) */
    panicbit = (ULONG)(LONG)AllocSignal(-1);
    panicmask = 1UL << (panicbit & 31);
    /* loadseg default debugger (blitz2:defaultdbug) */
    dseg = LoadSeg((CONST_STRPTR)debugname);
    /* goto editor with filename! (returns when the user quits) */
    memset(&r, 0, sizeof(r));
    r.a[0] = (ULONG)ext_editorarg(progfile);
    callregs(edmain, &r);
    /* @1448 .skip */
    FreeSignal((LONG)panicbit);
    if (dseg)
        UnLoadSeg(dseg);
    freeemup();
    freeres();
    freelibs();
fail3:                                  /* 1460 */
    UnLoadSeg(segadd);
fail2a:                                 /* 1462 */
    freeallox();
    /* fail2b 1463 - Fixed (#336): freed with the size of the allocation
       (macrobuffsize); the original takes the current macrolen, which
       options loaded later (an .xtra) change */
    FreeMem(macrobuff, macrobuffsize);
fail2:                                  /* 1467 closedos */
    /* DIFF: asm closes dos.library here; libnix owns DOSBase and closes
       it at exit.  fail1a/fail1 (1468/1469): */
    FreeMem((APTR)constpcat, constmaxpc);
failme:                                 /* 1474 */
    CloseLibrary(MathBase);
fail0:                                  /* 1477 */
    optreqdone();
    flushlibs();
    return returncode;
}

/* ================================================== main (wbheader) */

/* wbheader:4 wb_header / fromcli / wb_exit + blitz2.s 253-261 mainbit:
   to be included to allow things to be run from WBENCH.  Gets the stack
   size (into asm d2 -> stackfuck) and the file to edit (progfile). */
int main(int argc, char **argv)
{
    ULONG stack;
    BPTR fh;
    int rc;
    WORD result;

    us = FindTask(NULL);                /* asm: 276(execbase) = ThisTask */
    if (ext_cmdline(argc, argv, &rc))
        return rc;
    if (argc == 0) {
        /* program was run from workbench - somehow, we have to eventually
           get stacksize into D2!  DIFF: libnix already got the message
           (asm: WaitPort/GetMsg on pr_MsgPort) and replies it at exit. */
        struct WBArg *wa;

        wbmess = _WBenchMsg;
        wa = wbmess->sm_ArgList;
        /* Read in the .info file to find stack size! */
        CurrentDir(wa[0].wa_Lock);      /* dir of BLITZ2! */
        strcpy(progfile, (char *)wa[0].wa_Name);    /* make name+.info */
        strcat(progfile, wb_infoname);
        fh = Open((CONST_STRPTR)progfile, MODE_OLDFILE);
        stack = 0;
        if (fh) {
            if (Read(fh, progfile, 0x4e) == 0x4e)
                stack = L_(progfile + 0x4a);        /* do_StackSize */
            /* Fixed (#19): closed after a short read too (wbheader closes
               it only after a complete one) */
            Close(fh);
        }
        /* Fixed (#20): progfile is cleared when the .info could not be
           read too (wbheader leaves "<name>.info" or the partial .info
           bytes in it, and the editor is asked to load them) */
        progfile[0] = 0;
        if (!stack)
            stack = 4000;               /* .openfailed */
        /* .openok: a project icon given? (asm: word at sm_NumArgs+2) */
        if ((UWORD)wbmess->sm_NumArgs >= 2) {
            CurrentDir(wa[1].wa_Lock);
            /* the old current dir lock is dropped as in the original
               (BUG(orig), #21).  DIFF: libnix's startup code restores
               the directory it saved before main at exit, so the port
               ends in the start directory, the original in the
               project's (or blitz2's) directory */
            strcpy(progfile, (char *)wa[1].wa_Name);
        }
    } else {
        /* fromcli: stacksize from CLI (asm: 4(a7) at entry) */
        struct Process *pr = (struct Process *)us;
        struct CommandLineInterface *cli = BADDR(pr->pr_CLI);

        stack = cli ? (ULONG)cli->cli_DefaultStack * 4 : 4000;
        wbmess = NULL;
        /* DIFF: asm copied the raw argument line up to the first space or
           LF; libnix splits (and unquotes) it into argv */
        if (argc > 1) {
            strncpy(progfile, argv[1], sizeof(progfile) - 1);
            progfile[sizeof(progfile) - 1] = 0;
        }
    }
    /* DIFF: dosopen (wb:97) - libnix opens dos.library */
    /* wb_allgo / mainbit */
#if !final
    stack = 4000;
#endif
    stackfuck = stack;
    result = begin();
    if (ext_result(&rc))
        return rc;
    /* Fixed (#22): DOS gets returncode, sign-extended as in blitz2.s:1479
       (wb_exit overwrites d0: 0 from the CLI, from the Workbench whatever
       ReplyMsg/Permit leave) */
    return result;
}

/* ================================================== editor interface */

/* blitz2.s:1482 t, 1488 handler: empty labels */

static void ed_call(LONG off, struct m68kregs *r)
{
    callregs(CD_FN(off), r);
}

/* comdata-8 screenclose */
void ed_screenclose(void)
{
    struct m68kregs r = {{0}};
    ed_call(-8, &r);
}

/* comdata-4 screenopen2 */
void ed_screenopen2(void)
{
    struct m68kregs r = {{0}};
    ed_call(-4, &r);
}

/* comdata+0 openreq: a0 = requester template; d0 = 0 if not opened */
ULONG ed_openreq(struct Requester *req)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)req;
    ed_call(0, &r);
    return r.d[0];
}

/* comdata+4 closereq */
void ed_closereq(void)
{
    struct m68kregs r = {{0}};
    ed_call(4, &r);
}

/* comdata+8 getinput: wait for input; d7=-1 gadget (d6 = GadgetID) */
BOOL ed_getinput(WORD *gadget)
{
    struct m68kregs r = {{0}};
    ed_call(8, &r);
    *gadget = (WORD)r.d[6];
    return (WORD)r.d[7] == -1;
}

/* comdata+12 error: a0 = message, modal requester */
void ed_error(const char *msg)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)msg;
    ed_call(12, &r);
}

/* comdata+16 detokallz */
void ed_detokallz(void)
{
    struct m68kregs r = {{0}};
    ed_call(16, &r);
}

/* comdata+20 tokall */
void ed_tokall(void)
{
    struct m68kregs r = {{0}};
    ed_call(20, &r);
}

/* comdata+24 ezfreq: a0 path, a1 name, a2 title; Z=1 cancel, else d0 */
char *ed_ezfreq(char *path, char *name, const char *title)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)path;
    r.a[1] = (ULONG)name;
    r.a[2] = (ULONG)title;
    if (callregs(CD_FN(24), &r) & CCR_Z)
        return NULL;
    return (char *)r.d[0];
}

/* comdata+28 refgads: a0 gadget, d0 number */
void ed_refgads(struct Gadget *g, WORD n)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)g;
    r.d[0] = (ULONG)(LONG)n;
    ed_call(28, &r);
}

/* comdata+32 getinput2: poll; returns d7 (0 = nothing), d6 = GadgetID */
LONG ed_getinput2(WORD *gadget)
{
    struct m68kregs r = {{0}};
    ed_call(32, &r);
    *gadget = (WORD)r.d[6];
    return (LONG)r.d[7];
}

/* comdata+56 tokone: tokenise one line in a0 */
void ed_tokone(UBYTE *line)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)line;
    ed_call(56, &r);
}

/* comdata+60 openio: open hendrix (program IO window) */
void ed_openio(void)
{
    struct m68kregs r = {{0}};
    ed_call(60, &r);
}

/* comdata+64 closeio */
void ed_closeio(void)
{
    struct m68kregs r = {{0}};
    ed_call(64, &r);
}

/* comdata+68 actgad: activate gadget in a0; d0 = ActivateGadget result */
ULONG ed_actgad(struct Gadget *g)
{
    struct m68kregs r = {{0}};
    r.a[0] = (ULONG)g;
    ed_call(68, &r);
    return r.d[0];
}

/* blitz2.s:1490 pointeron - jmp comdata+72 */
void pointeron(void)
{
    struct m68kregs r = {{0}};
    ed_call(72, &r);
}

/* blitz2.s:1494 sleepon - jmp comdata+76 */
void sleepon(void)
{
    struct m68kregs r = {{0}};
    ed_call(76, &r);
}

/* ================================================== menu handler */

/* blitz2.s:1498 menu - called by the editor (menu_stub): d6 = item,
   a5 = first line, d7 = number of lines; returns d0 = menuret */
void menu_c(struct m68kregs *r)
{
    firstitem = (struct editem *)r->a[5];
    numlines = (UWORD)r->d[7];
    linenumat = -1;
    menuret = -1;
    menuadds[(UWORD)r->d[6]]();
    /* asm: move menuret,d0 (word, upper half of d0 untouched) */
    r->d[0] = (r->d[0] & 0xffff0000UL) | (UWORD)menuret;
}

static char dtext[] = "Debug module to load";   /* 1509 */
static char dpath[7 + 192] = "Blitz2:";         /* 1511 */
static char dname[192];                         /* 1514 */
static const char *dname2;                      /* 1516 */
BPTR dseg;                                      /* 1485 */

/* blitz2.s:1518 menu_d - select debug module */
void menu_d(void)
{
    char *name = ed_ezfreq(dpath, dname, dtext);

    if (!name) {
        nodload();
        return;
    }
    loaddmod(name);                     /* falls through into loaddmod */
}

/* blitz2.s:1528 loaddmod - d0=name of module to load! */
void loaddmod(const char *name)
{
    jmp_buf jb, *olderr = errjmp;
    BPTR seg;

    dname2 = name;
    sleepon();
    errjmp = &jb;                       /* errstack=a7, errcont=nodload */
    if (setjmp(jb)) {
        errjmp = olderr;
        nodload();
        return;
    }
    seg = LoadSeg((CONST_STRPTR)dname2);
    if (!seg)
        dloaderr();
    if (dseg)
        UnLoadSeg(dseg);
    /* Fixed (#23): the busy pointer goes off without an old dseg too
       (blitz2.s calls pointeron only after UnLoadSeg) */
    pointeron();
    dseg = seg;
    errjmp = olderr;                    /* DIFF: asm never restored errstack */
    nodload();
}

/* blitz2.s:1544 nodload */
void nodload(void)
{
}

/* blitz2.s:1546 dloaderr */
void dloaderr(void)
{
    err("Can't LoadSeg debug module!");
}

static char xtext[] = "Name of Executable to create";  /* 1550 */
char xpathname[384];                    /* 1552 xpath, 1553 xname (see main.h) */
char *execname;                         /* 1554 */

/* blitz2.s:1556 menu1 - compile/save */
void menu1(void)
{
    char *name = ed_ezfreq(xpath, xname, xtext);
    UWORD saved;

    if (!name)
        return;
    /* O.K.... Now to create it */
    execname = name;
    makeexec = -1;
    saved = optreqga15.Flags;
    optreqga15.Flags |= GFLG_SELECTED;  /* make small! (bset #7,+13) */
    compile();
    if (!anyerrs)
        savefile();
    optreqga15.Flags = saved;
}

WORD closeed;                           /* 1584 */
WORD nocli;                             /* 1585 */

/* blitz2.s:1578 docloseed - CloseEd statement */
void docloseed(UWORD c)
{
    (void)c;
    closeed = -1;
}

/* blitz2.s:1581 donocli - NoCli statement */
void donocli(UWORD c)
{
    (void)c;
    nocli = -1;
}

/* blitz2.s:1587 flushc - exec CacheClearU if the library has it */
void flushc(void)
{
    if (SysBase->LibNode.lib_NegSize >= 636)
        CacheClearU();
}

volatile BPTR tasklock;                 /* 1675 */
struct Task * volatile panictask;       /* 1676 */
ULONG panicbit;                         /* 1677 */
ULONG panicmask;                        /* 1678 */
static char procname[] = "Blitz ][ Program Proc";   /* 1680 */

/* DIFF: the new process enters runtheprog from CreateProc; with -fbaserel
   the C code needs a4 = the base of the globals, which the process does
   not have: __saveds loads it (#338) */
static __saveds void runtheprog(void);

/* 1683-1687 procstart: a fake segment list for CreateProc: length, next
   BPTR, then the code (asm: runtheprog itself; here a jmp.l to it). */
static struct {
    ULONG len;                          /* dc.l 0 ;length */
    ULONG next;                         /* procstart: dc.l 0 ;next */
    UWORD jmp;                          /* jmp abs.l */
    void (*addr)(void);
} __attribute__((aligned(4))) procstart = { 0, 0, 0x4ef9, runtheprog };

/* blitz2.s:1595 menu0 - compile/run (falls into menu10) */
void menu0(void)
{
    makeexec = 0;
    closeed = 0;
    nocli = 0;
    compile();
    menu10();                           /* RUN! */
}

/* blitz2.s:1605 menu10 - run */
void menu10(void)
{
    if (anyerrs)
        return;                         /* dontrun */
    if (closeed)
        ed_screenclose();               /* close editor */
    else
        openstopreq(executing);         /* .noclose */
    /* .nopanic: find current dir... */
    tasklock = CurrentDir(0);
    CurrentDir(tasklock);
    panictask = NULL;
    if (CreateProc((CONST_STRPTR)procname, 0, MKBADDR(&procstart.next),
                   (LONG)stackfuck)) {
        /* OK, task should be running here! see if we get a panic! */
        for (;;) {                      /* .nostop */
            Wait(panicmask);
            if (stopit == 0)
                continue;
            if (stopit > 0)
                break;                  /* ended OK */
            if (!chkrealstop())
                continue;
            if (panictask)
                RemTask(panictask);     /* BUG(orig) (#24): no cleanup at
                                           all; not fixed on purpose: a clean
                                           abort of a running program is not
                                           worth the complexity */
            break;
        }
    }
    /* .notpanic.  Fixed (#25): also when CreateProc fails (blitz2.s jumps
       to .fatal, past endstop: the stop requester stayed open, its
       exception code installed) */
    if (!closeed)
        endstop();
    /* .fatal */
    firstlocal = NULL;
    firstglob = NULL;
    if (closeed)
        ed_screenopen2();
    /* .noopen */
    ActivateWindow((struct Window *)CD_L(CD_MYWINDOW2));
}

/* blitz2.s:1688 runtheprog - runs as the new process (procstart) */
static __saveds void runtheprog(void)
{
    struct m68kregs r = {{0}};

    panictask = FindTask(NULL);         /* 276(execbase) */
    CurrentDir(tasklock);
    if (!nocli)
        ed_openio();                    /* open hendrix */
    calcvbr();
    L_(vbr + 132) = (ULONG)&tokens;     /* trap #1 vector as mailbox */
    L_(vbr + 136) = (ULONG)firsttype;   /* trap #2 vector as mailbox */
    L_(vbr + 128 + 15 * 4) = (ULONG)directtrap;
    dirmode = 0;
    firstglob = NULL;
    firstlocal = NULL;
    dirpcat = pc;
    flushc();
    /* @1713 */
    r.d[2] = stackfuck;
    r.d[7] = 0x624c745aUL;              /* 'bLtZ' force cli-type run */
    r.a[0] = (ULONG)argreqla8;          /* cli arguement */
    r.a[1] = pcat;
    r.a[6] = (ULONG)SysBase;            /* asm: a6 = exec after calcvbr */
    /* runthing (1718): Run the thing! */
    callregs((APTR)pcat, &r);
    if (!nocli)
        ed_closeio();                   /* close the Hendrix IO Window */
    stopit = 1;                         /* ended OK! */
    Signal(us, panicmask);
}

/* blitz2.s:1734 calcvbr */
ULONG calcvbr(void)
{
    UWORD cpubits = SysBase->AttnFlags & 15;
    UWORD cpu = 0;
    ULONG base = 0;

    while (cpubits) {                   /* position of highest CPU bit */
        cpu++;
        cpubits >>= 1;
    }
    /* fixed (intended deviation, #42): from the 68010 on (cpu >= 1); the
       original reads the VBR only for cpu >= 2, a 68010 got 0 */
    if (cpu >= 1)
        base = getvbr();                /* DIFF: Supervisor() instead of
                                           SuperState/movec/UserState */
    vbr = base;
    return base;
}

/* blitz2.s:1751 flash - debug: flicker COLOR00 */
void flash(void)
{
    UWORD count = 0xffff;

    do {
        *(volatile UWORD *)0xdff180 = count;
    } while (count-- != 0);
}

/* blitz2.s:1758 flash2 */
void flash2(void)
{
    UWORD count = 0xffff;

    do {
        *(volatile UWORD *)0xdff180 = 15;
    } while (count-- != 0);
}

/* blitz2.s:1765 flushlibs - ask for 256 MEG! (forces library expunge).
   Fixed (#26): freed again if the allocation succeeds (blitz2.s neither
   checks nor frees it) */
void flushlibs(void)
{
    APTR m = AllocMem(1024 * 1024 * 256, MEMF_PUBLIC);

    if (m)
        FreeMem(m, 1024 * 1024 * 256);
}

char defname[] = "blitz2:deflibs";      /* 1770 */
ULONG deflibs;                          /* 1772 */
char acidname[] = "blitz2:acidlibs";    /* 1774 */
ULONG acidlibs;                         /* 1776 */

/* blitz2.s:1778 menu8 - reload all libs */
void menu8(void)
{
    sleepon();
    freeblitzlibs();
    makeblitzlibs();
    loadblitzlibs();
    pointeron();
}

/* blitz2.s:1784 checklib - size of file `name`, 0 if it can't be locked */
ULONG checklib(const char *name)
{
    BPTR lock = Lock((CONST_STRPTR)name, ACCESS_READ);

    if (!lock)
        return 0;
    /* 'deflibs' has been found! - pick up file length.
       Fixed (#27): a failed Examine counts as not found (blitz2.s takes
       fib_Size unchecked, a length left in the buffer) */
    if (!Examine(lock, namebufffib)) {
        UnLock(lock);
        return 0;
    }
    UnLock(lock);
    return (ULONG)namebufffib->fib_Size;
}

/* blitz2.s:1801 makelibs - find all lib dirs in 'libsdir' and make a list
   of 'em.  return FALSE (asm eq) if error */
BOOL makelibs(void)
{
    BOOL ok;

    if (ext_makelibs(&ok))
        return ok;
    deflibs = checklib(defname);
    acidlibs = checklib(acidname);
    return makeblitzlibs();             /* falls through */
}

/* blitz2.s:1816 makeblitzlibs */
BOOL makeblitzlibs(void)
{
    struct FileInfoBlock *fib = namebufffib;
    BPTR lock = Lock((CONST_STRPTR)libsdir, ACCESS_READ);

    if (!lock)
        return FALSE;
    if (!Examine(lock, fib) || fib->fib_DirEntryType < 0) {
        UnLock(lock);                   /* .done2 */
        return FALSE;
    }
    /* OK to do exnext on dir... */
    while (ExNext(lock, fib)) {
        struct libsnode *node, **link;

        if (fib->fib_DirEntryType < 0)
            continue;
        if (endininfo(fib->fib_FileName))
            continue;
        /* Fixed (#28): a name that does not fit the node is skipped
           (blitz2.s copies it without a limit, behind the node) */
        if (strlen((const char *)fib->fib_FileName) >= sizeof node->name)
            continue;
        node = doallocmem(sizeof(struct libsnode), MEMF_PUBLIC);
        /* insert sorted (case insensitive); &libslist acts as a node */
        link = &libslist;
        for (;;) {                      /* .insloop */
            struct libsnode *cur = *link;
            const UBYTE *mine = (const UBYTE *)fib->fib_FileName;
            const UBYTE *theirs;
            BOOL here = FALSE;

            if (!cur)
                break;
            theirs = (const UBYTE *)cur->name;
            for (;;) {                  /* .comloop */
                UWORD c0 = *mine++, c1;

                if (!c0) {
                    here = TRUE;        /* I'm shorter */
                    break;
                }
                c1 = *theirs++;
                if (!c1)
                    break;              /* I'm longer -> .next */
                if (tstalpha(c0))
                    c0 &= 255 - 32;
                if (tstalpha(c1))
                    c1 &= 255 - 32;
                if ((UBYTE)c0 == (UBYTE)c1)
                    continue;
                if ((UBYTE)c0 < (UBYTE)c1)
                    here = TRUE;        /* I'm less than */
                break;
            }
            if (here)
                break;
            link = &cur->next;          /* .next */
        }
        /* .here */
        node->next = *link;             /* next of last is next of me */
        *link = node;                   /* I am next of last */
        node->used = -1;                /* set 'Used' flag */
        /* the name fits (checked above, #28) */
        strcpy(node->name, (const char *)fib->fib_FileName);
    }
    /* .ok */
    UnLock(lock);
    return TRUE;
}

/* blitz2.s:1898 frlibslist - free up a libs list */
void frlibslist(void)
{
    struct libsnode *node = libslist;

    libslist = NULL;
    while (node) {
        struct libsnode *dead = node;

        node = node->next;
        FreeMem(dead, sizeof(struct libsnode));
    }
}

/* ================================================== asmmac checkers
 * the last parameter of the asm token macro is a 'do it yourself'
 * subroutine that allows you to do your own error chking for silly
 * opmodes!  size/ea's have already been created.
 * struct asmregs: opcode (d1), srcreg (d2), srcmode (d3), destreg (d4),
 * destmode (d5), size (d6).
 * return ne if no can do!  (noerror/eaerror/szerror/swapem: main.h) */

/* asmmac movemchk */
WORD movemchk(struct asmregs *r)
{
    ULONG modebit;
    UWORD reversed = 0;
    int i;

    if (r->srcmode == r->destmode)
        return eaerror;
    if (r->srcmode != 15) {
        if (r->destmode != 15)
            return eaerror;
        /* .regdest */
        modebit = 1UL << (r->srcmode & 31);
        if (modebit & 0x813)
            return eaerror;
        r->opcode |= 1 << 10;
        return swapem;
    }
    /* .regsrc */
    modebit = 1UL << (r->destmode & 31);
    if (modebit & 0xe0b)
        return eaerror;
    if (r->destmode != 4)
        return noerror;
    /* -(An): reverse the register mask (lsl d2 / roxr d6, 16 times; the
       asm builds it in d6 = size, which asmit does not read back) */
    for (i = 0; i < 16; i++) {
        UWORD x = r->srcreg >> 15;

        r->srcreg <<= 1;
        reversed = (reversed >> 1) | (x << 15);
    }
    r->srcreg = reversed;
    return noerror;
}

/* asmmac movepchk */
WORD movepchk(struct asmregs *r)
{
    if (r->srcmode == r->destmode)
        return eaerror;
    if (r->srcmode != 0)
        return noerror;
    r->opcode |= 1 << 7;
    return noerror;
}

/* asmmac movechk (move to/from ccr/sr/usp, size bits) */
WORD movechk(struct asmregs *r)
{
    if (r->srcmode >= 12) {
        if (r->destmode >= 12)
            return eaerror;
        if (r->srcmode == 12) {         /* .fromccr */
            r->opcode = 0x42c0;
            goto from;
        }
        if (r->srcmode == 13) {         /* .fromsr */
            r->opcode = 0x40c0;
            goto from;
        }
        if (r->srcmode == 14) {         /* .fromusp: to address reg? */
            if (r->destmode != 1)
                return eaerror;
            r->opcode = 0x4e68 | r->destreg;
            goto usp;
        }
        return eaerror;
    }
    /* .notfrom */
    if (r->destmode < 12) {             /* .dosize */
        r->opcode = 0x1000;
        if (r->size == 0)
            return noerror;
        r->opcode = 0x3000;
        if (r->size == 4)
            return noerror;
        r->opcode = 0x2000;
        return noerror;
    }
    if (r->srcmode >= 12)
        return eaerror;
    if (r->destmode == 12) {            /* .toccr */
        r->opcode = 0x44c0;
        goto to;
    }
    if (r->destmode == 13) {            /* .tosr */
        r->opcode = 0x46c0;
        goto to;
    }
    if (r->destmode == 14) {            /* .tousp */
        if (r->srcmode != 1)
            return eaerror;
        r->opcode = 0x4e60 | r->srcreg;
        goto usp;
    }
    return eaerror;
usp:
    r->srcreg = 0;
    r->srcmode = 0;
    r->destreg = 0;
    r->destmode = 0;
    return noerror;
from:
    if (r->size != 4)
        return szerror;
    r->srcmode = 0;
    r->srcreg = 0;
    return swapem;
to:
    if (r->size != 4)
        return szerror;
    r->destmode = 0;
    r->destreg = 0;
    return noerror;
}

/* asmmac exgchk */
WORD exgchk(struct asmregs *r)
{
    if (r->srcmode != r->destmode) {
        r->opcode |= 0x88;
        if (r->srcmode == 0)
            return noerror;
        return swapem;
    }
    /* .skip */
    if (r->srcmode != 0)
        r->opcode |= 0x48;
    else
        r->opcode |= 0x40;              /* .data */
    return noerror;
}

/* asmmac bitchk */
WORD bitchk(struct asmregs *r)
{
    if (r->srcmode == 0) {
        r->srcreg <<= 9;                /* review: asm shifts d2 itself */
        r->opcode |= r->srcreg;
        return noerror;
    }
    /* .notdn */
    if (r->size >= 8)
        return szerror;
    r->opcode ^= 0x900;
    return noerror;
}

/* asmmac aslchk2 - d0 = opcode of the memory form */
static WORD aslchk2(struct asmregs *r, UWORD memop)
{
    if ((WORD)r->destmode < 0) {        /* .word: memory shift */
        UWORD reg = r->srcreg, mode = r->srcmode;

        r->destmode = 0;                /* dummy d0 */
        if (r->srcmode == 0)
            return eaerror;
        if (r->size != 4)
            return szerror;
        r->opcode &= 0x100;
        r->opcode |= memop;
        if (mode >= 7) {
            reg = mode - 7;
            mode = 7;
        }
        r->opcode |= reg;
        r->opcode |= mode << 3;         /* d2/d3 restored (movem) */
        return noerror;
    }
    if (r->destmode != 0)
        return eaerror;
    r->size <<= 4;
    r->opcode |= r->size;
    r->opcode |= r->destreg;
    if (r->srcmode == 11)
        return noerror;
    if (r->srcmode != 0)
        return eaerror;
    r->srcreg <<= 9;                    /* review: asm shifts d2 itself */
    r->opcode |= r->srcreg;
    r->opcode |= 1 << 5;
    return noerror;
}

/* asmmac roxlchk */
WORD roxlchk(struct asmregs *r) { return aslchk2(r, 0xe4c0); }
/* asmmac rolchk */
WORD rolchk(struct asmregs *r) { return aslchk2(r, 0xe6c0); }
/* asmmac lslchk */
WORD lslchk(struct asmregs *r) { return aslchk2(r, 0xe2c0); }
/* asmmac aslchk */
WORD aslchk(struct asmregs *r) { return aslchk2(r, 0xe0c0); }

/* asmmac abcdchk (asm returned with Z=1 and d0 untouched on success =
   noerror for asmit) */
WORD abcdchk(struct asmregs *r)
{
    if (r->srcmode != r->destmode)
        return eaerror;
    if (r->srcmode != 0)
        r->opcode |= 1 << 3;
    return noerror;
}

/* asmmac addchk - if dest is dn, ok */
WORD addchk(struct asmregs *r)
{
    if (r->destmode == 0)
        return noerror;                 /* asm: rts with Z=1 */
    if (r->srcmode != 0)
        return eaerror;                 /* src not dn - fuct */
    r->opcode |= 1 << 8;                /* turn into dn,<ea> */
    return swapem;
}

/* asmmac andichk */
WORD andichk(struct asmregs *r)
{
    if (r->destmode == 12) {            /* dest ccr? */
        if (r->size != 0)
            return szerror;
        return noerror;
    }
    if (r->destmode != 13)              /* or sr? */
        return noerror;
    if (r->size != 4)
        return szerror;
    return noerror;
}
