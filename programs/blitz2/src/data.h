/*
 * data.h - chunk 10 (blitz2.s 14656-16015 + options2.src.new, typereq,
 * options.src, calcreq): the `mydata` globals, the built-in type nodes,
 * the Type-Info browser (menu6), the .xtra load/save callbacks, the
 * Options requester (menu4), Calculator (menu7) and CLI argument
 * requester (menu9).  See notes/survey-10.md.
 *
 * Not here although defined in this line range:
 *   convtable, operators, opeq..opge  -> eval.h (chunk 06)
 *   modetable                         -> compile.h (chunk 02)
 *   int (IntuitionBase)               -> blitz2.h / chunk 01
 *   asmmac checker routines           -> main.h (chunk 01)
 */
#ifndef BLITZ2_DATA_H
#define BLITZ2_DATA_H

#include <exec/types.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include "structs.h"
#include "glue.h"

#ifndef vcodelen
#define vcodelen 256            /* blitz2.s:221 (chunk 01 equate) */
#endif

/* ===================================================================
 * mydata (14658-15016)   dc = WORD, dc.l = ULONG / pointer
 * =================================================================== */

extern ULONG vbr;                       /* 14660 saved VBR (68010+) */
#define numclibs 3                      /* 14662 */
extern WORD  cfetchmode;                /* 14664 1 = fetching a constant (uselib intercepted) */
extern UWORD constlibs[numclibs];       /* 14667 64935,64735,64835 libs allowed in const eval */
extern ULONG constpcat;                 /* 14671 PC buffer of the constant evaluator */
extern ULONG constlibat;                /* 14672 */
extern ULONG constmaxpc;                /* 14673 =1024 */
extern ULONG stackfuck;                 /* 14675 stack size (wbheader d2) */
/* 14676-14678: last resident node of each list; initialised to the
   address of the list head variable (node trick: next at offset 0) */
extern struct label *lastrescon;        /* = (struct label *)&firstlabel */
extern struct macro *lastresmac;        /* = (struct macro *)&firstmacro */
extern struct type  *lastrestype;       /* = (struct type *)&firsttype (= &stringtype) */
extern WORD  rescnt;                    /* 14679 # residents */
extern struct resentry *resat;          /* 14681 top visible resident in optreq (=resbuff) */
extern WORD  linedone;                  /* 14682 */
extern ULONG dirpcat;                   /* 14683 direct mode pc */
extern WORD  diropt;                    /* 14684 */
extern ULONG dira4, dira5, dira6;       /* 14685 direct mode saved regs */
extern ULONG dirstack;                  /* 14688 */
/* 14689 editor callback table (ted.s comdata): longword slots, mostly
   code pointers (0 openreq, 8 getinput, 28 refgads, 32 getinput2,
   56 tokone, 68 actgad ...), some data (40 = myscreen).  Index = off/4.
   Use the ed_*() wrappers of main.h. */
extern APTR *comdata;

/* --- built-in type nodes (14833-14860) ---------------------------
 * alltypes -> bytetype -> wordtype -> longtype -> quicktype -> floattype
 *          -> addresstype -> stringtype -> (user NewTypes)
 * The asm label `firsttype` sits ON stringtype, so the long at firsttype
 * is stringtype.next (= first user type) and `lea firsttype,a2` is the
 * list head of the user types.  firsttype is therefore an lvalue macro:
 *   firsttype                         first NewType (struct type *)
 *   (struct type *)&firsttype         list head for findtype/addhere
 * `lea alltypes,a2` (all types incl. built-ins) = (struct type *)&alltypes.
 * The nodes are defined with GCC's flexible-array-member initialiser
 * (name "b".."s", len 13). */
extern struct type *alltypes;           /* 14833 = &bytetype */
extern struct type bytetype, wordtype, longtype, quicktype, floattype,
                   addresstype, stringtype;
#define firsttype (stringtype.next)     /* 14853 */
extern struct type dummytype;           /* 14858 {0,0,0,len 1} */

extern char  incdir[96];                /* 14862 INCDIR path */
extern WORD  dirmode;                   /* 14865 direct mode flag */
extern ULONG maxsat;                    /* 14868 data1 info pos */
extern WORD  nummaxs;                   /* 14869 # libs with an object maximum */
extern WORD  maxsused;                  /* 14871 maximums used in prog */
/* defaults saved with source */
extern WORD  dark;                      /* 14875 dark compile? */
extern WORD  qlab;                      /* 14876 quick lab allocate */
extern struct type *defaulttype;        /* 14877 = &quicktype */

extern WORD  anyerrs;                   /* 14882 =-1, any errors in compile */
extern WORD  makeexec;                  /* 14883 making an executable */
extern ULONG noinits;                   /* 14884 init routine flag (6.l = none); low word
                                           written separately (noinits+2) */
extern ULONG letstart;                  /* 14886 */
extern WORD  nomemleft;                 /* 14887 */
extern char  lastbiglab[128];           /* 14888 last non-local label */
extern ULONG locdatast;                 /* 14890 */
extern struct addrnode *firstpend;      /* 14891 */
/* one pass compiler buffers: xxxat = base, xxx = current address */
extern ULONG data2at, data2, data1at, data1, pcat, pc, libat, lib;
extern ULONG allat;                     /* 14903 highest allocated code location */
extern struct asment dummyasm;        /* 14905 all zero */

/* assembler */
extern WORD  constmode;                 /* 14913 0 norm, <>0 assembler */
extern WORD  asmtype;                   /* 14914 0 = no prog reference */
/* 14915 extraword, extraword2: contiguous, also read/written as one long
   (asmit: move.l extraword,d6 / swap) -> L_(extrawords) */
extern UWORD extrawords[2];
#define extraword  (extrawords[0])
#define extraword2 (extrawords[1])
extern UBYTE *asmbuff;                  /* 14918 */
extern WORD  asmlen, asmlen2;           /* 14919 src/dest text len */
extern struct asmnode *firstasm;        /* 14921 expressions to resolve */

extern WORD  macnum;                    /* 14924 */
extern UBYTE *nextinc0;                 /* 14925 address of next 0 in include file */
extern ULONG titleat;                   /* 14926 compile streaks */
extern WORD  titleadd[5];               /* 14927 */
extern WORD  connest;                   /* 14928 conditional nest level */
extern struct constant *firstconst;     /* 14929 */
extern WORD  numarg;                    /* 14930 # macro arguments */
extern UBYTE *myoline;                  /* 14931 macro done yet? (old myline) */
extern UBYTE *myline;                   /* 14932 line build buffer */
extern UBYTE *mylinef;                  /* 14934 */
extern WORD  concomstack[32];           /* 14935 CNIF stack */
extern WORD  *concomsp;                 /* 14936 = concomstack */
extern WORD  oldqflag;                  /* 14937 */
extern ULONG lastconloc;                /* 14939 */
extern WORD  lastconop;                 /* 14940 */
extern struct fornode *firstfor;        /* 14941 */
extern WORD  pushflag;                  /* 14942 */
extern WORD  sbgot;                     /* 14943 string got 2 */
extern WORD  usertype;                  /* 14944 */
extern WORD  userp;                     /* 14945 */
extern struct libsnode *libslist;       /* 14948 */
extern WORD  stackuse;                  /* 14949 stack used gathering lib params */
extern WORD  numreps;                   /* 14950 repeats in a repeatable param list */
extern UBYTE *locloc;                   /* 14952 saved source pointer (storeloc) */
extern WORD  locchar;                   /* 14953 */
extern ULONG stbuff[10];                /* 14954 string varoffs (AB3 mode: ten) */
extern WORD  commode;                   /* 14955 in comma mode */
extern ULONG lastoffset;                /* 14956 last offset in calcvar */
extern WORD  linenumat;                 /* 14957 =-1 line being processed */
extern WORD  menuret;                   /* 14958 =-1 menu return code */
extern WORD  optresoff;                 /* 14959 */
extern UBYTE varcode[vcodelen];         /* 14960 code of last var access */
extern UBYTE varcode2[vcodelen];        /* 14962 above for let */
extern WORD  varcodelen, varcodelen2;   /* 14963 */
extern WORD  oldvcodelen;               /* 14965 */
extern ULONG procjmppc;                 /* 14967 pc of jmp of proc */
extern struct blitzlib *memlib;         /* 14968 memory library (65530) */
extern WORD  memlibstat;                /* 14969 */
extern WORD  tempvmode, tempvmode2;     /* 14971 */
extern WORD  varmode;                   /* 14973 0 = a5 globals, else locals */
extern struct proc *thisproc;           /* 14975 */
extern struct proc *firstproc;          /* 14976 */
extern WORD  procmode;                  /* 14977 0 norm, 1 statement, 2 function */
extern WORD  comflag;                   /* 14979 0 = no compile */
extern ULONG passstack;                 /* 14980 (unused in C, see compile.h passjmp) */
extern ULONG zero;                      /* 14981 constant 0 */
extern WORD  numincs;                   /* 14982 # included files */
extern struct xinclude *firstxinc;      /* 14984 */
extern struct include *firstinc;        /* 14985 */
/* 14986 lasta6 (=-1), quoteflag: contiguous, read/written as one long
   (forth stack lasta6q, blitz2.s 9025/9737) -> L_(lasta6qf) */
extern WORD  lasta6qf[2];
#define lasta6    (lasta6qf[0])
#define quoteflag (lasta6qf[1])
extern UBYTE *macrobuff;                /* 14988 macro collection buffer */
extern UBYTE *macrobufff;               /* 14989 end of above */
extern ULONG macrobuffsize;             /* bytes allocated at macrobuff (#336) */
extern struct macro *firstmacro;        /* 14990 */
extern ULONG endop;                     /* 14991 end of program pc before finishups */
extern struct label *firstlabel;        /* 14992 */
extern ULONG lasttoken;                 /* 14993 */
extern WORD  numstatic;                 /* 14994 */
extern ULONG staticdata;                /* 14995 data1 val for statics */
extern struct blitzlib *firstlib;       /* 14996 */
extern WORD  sbasegot;                  /* 14997 */
extern ULONG litdata1;                  /* 14998 */
/* optimisation / relocs */
extern ULONG objlen;                    /* 15002 */
extern ULONG numoffs;                   /* 15003 */
extern struct offnode *firstoff;        /* 15004 amigados reloc offsets */
extern UWORD flagmask;                  /* 15006 var flag mask for search */
extern WORD  added;                     /* 15008 variable was added */
extern WORD  nonew;                     /* 15009 1 = inhibit new variable */

/* 15012-15015: cnop 0,4 / namebuff ds.b 128 / namebuff2 ds.b 128 /
   ds.b 8 "for fileinfoblock".  One long-aligned 264 byte block; Examine /
   ExNext use it as a struct FileInfoBlock (260 bytes).
   NB: sizeof(namebuff2) is not 128 - it is a pointer macro. */
extern UBYTE namebuff[264] __attribute__((aligned(4)));
#define namebuff2   (namebuff + 128)
#define namebufffib ((struct FileInfoBlock *)namebuff)
extern UBYTE optstuff[64];              /* 15016 temp opt make buffer */

/* ===================================================================
 * Type information requester (15018-15288, typereq)
 * =================================================================== */

/* The list state (asm typeat, maxtypes, f1-f3) and the list routines
   (makebody, countlist, showlist, showlist2, showlist3, typeshow,
   reflist, allofem) are local to data.c: struct typelist. */
void zerotypes(void);                   /* 15018 blank the 12 list lines */
void menu6(void);                       /* 15165 menu: View Type Information */
void typename(const char *name);        /* 15281 copy name into TYPE: string (<=31) */

extern struct Requester typereq;        /* typereq */

/* ===================================================================
 * .xtra save/load and the Options requester (15290-15733, options*.src)
 * =================================================================== */

/* The option values live in the gadgets (as in the asm):
 *   optreqga9  SELECTED: create icons for executables
 *   optreqga10 SELECTED: runtime error debugger (click -> runerropts)
 *   optreqga11..14 (unlinked, hidden): debugger, long arrays, expand
 *              macros, quoted macros - flags still saved
 *   optreqga15 SELECTED: make smallest code
 *   optreqga16 SELECTED: create debug info for executables
 *   optreq2ga1 auto run, ga2 interrupt checking, ga3 assembler checking,
 *   ga4 overflow checking, ga5..7 hidden old layout (ga6 old interrupt,
 *   ga7 old overflow).
 * Test: optreqga15.Flags & GFLG_SELECTED  (asm: btst #7,optreqga15+13). */
extern struct Requester optreq, optreq2;
extern struct Gadget optreqga1, optreqga2, optreqga3, optreqga4, optreqga5,
    optreqga6, optreqga7, optreqga8, optreqga9, optreqga10, optreqga11,
    optreqga12, optreqga13, optreqga14, optreqga15, optreqga16,
    optreqga17, optreqga18, optreqga19, optreqga20, optreqga21,
    optreqga22, optreqga23, optreqga24, optreqga25, optreqga26,
    optreqga27, optreqga28;
extern struct Gadget optreq2ga1, optreq2ga2, optreq2ga3, optreq2ga4,
    optreq2ga5, optreq2ga6, optreq2ga7, optreq2ga8, optreq2ga9;

/* Buffer sizes = LongInt of the integer gadgets optreqga1..6 (options.src
   labels maxpc..stringwork are the StringInfo LongInt fields). */
extern struct StringInfo optreqsg1, optreqsg2, optreqsg3, optreqsg4,
    optreqsg5, optreqsg6, optreqsg7, optreqsg8, optreqsgz, optreqsgy;
#define maxpc      (optreqsg1.LongInt)  /* Object Buffer, 16384 */
#define maxlib     (optreqsg2.LongInt)  /* Libs Buffer, 32768 */
#define maxdata1   (optreqsg3.LongInt)  /* System Buffer, 4096 */
#define maxdata2   (optreqsg4.LongInt)  /* Data Buffer, 4096 */
#define macrolen   (optreqsg5.LongInt)  /* Macro Buffer, 8192 */
#define stringwork (optreqsg6.LongInt)  /* String Buffer, 10240 */

extern struct resentry resbuff[MOSTRES];  /* options.src: 8 residents */

void optreqprep(void);                  /* options.src: chip copy of arrow images */
void optreqdone(void);                  /* options.src: free it */

extern UBYTE vers;                      /* 15343 .xtra version (asm word, used as byte) */

/* 15345/15412: called by the editor through edstruct via loadxtra_stub /
   savextra_stub (glue.h): r->d[7] = file handle, r->a[6] = DOSBase.
   loadxtra_c and savextra_c are declared in glue.h. */
void menu4(void);                       /* 15456 menu: compiler options */
void menu4c(void);                      /* 15483 (menu4 fallthrough) */
/* 15592 gadget 17 ok / 18 -> menu0 / 19 -> menu1; allchg/reschg: menu4c's
   change flags (asm globals) */
void optsdone(UWORD gadget, WORD allchg, WORD reschg);
void menu4canc(void);                   /* 15612 restore snapshot */
void newres(void);                      /* 15627 */
void makeres(void);                     /* 15637 */
void makemax(WORD maxat);               /* 15662 */
/* 15713 which = index among libs with a max; writes name+'s' to label (not
   NUL terminated) and the max (makelong) to num */
void maxit(WORD which, UBYTE *label, UBYTE *num);

/* ===================================================================
 * Calculator / CLI argument (15735-16015, calcreq)
 * =================================================================== */

extern struct Requester calcreq, argreq;
void menu7(void);                       /* 15739 menu: calculator */
extern UBYTE argreqla8[128];             /* 15966 CLI argument (saved in .xtra) */
void menu9(void);                       /* 15979 menu: CLI argument */

#endif
