/*
 * data.c - chunk 10 (blitz2.s 14656-16015 + options2.src.new, typereq,
 * options.src, calcreq): the `mydata` globals, the built-in type nodes,
 * the Intuition requesters (as real NDK structs), the Type-Info browser
 * (menu6), .xtra load/save (loadxtra_c/savextra_c), the Options requester
 * (menu4), the Calculator (menu7) and the CLI argument requester (menu9).
 * See notes/survey-10.md.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include "blitz2.h"

/* ===================================================================
 * mydata (blitz2.s 14658-15016)
 * =================================================================== */

ULONG vbr = 0;                          /* 14660 */
WORD  cfetchmode = 0;                   /* 14664 1=fetching a constant; intercept uselib! */
UWORD constlibs[numclibs] = {64935, 64735, 64835};  /* 14667 */
/* below is a PC for the constant evaluation routine */
ULONG constpcat = 0;                    /* 14671 */
ULONG constlibat = 0;                   /* 14672 */
ULONG constmaxpc = 1024;                /* 14673 */
ULONG stackfuck = 0;                    /* 14675 */
struct label *lastrescon  = (struct label *)&firstlabel;   /* 14676 dc.l firstlabel */
struct macro *lastresmac  = (struct macro *)&firstmacro;   /* 14677 dc.l firstmacro */
struct type  *lastrestype = (struct type *)&stringtype;    /* 14678 dc.l firsttype
                                                (the label sits on stringtype) */
WORD  rescnt = 0;                       /* 14679 # resident */
/* 14680 reschg: local to menu4c */
struct resentry *resat = resbuff;       /* 14681 */
WORD  linedone = 0;                     /* 14682 */
ULONG dirpcat = 0;                      /* 14683 */
WORD  diropt = 0;                       /* 14684 */
ULONG dira4 = 0, dira5 = 0, dira6 = 0;  /* 14685 */
ULONG dirstack = 0;                     /* 14688 */
APTR *comdata = NULL;                   /* 14689 */

/* 14693 convtable / 14704 operators: chunk 06 (eval.c).
   14831 modetable: chunk 02 (compile.c). */

/* --- built-in types (14833-14860) ---------------------------------
   node: next.l, code.l, size.w, len.b (=11+namelen+1), name\0 */
struct type *alltypes = &bytetype;      /* 14833 */
struct type bytetype    = {&wordtype,    {1}, 1, 13, "b"};  /* 14835 */
struct type wordtype    = {&longtype,    {2}, 2, 13, "w"};  /* 14838 */
struct type longtype    = {&quicktype,   {3}, 4, 13, "l"};  /* 14841 */
struct type quicktype   = {&floattype,   {4}, 4, 13, "q"};  /* 14844 */
struct type floattype   = {&addresstype, {5}, 4, 13, "f"};  /* 14847 */
struct type addresstype = {&stringtype,  {6}, 4, 13, "a"};  /* 14850 */
/* 14853 firsttype: label on stringtype -> #define firsttype (stringtype.next) */
struct type stringtype  = {NULL,         {7}, 4, 13, "s"};  /* 14854 */
struct type dummytype   = {NULL,         {0}, 0, 1};        /* 14858 */

/* BUG(orig) (#342): set only by doincdir and never cleared - the IncDir of one
   compile stays for the following ones of the same session.  Kept. */
char  incdir[96];                       /* 14862 */
WORD  dirmode = 0;                      /* 14865 direct mode flag */
/* 14866 int: IntuitionBase, defined in main.c */
/* 14867 maxat: local to menu4c */
ULONG maxsat = 0;                       /* 14868 data1 info pos */
WORD  nummaxs = 0;                      /* 14869 number of maximums in libs */
WORD  maxsused = 0;                     /* 14871 maximums used in prog */
/* below are defaults to be saved with source */
WORD  dark = 0;                         /* 14875 dark compile? */
WORD  qlab = 0;                         /* 14876 quick lab allocate */
struct type *defaulttype = &quicktype;  /* 14877 */
/* end of defaults */

WORD  anyerrs = -1;                     /* 14882 Any Errors in compile ? */
WORD  makeexec = 0;                     /* 14883 make an executable file! */
ULONG noinits = 0;                      /* 14884 flag for if we have any init routines! 6.l=no! */
ULONG letstart = 0;                     /* 14886 */
WORD  nomemleft = 0;                    /* 14887 */
char  lastbiglab[128];                  /* 14888 */
ULONG locdatast = 0;                    /* 14890 */
struct addrnode *firstpend = NULL;      /* 14891 */
/* one pass compiler stuff! */
ULONG data2at = 0;                      /* 14895 */
ULONG data2 = 0;                        /* 14896 */
ULONG data1at = 0;                      /* 14897 */
ULONG data1 = 0;                        /* 14898 pc for initialized data! */
ULONG pcat = 0;                         /* 14899 */
ULONG pc = 0;                           /* 14900 */
ULONG libat = 0;                        /* 14901 */
ULONG lib = 0;                          /* 14902 */
ULONG allat = 0;                        /* 14903 highest allocmemd location for code */
struct asment dummyasm;                 /* 14905 dc 0,0,0,0,0 / dc.l 0 / dc 0 */
/* data for assembler */
/* 14911 buff1, 14912 buff2: locals of asmit */
WORD  constmode = 0;                    /* 14913 0 for norm, <>0 for assembler */
WORD  asmtype = 0;                      /* 14914 0=contains no prog reference */
UWORD extrawords[2] = {0, 0};           /* 14915 extraword, extraword2 */
/* 14917 asmsize: local of asmit, a parameter of doea/makemode */
UBYTE *asmbuff = NULL;                  /* 14918 */
WORD  asmlen = 0;                       /* 14919 first (src) text len */
WORD  asmlen2 = 0;                      /* 14920 second (dest) */
struct asmnode *firstasm = NULL;        /* 14921 linked list of expressions to resolve */

WORD  macnum = 0;                       /* 14924 */
UBYTE *nextinc0 = NULL;                 /* 14925 address of next 0 in include file */
ULONG titleat = 0;                      /* 14926 */
WORD  titleadd[5];                      /* 14927 5 adds for compile streaks */
WORD  connest = 0;                      /* 14928 conditional nest level */
struct constant *firstconst = NULL;     /* 14929 first constant (#a) */
WORD  numarg = 0;                       /* 14930 number of macro arguements */
UBYTE *myoline = NULL;                  /* 14931 macro done yet? */
UBYTE *myline = NULL;                   /* 14932 memory to create line in */
UBYTE *mylinef = NULL;                  /* 14934 */
WORD  concomstack[32];                  /* 14935 32 deep conditional compilation */
WORD  *concomsp = concomstack;          /* 14936 stack pointer */
WORD  oldqflag = 0;                     /* 14937 */
ULONG lastconloc = 0;                   /* 14939 */
WORD  lastconop = 0;                    /* 14940 */
struct fornode *firstfor = NULL;        /* 14941 */
WORD  pushflag = 0;                     /* 14942 */
WORD  sbgot = 0;                        /* 14943 flag for string got 2 */
WORD  usertype = 0;                     /* 14944 */
WORD  userp = 0;                        /* 14945 */
struct libsnode *libslist = NULL;       /* 14948 */
WORD  stackuse = 0;                     /* 14949 stack used when gathering lib parameters */
WORD  numreps = 0;                      /* 14950 repeats in a repeatable parameter list */
UBYTE *locloc = NULL;                   /* 14952 */
WORD  locchar = 0;                      /* 14953 */
ULONG stbuff[10];                       /* 14954 string varoffs (asm: 8; AB3 mode: ten) */
WORD  commode = 0;                      /* 14955 in comma mode... */
ULONG lastoffset = 0;                   /* 14956 last offset in calcvar! */
WORD  linenumat = -1;                   /* 14957 line being processed */
WORD  menuret = -1;                     /* 14958 menu return code */
WORD  optresoff = 0;                    /* 14959 opt reset offset */
UBYTE varcode[vcodelen];                /* 14960 code used to generate last var access */
UBYTE varcode2[vcodelen];               /* 14962 above for let..... */
WORD  varcodelen = 0;                   /* 14963 and length */
WORD  varcodelen2 = 0;                  /* 14964 */
WORD  oldvcodelen = 0;                  /* 14965 old varcodelen */
ULONG procjmppc = 0;                    /* 14967 pc of jmp of proc */
struct blitzlib *memlib = NULL;         /* 14968 address of memory library */
WORD  memlibstat = 0;                   /* 14969 status of memory library during procs */
WORD  tempvmode = 0;                    /* 14971 */
WORD  tempvmode2 = 0;                   /* 14972 */
WORD  varmode = 0;                      /* 14973 0=a5 is pointer to globals else locals */
struct proc *thisproc = NULL;           /* 14975 this procedure being 'done' */
struct proc *firstproc = NULL;          /* 14976 */
WORD  procmode = 0;                     /* 14977 1 if statement, 2 if function, 0 if norm! */
WORD  comflag = 0;                      /* 14979 0 if no compile */
ULONG passstack = 0;                    /* 14980 */
ULONG zero = 0;                         /* 14981 ZERO! */
WORD  numincs = 0;                      /* 14982 number of included files, 0=main RAM code */
struct xinclude *firstxinc = NULL;      /* 14984 first exclusive include. */
struct include *firstinc = NULL;        /* 14985 first include */
WORD  lasta6qf[2] = {-1, 0};            /* 14986 lasta6 dc -1 / quoteflag dc 0 */
UBYTE *macrobuff = NULL;                /* 14988 pointer to macro collection buffer */
UBYTE *macrobufff = NULL;               /* 14989 end of above */
ULONG macrobuffsize = 0;                /* bytes allocated at macrobuff (#336) */
struct macro *firstmacro = NULL;        /* 14990 linked list of macro's */
ULONG endop = 0;                        /* 14991 end of program pc - before finishups! */
struct label *firstlabel = NULL;        /* 14992 label list */
ULONG lasttoken = 0;                    /* 14993 */
WORD  numstatic = 0;                    /* 14994 number of static structs */
ULONG staticdata = 0;                   /* 14995 data1 val for statics */
struct blitzlib *firstlib = NULL;       /* 14996 first lib struct */
WORD  sbasegot = 0;                     /* 14997 string base got? */
ULONG litdata1 = 0;                     /* 14998 */
/* Optimization stuff */
ULONG objlen = 0;                       /* 15002 */
ULONG numoffs = 0;                      /* 15003 */
struct offnode *firstoff = NULL;        /* 15004 amigados type reloc offsets */
UWORD flagmask = 0;                     /* 15006 mask for flag of variable when searching */
WORD  added = 0;                        /* 15008 flag for seeing if variable was added! */
WORD  nonew = 0;                        /* 15009 1 to inhibit new variable creation */
/* 15010 temp1, 15011 temp2: scratch, locals of their users in C */
/* 15012 cnop 0,4 / namebuff 128 / namebuff2 128 / 8 for fileinfoblock */
UBYTE namebuff[264] __attribute__((aligned(4)));
UBYTE optstuff[64];                     /* 15016 temp opt make buffer */

/* ===================================================================
 * options.src: arrow images, residents (images first: the gadget data
 * below points at them)
 * =================================================================== */

/* uparrow / downarrow: 2 planes of 16x8 each, contiguous (arrowf) */
static const UWORD uparrow[32] = {
    0xffff, 0x8000, 0x8180, 0x83c0, 0x87e0, 0x8ff0, 0x8000, 0x8000,
    0x0000, 0x0001, 0x0181, 0x03c1, 0x07e1, 0x0ff1, 0x0001, 0xffff,
    /* downarrow */
    0xffff, 0x8000, 0x8ff0, 0x87e0, 0x83c0, 0x8180, 0x8000, 0x8000,
    0x0000, 0x0001, 0x0ff1, 0x07e1, 0x03c1, 0x0181, 0x0001, 0x7fff,
};
#define downarrow (&uparrow[16])
#define arrowsize sizeof(uparrow)       /* arrowf-uparrow = 64 */

static struct Image upimage   = {0, 0, 16, 8, 2, NULL, 3, 0, NULL};
static struct Image downimage = {0, 0, 16, 8, 2, NULL, 3, 0, NULL};
#define uimageloc (upimage.ImageData)
#define dimageloc (downimage.ImageData)

struct resentry resbuff[MOSTRES];       /* options.src: dcb.b 8*68,0 ;8 res max */

/* ===================================================================
 * Requester data (options2.src.new, typereq, options.src, calcreq,
 * blitz2.s 15895-15977 argreq).  Generated 1:1 from the asm data:
 * xxxboN/xxxlaN Border pairs + XY lists, xxxinN IntuiTexts, xxxgaN
 * Gadgets (Flags $80 SELECTED, $100 DISABLED; Activation 4 = ENDGADGET:
 * the editor's getinput closes the requester), xxxsgN StringInfos (the
 * options.src labels maxpc..stringwork are their LongInt fields, see
 * data.h).  Hidden gadgets (not in the Next chain) exist for their Flags:
 * optreqga11-14, optreq2ga5-7.
 * =================================================================== */

/* optreqin4, optreqin36, optreqin37 are not referenced in the original either */
#pragma GCC diagnostic ignored "-Wunused-variable"

/* forward declarations */
struct Requester optreq2;
static struct Border optreq2bo1;
static WORD optreq2la1[6];
static struct Border optreq2la2;
static WORD optreq2la3[6];
static struct Border optreq2bo2;
static WORD optreq2la4[6];
static struct Border optreq2la5;
static WORD optreq2la6[6];
static struct Border optreq2bo3;
static WORD optreq2la15[6];
static struct Border optreq2la16;
static WORD optreq2la17[6];
static struct Border optreq2bo4;
static WORD optreq2la19[6];
static struct Border optreq2la20;
static WORD optreq2la21[6];
static struct Border optreq2bo5;
static WORD optreq2la23[6];
static struct Border optreq2la24;
static WORD optreq2la25[6];
static struct Border optreq2bo6;
static WORD optreq2la27[6];
static struct Border optreq2la28;
static WORD optreq2la29[6];
static struct Border optreq2bo7;
static WORD optreq2la31[6];
static struct Border optreq2la32;
static WORD optreq2la33[6];
static struct Border optreq2bo8;
static WORD optreq2la35[6];
static struct Border optreq2la36;
static WORD optreq2la37[6];
static struct Border optreq2bo9;
static WORD optreq2la39[6];
static struct Border optreq2la40;
static WORD optreq2la41[6];
static struct Border optreq2bo10;
static WORD optreq2la43[6];
static struct Border optreq2la44;
static WORD optreq2la45[6];
static struct Border optreq2bo11;
static WORD optreq2la47[6];
static struct Border optreq2la48;
static WORD optreq2la49[6];
static struct IntuiText optreq2in1;
static UBYTE optreq2la7[22];
static struct IntuiText optreq2in2;
static UBYTE optreq2la8[10];
static struct IntuiText optreq2in3;
static UBYTE optreq2la9[19];
static struct IntuiText optreq2in4;
static UBYTE optreq2la10[20];
static struct IntuiText optreq2in5;
static UBYTE optreq2la11[19];
static struct IntuiText optreq2in9;
static UBYTE optreq2la18[3];
static struct IntuiText optreq2in10;
static UBYTE optreq2la22[3];
static struct IntuiText optreq2in11;
static UBYTE optreq2la26[3];
static struct IntuiText optreq2in12;
static UBYTE optreq2la30[3];
static struct IntuiText optreq2in13;
static UBYTE optreq2la34[3];
static struct IntuiText optreq2in14;
static UBYTE optreq2la38[3];
static struct IntuiText optreq2in15;
static UBYTE optreq2la42[3];
static struct IntuiText optreq2in16;
static UBYTE optreq2la46[5];
static struct IntuiText optreq2in17;
static UBYTE optreq2la50[9];
struct Gadget optreq2ga1;
struct Gadget optreq2ga2;
struct Gadget optreq2ga3;
struct Gadget optreq2ga4;
struct Gadget optreq2ga5;
struct Gadget optreq2ga6;
struct Gadget optreq2ga7;
struct Gadget optreq2ga8;
struct Gadget optreq2ga9;
struct Requester typereq;
static struct Border typereqbo1;
static WORD typereqla1[6];
static struct Border typereqla2;
static WORD typereqla3[6];
static struct Border typereqbo2;
static WORD typereqla4[6];
static struct Border typereqla5;
static WORD typereqla6[6];
static struct Border typereqbo3;
static WORD typereqla7[6];
static struct Border typereqla8;
static WORD typereqla9[6];
static struct Border typereqbo4;
static WORD typereqla10[6];
static struct Border typereqla11;
static WORD typereqla12[6];
static struct Border typereqbo5;
static WORD typereqla13[6];
static struct Border typereqla14;
static WORD typereqla15[6];
static struct Border typereqbo6;
static WORD typereqla18[6];
static struct Border typereqla19;
static WORD typereqla20[6];
static struct Border typereqbo7;
static WORD typereqla22[6];
static struct Border typereqla23;
static WORD typereqla24[6];
static struct Border typereqbo8;
static WORD typereqla26[6];
static struct Border typereqla27;
static WORD typereqla28[6];
static struct IntuiText typereqin1;
static UBYTE typereqla16[22];
static struct IntuiText typereqin2;
static UBYTE typereqla25[6];
static struct IntuiText typereqin3;
static UBYTE typereqla29[11];
static struct IntuiText typereqin4;
static UBYTE typereqla30[28];
static struct IntuiText typereqin5;
static UBYTE typereqla31[28];
static struct IntuiText typereqin6;
static UBYTE typereqla32[28];
static struct IntuiText typereqin7;
static UBYTE typereqla33[28];
static struct IntuiText typereqin8;
static UBYTE typereqla34[28];
static struct IntuiText typereqin9;
static UBYTE typereqla35[28];
static struct IntuiText typereqin10;
static UBYTE typereqla36[28];
static struct IntuiText typereqin11;
static UBYTE typereqla37[28];
static struct IntuiText typereqin12;
static UBYTE typereqla38[28];
static struct IntuiText typereqin13;
static UBYTE typereqla39[28];
static struct IntuiText typereqin14;
static UBYTE typereqla40[28];
static struct IntuiText typereqin15;
static UBYTE typereqla41[28];
static struct Gadget typereqga1;
static struct Image typereqla17;
static struct PropInfo typereqvp1;
static struct Gadget typereqga2;
static struct StringInfo typereqsg1;
static UBYTE typereqla21[32];
static struct Gadget typereqga3;
static struct Gadget typereqga4;
static struct Gadget typereqga5;
static struct Gadget typereqga6;
static struct Gadget typereqga7;
static struct Gadget typereqga8;
static struct Gadget typereqga9;
static struct Gadget typereqga10;
static struct Gadget typereqga11;
static struct Gadget typereqga12;
static struct Gadget typereqga13;
static struct Gadget typereqga14;
static struct Gadget typereqga15;
static struct Gadget typereqga16;
struct Requester optreq;
static struct Border optreqbo1;
static WORD optreqla1[6];
static struct Border optreqla2;
static WORD optreqla3[6];
static struct Border optreqbo2;
static WORD optreqla4[6];
static struct Border optreqla5;
static WORD optreqla6[6];
static struct Border optreqbo3;
static WORD optreqla7[6];
static struct Border optreqla8;
static WORD optreqla9[6];
static struct Border optreqbo4;
static WORD optreqla10[6];
static struct Border optreqla11;
static WORD optreqla12[6];
static struct Border optreqbo5;
static WORD optreqla13[6];
static struct Border optreqla14;
static WORD optreqla15[6];
static struct Border optreqbo6;
static WORD optreqla16[6];
static struct Border optreqla17;
static WORD optreqla18[6];
static struct Border optreqbo7;
static WORD optreqla19[6];
static struct Border optreqla20;
static WORD optreqla21[6];
static struct Border optreqbo8;
static WORD optreqla22[6];
static struct Border optreqla23;
static WORD optreqla24[6];
static struct Border optreqbo9;
static WORD optreqla25[6];
static struct Border optreqla26;
static WORD optreqla27[6];
static struct Border optreqbo10;
static WORD optreqla28[6];
static struct Border optreqla29;
static WORD optreqla30[6];
static struct Border optreqbo11;
static WORD optreqla56[6];
static struct Border optreqla57;
static WORD optreqla58[6];
static struct Border optreqbo12;
static WORD optreqla60[6];
static struct Border optreqla61;
static WORD optreqla62[6];
static struct Border optreqbo13;
static WORD optreqla64[6];
static struct Border optreqla65;
static WORD optreqla66[6];
static struct Border optreqbo14;
static WORD optreqla68[6];
static struct Border optreqla69;
static WORD optreqla70[6];
static struct Border optreqbo15;
static WORD optreqla72[6];
static struct Border optreqla73;
static WORD optreqla74[6];
static struct Border optreqbo16;
static WORD optreqla76[6];
static struct Border optreqla77;
static WORD optreqla78[6];
static struct Border optreqbo17;
static WORD optreqla80[6];
static struct Border optreqla81;
static WORD optreqla82[6];
static struct Border optreqbo18;
static WORD optreqla84[6];
static struct Border optreqla85;
static WORD optreqla86[6];
static struct Border optreqbo19;
static WORD optreqla88[6];
static struct Border optreqla89;
static WORD optreqla90[6];
static struct Border optreqbo20;
static WORD optreqla92[6];
static struct Border optreqla93;
static WORD optreqla94[6];
static struct Border optreqbo21;
static WORD optreqla96[6];
static struct Border optreqla97;
static WORD optreqla98[6];
static struct Border optreqbo22;
static WORD optreqla100[6];
static struct Border optreqla101;
static WORD optreqla102[6];
static struct IntuiText optreqin1;
static UBYTE optreqla31[31];
static struct IntuiText optreqin2;
static UBYTE optreqla32[35];
static struct IntuiText optreqin3;
static UBYTE optreqla33[26];
static struct IntuiText optreqin4;
static UBYTE optreqla34[10];
static struct IntuiText optreqin5;
static UBYTE optreqla35[13];
static struct IntuiText optreqin6;
static UBYTE optreqla36[15];
static struct IntuiText optreqin7;
static UBYTE optreqla37[15];
static struct IntuiText optreqin8;
static UBYTE optreqla38[20];
static struct IntuiText optreqin9;
static UBYTE optreqla39[40];
static struct IntuiText optreqin10;
static UBYTE optreqla40[15];
static struct IntuiText optreqin11;
static UBYTE optreqla41[13];
static struct IntuiText optreqin12;
static UBYTE optreqla42[15];
static struct IntuiText optreqin13;
static UBYTE optreqla43[13];
static struct IntuiText optreqin14;
static UBYTE optreqla44[14];
static struct IntuiText optreqin15;
static UBYTE optreqla45[15];
static struct IntuiText optreqin16;
static UBYTE optreqla46[16];
static struct IntuiText optreqin17;
static UBYTE optreqla47[9];
static struct IntuiText optreqin18;
static UBYTE optreqla59[3];
static struct IntuiText optreqin19;
static UBYTE optreqla63[3];
static struct IntuiText optreqin20;
static UBYTE optreqla67[3];
static struct IntuiText optreqin21;
static UBYTE optreqla71[3];
static struct IntuiText optreqin22;
static UBYTE optreqla75[3];
static struct IntuiText optreqin23;
static UBYTE optreqla79[3];
static struct IntuiText optreqin24;
static UBYTE optreqla83[3];
static struct IntuiText optreqin25;
static UBYTE optreqla87[3];
static struct IntuiText optreqin26;
static UBYTE optreqla91[5];
static struct IntuiText optreqin27;
static UBYTE optreqla95[14];
static struct IntuiText optreqin28;
static UBYTE optreqla99[20];
static struct IntuiText optreqin29;
static UBYTE optreqla103[9];
static struct IntuiText optreqin34;
static UBYTE optreqla108[15];
static struct IntuiText optreqin35;
static UBYTE optreqla109[15];
static struct IntuiText optreqin36;
static UBYTE optreqla110[22];
static struct IntuiText optreqin37;
static UBYTE optreqla111[22];
struct Gadget optreqga1;
struct StringInfo optreqsg1;
static UBYTE optreqla48[12];
struct Gadget optreqga2;
struct StringInfo optreqsg2;
static UBYTE optreqla49[12];
struct Gadget optreqga3;
struct StringInfo optreqsg3;
static UBYTE optreqla50[12];
struct Gadget optreqga4;
struct StringInfo optreqsg4;
static UBYTE optreqla51[12];
struct Gadget optreqga5;
struct StringInfo optreqsg5;
static UBYTE optreqla52[12];
struct Gadget optreqga6;
struct StringInfo optreqsg6;
static UBYTE optreqla53[12];
struct Gadget optreqga7;
struct StringInfo optreqsg7;
static UBYTE optreqla54[12];
struct Gadget optreqga8;
struct StringInfo optreqsg8;
static UBYTE optreqla55[12];
struct Gadget optreqga9;
struct Gadget optreqga10;
struct Gadget optreqga11;
struct Gadget optreqga12;
struct Gadget optreqga13;
struct Gadget optreqga14;
struct Gadget optreqga15;
struct Gadget optreqga16;
struct Gadget optreqga17;
struct Gadget optreqga18;
struct Gadget optreqga19;
struct Gadget optreqga20;
struct Gadget optreqga21;
struct Gadget optreqga22;
struct Gadget optreqga23;
struct Gadget optreqga24;
struct Gadget optreqga25;
struct Gadget optreqga26;
struct Gadget optreqga27;
struct StringInfo optreqsgz;
struct Gadget optreqga28;
struct StringInfo optreqsgy;
struct Requester calcreq;
static struct Border calcreqbo1;
static WORD calcreqla1[6];
static struct Border calcreqla2;
static WORD calcreqla3[6];
static struct Border calcreqbo2;
static WORD calcreqla4[6];
static struct Border calcreqla5;
static WORD calcreqla6[6];
static struct Border calcreqbo3;
static WORD calcreqla7[6];
static struct Border calcreqla8;
static WORD calcreqla9[6];
static struct Border calcreqbo4;
static WORD calcreqla10[6];
static struct Border calcreqla11;
static WORD calcreqla12[6];
static struct Border calcreqbo5;
static WORD calcreqla16[6];
static struct Border calcreqla17;
static WORD calcreqla18[6];
static struct Border calcreqbo6;
static WORD calcreqla20[6];
static struct Border calcreqla21;
static WORD calcreqla22[6];
static struct Border calcreqbo7;
static WORD calcreqla24[6];
static struct Border calcreqla25;
static WORD calcreqla26[6];
static struct Border calcreqbo8;
static WORD calcreqla28[6];
static struct Border calcreqla29;
static WORD calcreqla30[6];
static struct IntuiText calcreqin1;
static UBYTE calcreqla13[12];
static struct IntuiText calcreqin2;
static UBYTE calcreqla14[11];
static struct IntuiText calcreqin3;
static UBYTE calcreqla19[6];
static struct IntuiText calcreqin4;
static UBYTE calcreqla23[6];
static struct IntuiText calcreqin5;
static UBYTE calcreqla27[6];
static struct IntuiText calcreqin6;
static UBYTE calcreqla31[10];
static struct Gadget calcreqga1;
static struct StringInfo calcreqsg1;
static UBYTE calcreqla15[128];
static struct Gadget calcreqga2;
static struct Gadget calcreqga3;
static struct Gadget calcreqga4;
static struct Gadget calcreqga5;
struct Requester argreq;
static struct Border argreqbo1;
static WORD argreqla1[6];
static struct Border argreqla2;
static WORD argreqla3[6];
static struct Border argreqbo2;
static WORD argreqla4[6];
static struct Border argreqla5;
static WORD argreqla6[6];
static struct Border argreqbo3;
static WORD argreqla9[6];
static struct Border argreqla10;
static WORD argreqla11[6];
static struct Border argreqbo4;
static WORD argreqla13[6];
static struct Border argreqla14;
static WORD argreqla15[6];
static struct IntuiText argreqin1;
static UBYTE argreqla7[15];
static struct IntuiText argreqin2;
static UBYTE argreqla12[5];
static struct IntuiText argreqin3;
static UBYTE argreqla16[9];
static struct Gadget argreqga1;
static struct StringInfo argreqsg1;
UBYTE argreqla8[128];
static struct Gadget argreqga2;
static struct Gadget argreqga3;

struct Requester optreq2 = {NULL, 276, 10, 240, 132, 0, 0, (struct Gadget *)&optreq2ga1, (struct Border *)&optreq2bo1, (struct IntuiText *)&optreq2in1};
static struct Border optreq2bo1 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la1, (struct Border *)&optreq2la2};
static WORD optreq2la1[6] = {231, 20, 8, 20, 8, 111};
static struct Border optreq2la2 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la3, (struct Border *)&optreq2bo2};
static WORD optreq2la3[6] = {231, 21, 231, 111, 9, 111};
static struct Border optreq2bo2 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la4, (struct Border *)&optreq2la5};
static WORD optreq2la4[6] = {239, 0, 0, 0, 0, 131};
static struct Border optreq2la5 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la6, NULL};
static WORD optreq2la6[6] = {239, 1, 239, 131, 1, 131};
static struct Border optreq2bo3 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la15, (struct Border *)&optreq2la16};
static WORD optreq2la15[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la16 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la17, NULL};
static WORD optreq2la17[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo4 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la19, (struct Border *)&optreq2la20};
static WORD optreq2la19[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la20 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la21, NULL};
static WORD optreq2la21[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo5 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la23, (struct Border *)&optreq2la24};
static WORD optreq2la23[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la24 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la25, NULL};
static WORD optreq2la25[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo6 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la27, (struct Border *)&optreq2la28};
static WORD optreq2la27[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la28 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la29, NULL};
static WORD optreq2la29[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo7 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la31, (struct Border *)&optreq2la32};
static WORD optreq2la31[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la32 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la33, NULL};
static WORD optreq2la33[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo8 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la35, (struct Border *)&optreq2la36};
static WORD optreq2la35[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la36 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la37, NULL};
static WORD optreq2la37[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo9 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la39, (struct Border *)&optreq2la40};
static WORD optreq2la39[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreq2la40 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la41, NULL};
static WORD optreq2la41[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreq2bo10 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la43, (struct Border *)&optreq2la44};
static WORD optreq2la43[6] = {39, 0, 0, 0, 0, 11};
static struct Border optreq2la44 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la45, NULL};
static WORD optreq2la45[6] = {39, 1, 39, 11, 1, 11};
static struct Border optreq2bo11 = {0, 0, 1, 2, 1, 3, (WORD *)optreq2la47, (struct Border *)&optreq2la48};
static WORD optreq2la47[6] = {71, 0, 0, 0, 0, 11};
static struct Border optreq2la48 = {0, 0, 2, 1, 1, 3, (WORD *)optreq2la49, NULL};
static WORD optreq2la49[6] = {71, 1, 71, 11, 1, 11};
static struct IntuiText optreq2in1 = {1, 0, 1, 12, 6, NULL, (UBYTE *)optreq2la7, (struct IntuiText *)&optreq2in2};
static UBYTE optreq2la7[22] = "Runtime Error Options";
static struct IntuiText optreq2in2 = {1, 0, 1, 20, 26, NULL, (UBYTE *)optreq2la8, (struct IntuiText *)&optreq2in3};
static UBYTE optreq2la8[10] = "AUTO RUN:";
static struct IntuiText optreq2in3 = {1, 0, 1, 20, 38, NULL, (UBYTE *)optreq2la9, (struct IntuiText *)&optreq2in4};
static UBYTE optreq2la9[19] = "INTERUPT CHECKING:";
static struct IntuiText optreq2in4 = {1, 0, 1, 20, 50, NULL, (UBYTE *)optreq2la10, (struct IntuiText *)&optreq2in5};
static UBYTE optreq2la10[20] = "ASSEMBLER CHECKING:";
static struct IntuiText optreq2in5 = {1, 0, 1, 20, 62, NULL, (UBYTE *)optreq2la11, NULL};
static UBYTE optreq2la11[19] = "OVERFLOW CHECKING:";
static struct IntuiText optreq2in9 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la18, NULL};
static UBYTE optreq2la18[3] = "  ";
static struct IntuiText optreq2in10 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la22, NULL};
static UBYTE optreq2la22[3] = "  ";
static struct IntuiText optreq2in11 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la26, NULL};
static UBYTE optreq2la26[3] = "  ";
static struct IntuiText optreq2in12 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la30, NULL};
static UBYTE optreq2la30[3] = "  ";
static struct IntuiText optreq2in13 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la34, NULL};
static UBYTE optreq2la34[3] = "  ";
static struct IntuiText optreq2in14 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la38, NULL};
static UBYTE optreq2la38[3] = "  ";
static struct IntuiText optreq2in15 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la42, NULL};
static UBYTE optreq2la42[3] = "  ";
static struct IntuiText optreq2in16 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la46, NULL};
static UBYTE optreq2la46[5] = " OK ";
static struct IntuiText optreq2in17 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreq2la50, NULL};
static UBYTE optreq2la50[9] = " CANCEL ";
struct Gadget optreq2ga1 = {(struct Gadget *)&optreq2ga2, 200, 24, 24, 12, 128, 257, 1, (APTR)&optreq2bo3, NULL, (struct IntuiText *)&optreq2in9, 0, NULL, 1, NULL};
struct Gadget optreq2ga2 = {(struct Gadget *)&optreq2ga3, 200, 36, 24, 12, 128, 257, 1, (APTR)&optreq2bo4, NULL, (struct IntuiText *)&optreq2in10, 0, NULL, 2, NULL};
struct Gadget optreq2ga3 = {(struct Gadget *)&optreq2ga4, 200, 48, 24, 12, 0, 257, 1, (APTR)&optreq2bo5, NULL, (struct IntuiText *)&optreq2in11, 0, NULL, 3, NULL};
struct Gadget optreq2ga4 = {(struct Gadget *)&optreq2ga8, 200, 60, 24, 12, 0, 257, 1, (APTR)&optreq2bo6, NULL, (struct IntuiText *)&optreq2in12, 0, NULL, 4, NULL};
struct Gadget optreq2ga5 = {(struct Gadget *)&optreq2ga6, 200, 72, 24, 12, 128, 257, 1, (APTR)&optreq2bo7, NULL, (struct IntuiText *)&optreq2in13, 0, NULL, 5, NULL};
struct Gadget optreq2ga6 = {(struct Gadget *)&optreq2ga7, 200, 84, 24, 12, 128, 257, 1, (APTR)&optreq2bo8, NULL, (struct IntuiText *)&optreq2in14, 0, NULL, 6, NULL};
struct Gadget optreq2ga7 = {(struct Gadget *)&optreq2ga8, 200, 96, 24, 12, 128, 257, 1, (APTR)&optreq2bo9, NULL, (struct IntuiText *)&optreq2in15, 0, NULL, 7, NULL};
struct Gadget optreq2ga8 = {(struct Gadget *)&optreq2ga9, 8, 116, 40, 12, 0, 5, 1, (APTR)&optreq2bo10, NULL, (struct IntuiText *)&optreq2in16, 0, NULL, 8, NULL};
struct Gadget optreq2ga9 = {NULL, 160, 116, 72, 12, 0, 5, 1, (APTR)&optreq2bo11, NULL, (struct IntuiText *)&optreq2in17, 0, NULL, 9, NULL};
struct Requester typereq = {NULL, 148, 2, 272, 172, 0, 0, (struct Gadget *)&typereqga1, (struct Border *)&typereqbo1, (struct IntuiText *)&typereqin1};
static struct Border typereqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla1, (struct Border *)&typereqla2};
static WORD typereqla1[6] = {239, 24, 16, 24, 16, 123};
static struct Border typereqla2 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla3, (struct Border *)&typereqbo2};
static WORD typereqla3[6] = {239, 25, 239, 123, 17, 123};
static struct Border typereqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla4, (struct Border *)&typereqla5};
static WORD typereqla4[6] = {263, 20, 8, 20, 8, 127};
static struct Border typereqla5 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla6, (struct Border *)&typereqbo3};
static WORD typereqla6[6] = {263, 21, 263, 127, 9, 127};
static struct Border typereqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla7, (struct Border *)&typereqla8};
static WORD typereqla7[6] = {263, 132, 8, 132, 8, 151};
static struct Border typereqla8 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla9, (struct Border *)&typereqbo4};
static WORD typereqla9[6] = {263, 133, 263, 151, 9, 151};
static struct Border typereqbo4 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla10, (struct Border *)&typereqla11};
static WORD typereqla10[6] = {255, 136, 72, 136, 72, 147};
static struct Border typereqla11 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla12, (struct Border *)&typereqbo5};
static WORD typereqla12[6] = {255, 137, 255, 147, 73, 147};
static struct Border typereqbo5 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla13, (struct Border *)&typereqla14};
static WORD typereqla13[6] = {271, 0, 0, 0, 0, 171};
static struct Border typereqla14 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla15, (struct Border *)&typereqbo6};
static WORD typereqla15[6] = {271, 1, 271, 171, 1, 171};
static struct Border typereqbo6 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla18, (struct Border *)&typereqla19};
static WORD typereqla18[6] = {259, 26, 244, 26, 244, 121};
static struct Border typereqla19 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla20, NULL};
static WORD typereqla20[6] = {259, 27, 259, 121, 245, 121};
static struct Border typereqbo7 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla22, (struct Border *)&typereqla23};
static WORD typereqla22[6] = {47, 0, 0, 0, 0, 11};
static struct Border typereqla23 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla24, NULL};
static WORD typereqla24[6] = {47, 1, 47, 11, 1, 11};
static struct Border typereqbo8 = {0, 0, 1, 2, 1, 3, (WORD *)typereqla26, (struct Border *)&typereqla27};
static WORD typereqla26[6] = {87, 0, 0, 0, 0, 11};
static struct Border typereqla27 = {0, 0, 2, 1, 1, 3, (WORD *)typereqla28, NULL};
static WORD typereqla28[6] = {87, 1, 87, 11, 1, 11};
static struct IntuiText typereqin1 = {1, 0, 1, 12, 6, NULL, (UBYTE *)typereqla16, NULL};
static UBYTE typereqla16[22] = "View Type Information";
static struct IntuiText typereqin2 = {1, 0, 1, 4, 2, NULL, (UBYTE *)typereqla25, NULL};
static UBYTE typereqla25[6] = "TYPE:";
static struct IntuiText typereqin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)typereqla29, NULL};
static UBYTE typereqla29[11] = " FAREWELL ";
static struct IntuiText typereqin4 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla30, NULL};
static UBYTE typereqla30[28] = "                           ";
static struct IntuiText typereqin5 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla31, NULL};
static UBYTE typereqla31[28] = "                           ";
static struct IntuiText typereqin6 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla32, NULL};
static UBYTE typereqla32[28] = "                           ";
static struct IntuiText typereqin7 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla33, NULL};
static UBYTE typereqla33[28] = "                           ";
static struct IntuiText typereqin8 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla34, NULL};
static UBYTE typereqla34[28] = "                           ";
static struct IntuiText typereqin9 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla35, NULL};
static UBYTE typereqla35[28] = "                           ";
static struct IntuiText typereqin10 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla36, NULL};
static UBYTE typereqla36[28] = "                           ";
static struct IntuiText typereqin11 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla37, NULL};
static UBYTE typereqla37[28] = "                           ";
static struct IntuiText typereqin12 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla38, NULL};
static UBYTE typereqla38[28] = "                           ";
static struct IntuiText typereqin13 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla39, NULL};
static UBYTE typereqla39[28] = "                           ";
static struct IntuiText typereqin14 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla40, NULL};
static UBYTE typereqla40[28] = "                           ";
static struct IntuiText typereqin15 = {1, 0, 1, 0, 0, NULL, (UBYTE *)typereqla41, NULL};
static UBYTE typereqla41[28] = "                           ";
static struct Gadget typereqga1 = {(struct Gadget *)&typereqga2, 247, 28, 10, 92, 0, 3, 3, (APTR)&typereqla17, NULL, NULL, 0, (APTR)&typereqvp1, 1, NULL};
static struct Image typereqla17 = {0};
static struct PropInfo typereqvp1 = {13, (UWORD)-1, 0, (UWORD)-1, 32768, 0, 0, 0, 0, 0, 0};
static struct Gadget typereqga2 = {(struct Gadget *)&typereqga3, 76, 138, 176, 8, 0, 1, 4, NULL, NULL, NULL, 0, (APTR)&typereqsg1, 2, NULL};
static struct StringInfo typereqsg1 = {(UBYTE *)typereqla21, (UBYTE *)undobuffer, 0, 32, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};
static UBYTE typereqla21[32] = "";
static struct Gadget typereqga3 = {(struct Gadget *)&typereqga4, 16, 136, 48, 12, 0, 1, 1, (APTR)&typereqbo7, NULL, (struct IntuiText *)&typereqin2, 0, NULL, 3, NULL};
static struct Gadget typereqga4 = {(struct Gadget *)&typereqga5, 96, 156, 88, 12, 0, 5, 1, (APTR)&typereqbo8, NULL, (struct IntuiText *)&typereqin3, 0, NULL, 4, NULL};
static struct Gadget typereqga5 = {(struct Gadget *)&typereqga6, 20, 26, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin4, 0, NULL, 5, NULL};
static struct Gadget typereqga6 = {(struct Gadget *)&typereqga7, 20, 34, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin5, 0, NULL, 6, NULL};
static struct Gadget typereqga7 = {(struct Gadget *)&typereqga8, 20, 42, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin6, 0, NULL, 7, NULL};
static struct Gadget typereqga8 = {(struct Gadget *)&typereqga9, 20, 50, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin7, 0, NULL, 8, NULL};
static struct Gadget typereqga9 = {(struct Gadget *)&typereqga10, 20, 58, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin8, 0, NULL, 9, NULL};
static struct Gadget typereqga10 = {(struct Gadget *)&typereqga11, 20, 66, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin9, 0, NULL, 10, NULL};
static struct Gadget typereqga11 = {(struct Gadget *)&typereqga12, 20, 74, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin10, 0, NULL, 11, NULL};
static struct Gadget typereqga12 = {(struct Gadget *)&typereqga13, 20, 82, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin11, 0, NULL, 12, NULL};
static struct Gadget typereqga13 = {(struct Gadget *)&typereqga14, 20, 90, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin12, 0, NULL, 13, NULL};
static struct Gadget typereqga14 = {(struct Gadget *)&typereqga15, 20, 98, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin13, 0, NULL, 14, NULL};
static struct Gadget typereqga15 = {(struct Gadget *)&typereqga16, 20, 106, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin14, 0, NULL, 15, NULL};
static struct Gadget typereqga16 = {NULL, 20, 114, 216, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&typereqin15, 0, NULL, 16, NULL};
struct Requester optreq = {NULL, 44, 2, 424, 172, 0, 0, (struct Gadget *)&optreqga1, (struct Border *)&optreqbo1, (struct IntuiText *)&optreqin1};
static struct Border optreqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla1, (struct Border *)&optreqla2};
static WORD optreqla1[6] = {199, 76, 136, 76, 136, 87};
static struct Border optreqla2 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla3, (struct Border *)&optreqbo2};
static WORD optreqla3[6] = {199, 77, 199, 87, 137, 87};
static struct Border optreqbo2 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla4, (struct Border *)&optreqla5};
static WORD optreqla4[6] = {407, 76, 344, 76, 344, 87};
static struct Border optreqla5 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla6, (struct Border *)&optreqbo3};
static WORD optreqla6[6] = {407, 77, 407, 87, 345, 87};
static struct Border optreqbo3 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla7, (struct Border *)&optreqla8};
static WORD optreqla7[6] = {199, 88, 136, 88, 136, 99};
static struct Border optreqla8 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla9, (struct Border *)&optreqbo4};
static WORD optreqla9[6] = {199, 89, 199, 99, 137, 99};
static struct Border optreqbo4 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla10, (struct Border *)&optreqla11};
static WORD optreqla10[6] = {407, 88, 344, 88, 344, 99};
static struct Border optreqla11 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla12, (struct Border *)&optreqbo5};
static WORD optreqla12[6] = {407, 89, 407, 99, 345, 99};
static struct Border optreqbo5 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla13, (struct Border *)&optreqla14};
static WORD optreqla13[6] = {199, 100, 136, 100, 136, 111};
static struct Border optreqla14 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla15, (struct Border *)&optreqbo6};
static WORD optreqla15[6] = {199, 101, 199, 111, 137, 111};
static struct Border optreqbo6 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla16, (struct Border *)&optreqla17};
static WORD optreqla16[6] = {407, 100, 344, 100, 344, 111};
static struct Border optreqla17 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla18, (struct Border *)&optreqbo7};
static WORD optreqla18[6] = {407, 101, 407, 111, 345, 111};
static struct Border optreqbo7 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla19, (struct Border *)&optreqla20};
static WORD optreqla19[6] = {199, 128, 16, 128, 16, 147};
static struct Border optreqla20 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla21, (struct Border *)&optreqbo8};
static WORD optreqla21[6] = {199, 129, 199, 147, 17, 147};
static struct Border optreqbo8 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla22, (struct Border *)&optreqla23};
static WORD optreqla22[6] = {407, 128, 216, 128, 216, 147};
static struct Border optreqla23 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla24, (struct Border *)&optreqbo9};
static WORD optreqla24[6] = {407, 129, 407, 147, 217, 147};
static struct Border optreqbo9 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla25, (struct Border *)&optreqla26};
static WORD optreqla25[6] = {415, 20, 8, 20, 8, 151};
static struct Border optreqla26 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla27, (struct Border *)&optreqbo10};
static WORD optreqla27[6] = {415, 21, 415, 151, 9, 151};
static struct Border optreqbo10 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla28, (struct Border *)&optreqla29};
static WORD optreqla28[6] = {423, 0, 0, 0, 0, 171};
static struct Border optreqla29 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla30, NULL};
static WORD optreqla30[6] = {423, 1, 423, 171, 1, 171};
static struct Border optreqbo11 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla56, (struct Border *)&optreqla57};
static WORD optreqla56[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla57 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla58, NULL};
static WORD optreqla58[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo12 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla60, (struct Border *)&optreqla61};
static WORD optreqla60[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla61 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla62, NULL};
static WORD optreqla62[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo13 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla64, (struct Border *)&optreqla65};
static WORD optreqla64[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla65 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla66, NULL};
static WORD optreqla66[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo14 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla68, (struct Border *)&optreqla69};
static WORD optreqla68[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla69 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla70, NULL};
static WORD optreqla70[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo15 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla72, (struct Border *)&optreqla73};
static WORD optreqla72[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla73 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla74, NULL};
static WORD optreqla74[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo16 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla76, (struct Border *)&optreqla77};
static WORD optreqla76[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla77 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla78, NULL};
static WORD optreqla78[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo17 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla80, (struct Border *)&optreqla81};
static WORD optreqla80[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla81 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla82, NULL};
static WORD optreqla82[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo18 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla84, (struct Border *)&optreqla85};
static WORD optreqla84[6] = {23, 0, 0, 0, 0, 11};
static struct Border optreqla85 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla86, NULL};
static WORD optreqla86[6] = {23, 1, 23, 11, 1, 11};
static struct Border optreqbo19 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla88, (struct Border *)&optreqla89};
static WORD optreqla88[6] = {39, 0, 0, 0, 0, 11};
static struct Border optreqla89 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla90, NULL};
static WORD optreqla90[6] = {39, 1, 39, 11, 1, 11};
static struct Border optreqbo20 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla92, (struct Border *)&optreqla93};
static WORD optreqla92[6] = {111, 0, 0, 0, 0, 11};
static struct Border optreqla93 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla94, NULL};
static WORD optreqla94[6] = {111, 1, 111, 11, 1, 11};
static struct Border optreqbo21 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla96, (struct Border *)&optreqla97};
static WORD optreqla96[6] = {159, 0, 0, 0, 0, 11};
static struct Border optreqla97 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla98, NULL};
static WORD optreqla98[6] = {159, 1, 159, 11, 1, 11};
static struct Border optreqbo22 = {0, 0, 1, 2, 1, 3, (WORD *)optreqla100, (struct Border *)&optreqla101};
static WORD optreqla100[6] = {71, 0, 0, 0, 0, 11};
static struct Border optreqla101 = {0, 0, 2, 1, 1, 3, (WORD *)optreqla102, NULL};
static WORD optreqla102[6] = {71, 1, 71, 11, 1, 11};
static struct IntuiText optreqin1 = {1, 0, 1, 84, 6, NULL, (UBYTE *)optreqla31, (struct IntuiText *)&optreqin2};
static UBYTE optreqla31[31] = "BLITZ BASIC 2 COMPILER OPTIONS";
static struct IntuiText optreqin2 = {1, 0, 1, 20, 26, NULL, (UBYTE *)optreqla32, (struct IntuiText *)&optreqin3};
static UBYTE optreqla32[35] = "Create Icons for Executable Files:";
static struct IntuiText optreqin3 = {1, 0, 1, 20, 38, NULL, (UBYTE *)optreqla33, (struct IntuiText *)&optreqin8};
static UBYTE optreqla33[26] = "Runtime Error Debugger...";
static struct IntuiText optreqin4 = {1, 0, 1, 20, 38, NULL, (UBYTE *)optreqla34, (struct IntuiText *)&optreqin5};
static UBYTE optreqla34[10] = "Debugger:";
static struct IntuiText optreqin5 = {1, 0, 1, 212, 38, NULL, (UBYTE *)optreqla35, (struct IntuiText *)&optreqin6};
static UBYTE optreqla35[13] = "Long Arrays:";
static struct IntuiText optreqin6 = {1, 0, 1, 20, 50, NULL, (UBYTE *)optreqla36, (struct IntuiText *)&optreqin7};
static UBYTE optreqla36[15] = "Expand Macros:";
static struct IntuiText optreqin7 = {1, 0, 1, 212, 50, NULL, (UBYTE *)optreqla37, (struct IntuiText *)&optreqin8};
static UBYTE optreqla37[15] = "Quoted Macros:";
static struct IntuiText optreqin8 = {1, 0, 1, 20, 50, NULL, (UBYTE *)optreqla38, (struct IntuiText *)&optreqin9};
static UBYTE optreqla38[20] = "Make Smallest Code:";
static struct IntuiText optreqin9 = {1, 0, 1, 20, 62, NULL, (UBYTE *)optreqla39, (struct IntuiText *)&optreqin10};
static UBYTE optreqla39[40] = "Create Debug Info for Executable Files:";
static struct IntuiText optreqin10 = {1, 0, 1, 20, 78, NULL, (UBYTE *)optreqla40, (struct IntuiText *)&optreqin11};
static UBYTE optreqla40[15] = "Object Buffer:";
static struct IntuiText optreqin11 = {1, 0, 1, 212, 78, NULL, (UBYTE *)optreqla41, (struct IntuiText *)&optreqin12};
static UBYTE optreqla41[13] = "Libs Buffer:";
static struct IntuiText optreqin12 = {1, 0, 1, 20, 90, NULL, (UBYTE *)optreqla42, (struct IntuiText *)&optreqin13};
static UBYTE optreqla42[15] = "System Buffer:";
static struct IntuiText optreqin13 = {1, 0, 1, 212, 90, NULL, (UBYTE *)optreqla43, (struct IntuiText *)&optreqin14};
static UBYTE optreqla43[13] = "Data Buffer:";
static struct IntuiText optreqin14 = {1, 0, 1, 20, 102, NULL, (UBYTE *)optreqla44, (struct IntuiText *)&optreqin15};
static UBYTE optreqla44[14] = "Macro Buffer:";
static struct IntuiText optreqin15 = {1, 0, 1, 212, 102, NULL, (UBYTE *)optreqla45, (struct IntuiText *)&optreqin16};
static UBYTE optreqla45[15] = "String Buffer:";
static struct IntuiText optreqin16 = {1, 0, 1, 76-24, 118, NULL, (UBYTE *)optreqla46, (struct IntuiText *)&optreqin17};
static UBYTE optreqla46[16] = "Object Maximums";
static struct IntuiText optreqin17 = {1, 0, 1, 268, 118, NULL, (UBYTE *)optreqla47, NULL};
static UBYTE optreqla47[9] = "Resident";
static struct IntuiText optreqin18 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla59, NULL};
static UBYTE optreqla59[3] = "  ";
static struct IntuiText optreqin19 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla63, NULL};
static UBYTE optreqla63[3] = "  ";
static struct IntuiText optreqin20 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla67, NULL};
static UBYTE optreqla67[3] = "  ";
static struct IntuiText optreqin21 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla71, NULL};
static UBYTE optreqla71[3] = "  ";
static struct IntuiText optreqin22 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla75, NULL};
static UBYTE optreqla75[3] = "  ";
static struct IntuiText optreqin23 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla79, NULL};
static UBYTE optreqla79[3] = "  ";
static struct IntuiText optreqin24 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla83, NULL};
static UBYTE optreqla83[3] = "  ";
static struct IntuiText optreqin25 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla87, NULL};
static UBYTE optreqla87[3] = "  ";
static struct IntuiText optreqin26 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla91, NULL};
static UBYTE optreqla91[5] = " OK ";
static struct IntuiText optreqin27 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla95, NULL};
static UBYTE optreqla95[14] = " COMPILE/RUN ";
static struct IntuiText optreqin28 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla99, NULL};
static UBYTE optreqla99[20] = " CREATE EXECUTABLE ";
static struct IntuiText optreqin29 = {1, 0, 1, 4, 2, NULL, (UBYTE *)optreqla103, NULL};
static UBYTE optreqla103[9] = " CANCEL ";
static struct IntuiText optreqin34 = {1, 0, 1, 0, 0, NULL, (UBYTE *)optreqla108, NULL};
static UBYTE optreqla108[15] = "              ";
static struct IntuiText optreqin35 = {1, 0, 1, 0, 0, NULL, (UBYTE *)optreqla109, NULL};
static UBYTE optreqla109[15] = "              ";
static struct IntuiText optreqin36 = {1, 0, 1, 0, 0, NULL, (UBYTE *)optreqla110, NULL};
static UBYTE optreqla110[22] = "                     ";
static struct IntuiText optreqin37 = {1, 0, 1, 0, 0, NULL, (UBYTE *)optreqla111, NULL};
static UBYTE optreqla111[22] = "                     ";
struct Gadget optreqga1 = {(struct Gadget *)&optreqga2, 140, 78, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg1, 1, NULL};
struct StringInfo optreqsg1 = {(UBYTE *)optreqla48, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 16384, NULL};
static UBYTE optreqla48[12] = "16384";
struct Gadget optreqga2 = {(struct Gadget *)&optreqga3, 348, 78, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg2, 2, NULL};
struct StringInfo optreqsg2 = {(UBYTE *)optreqla49, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 32768, NULL};
static UBYTE optreqla49[12] = "16384";
struct Gadget optreqga3 = {(struct Gadget *)&optreqga4, 140, 90, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg3, 3, NULL};
struct StringInfo optreqsg3 = {(UBYTE *)optreqla50, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 4096, NULL};
static UBYTE optreqla50[12] = "4096";
struct Gadget optreqga4 = {(struct Gadget *)&optreqga5, 348, 90, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg4, 4, NULL};
struct StringInfo optreqsg4 = {(UBYTE *)optreqla51, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 4096, NULL};
static UBYTE optreqla51[12] = "4096";
struct Gadget optreqga5 = {(struct Gadget *)&optreqga6, 140, 102, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg5, 5, NULL};
struct StringInfo optreqsg5 = {(UBYTE *)optreqla52, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 8192, NULL};
static UBYTE optreqla52[12] = "8192";
struct Gadget optreqga6 = {(struct Gadget *)&optreqga7, 348, 102, 56, 8, 0, 2050, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg6, 6, NULL};
struct StringInfo optreqsg6 = {(UBYTE *)optreqla53, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 10240, NULL};
static UBYTE optreqla53[12] = "10240";
struct Gadget optreqga7 = {(struct Gadget *)&optreqga8, 132, 130, 40, 8, 0, 2051, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg7, 7, NULL};
struct StringInfo optreqsg7 = {(UBYTE *)optreqla54, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 100, NULL};
static UBYTE optreqla54[12] = "100";
struct Gadget optreqga8 = {(struct Gadget *)&optreqga9, 132, 138, 40, 8, 0, 2051, 4, NULL, NULL, NULL, 0, (APTR)&optreqsg8, 8, NULL};
struct StringInfo optreqsg8 = {(UBYTE *)optreqla55, (UBYTE *)undobuffer, 0, 12, 0, 0, 0, 0, 0, 0, NULL, 20, NULL};
static UBYTE optreqla55[12] = "20";
struct Gadget optreqga9 = {(struct Gadget *)&optreqga10, 384, 24, 24, 12, 128, 257, 1, (APTR)&optreqbo11, NULL, (struct IntuiText *)&optreqin18, 0, NULL, 9, NULL};
struct Gadget optreqga10 = {(struct Gadget *)&optreqga15, 384, 36, 24, 12, 128, 257, 1, (APTR)&optreqbo12, NULL, (struct IntuiText *)&optreqin19, 0, NULL, 10, NULL};
struct Gadget optreqga11 = {(struct Gadget *)&optreqga12, 176, 36, 24, 12, 128, 257, 1, (APTR)&optreqbo13, NULL, (struct IntuiText *)&optreqin20, 0, NULL, 11, NULL};
struct Gadget optreqga12 = {(struct Gadget *)&optreqga13, 384, 36, 24, 12, 128, 257, 1, (APTR)&optreqbo14, NULL, (struct IntuiText *)&optreqin21, 0, NULL, 12, NULL};
struct Gadget optreqga13 = {(struct Gadget *)&optreqga14, 176, 48, 24, 12, 128, 257, 1, (APTR)&optreqbo15, NULL, (struct IntuiText *)&optreqin22, 0, NULL, 13, NULL};
struct Gadget optreqga14 = {(struct Gadget *)&optreqga15, 384, 48, 24, 12, 0, 257, 1, (APTR)&optreqbo16, NULL, (struct IntuiText *)&optreqin23, 0, NULL, 14, NULL};
struct Gadget optreqga15 = {(struct Gadget *)&optreqga16, 384, 48, 24, 12, 0, 257, 1, (APTR)&optreqbo17, NULL, (struct IntuiText *)&optreqin24, 0, NULL, 15, NULL};
struct Gadget optreqga16 = {(struct Gadget *)&optreqga17, 384, 60, 24, 12, 0, 257, 1, (APTR)&optreqbo18, NULL, (struct IntuiText *)&optreqin25, 0, NULL, 16, NULL};
struct Gadget optreqga17 = {(struct Gadget *)&optreqga18, 8, 156, 40, 12, 0, 5, 1, (APTR)&optreqbo19, NULL, (struct IntuiText *)&optreqin26, 0, NULL, 17, NULL};
struct Gadget optreqga18 = {(struct Gadget *)&optreqga19, 56, 156, 112, 12, 0, 5, 1, (APTR)&optreqbo20, NULL, (struct IntuiText *)&optreqin27, 0, NULL, 18, NULL};
struct Gadget optreqga19 = {(struct Gadget *)&optreqga20, 176, 156, 160, 12, 0, 5, 1, (APTR)&optreqbo21, NULL, (struct IntuiText *)&optreqin28, 0, NULL, 19, NULL};
struct Gadget optreqga20 = {(struct Gadget *)&optreqga21, 344, 156, 72, 12, 0, 5, 1, (APTR)&optreqbo22, NULL, (struct IntuiText *)&optreqin29, 0, NULL, 20, NULL};
struct Gadget optreqga21 = {(struct Gadget *)&optreqga22, 180, 130, 16, 8, 4, 1, 1, (APTR)&upimage, NULL, NULL, 0, NULL, 21, NULL};
struct Gadget optreqga22 = {(struct Gadget *)&optreqga23, 180, 138, 16, 8, 4, 1, 1, (APTR)&downimage, NULL, NULL, 0, NULL, 22, NULL};
struct Gadget optreqga23 = {(struct Gadget *)&optreqga24, 388, 130, 16, 8, 4, 1, 1, (APTR)&upimage, NULL, NULL, 0, NULL, 23, NULL};
struct Gadget optreqga24 = {(struct Gadget *)&optreqga25, 388, 138, 16, 8, 4, 1, 1, (APTR)&downimage, NULL, NULL, 0, NULL, 24, NULL};
struct Gadget optreqga25 = {(struct Gadget *)&optreqga26, 20, 130, 112, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&optreqin34, 0, NULL, 25, NULL};
struct Gadget optreqga26 = {(struct Gadget *)&optreqga27, 20, 138, 112, 8, 0, 1, 1, NULL, NULL, (struct IntuiText *)&optreqin35, 0, NULL, 26, NULL};
struct Gadget optreqga27 = {(struct Gadget *)&optreqga28, 220, 130, 168, 8, 0, 3, 4, NULL, NULL, NULL, 0, (APTR)&optreqsgz, 27, NULL};
struct StringInfo optreqsgz = {(UBYTE *)((UBYTE *)resbuff), (UBYTE *)undobuffer, 0, 64, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};
struct Gadget optreqga28 = {NULL, 220, 138, 168, 8, 0, 3, 4, NULL, NULL, NULL, 0, (APTR)&optreqsgy, 28, NULL};
struct StringInfo optreqsgy = {(UBYTE *)((UBYTE *)resbuff+68), (UBYTE *)undobuffer, 0, 64, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};
struct Requester calcreq = {NULL, 100, 34, 376, 84, 0, 0, (struct Gadget *)&calcreqga1, (struct Border *)&calcreqbo1, (struct IntuiText *)&calcreqin1};
static struct Border calcreqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla1, (struct Border *)&calcreqla2};
static WORD calcreqla1[6] = {351, 28, 120, 28, 120, 39};
static struct Border calcreqla2 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla3, (struct Border *)&calcreqbo2};
static WORD calcreqla3[6] = {351, 29, 351, 39, 121, 39};
static struct Border calcreqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla4, (struct Border *)&calcreqla5};
static WORD calcreqla4[6] = {359, 24, 16, 24, 16, 43};
static struct Border calcreqla5 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla6, (struct Border *)&calcreqbo3};
static WORD calcreqla6[6] = {359, 25, 359, 43, 17, 43};
static struct Border calcreqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla7, (struct Border *)&calcreqla8};
static WORD calcreqla7[6] = {367, 20, 8, 20, 8, 63};
static struct Border calcreqla8 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla9, (struct Border *)&calcreqbo4};
static WORD calcreqla9[6] = {367, 21, 367, 63, 9, 63};
static struct Border calcreqbo4 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla10, (struct Border *)&calcreqla11};
static WORD calcreqla10[6] = {375, 0, 0, 0, 0, 83};
static struct Border calcreqla11 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla12, NULL};
static WORD calcreqla12[6] = {375, 1, 375, 83, 1, 83};
static struct Border calcreqbo5 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla16, (struct Border *)&calcreqla17};
static WORD calcreqla16[6] = {47, 0, 0, 0, 0, 11};
static struct Border calcreqla17 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla18, NULL};
static WORD calcreqla18[6] = {47, 1, 47, 11, 1, 11};
static struct Border calcreqbo6 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla20, (struct Border *)&calcreqla21};
static WORD calcreqla20[6] = {47, 0, 0, 0, 0, 11};
static struct Border calcreqla21 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla22, NULL};
static WORD calcreqla22[6] = {47, 1, 47, 11, 1, 11};
static struct Border calcreqbo7 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla24, (struct Border *)&calcreqla25};
static WORD calcreqla24[6] = {47, 0, 0, 0, 0, 11};
static struct Border calcreqla25 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla26, NULL};
static WORD calcreqla26[6] = {47, 1, 47, 11, 1, 11};
static struct Border calcreqbo8 = {0, 0, 1, 2, 1, 3, (WORD *)calcreqla28, (struct Border *)&calcreqla29};
static WORD calcreqla28[6] = {79, 0, 0, 0, 0, 11};
static struct Border calcreqla29 = {0, 0, 2, 1, 1, 3, (WORD *)calcreqla30, NULL};
static WORD calcreqla30[6] = {79, 1, 79, 11, 1, 11};
static struct IntuiText calcreqin1 = {1, 0, 1, 28, 30, NULL, (UBYTE *)calcreqla13, (struct IntuiText *)&calcreqin2};
static UBYTE calcreqla13[12] = "EXPRESSION:";
static struct IntuiText calcreqin2 = {1, 0, 1, 12, 6, NULL, (UBYTE *)calcreqla14, NULL};
static UBYTE calcreqla14[11] = "Calculator";
static struct IntuiText calcreqin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)calcreqla19, NULL};
static UBYTE calcreqla19[6] = " DEC ";
static struct IntuiText calcreqin4 = {1, 0, 1, 4, 2, NULL, (UBYTE *)calcreqla23, NULL};
static UBYTE calcreqla23[6] = " BIN ";
static struct IntuiText calcreqin5 = {1, 0, 1, 4, 2, NULL, (UBYTE *)calcreqla27, NULL};
static UBYTE calcreqla27[6] = " HEX ";
static struct IntuiText calcreqin6 = {1, 0, 1, 4, 2, NULL, (UBYTE *)calcreqla31, NULL};
static UBYTE calcreqla31[10] = " BYE BYE ";
static struct Gadget calcreqga1 = {(struct Gadget *)&calcreqga2, 124, 30, 224, 8, 0, 1, 0x1004, NULL, NULL, NULL, 0, (APTR)&calcreqsg1, 1, NULL};
static struct StringInfo calcreqsg1 = {(UBYTE *)calcreqla15, (UBYTE *)undobuffer, 0, 128, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};
static UBYTE calcreqla15[128] = "";
static struct Gadget calcreqga2 = {(struct Gadget *)&calcreqga3, 16, 48, 48, 12, 128, 257, 1, (APTR)&calcreqbo5, NULL, (struct IntuiText *)&calcreqin3, 0, NULL, 2, NULL};
static struct Gadget calcreqga3 = {(struct Gadget *)&calcreqga4, 312, 48, 48, 12, 0, 257, 1, (APTR)&calcreqbo6, NULL, (struct IntuiText *)&calcreqin4, 0, NULL, 3, NULL};
static struct Gadget calcreqga4 = {(struct Gadget *)&calcreqga5, 168, 48, 48, 12, 0, 257, 1, (APTR)&calcreqbo7, NULL, (struct IntuiText *)&calcreqin5, 0, NULL, 4, NULL};
static struct Gadget calcreqga5 = {NULL, 152, 68, 80, 12, 0, 5, 1, (APTR)&calcreqbo8, NULL, (struct IntuiText *)&calcreqin6, 0, NULL, 5, NULL};
struct Requester argreq = {NULL, 52, 42, 408, 56, 0, 0, (struct Gadget *)&argreqga1, (struct Border *)&argreqbo1, (struct IntuiText *)&argreqin1};
static struct Border argreqbo1 = {0, 0, 2, 1, 1, 3, (WORD *)argreqla1, (struct Border *)&argreqla2};
static WORD argreqla1[6] = {399, 20, 8, 20, 8, 31};
static struct Border argreqla2 = {0, 0, 1, 2, 1, 3, (WORD *)argreqla3, (struct Border *)&argreqbo2};
static WORD argreqla3[6] = {399, 21, 399, 31, 9, 31};
static struct Border argreqbo2 = {0, 0, 1, 2, 1, 3, (WORD *)argreqla4, (struct Border *)&argreqla5};
static WORD argreqla4[6] = {407, 0, 0, 0, 0, 55};
static struct Border argreqla5 = {0, 0, 2, 1, 1, 3, (WORD *)argreqla6, NULL};
static WORD argreqla6[6] = {407, 1, 407, 55, 1, 55};
static struct Border argreqbo3 = {0, 0, 1, 2, 1, 3, (WORD *)argreqla9, (struct Border *)&argreqla10};
static WORD argreqla9[6] = {39, 0, 0, 0, 0, 11};
static struct Border argreqla10 = {0, 0, 2, 1, 1, 3, (WORD *)argreqla11, NULL};
static WORD argreqla11[6] = {39, 1, 39, 11, 1, 11};
static struct Border argreqbo4 = {0, 0, 1, 2, 1, 3, (WORD *)argreqla13, (struct Border *)&argreqla14};
static WORD argreqla13[6] = {71, 0, 0, 0, 0, 11};
static struct Border argreqla14 = {0, 0, 2, 1, 1, 3, (WORD *)argreqla15, NULL};
static WORD argreqla15[6] = {71, 1, 71, 11, 1, 11};
static struct IntuiText argreqin1 = {1, 0, 1, 12, 6, NULL, (UBYTE *)argreqla7, NULL};
static UBYTE argreqla7[15] = "CLI Arguement:";
static struct IntuiText argreqin2 = {1, 0, 1, 4, 2, NULL, (UBYTE *)argreqla12, NULL};
static UBYTE argreqla12[5] = " OK ";
static struct IntuiText argreqin3 = {1, 0, 1, 4, 2, NULL, (UBYTE *)argreqla16, NULL};
static UBYTE argreqla16[9] = " CANCEL ";
static struct Gadget argreqga1 = {(struct Gadget *)&argreqga2, 12, 22, 384, 8, 0, 5, 4, NULL, NULL, NULL, 0, (APTR)&argreqsg1, 1, NULL};
static struct StringInfo argreqsg1 = {(UBYTE *)argreqla8, (UBYTE *)undobuffer, 0, 128, 0, 0, 0, 0, 0, 0, NULL, 0, NULL};
UBYTE argreqla8[128] = "";
static struct Gadget argreqga2 = {(struct Gadget *)&argreqga3, 8, 40, 40, 12, 0, 5, 1, (APTR)&argreqbo3, NULL, (struct IntuiText *)&argreqin2, 0, NULL, 2, NULL};
static struct Gadget argreqga3 = {NULL, 328, 40, 72, 12, 0, 5, 1, (APTR)&argreqbo4, NULL, (struct IntuiText *)&argreqin3, 0, NULL, 3, NULL};

/* ===================================================================
 * Type information requester (blitz2.s 15018-15286)
 * =================================================================== */

/* the 12 list lines (asm: typereqla30 + n*(typereqla31-typereqla30)) */
static UBYTE *const typelines[12] = {
    typereqla30, typereqla31, typereqla32, typereqla33, typereqla34, typereqla35,
    typereqla36, typereqla37, typereqla38, typereqla39, typereqla40, typereqla41,
};

/* blitz2.s:15018 zerotypes */
void zerotypes(void)
{
    int i;
    for (i = 0; i < 12; i++)
        memset(typelines[i], ' ', 27);
}

/* State of one type requester session, on menu6's stack and handed down.
   DIFF: the asm keeps it in globals (typeat 15028, maxtypes 15029, f1
   15054, f2 15055, f3 15056); nothing of it outlives menu6. */
struct typelist {
    WORD at;                            /* typeat: first item shown */
    WORD max;                           /* maxtypes: number of items */
    struct node *list;                  /* f1: first item (showlist) */
    WORD fields;                        /* f2: != 0 field list */
    WORD nameoff;                       /* f3: offset of the name */
};

static void showlist2(struct typelist *tl, struct node *item, WORD fields,
                      WORD nameoff);
static void reflist(void);

/* blitz2.s:15031 makebody - prop body: full if <=12 items, else 12/maxtypes */
static void makebody(const struct typelist *tl)
{
    UWORD body = 0xffff;                /* full bodied */
    if ((UWORD)tl->max > 12)
        body = (UWORD)((12UL << 16) / (UWORD)tl->max); /* divu */
    typereqvp1.VertBody = body;
    typereqvp1.VertPot = 0;
}

/* blitz2.s:15043 countlist */
static void countlist(struct typelist *tl, struct node *list)
{
    WORD count = 0;
    tl->at = 0;
    while (list) {
        count++;
        list = list->next;
    }
    tl->max = count;
}

/* blitz2.s:15059 showlist3 */
static void showlist3(struct typelist *tl)
{
    showlist2(tl, tl->list, tl->fields, tl->nameoff);
}

/* blitz2.s:15064 showlist */
static void showlist(struct typelist *tl, struct node *list, WORD fields,
                     WORD nameoff)
{
    tl->list = list;
    tl->fields = fields;
    tl->nameoff = nameoff;
    showlist2(tl, list, fields, nameoff);   /* falls through into showlist2 */
}

/* blitz2.s:15067 showlist2 - item (a2) = first item, fields (d2) = offset
   for .offset (0 for none) - i.e. field list mode, nameoff (d3) = offset
   for chars; tl->at = pos, tl->max = max */
static void showlist2(struct typelist *tl, struct node *item, WORD fields,
                      WORD nameoff)
{
    WORD skip, pos, rows;
    UBYTE *src, *dst, *end;
    UBYTE dims[8];                      /* asm: fuckoff 15057 */
    int line = 0;

    zerotypes();
    if ((UWORD)tl->at >= (UWORD)tl->max)
        return;
    for (skip = tl->at - 1; skip >= 0; skip--)  /* skip typeat items */
        item = item->next;

    pos = tl->at;
    for (rows = 11; rows >= 0; rows--) {
        struct var *fld = (struct var *)item;   /* field mode only */
        UBYTE c;

        src = (UBYTE *)item + nameoff;
        dst = typelines[line];              /* start */
        end = dst + 27;                     /* end */
        if (fields && (fld->flags & 1)) {
            c = '*';
            if (fld->flags & 4)
                c = '@';
            *dst++ = c;
        }
        for (;;) {                          /* .ok2 */
            c = *src++;
            *dst++ = c;
            if (c)  {                       /* .hello */
                if (dst < end)
                    continue;
                goto next;
            }
            break;
        }
        *--dst = ' ';
        if (!fields)
            goto next;                      /* no offset */
        *dst++ = '.';
        src = (UBYTE *)fld->type->name;     /* type stuff */
        for (;;) {                          /* .ok3 */
            if (dst >= end)
                goto next;
            c = *src++;
            *dst++ = c;
            if (!c)
                break;
        }
        *--dst = ' ';
        if (!(fld->flags & 2))
            goto next;
        *dst++ = '[';
        makelong(dims, (UWORD)fld->ndims);
        src = dims;
        for (;;) {                          /* .penis */
            if (dst >= end)
                goto next;
            c = *src++;
            *dst++ = c;
            if (!c)
                break;
        }
        *--dst = ']';
next:
        pos++;
        if ((UWORD)pos >= (UWORD)tl->max)
            return;
        line++;
        item = item->next;
    }
}

/* blitz2.s:15140 typeshow - list position from the prop gadget */
static void typeshow(struct typelist *tl)
{
    WORD oldat = tl->at;
    WORD range = tl->max - 11;

    if (range >= 0)
        tl->at = (WORD)(((ULONG)(UWORD)range * typereqvp1.VertPot) >> 16); /* mulu/swap */
    if (tl->at == oldat)
        return;
    showlist3(tl);
    reflist();                          /* falls through into reflist */
}

/* blitz2.s:15152 reflist */
static void reflist(void)
{
    ed_refgads(&typereqga5, 12);
}

/* blitz2.s:15158 allofem */
static void allofem(struct typelist *tl)
{
    struct node *types = (struct node *)firsttype;
    countlist(tl, types);
    makebody(tl);
    showlist(tl, types, 0, 11);
}

/* beep: DisplayBeep on the editor screen (comdata+40) */
static void typebeep(void)
{
    DisplayBeep((struct Screen *)CD_L(CD_MYSCREEN));
}

/* blitz2.s:15165 menu6 - Flashy Type Information.... */
void menu6(void)
{
    struct typelist tl;
    WORD gadget, idx;
    struct type *type;

    allofem(&tl);
    ed_openreq(&typereq);
    for (;;) {                                          /* .loop */
        ed_actgad(&typereqga2);
        if (!ed_getinput(&gadget))
            continue;
        if (gadget == 1) {
            /* prop gadget: poll and follow the knob until GADGETUP */
            for (;;) {                                  /* .vloop */
                LONG input = ed_getinput2(&gadget);
                if (input != 0 && (WORD)input == -1 && gadget == 1)
                    break;
                typeshow(&tl);                          /* .vdo */
            }
            typeshow(&tl);                              /* .vdone */
            continue;
        }
        if (gadget == 2) {
            /* name typed into TYPE: string gadget */
            UBYTE *src = typereqla21, *dst = namebuff;
            UWORD len;
            if (!*src) {
                allofem(&tl);
                goto reflist_;
            }
            len = (UWORD)-1;
            do {
                len++;
            } while ((*dst++ = *src++));
            type = findtype((struct node *)&firsttype, len, NULL);
            if (type)
                goto all;
            typebeep();
            continue;
        }
        if (gadget == 3) {
            typereqla21[0] = 0;
            allofem(&tl);
            goto reflist_;
        }
        if (gadget == 4)
            return;                                     /* FAREWELL */
        idx = gadget - 5;
        if (idx < 0 || (UWORD)idx >= 12)
            continue;
        /* idx = 0 to 11 */
        idx += tl.at;
        if ((UWORD)idx >= (UWORD)tl.max)
            continue;
        /* idx selected */
        if (tl.fields == 0) {
            /* goto type */
            type = firsttype;
            for (idx--; idx >= 0; idx--)
                type = type->next;
        } else {
            /* .intype: field -> its type */
            struct node *n = tl.list;
            for (idx--; idx >= 0; idx--)
                n = n->next;
            type = ((struct var *)n)->type;
            if (type->u.code < 256) {
                typebeep();
                continue;
            }
        }
        typename(type->name);                           /* .gotit */
all:
        {
            struct node *fl = (struct node *)type->u.fields;
            countlist(&tl, fl);
            makebody(&tl);
            showlist(&tl, fl, -1, 15);
        }
reflist_:
        reflist();
        ed_refgads(&typereqga1, 2);
    }
}

/* blitz2.s:15281 typename - copy into the TYPE: string buffer.
   A 31 char name ends at buffer byte 31, which stays 0 (MaxChars 32). */
void typename(const char *name)
{
    UBYTE *dst = typereqla21;
    do {
        if (!(*dst++ = *name++))
            break;
    } while (dst < typereqla21 + 31);
}

/* ===================================================================
 * .xtra / options (blitz2.s 15290-15731)
 * =================================================================== */

UBYTE vers = 0;                         /* 15343 (asm dc, accessed as byte) */
/* 15341 allchg: local to menu4c */

/* 15290 optspnts - pointers to options involved in source dependant stuff.
   groups of {byte size, address list}. */
static APTR const optsp1[] = {          /* one byte stuff... */
    &vers, (UBYTE *)&optreqga9.Flags + 1, NULL,
};
static APTR const jimi0[] = {           /* two byte stuff... */
    &optreqga10.Flags, &optreqga11.Flags, &optreqga12.Flags,
    &optreqga13.Flags, &optreqga14.Flags,
    &optreqga15.Flags, &optreqga16.Flags,
    &optreq2ga1.Flags, &optreq2ga2.Flags, &optreq2ga3.Flags,
    &optreq2ga4.Flags, &optreq2ga5.Flags, &optreq2ga6.Flags, &optreq2ga7.Flags,
    NULL,                               /* jimi1 */
};
static APTR const jimi2[] = {           /* four byte stuff: Maximums Int String Gadgets */
    &optreqsg1.LongInt, &optreqsg2.LongInt, &optreqsg3.LongInt,
    &optreqsg4.LongInt, &optreqsg5.LongInt, &optreqsg6.LongInt,
    NULL,                               /* jimi3 */
};
static struct StringInfo *const jimisg[] = {    /* their StringInfos (asm: la-8) */
    &optreqsg1, &optreqsg2, &optreqsg3, &optreqsg4, &optreqsg5, &optreqsg6, NULL,
};
static APTR const optspres[] = { resbuff, NULL };       /* Resident Buffer... */
static APTR const optspx[] = { xpathname, NULL };       /* Executable name! (xpath+xname) */
static const struct { UWORD size; APTR const *addrs; } optspnts[] = {
    {1, optsp1}, {2, jimi0}, {4, jimi2},
    {sizeof(resbuff), optspres}, {sizeof(xpathname), optspx},
    {0, NULL},                          /* all done... */
};

/* 15339 optssave: 2*1 + 14*2 + 6*4 + 544 + 384 = 982 bytes */
static UBYTE optssave[2 + 14 * 2 + 6 * 4 + sizeof(resbuff) + sizeof(xpathname)];

/* blitz2.s:15345 loadxtra - editor callback: r->d[7] file handle,
   r->a[6] DOSBase */
void loadxtra_c(struct m68kregs *r)
{
    BPTR fh = (BPTR)r->d[7];
    int g, i;
    jmp_buf jb, *olderr;

    freeres2();
    for (g = 0; optspnts[g].size; g++)
        for (i = 0; optspnts[g].addrs[i]; i++)
            if (Read(fh, optspnts[g].addrs[i], optspnts[g].size) == 0)
                goto done2;             /* only 0 (EOF) stops, -1 goes on */

    if (!vers) {
        /* old .xtra layout */
        optreq2ga1.Flags = 128;                 /* auto run on */
        optreq2ga2.Flags = optreq2ga6.Flags;    /* interupt checking on */
        optreq2ga3.Flags = 0;                   /* assembler checking off */
        optreq2ga4.Flags = optreq2ga7.Flags;    /* overflow chex */
    } else {
        Read(fh, argreqla8, 128);               /* load cli arg! */
    }

    /* .ver0: here, we load in current max settings. done last coz of
       unknown number of max's */
    olderr = errjmp;
    errjmp = &jb;
    ezerr = -1;                 /* not reset here: compile clears it */
    if (!setjmp(jb)) {
        for (;;) {                                      /* .dloop */
            struct blitzlib *l;
            ULONG entry;                                /* asm: temp1 */
            if (Read(fh, &entry, 4) == 0)
                break;
            l = findlibnoerr((UWORD)(entry >> 16));
            if (!l)
                break;
            LIBMAX_COUNT(l) = (WORD)entry;              /* temp1+2 */
        }
    }
    /* DIFF: asm left errstack/errcont pointing at .done2 */
    errjmp = olderr;
done2:
    /* load in residents */
    for (i = 0; i < MOSTRES; i++)
        resbuff[i].seg = 0;
    makeiallox();
    makemacbuff();                      /* fixed (#336) */
    loadres();
}

/* blitz2.s:15412 savextra - editor callback: r->d[7] file handle */
void savextra_c(struct m68kregs *r)
{
    BPTR fh = (BPTR)r->d[7];
    struct blitzlib *l;
    int g, i;

    vers = 1;                           /* .xtra version number... */
    for (g = 0; optspnts[g].size; g++)
        for (i = 0; optspnts[g].addrs[i]; i++)
            Write(fh, optspnts[g].addrs[i], optspnts[g].size);
    /* write out cli arguement... */
    Write(fh, argreqla8, 128);
    /* OK, now to write out max's: lib#.w, max.w */
    for (l = firstlib; l; l = l->next) {
        if (!LIBMAX_SHIFT(l))
            continue;
        Write(fh, &l->libnum, 2);
        Write(fh, &LIBMAX_COUNT(l), 2);
    }
}

/* copy all optspnts items to (save=1) / from (save=0) optssave */
static void optsnap(int save)
{
    UBYTE *snap = optssave;
    int g, i;
    for (g = 0; optspnts[g].size; g++)
        for (i = 0; optspnts[g].addrs[i]; i++) {
            if (save)
                memcpy(snap, optspnts[g].addrs[i], optspnts[g].size);
            else
                memcpy(optspnts[g].addrs[i], snap, optspnts[g].size);
            snap += optspnts[g].size;
        }
}

/* blitz2.s:15456 menu4 - options requester */
void menu4(void)
{
    int i;
    optsnap(1);
    /* MAKE INT GADS! */
    for (i = 0; jimisg[i]; i++)
        makelong(jimisg[i]->Buffer, jimisg[i]->LongInt);
    menu4c();                           /* falls through into menu4c */
}

/* blitz2.s:15483 menu4c */
void menu4c(void)
{
    WORD gadget, maxnum;
    WORD reschg = 0;                    /* 14680: resident change */
    WORD allchg = 0;                    /* 15341: alloc change */
    WORD maxat = 0;                     /* 14867: top row of the maximums */
    BOOL gad;
    struct blitzlib *l;
    struct StringInfo *sinfo;

    makemax(maxat);
    makeres();
    ed_openreq(&optreq);
    for (;;) {
        gad = ed_getinput(&gadget);
igot:
        if (!gad)
            continue;
        if ((UWORD)gadget >= 1 && (UWORD)gadget <= 6) {
            allchg = gadget;
            continue;
        }
        if (gadget == 21 || gadget == 22) {
            if (gadget == 21)
                maxat--;
            else
                maxat++;
            makemax(maxat);                             /* .newmax */
            ed_refgads(&optreqga7, 2);
            ed_refgads(&optreqga21, 2);
            ed_refgads(&optreqga25, 2);
            continue;
        }
        if (gadget == 7 || gadget == 8) {
            maxnum = maxat;
            sinfo = &optreqsg7;
            if (gadget == 8) {
                maxnum++;
                sinfo = &optreqsg8;
            }
            /* .ugw: next input first (ungadget etc), then store the value
               (low word of LongInt, asm optreqla54-6) and handle that input */
            gad = ed_getinput(&gadget);
            for (l = firstlib; l; l = l->next) {
                if (!LIBMAX_SHIFT(l))
                    continue;
                if (--maxnum < 0) {
                    LIBMAX_COUNT(l) = (WORD)sinfo->LongInt;
                    break;
                }
            }
            goto igot;
        }
        if (gadget == 10) {
            if (optreqga10.Flags & GFLG_SELECTED)
                runerropts();
            continue;
        }
        if (gadget == 23 || gadget == 24) {
            if (gadget == 23)
                resat--;                /* sub.l #68,resat */
            else
                resat++;
            newres();
            continue;
        }
        if (gadget == 27 || gadget == 28) {
            reschg = -1;
            newres();
            continue;
        }
        if ((UWORD)gadget < 17 || (UWORD)gadget > 20)
            continue;
        if (gadget == 20) {
            menu4canc();
            return;
        }
        optsdone(gadget, allchg, reschg);
        return;
    }
}

/* blitz2.s:15592 optsdone */
void optsdone(UWORD gadget, WORD allchg, WORD reschg)
{
    if (allchg) {
        makeiallox();
        makemacbuff();                  /* fixed (#336) */
    }
    if (reschg)
        loadres();
    if (gadget == 18)
        menu0();                        /* jmp menu0 */
    else if (gadget == 19)
        menu1();                        /* jmp menu1 */
}

/* blitz2.s:15612 menu4canc - cancel */
void menu4canc(void)
{
    optsnap(0);
}

/* blitz2.s:15627 newres */
void newres(void)
{
    makeres();
    ed_refgads(&optreqga23, 2);
    ed_refgads(&optreqga27, 2);
}

/* blitz2.s:15637 makeres - convert resident list to string gads;
   resat=top gad. */
void makeres(void)
{
    optreqsgz.BufferPos = 0;
    optreqsgz.DispPos = 0;
    optreqsgy.BufferPos = 0;
    optreqsgy.DispPos = 0;
    optreqga23.Flags &= ~GFLG_DISABLED;
    optreqga24.Flags &= ~GFLG_DISABLED;
    optreqsgz.Buffer = (UBYTE *)resat->name;
    if (resat == resbuff)
        optreqga23.Flags |= GFLG_DISABLED;
    optreqsgy.Buffer = (UBYTE *)resat[1].name;
    if (&resat[1] == &resbuff[7])
        optreqga24.Flags |= GFLG_DISABLED;
}

/* blitz2.s:15662 makemax - put from maxat into gadgets..... */
void makemax(WORD maxat)
{
    WORD num;

    memset(optreqla54, ' ', 12);
    memset(optreqla55, ' ', 12);
    memset(optreqla108, ' ', 14);
    memset(optreqla109, ' ', 14);
    optreqga7.Flags |= GFLG_DISABLED;
    optreqga25.Flags |= GFLG_DISABLED;
    optreqga21.Flags &= ~GFLG_DISABLED;         /* up! */
    optreqga22.Flags &= ~GFLG_DISABLED;         /* down! */
    num = maxat;
    if (num == 0)
        optreqga21.Flags |= GFLG_DISABLED;      /* no up arrow */
    if ((UWORD)num >= (UWORD)nummaxs)
        goto nsec;
    optreqga7.Flags &= ~GFLG_DISABLED;          /* .doit */
    optreqga25.Flags &= ~GFLG_DISABLED;
    maxit(maxat, optreqla108, optreqla54);
    num = maxat + 1;
    if (num == nummaxs)
        goto nsec;
    optreqga8.Flags &= ~GFLG_DISABLED;
    optreqga26.Flags &= ~GFLG_DISABLED;
    maxit(maxat + 1, optreqla109, optreqla55);
    num = maxat + 2;
    if ((UWORD)num >= (UWORD)nummaxs)
        goto ndown;
    return;
nsec:
    optreqga8.Flags |= GFLG_DISABLED;
    optreqga26.Flags |= GFLG_DISABLED;
ndown:
    optreqga22.Flags |= GFLG_DISABLED;          /* no down */
}

/* blitz2.s:15713 maxit - which-th lib with a maximum: object name + 's'
   (no NUL) to label, maximum (makelong) to num */
void maxit(WORD which, UBYTE *label, UBYTE *num)
{
    struct blitzlib *l;
    const UBYTE *name;

    for (l = firstlib; ; l = l->next) {
        if (!l)
            return;     /* DIFF: asm walks on through address 0 (cannot
                           happen, makemax checks nummaxs) */
        if (!LIBMAX_SHIFT(l))
            continue;
        if (--which < 0)
            break;
    }
    name = (const UBYTE *)l + L_(LIBMAX_E(l) - 26); /* token offset */
    while ((*label++ = *name++))
        ;
    label[-1] = 's';
    makelong(num, (UWORD)LIBMAX_COUNT(l));
}

/* options.src optreqprep - chip mem copy of the arrow images.
   BUG(orig) (#8): AllocMem result not checked. */
void optreqprep(void)
{
    UBYTE *m = AllocMem(arrowsize, MEMF_CHIP);
    uimageloc = (UWORD *)m;
    dimageloc = (UWORD *)(m + arrowsize / 2);
    CopyMem((APTR)uparrow, m, arrowsize);
}

/* options.src optreqdone */
void optreqdone(void)
{
    FreeMem(uimageloc, arrowsize);
}

/* ===================================================================
 * Calculator (blitz2.s 15737-15893)
 * =================================================================== */

/* 15737 calclibat: local to menu7 */

/* blitz2.s:15739 menu7 - calculator */
void menu7(void)
{
    static struct Gadget *const basegads[3] = {&calcreqga2, &calcreqga3, &calcreqga4};
    jmp_buf jb, *volatile olderr;
    WORD gadget;
    ULONG calclibat = libat;            /* 15737: libat before the expression */

    olderr = errjmp;
    errjmp = &jb;
    calcreqla15[0] = 0;
    ed_openreq(&calcreq);
    if (setjmp(jb)) {
        /* .calcerr (errcont, a7 = errstack) */
        libat = calclibat;
        calcreqla15[0] = 0;
        goto pc_;
    }
loopq:
    ed_refgads(&calcreqga2, 3);
    for (;;) {                                          /* .loop */
        while (!ed_actgad(&calcreqga1))
            ;
        if (!ed_getinput(&gadget))
            continue;
        if (gadget == 1) {
            /* calculate the expression */
            LONG value;
            ULONG v;
            UBYTE *dst;
            WORD bitcount, started;

            ed_tokone(calcreqla15);             /* tokenise line..... */
            setcvars();
            setsvars();
            connest = 0;                        /* no idea... */
            srcptr = calcreqla15;
            value = (LONG)evalconst2(NULL, NULL);     /* evaluate constant */
            dst = calcreqla15;
            v = (ULONG)value;
            if (value < 0) {
                *dst++ = '-';
                v = -v;     /* review: neg.l; unsigned so $80000000 stays defined */
            }
            if (calcreqga2.Flags == 128) {
                makelong(dst, v);               /* .dec: to long... */
            } else {
                started = 0;
                if (calcreqga3.Flags == 128) {
                    /* bin Print */
                    *dst++ = '%';
                    for (bitcount = 31; bitcount >= 0; bitcount--) {
                        ULONG bit = v & 0x80000000UL;
                        v <<= 1;
                        if (bit) {
                            *dst++ = '1';
                            started = -1;
                        } else if (started)
                            *dst++ = '0';
                    }
                } else {
                    /* .hex */
                    *dst++ = '$';
                    for (bitcount = 7; bitcount >= 0; bitcount--) {
                        UWORD digit = (UWORD)(v >> 28) + '0';
                        if (digit >= 58)
                            digit += 7;
                        if (digit != '0') {
                            *dst++ = (UBYTE)digit;
                            started = -1;
                        } else if (started)
                            *dst++ = (UBYTE)digit;
                        v <<= 4;
                    }
                }
                if (!started)                   /* .bdone */
                    *dst++ = '0';
                *dst = 0;                       /* .pc1 */
            }
            goto pc_;
        }
        if ((UWORD)gadget < 5) {
            /* new number base selected */
            WORD idx = gadget - 2, i;
            basegads[idx]->Flags = 0;
            for (i = 0; i < 3; i++)
                if (basegads[i]->Flags == 128) {
                    basegads[i]->Flags = 0;
                    break;
                }
            basegads[idx]->Flags = 128;
            goto loopq;
        }
        if (gadget == 5)
            break;                              /* BYE BYE */
        continue;
pc_:
        calcreqsg1.BufferPos = 0;
        calcreqsg1.DispPos = 0;
        ed_refgads(&calcreqga1, 1);
    }
    errjmp = olderr;    /* DIFF: asm left errstack/errcont set */
}

/* ===================================================================
 * CLI argument requester (blitz2.s 15895-16015)
 * =================================================================== */

/* blitz2.s:15979 menu9 - CLI ARGUEMENT */
void menu9(void)
{
    WORD gadget;

    memcpy(namebuff, argreqla8, 128);
    ed_openreq(&argreq);
    ed_actgad(&argreqga1);
    for (;;) {
        if (!ed_getinput(&gadget))
            continue;
        if (gadget == 1 || gadget == 2)
            return;
        if (gadget == 3) {
            /* cancel */
            memcpy(argreqla8, namebuff, 128);
            return;
        }
    }
}
