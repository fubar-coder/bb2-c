/*
 * main.h - chunk 01 (blitz2.s 1-1911 + wbheader, options2.src.new data
 * is chunk 10, asmmac): startup (main() replaces wbheader), equates, the
 * token list / edstruct / menus handed to the editor, editor (comdata)
 * wrappers, object-code templates (`objects`), menu handlers, running the
 * compiled program, libs directory scan, asmmac checker routines.
 * See notes/survey-01.md.
 *
 * Defined in this line range but declared elsewhere (CONVENTIONS §6):
 *   atokens/atokensend -> compile.h (02), cifbras -> tokens1.h (08),
 *   asmpnts -> stmts.h (03), errstack/errcont -> errjmp/errmsg (parse.h),
 *   dos -> DOSBase (libnix), int -> IntuitionBase, maths -> MathBase
 *   (both defined in main.c, declared in blitz2.h).
 */
#ifndef BLITZ2_MAIN_H
#define BLITZ2_MAIN_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <workbench/startup.h>
#include "structs.h"
#include "glue.h"

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/* ================================================== equates (13-251) */

#define demo     0
#define release  (-1)
#define cripple  0
#define final    (-1)          /* -1: final version (no PRINT TOKENS menu) */
/* DIFF: 210 as in the compiler of the CD (blitz2.s says 195; docs/v2.10) */
#define blitz2version 210     /* asm: version (renamed: `version` clashes with NDK struct fields) */

/* internal library token numbers (passed to tokejsr / uselib) */
#define wbstart     0xbf00     /* lib 65533 wbstartuplib */
#define allocvars   65534      /* varslib */
#define localloc    0xc000     /* 65530 memlib */
#define locfree     0xc001
#define globalloc   0xc002
#define globfree    0xc003
#define newmem      0xc004
#define freelast    0xc005
#define setint      0xc100     /* 65520 intlib */
#define clrint      0xc101
#define newint      0xc102
#define oldint      0xc103
#define intvwait    0xc104
#define intcleanup  0xc105
#define intgoblitz  0xc106
#define intgoamiga  0xc107
#define goblitz     0xc200     /* 65510 switchlib */
#define goamiga     0xc201
#define goqamiga    0xc202
#define ownblit     0xc203
#define disownblit  0xc204
#define runerrlib   65500      /* runerrlib */
#define finalscheck 0xc300
#define remline     0xc301
#define contjmp     0xc304
#define chkret      0xc305
#define snxtchk     0xc306
#define lnxtchk     0xc307
#define stchk       0xc308
#define arrerr      0xc309
#define maxerr      0xc30a
#define gosup       0xc30b
#define gouse       0xc30c
#define eschk       0xc30d
#define sachk       0xc30e
#define arrchk      0xc30f
#define datachk     0xc310
#define inblitz     0xc311
#define inamiga     0xc312
#define casechkw    0xc313
#define casechkl    0xc314
#define currentchk  0xc315
#define ltobover    0xc316
#define wtobover    0xc317
#define ltowover    0xc318
#define qtobover    0xc319
#define ftobover    0xc31a
#define ftowover    0xc31b
#define ftolover    0xc31c
#define newarr      0xc400     /* 65434 arrayslib */
#define newlocarr   0xc401
#define arrmult     0xc402
#define alstat      0xc500     /* 65335 staticslib */
#define localstat   0xc501
#define copstring   0xc600     /* 65235 stringslib1 */
#define workstart   0xc601
#define lastring    0xc700     /* 65135 stringslib2 */
#define astring     0xc701
#define endjmp      0xc800     /* 65035 exitslib */
#define getffpbase  0xc900     /* 64935 ffplib */
#define quickmult   0xca00     /* 64835 lmullib */
#define longmult    0xca01
#define quickdiv    0xcb00     /* 64735 ldivlib */
#define longdiv     0xcb01
#define clrloc      0xcc00     /* 64635 clrlib */
#define getdstart   0xcd00     /* 64535 datalib */
#define strcomp     0xce00     /* 64435 strcomplib */
#define casestrcomp 0xce01
#define setmaxs     0xcf00     /* 64335 maxslib */
#define allocst     0xcf01
#define freest      0xcf02
#define maxlen      0xd000     /* 64135 maxlenlib */
#define lmaxlen     0xd001
#define qutofl      0xd300     /* 64080 floatquicklib */
#define fltoqu      0xd301
#define modbyte     0xd400     /* 64070 modlib */
#define modword     0xd401
#define modlong     0xd402
#define modquick    0xd403
#define modfloat    0xd404
#define debuglib    64060
#define dhandlerlib 1
#define rundebuglib 64050
#define ern         0xd200     /* 64040 errtraplib */
#define seterr      0xd201
#define clrerr      0xd202
#define errfail     0xd205

#define loclabch    39         /* "'" local label char */
#ifndef inclen
#define inclen      INCLEN     /* 1024, structs.h */
#endif
#define maxmacros   8
#define ifmacmax    8
#ifndef vcodelen
#define vcodelen    256
#endif
#ifndef mostres
#define mostres     MOSTRES    /* 8, structs.h */
#endif
#define userand     0x7f80
#define mathq       (247-55)

/* token numbering: 1..59 built-ins, fnum..tnum-1 asm mnemonics (118 + EndAsm),
   tnum..tnum+32 the second built-in block (tnum+3 "Ern" removed) */
#define lnum 60
#ifndef fnum
#define fnum 60
#endif
#ifndef tnum
#define tnum 179
#endif

/* ============================================ startup (wbheader / main) */

/* main() replaces wbheader + begin.  Stack size logic kept: from CLI the
   stack of this process (asm: 4(a7) at entry; C: pr_CLI->cli_DefaultStack*4),
   from WB do_StackSize of <tool>.info (read 0x4e bytes), 4000 on failure;
   result -> stackfuck (data.h).  progfile = first CLI arg (up to ' '/LF) or
   2nd WBArg name (CurrentDir to its lock). */
extern struct Task *us;               /* wb:108 our task (Signal target) */
extern struct WBStartup *wbmess;      /* wb:109 WB message or NULL */
extern char progfile[192];            /* wb:110 file to edit (also .info buffer) */
extern WORD returncode;               /* 1285 */

/* ====================================================== editor interface */

/* comdata (data.h: APTR *comdata) = ted.s `comdata` table.  Raw slots: */
#define CD_L(off)  (*(ULONG *)((UBYTE *)comdata + (off)))
#define CD_W(off)  (*(UWORD *)((UBYTE *)comdata + (off)))
#define CD_FN(off) ((APTR)CD_L(off))
/* data fields */
#define CD_COMBITMAP 36   /* L: plane pointer used for the racing stripe */
#define CD_MYSCREEN  40   /* L: editor screen */
#define CD_EXTINT    44   /* L: editor's IntuitionBase */
#define CD_MYWINDOW2 80   /* L: editor window */
#define CD_BMAPMUL   92   /* W: bytes per bitplane row */
#define CD_REQWINDOW 94   /* L: requester window (UserPort for stop signal) */

/* typed wrappers, all via callregs(); asm register usage in comments */
void  ed_screenclose(void);                 /* -8  close editor screen */
void  ed_screenopen2(void);                 /* -4  reopen it */
ULONG ed_openreq(struct Requester *req);    /* 0   a0=req template; d0=0 fail */
void  ed_closereq(void);                    /* 4 */
BOOL  ed_getinput(WORD *gadget);            /* 8   wait; TRUE if d7==-1 (gadget),
                                                   *gadget = GadgetID (d6, or menu code) */
void  ed_error(const char *msg);            /* 12  a0=msg, modal, waits OK */
void  ed_detokallz(void);                   /* 16 */
void  ed_tokall(void);                      /* 20 */
char *ed_ezfreq(char *path, char *name, const char *title);
                                            /* 24  a0 path,a1 name,a2 title;
                                                   Z=1 cancel -> NULL, else d0 */
void  ed_refgads(struct Gadget *g, WORD n); /* 28  a0 gadget, d0 count */
LONG  ed_getinput2(WORD *gadget);           /* 32  poll: returns d7 (0 = no
                                                   input, -1 gadget, *gadget id) */
void  ed_tokone(UBYTE *line);               /* 56  a0 line, tokenise in place */
void  ed_openio(void);                      /* 60  open program IO window */
void  ed_closeio(void);                     /* 64 */
ULONG ed_actgad(struct Gadget *g);          /* 68  a0 gadget; returns d0 (0 = failed) */
void  pointeron(void);                      /* 1490 comdata+72 normal pointer */
void  sleepon(void);                        /* 1494 comdata+76 busy pointer */
/* comdata+88 extalloc: doallocmem() in stmts.h */

/* data handed to the editor (exact byte layout, see structs.h) */
/* 283 first token; list ends at toktnum33.  The nodes are members of one
   object (struct tokimage, defined in main.c), `tokens` is its first node */
extern struct tokimage tokimg;
#define tokens (*(struct token *)&tokimg)
extern struct edstruct edstruct;      /* 274 */
extern const char menus[];            /* 724 "COMPILER\0" + items "TEXT\0" key, 0 */

/* callbacks called BY the editor: menu_stub/menu_c (glue.h), loadxtra_c /
   savextra_c (chunk 10).  menu_c: in d6 item, a5 firstitem, d7 numlines;
   out d0 = menuret.  Dispatch table menuadds (741) is static in main.c. */

/* ============================================= object-code templates */

/* 914-1254 `objects`: writable copy of the original bytes (verified against
   the shipped binary; exg a4,a5 = $CB4C as Devpac encodes it).  Patched in
   place by the users (move x,label+2) - see survey-01 D. */
extern UWORD objects[];
#define lslimm         (&objects[0])     /* +0 */
#define use            (&objects[1])   /* +2 */
#define beginarr       (&objects[6])   /* +12 */
#define usef           (&objects[6])   /* +12 */
#define mularr         (&objects[9])   /* +18 */
#define putarr         (&objects[10])   /* +20 */
#define doarr          (&objects[12])   /* +24 */
#define doarrf         (&objects[16])   /* +32 */
#define tstword        (&objects[16])   /* +32 */
#define tstlong        (&objects[17])   /* +34 */
#define nops8          (&objects[18])   /* +36 */
#define nops4          (&objects[22])   /* +44 */
#define nops3          (&objects[23])   /* +46 */
#define dataget        (&objects[26])   /* +52 */
#define nopsf          (&objects[26])   /* +52 */
#define dataput        (&objects[28])   /* +56 */
#define stlenget       (&objects[30])   /* +60 */
#define stalign        (&objects[31])   /* +62 */
#define dataletb       (&objects[36])   /* +72 */
#define stalignf       (&objects[36])   /* +72 */
#define dataletbf      (&objects[39])   /* +78 */
#define dataletbl      (&objects[39])   /* +78 */
#define dataletb2      (&objects[42])   /* +84 */
#define dataletblf     (&objects[42])   /* +84 */
#define dataletb2f     (&objects[44])   /* +88 */
#define dataletw       (&objects[44])   /* +88 */
#define dataletl       (&objects[49])   /* +98 */
#define nextfixw       (&objects[54])   /* +108 */
#define nextfixw2      (&objects[55])   /* +110 */
#define nextfixl       (&objects[57])   /* +114 */
#define nextfixl2      (&objects[59])   /* +118 */
#define nextb          (&objects[61])   /* +122 */
#define nextbf         (&objects[65])   /* +130 */
#define nextw          (&objects[65])   /* +130 */
#define nextl          (&objects[69])   /* +138 */
#define nextwf         (&objects[69])   /* +138 */
#define nextf          (&objects[73])   /* +146 */
#define nextlf         (&objects[73])   /* +146 */
#define forcompb       (&objects[80])   /* +160 */
#define nextff         (&objects[80])   /* +160 */
#define forcompbf      (&objects[90])   /* +180 */
#define forcompw       (&objects[90])   /* +180 */
#define forcompl       (&objects[100])   /* +200 */
#define forcompwf      (&objects[100])   /* +200 */
#define forcompf       (&objects[110])   /* +220 */
#define forcomplf      (&objects[110])   /* +220 */
#define forcompf2      (&objects[121])   /* +242 */
#define forcompff      (&objects[121])   /* +242 */
#define forcompf2f     (&objects[123])   /* +246 */
#define swapd0         (&objects[123])   /* +246 */
#define pushindex      (&objects[124])   /* +248 */
#define leaamp         (&objects[125])   /* +250 */
#define leaamp2        (&objects[127])   /* +254 */
#define leaampl        (&objects[128])   /* +256 */
#define stamp          (&objects[130])   /* +260 */
#define stamp2         (&objects[131])   /* +262 */
#define stol           (&objects[133])   /* +266 */
#define pushd0wd       (&objects[134])   /* +268 */
#define pushd0l        (&objects[135])   /* +270 */
#define movenumrep     (&objects[136])   /* +272 */
#define preplibst      (&objects[138])   /* +276 */
#define fixlibst       (&objects[140])   /* +280 */
#define pushdn         (&objects[142])   /* +284 */
#define movea3a7       (&objects[143])   /* +286 */
#define movestst2      (&objects[144])   /* +288 */
#define movea7dn       (&objects[150])   /* +300 */
#define movestst2f     (&objects[150])   /* +300 */
#define movea7dnf      (&objects[153])   /* +306 */
#define moved0a3       (&objects[153])   /* +306 */
#define movestdn       (&objects[154])   /* +308 */
#define movea3dn       (&objects[156])   /* +312 */
#define putstlen       (&objects[157])   /* +314 */
#define getstlen       (&objects[158])   /* +316 */
#define moverts        (&objects[159])   /* +318 */
#define fixstack       (&objects[161])   /* +322 */
#define fixq           (&objects[163])   /* +326 */
#define fix1           (&objects[164])   /* +328 */
#define movedna3       (&objects[165])   /* +330 */
#define least          (&objects[166])   /* +332 */
#define funcst         (&objects[168])   /* +336 */
#define addq4a2        (&objects[170])   /* +340 */
#define savesbase      (&objects[171])   /* +342 */
#define savesbase2     (&objects[172])   /* +344 */
#define ressbase       (&objects[174])   /* +348 */
#define savesbasef     (&objects[174])   /* +348 */
#define ressbase2      (&objects[175])   /* +350 */
#define exga4a5        (&objects[177])   /* +354 */
#define ressbasef      (&objects[177])   /* +354 */
#define linka4         (&objects[178])   /* +356 */
#define unlinka4       (&objects[180])   /* +360 */
#define a2toa7         (&objects[181])   /* +362 */
#define a7toa2         (&objects[182])   /* +364 */
#define gotocode       (&objects[183])   /* +366 */
#define gosubcode      (&objects[186])   /* +372 */
#define mwait          (&objects[189])   /* +378 */
#define libreg         (&objects[195])   /* +390 */
#define mwaitf         (&objects[195])   /* +390 */
#define libbase        (&objects[196])   /* +392 */
#define libjsr         (&objects[198])   /* +396 */
#define regtransd      (&objects[200])   /* +400 */
#define regtransa      (&objects[202])   /* +404 */
#define litget         (&objects[204])   /* +408 */
#define litget2        (&objects[207])   /* +414 */
#define movea2dn       (&objects[208])   /* +416 */
#define moved3a5dn     (&objects[209])   /* +418 */
#define stvarget2      (&objects[211])   /* +422 */
#define stvarget2f     (&objects[219])   /* +438 */
#define addcode        (&objects[222])   /* +444 */
#define varget2skip    (&objects[222])   /* +444 */
#define addcodef       (&objects[224])   /* +448 */
#define pushlen        (&objects[224])   /* +448 */
#define leaa5d3a2      (&objects[225])   /* +450 */
#define pusha2         (&objects[227])   /* +454 */
#define geta2          (&objects[228])   /* +456 */
#define pulla2         (&objects[229])   /* +458 */
#define pulla2st       (&objects[230])   /* +460 */
#define pulla2stf      (&objects[233])   /* +466 */
#define pusha3         (&objects[233])   /* +466 */
#define pusha32        (&objects[235])   /* +470 */
#define pulla3         (&objects[237])   /* +474 */
#define pusha3f        (&objects[237])   /* +474 */
#define geta3          (&objects[239])   /* +478 */
#define pulla3f        (&objects[239])   /* +478 */
#define geta3f         (&objects[241])   /* +482 */
#define numtoa0        (&objects[241])   /* +482 */
#define numtoa2        (&objects[244])   /* +488 */
#define endarray       (&objects[247])   /* +494 */
#define preparray      (&objects[249])   /* +498 */
#define dimbegin       (&objects[253])   /* +506 */
#define preparrayf     (&objects[253])   /* +506 */
#define dimb4          (&objects[256])   /* +512 */
#define dimbeginf      (&objects[256])   /* +512 */
#define dimaf          (&objects[258])   /* +516 */
#define dimb4f         (&objects[258])   /* +516 */
#define dimaff         (&objects[259])   /* +518 */
#define dimfin         (&objects[259])   /* +518 */
#define dimfinf        (&objects[261])   /* +522 */
#define movea0         (&objects[261])   /* +522 */
#define adda0a0        (&objects[262])   /* +524 */
#define getebase       (&objects[264])   /* +528 */
#define getebasef      (&objects[266])   /* +532 */
#define movea5a0       (&objects[266])   /* +532 */
#define movea0a0       (&objects[268])   /* +536 */
#define addqa0         (&objects[270])   /* +540 */
#define leaa0a0        (&objects[271])   /* +542 */
#define addrega2       (&objects[273])   /* +546 */
extern char tstname[];                /* 1256 "ram:test" (unused) */

/* 265-272 hunk file pieces, one contiguous array as in the asm:
   write(tsthead,32) = HUNK_HEADER..HUNK_CODE size; write(tstend,12) =
   HUNK_RELOC32,count,0; write(tstsize3,4) as zero long; write(tstdone,4) */
extern ULONG tsthead[12];
#define tstsize1 (&tsthead[5])        /* hunk size in longs (patched) */
#define tstsize2 (&tsthead[7])        /* code size in longs (patched) */
#define tstend   (&tsthead[8])        /* $3ec */
#define tstsize3 (&tsthead[9])        /* reloc count (patched), then hunk 0 */
#define tstdone  (&tsthead[11])       /* $3f2 */
/* fixed (#943): offsets per group of the executable's HUNK_RELOC32 block
   (V36+ LoadSeg loads no group of more) */
#define RELOC32_GROUP 65535UL

/* ======================================================= requesters */

extern struct Requester nomemreq;     /* 753 "Not Enough Object Memory"
                                         gadget ids 1 +150%, 2 smallest, 3 cancel */
extern struct Requester verreq;       /* 839 version box, gadget id 1 OK */

/* ======================================================= globals */

extern UBYTE undobuffer[128];         /* 748 long aligned; lib path buffer,
                                         argreq string gadget undo buffer */
extern char libsdir[];                /* 750 "blitz2:userlibs/" */
extern UWORD regat;                   /* 1266 register being used */
extern UWORD typelen;                 /* 1267 NEWTYPE size being built */
extern struct type *prevtype;         /* 1268 */
extern UBYTE *prevloc;                /* 1269 saved srcptr (getchar) */
extern UWORD lastchar;                /* 1270 */
extern struct type *thistype;         /* 1271 NEWTYPE being built */
extern WORD linemode;                 /* 1274 0 normal, else NEWTYPE mode */
extern UWORD linesleft;               /* 1275 */
extern struct editem *thisitem;       /* 1276 current line (starts at
                                         (struct editem *)&firstitem) */
extern struct var *firstvar;          /* 1277 */
extern struct alllocal *alllocals;    /* 1278 */
extern struct var *firstlocal;        /* 1279 */
extern struct var *firstglob;         /* 1280 */
extern WORD varoff;                   /* 1281 global var offset (a5) */
extern WORD locvaroff;                /* 1282 local var offset (a4) */
extern struct editem *firstitem;      /* 1283 first editor line (pseudo node) */
extern UWORD numlines;                /* 1284 */
extern struct ifnode *firstif;        /* 1286 */
extern BPTR segadd;                   /* 1288 editor seglist */

extern BPTR dseg;                     /* 1485 debugger seglist */
extern char xpathname[384];           /* 1552 xpath + 1553 xname: one block,
                                         loadxtra/savextra transfer both at once */
#define xpath (xpathname)              /* 1552 exe requester path (192) */
#define xname (xpathname + 192)        /* 1553 exe requester name (192) */
extern char *execname;                /* 1554 exe name (from ezfreq) */
extern WORD closeed;                  /* 1584 CloseEd */
extern WORD nocli;                    /* 1585 NoCli */
extern volatile BPTR tasklock;        /* 1675 cwd for the program (borrowed) */
extern struct Task * volatile panictask; /* 1676 program process */
extern ULONG panicbit;                /* 1677 */
extern ULONG panicmask;               /* 1678 1<<panicbit */
extern char defname[];                /* 1770 "blitz2:deflibs" */
extern ULONG deflibs;                 /* 1772 its size (0 = absent) */
extern char acidname[];               /* 1774 "blitz2:acidlibs" */
extern ULONG acidlibs;                /* 1776 */

/* ======================================================= routines */

void menu0(void);          /* 1595 compile and run (falls into menu10) */
void menu10(void);         /* 1605 run */
void menu1(void);          /* 1556 create executable */
void menu8(void);          /* 1778 reload all libs */
void menu_d(void);         /* 1518 load debug module */
void loaddmod(const char *name); /* 1528 errjmp -> nodload on failure */
void nodload(void);        /* 1544 (empty) */
NORETURN void dloaderr(void); /* 1546 err("Can't LoadSeg debug module!") */
void docloseed(UWORD c);   /* 1578 CloseEd statement (atokens) */
void donocli(UWORD c);     /* 1581 NoCli statement (atokens) */
void flushc(void);         /* 1587 CacheClearU if exec has it (NegSize>=636) */
ULONG calcvbr(void);       /* 1734 vbr = (AttnFlags highest bit >= 010) ?
                              getvbr() : 0; also returned.  Fixed (#42):
                              the original 0 on a 68010 */
void flash(void);          /* 1751 debug: COLOR00 flicker */
void flash2(void);         /* 1758 */
void flushlibs(void);      /* 1765 AllocMem(256MB) to expunge libs */
ULONG checklib(const char *name); /* 1784 file size via Examine, 0 if no lock */
BOOL makelibs(void);       /* 1801 deflibs/acidlibs, then makeblitzlibs */
BOOL makeblitzlibs(void);  /* 1816 scan libsdir into libslist (sorted);
                              asm: d0=-1/NE ok, 0/EQ error -> TRUE ok */
void frlibslist(void);     /* 1898 free libslist */

/* run program (menu10): CreateProc(procname, 0, BPTR of a static long
   aligned fake segment {0, 0, jmp.l runtheprog}, stackfuck) - plain data in
   main.c, no extra asm.  runtheprog (child process) calls the generated
   code via callregs (d2=stackfuck, d7='bLtZ', a0=argreqla8, fn=pcat).
   Globals shared with the child: stopit, panictask, tasklock (volatile). */

/* =============================================== asmmac checkers (asmit) */

/* called through asment.chk with struct asmregs (structs.h); result:
   szerror -2, eaerror -1, noerror 0, swapem 1 (asm d0 + CCR) */
#define noerror  0
#define eaerror  (-1)
#define szerror  (-2)
#define swapem   1
extern asmchkfn movemchk, movepchk, movechk, exgchk, bitchk,
                roxlchk, rolchk, lslchk, aslchk, abcdchk, addchk, andichk;

/* asm alias */
#define maths MathBase

#if BLITZ2_AB3
/* =============================================== AB3 mode (blitz2-ab3) */

#include "xtra.h"

/* the mode of the program being compiled (enum xtra_mode, shared/xtra):
   XTRA_MODE_BB2 or XTRA_MODE_AB3 (docs/ab3/variant.md).  One mode per
   compile; an editor session is always BB2 */
extern int progmode;
#endif

#endif
