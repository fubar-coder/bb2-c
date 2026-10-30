/*
 * state.h - the editor globals (ted.s data block 6057-6165 and 6267-6280,
 * chunk 06), plus the globals of other chunks that the core uses.
 *
 * Original names, original sizes (dc = 16 bit, dc.l = 32 bit), original
 * initial values (defined in state.c).  Signedness chosen by use; where the
 * asm mixes signed and unsigned compares on the same variable, the
 * translator writes the cast at the compare (WORD)/(UWORD).
 *
 * Word pairs that the asm copies/compares as ONE long (cursx/cursy,
 * cursx2/cursy2, cursoff/curstop, btopx/btopy, bbotx/bboty, bloktopx/
 * bloktopy, blokbotx/blokboty) are separate variables here: copy/compare
 * both fields explicitly (PLAN.md 1.2).  The asm abuses bloktopx(+y) as a
 * long pointer (blokload 1754/1793) and as a 1 byte write buffer
 * (bloksave 1694): use locals and keep the side effect on bloktopx/bloktopy
 * where it is observable (no defect: only read during a drag).
 *
 * Globals that only the Amiga side needs and that have AmigaOS types
 * (mywindow, myrast, myscreen, the comdata table ...) are NOT here: see
 * src/structs.h (comdata) and the chunk headers of src/.
 *
 * Owner: coordinator (state.c defines everything declared here).
 */
#ifndef TED_STATE_H
#define TED_STATE_H

#include <setjmp.h>
#include "tedcore.h"

/* -------------------------------------------- screen geometry (6057-6066) */

extern UWORD topskip;           /* 6057 0   pixels taken by intuition (top) */
extern UWORD botskip;           /* 6058 0   ditto (bottom) */
extern UWORD chcols;            /* 6059 128 columns in the character map
                                            (= max. line length; never changed) */
extern UWORD totsccols;         /* 6060 80  total on screen char columns */
extern UWORD edrows;            /* 6061 0   on screen text rows */
extern UWORD pixrows;           /* 6062 0   editable screen pixel rows */
extern UWORD pixrows2;          /* 6063 0   bpr * pixrows (word, truncated) */
extern UWORD edcols;            /* 6064 0   editable columns */
extern UWORD mousecols;         /* 6065 0   columns of the label column (0/12) */
extern UBYTE *bitmaps[2];       /* 6066 0,0 plane 0 / plane 1 + topskip*bpr */

/* ---------------------------------------------- cursor (6067-6078) */

extern UWORD cursx;             /* 6067 0 screen column */
extern UWORD cursy;             /* 6068 0 screen row */
extern LONG  scloc;             /* 6069 0 byte offset of the cursor row in a plane */
extern UWORD chloc;             /* 6070 0 offset of the cursor row in chmap */
extern UWORD cursx2;            /* 6071 storepos/restorepos copies */
extern UWORD cursy2;            /* 6072 */
extern LONG  scloc2;            /* 6073 */
extern UWORD chloc2;            /* 6074 */
extern WORD  cursoff;           /* 6075 0 horizontal scroll offset (can become
                                          negative/odd, survey-06 I.5) */
extern UWORD curstop;           /* 6076 0 first visible line */
extern UWORD color;             /* 6077 1 current pen (bits tested via color+1) */
extern UWORD qualifier;         /* 6078 0 qualifier of the last raw key */
extern UBYTE *chmap;            /* 6079 0 character map: edrows x chcols bytes,
                                          bit 7 = drawn in token colour; the
                                          line being edited exists ONLY here */
extern UWORD anyalt;            /* 6080 0 line altered (getline) */

/* --------------------------------------------------- line list (6081-6084) */

extern struct item *firstitem;  /* 6081 */
extern struct item *enditem;    /* 6082 */
extern struct item *thisitem;   /* 6083 line the cursor is on (a4 in mainedit) */
extern UWORD numlines;          /* 6084 */

/* ------------------------------------------------------ block (6085-6094) */

extern WORD  bloktopx, bloktopy;   /* 6085/6086 mouse marking: anchor */
extern WORD  blokbotx, blokboty;   /* 6087/6088 mouse marking: end */
extern WORD  btopx;             /* 6089 block start column (signed compares) */
extern UWORD btopy;             /* 6090 block start line (unsigned compares) */
extern WORD  bbotx;             /* 6091 block end column (inclusive; -1 possible) */
extern UWORD bboty;             /* 6092 block end line; $FFFF (-1) = no block */
extern UWORD invflag;           /* 6093 tokit quote mode AND mousedown direction */
extern UWORD inverse;           /* 6094 0/$FFFF: print inverted (byte mask) */

/* ------------------------------------------- memory / file (6095-6097) */

/* 6095 memstack: asm a7 of the dispatch in mainedit, target of outmem
   (`move.l memstack,a7 / bra memcont`).  C: the setjmp buffer of mainedit;
   NULL before the first dispatch (the original then sets a7 = 0 - see
   notes/PHASE2B-BRIEF.md, open decision O1). */
extern jmp_buf *memstack;
extern tedfh filehand;          /* 6096 0 open file, closed by outmem */
extern UWORD memalt;            /* 6097 0 memory changed -> printmem */

/* --------------------------------- option block defstart..defend (6099) */

/* 50 bytes = 25 words, the raw file image of l:blitzeditor.opts (loaddefs/
   savedefs read/write it as one block; setdefs and `defaults` copy it word
   by word; defaults reads the backup copy at fixed offsets 0/6/8/10).
   Words only, so no padding on any platform; on the host the file bytes
   are converted with defs_frombytes()/defs_tobytes() (fileio.h). */
struct defblock {
    UWORD d_fonthite;           /* +0  8 */
    UWORD d_fonthite2;          /* +2  7  (= fonthite - 1, for dbf) */
    UWORD d_fontmul;            /* +4  0  fonthite * bmapmul */
    UWORD d_tokenise;           /* +6  0  0, 1 (getstuff) or $FFFF (tokall) */
    UWORD d_makeicons;          /* +8  1 */
    UWORD d_normcol;            /* +10 1  normal colour */
    UWORD d_labcol;             /* +12 2  label colour */
    UWORD d_tokcol;             /* +14 3  token colour */
    UWORD d_propcol;            /* +16 0  colour edited with the sliders */
    UWORD d_hscmarg;            /* +18 12 margin for horizontal scroll */
    UWORD d_vscmarg;            /* +20 6  margin for vertical scroll */
    UWORD d_tabstop;            /* +22 2 */
    UWORD d_palit[12];          /* +24 4 x (r,g,b): 8,8,8 15,15,15 0,0,0 15,13,0 */
    UWORD d_origscrows;         /* +48 0  unused */
};
extern struct defblock defstart;        /* 6099 (defend = 6120 = +50) */
#define defend      (&defstart + 1)
#define fonthite    (defstart.d_fonthite)
#define fonthite2   (defstart.d_fonthite2)
#define fontmul     (defstart.d_fontmul)
#define tokenise    (defstart.d_tokenise)
#define makeicons   (defstart.d_makeicons)
#define normcol     (defstart.d_normcol)
#define labcol      (defstart.d_labcol)
#define tokcol      (defstart.d_tokcol)
#define propcol     (defstart.d_propcol)
#define hscmarg     (defstart.d_hscmarg)
#define vscmarg     (defstart.d_vscmarg)
#define tabstop     (defstart.d_tabstop)
#define palit       (defstart.d_palit)
#define origscrows  (defstart.d_origscrows)

/* ------------------------------------------- misc. data (6122-6165) */

extern const char defname[];    /* 6122 'l:blitzeditor.opts' */
extern UWORD mpalit[9];         /* 6125 mouse pointer colours 15,15,0 15,0,0 0,0,0 */
extern UBYTE *statsloc;         /* 6129 bitmap address of the status line */
extern char statstext[];        /* 6130 'Line:      Column:    Largest Mem (K):      ' */
extern char bltext[6];          /* 6133 'Block' / 5 spaces (bloken/blokdis) */
extern char numtext[6];         /* 6135 dc 0,0,0: number buffer (also written as long) */
extern UWORD defalt;            /* 6136 options changed -> savedefs at the end */
extern APTR req;                /* 6137 current requester template (openreq) */
extern UWORD allalt;            /* 6138 source changed */
/* 6139: dc ' :' sentinel in front of filepath (.tomain reads filepath[-2]
   for an empty path).  Not reproduced: outside.tomain tests the empty
   path explicitly (same result, survey-01 G). */
extern char filepath[192];      /* 6140 path of the source */
extern char filename[64];       /* 6142 name of the source */
extern const char filetext[];   /* 6144 'Name of file to load' */
extern const char filetext2[];  /* 6145 'Name to save file as' */
extern char blokpath[192];      /* 6146 */
extern char blokname[64];       /* 6148 */
extern const char bloktext[];   /* 6150 'Name of block to load' */
extern const char bloktext2[];  /* 6151 'Name to save block as' */
extern UWORD blokop;            /* 6152 block operation running (getbchar) */
/* 6154 mywindow, 6155 myrast: src/input.h (AmigaOS types) */

/* one AllocMem in outside (ted.s:41): doslen + 3*(chcols+2) bytes,
   dosbuff first; charbuff4 = END of the allocation (0 bytes, never
   used for data, survey-01 I.9).  The host allocates the same contiguous block. */
extern UBYTE *charbuff;         /* 6157 dosbuff + doslen */
extern UBYTE *charbuff2;        /* 6158 charbuff + chcols + 2 */
extern UBYTE *charbuff3;        /* 6159 charbuff2 + chcols + 2 */
extern UBYTE *charbuff4;        /* 6160 charbuff3 + chcols + 2 (end) */
extern UBYTE *dosbuff;          /* 6161 */

/* 6163 nummice is dc.l but only its UPPER word is used (move/addq .w on the
   label = big-endian high word, survey-03 G): a UWORD here. */
extern UWORD nummice;           /* 6163 */
extern struct micenode *mice;   /* 6164 */
extern UWORD mousealt;          /* 6165 label list changed -> showmice */

extern const char tokeerr[];    /* 6167 '?????',0,'Token Unavailable',0 */

extern const char error1[];     /* 6260 "Couldn't open file!" */
extern const char error2[];     /* 6262 'Error reading file!' */
extern const char error3[];     /* 6264 'Error writing file!' */

/* ---------------------------- returncode, external, edstruct copy (6267) */

extern WORD returncode;         /* 6267 0 (outside: -1, end of mainedit: 0) */
extern APTR external;           /* 6268 0 edstruct of blitz2, 0 = standalone */
/* 6269-6280: the 28 bytes outside copies from edstruct (struct edstruct in
   src/structs.h).  Separate variables here (outside copies field by field,
   no observable difference).  The callbacks are register based blitz2
   code: called only through the glue (call_loadxtra ... in hooks.h). */
extern struct tokennode *tokens;   /* 6269 token list (!= 0 -> tokenise = 1) */
extern UBYTE *menitems;         /* 6270 COMPILER menu text */
extern APTR loadxtra;           /* 6272 callback after loading (d7 fh, a6 DOS) */
extern APTR savextra;           /* 6273 callback after saving */
extern APTR menurout;           /* 6275 menu callback (extmenu) */
extern APTR cleanup;            /* 6276 callback of doclean (0 in blitz2) */
extern UBYTE colon;             /* 6277 255 = off; blitz2 ':' */
extern UBYTE quote;             /* 6278 255; '"' */
extern UBYTE label;             /* 6279 255; '.' */
extern UBYTE remark;            /* 6280 255; ';' */

/* -------------------------- globals of other chunks used by the core */

/* chunk 01 (ted.s:636-639, 742) - .xtra/.info names, defined in loadsave.c */
extern UBYTE *xtraat;           /* 636 name the extension is appended to */
extern UBYTE *xtranull;         /* 637 where the extension starts */
/* chunk 03 (ted.s:2455) - F1 block start, defined in services.c */
extern WORD bstx;               /* 2455 -1 = none */
extern WORD bsty;               /* 2456 0 */
/* chunk 07 (ted.s:7061/7072) - find/replace text: the string gadget buffers
   of findreq; defined in state.c so the core (isithere2, replace2) and
   data.c (StringInfo.Buffer) share them */
extern UBYTE findreqla16[64];   /* 7061 search text */
extern UBYTE findreqla17[64];   /* 7072 replace text */

/* ----------------------------------------------------- new (not in asm) */

/* escape (ted.s:1016) leaves mainedit with `lea 12(a7),a7` + freeall.
   C: escape sets edquit, mainedit returns right after the handler
   (DIFF: stack trick, survey-02 G). */
extern BOOL edquit;
/* asm d7/d6 at the entry of a jumptable/menutable handler (getline's
   result: event code, gadget id / menu code).  Set by mainedit before the
   dispatch; handlers that read d7/d6 (menu, blockend's d0 byte) use these.
   Globals (not parameters) because the jumptable/menutable handlers are
   `void name(void)` like the asm entries and are also called directly. */
extern LONG ev_code;
extern UWORD ev_data;

#endif
