/*
 * parse.h - chunk 05 (blitz2.s 6740-8450): static structs / data start,
 * savefile, the reloc offset list, the character fetch primitives
 * (getchar family incl. conditional compilation and `~expr~`), err and the
 * error stubs, char classes, name lists (findlab/addhere), freeing,
 * fetchvar and calcvar.  See notes/survey-05.md.
 */
#ifndef BLITZ2_PARSE_H
#define BLITZ2_PARSE_H

#include <setjmp.h>
#include <exec/types.h>
#include "structs.h"

#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

/* ================================================== persistent registers */

/* asm a5 of the parser: the source char pointer (tokenised line, NUL
   terminated).  getchar* advance it; it may temporarily point into
   cmakebuff (`~expr~`) or usedpath (`\var`), getchar returns from those
   transparently (marker bytes 1 and 2). */
extern UBYTE *srcptr;

/* asm Z=1 after getchar3 / reget / gchar2 (and routines ending in
   `bra reget`): end of statement.  TRUE if c==0, or (quoteflag==0 and
   (c==':' or c==0x8022 [Else token])).  Valid right after the call (uses the
   current quoteflag, like the asm). */
BOOL eos(UWORD c);

/* ================================================== character fetch */
/* Characters/tokens are UWORD: plain char, or 0x8000|n for a 2-byte token.
   0 = end of line.  All of them set lastchar (except reget/bakup). */

/* blitz2.s:7042 getchar - next char, handles CNIF/CSIF/CELSE/CEND
   ($801c-$801f), skips everything while comflag==0 (except 0), expands
   `~expr~` (cmake) and the end markers 1 (cmakea5) / 2 (usedfrom).
   Sets prevloc/oldqflag (1-level undo for bakup) and lastchar.
   asm flags not meaningful. */
UWORD getchar_(void);
/* blitz2.s:7147 getcharb - raw fetch: byte, 2-byte token, '"' toggles
   quoteflag, ';' outside quotes skips the comment (returns 0).  After a 0,
   srcptr points past the NUL. */
UWORD getcharb(void);
/* blitz2.s:7172 getchar2 - getchar_, skipping spaces outside quotes */
UWORD getchar2(void);
/* blitz2.s:7184 getchar3 - getchar2 + classification (use eos()).  Inside
   quotes chars with low byte < 32 are mapped via gs[] (1..25 -> 0, 26..31 ->
   umlauts). */
UWORD getchar3(void);
/* blitz2.s:7202 reget - lastchar, classified like getchar3 (use eos()) */
UWORD reget(void);
/* blitz2.s:7205 bakup - srcptr=prevloc, quoteflag=oldqflag (lastchar kept) */
void bakup(void);
/* blitz2.s:7209 storeloc - locchar=c, locloc=srcptr */
void storeloc(UWORD c);
/* blitz2.s:7215 resloc - srcptr=locloc; returns locchar (also -> lastchar) */
UWORD resloc(void);

extern UWORD cmake;         /* 7039 !=0 while evaluating `~expr~` */
extern UWORD ciflineat;     /* 7145 line of last CNIF/CSIF */

/* ================================================== errors */

/* blitz2.s:7641 err (= comperror) - never returns.
   dirmode: errmsg=msg, longjmp (asm .quiet: d0=msg, no pointeron/report).
   else: pointeron; unless ezerr: cfetchmode -> libat=oldlibat, constmode ->
   asmerr2(), locals -> droplocals(), anyerrs=-1, menuret=linenumat,
   endstop(); then the editor error callback (comdata+12, a0=msg);
   errmsg=msg; longjmp(*errjmp, 1).
   errjmp/errmsg replace the asm errstack/errcont pair (see CONVENTIONS §6).
   errjmp==NULL: cannot happen in the asm (errcont always set by
   compile/direct mode/loaders before anything can fail).  DIFF: C reports
   the error as above and then calls exit(RETURN_FAIL). */
NORETURN void err(const char *msg);
#define comperror err
extern jmp_buf *errjmp;
extern const char *errmsg;
extern WORD ezerr;          /* 7636 !=0: simple error, skip compile cleanup */

/* blitz2.s:7360/7361 asmerr2/asmerr - NOT errors: linenumat = asm line of
   the current asm entry (asmerr2: asmbuff; asmerr: given node), return. */
void asmerr2(void);
void asmerr(struct asmnode *a);

/* blitz2.s:6797 execerr -> makeexecerr */
NORETURN void execerr(void);

/* error stubs (asm: bsr err / dc.b 'text'), blitz2.s 7222-7634 */
NORETURN void badstoperr(void);    /* "Debugger must be enabled for STOP" */
NORETURN void makeexecerr(void);   /* "Error creating Executable" */
NORETURN void fornexterr(void);    /* "Duplicate For...Next Error" */
NORETURN void excerr2(void);       /* "Can't Exchange different types" */
NORETURN void excerr(void);        /* "Can't Exchange NewTypes" */
NORETURN void arrerr1(void);       /* "Illegal Array type" */
NORETURN void nocenderr(void);     /* "CNIF/CSIF without CEND" */
NORETURN void referr(void);        /* "Label reference out of context" */
NORETURN void illlaberr(void);     /* "Label has been used as a Constant" */
NORETURN void selerrz(void);       /* "Select without End Select" */
NORETURN void unterr1(void);       /* "Until without Repeat" */
NORETURN void unterr2(void);       /* "Repeat Block too large" */
NORETURN void unterr3(void);       /* "Repeat without Until" */
NORETURN void vbokerr1(void);      /* "AbortVBOK MUST be in a SETINT 5" */
NORETURN void errerr1(void);       /* "SetErr not allowed in Procedures" */
NORETURN void errerr2(void);       /* "Can't nest SetErr" */
NORETURN void errerr3(void);       /* "End SetErr without SetErr" */
NORETURN void noarrayerr(void);    /* "Array not found" */
NORETURN void nolisterr(void);     /* "Array is not a List" */
NORETURN void defaerr(void);       /* "Default without Select" */
NORETURN void eselerr(void);       /* "End Select without Select" */
NORETURN void cbraerr(void);       /* "Previous Case Block too Large" */
NORETURN void caseerr(void);       /* "Case Without Select" */
NORETURN void tokeerr(void);       /* "Illegal TokeJsr token number" */
NORETURN void opterr(void);        /* "Optimizer Error! - $ffff,$ffff." (opchars/opchars2 never patched) */
NORETURN void blitzerr(void);      /* "Only Available in Blitz mode" */
NORETURN void amigaerr(void);      /* "Only Available in Amiga mode" */
NORETURN void interr4(void);       /* "Illegal Interrupt Number" */
NORETURN void interr5(void);       /* "Illegally nested Interrupts" */
NORETURN void interr3(void);       /* "Can't use Set/ClrInt in Local Mode" */
NORETURN void interr(void);        /* "End SetInt without SetInt" */
NORETURN void interr2(void);       /* "SetInt without End SetInt" - sets linenumat=intline first */
NORETURN void conmemerr(void);     /* "Expression too Complex" */
NORETURN void pserr(void);         /* "Precedence Stack Overflow" */
NORETURN void clasherr(void);      /* "Clash in Residents" */
NORETURN void reserr(void);        /* "Can't Load Resident" */
NORETURN void dupofferr(void);     /* "Duplicate Offset Error" */
NORETURN void notparerr(void);     /* "Not Enough Parameters" */
NORETURN void badconterr(void);    /* "Cont not Currently Available" */
NORETURN void dirliberr(void);     /* "Library not Available in Direct Mode" */
NORETURN void baddirerr(void);     /* "Illegal direct mode command" */
NORETURN void nodirmem(void);      /* "Not enough room in direct mode buffer" */
NORETURN void illdirerr(void);     /* "Can't Create in Direct Mode" */
NORETURN void nosuperr(void);      /* "Not Supported" */
NORETURN void freeerr(void);       /* "Illegal Token" */
NORETURN void mbovererr(void);     /* "Macro Buffer Overflow" */
NORETURN void illvgerr(void);      /* "Illegal VGoto/VGosub Parameter" */
NORETURN void notqerr(void);       /* "No Terminating Quote" */
NORETURN void illcgeterr(void);    /* "Can't Use Constant" */
NORETURN void illconerr(void);     /* "Can't Assign Constant" */
NORETURN void redimerr(void);      /* "Illegal number of Dimensions" */
NORETURN void illeolerr(void);     /* "Garbage at End of Line" */
NORETURN void illelseerr(void);    /* "Illegal Else in While Block" */
NORETURN void illgotoerr(void);    /* "Can't Goto/Gosub a Procedure" */
NORETURN void illtraperr(void);    /* "Illegal Trap Vector" */
NORETURN void illimmerr(void);     /* "Illegal Immediate Value" */
NORETURN void illabserr(void);     /* "Illgeal Absolute" */
NORETURN void illdiserr(void);     /* "Illegal Displacement" */
NORETURN void illsizeerr(void);    /* "Illegal Assembler Instruction Size" */
NORETURN void illeaerr(void);      /* "Illegal Assembler Addressing Mode" */
NORETURN void nolocerr(void);      /* "Illegal Local Name" */
NORETURN void fpconerr(void);      /* "Fractions Not allowed in Constants" */
NORETURN void syntaxerr(void);     /* "Syntax Error" */
NORETURN void typeerr(void);       /* "Duplicated Type" */
NORETURN void laberr(void);        /* "Illegal Label Name" */
NORETURN void modeerr(void);       /* "End NewType without NewType" */
NORETURN void notypeerr(void);     /* "Type Not Found" */
NORETURN void illtypeerr(void);    /* "Illegal Type" */
NORETURN void offerr(void);        /* "Duplicate Offset" */
NORETURN void overerr(void);       /* "Numeric Over Flow" */
NORETURN void toolongerr(void);    /* "Type too Big" */
NORETURN void rongtypeerr(void);   /* "Mismatched Types" */
NORETURN void noarrerr(void);      /* "Array not yet Dim'd" */
NORETURN void noofferr(void);      /* "Offset not Found" */
NORETURN void pointerr(void);      /* "Element isn't a pointer" */
NORETURN void noleterr(void);      /* "Can't Assign Expression" */
NORETURN void illoperr(void);      /* "Illegal Operator for Type" */
NORETURN void mismatcherr(void);   /* "Type Mismatch" */
NORETURN void arraldimerr(void);   /* "Array already Dim'd" */
NORETURN void nonewerr(void);      /* "Can't Create Variable inside Dim" */
NORETURN void arrnotdimd(void);    /* "Array not Dim'd" */
NORETURN void noconsterr(void);    /* "Constant not defined" */
NORETURN void alconsterr(void);    /* "Constant already defined" */
NORETURN void ilconsterr(void);    /* "Illegal Constant" */
NORETURN void noliberr(UWORD libnum); /* "Library not Found : $XXXX." (asm: d1.w, hexascii into liberrnum) */
NORETURN void notokerr(UWORD token);  /* "Token Not Found : $XXXX." (asm: d1.w, hexascii into tokerrnum) */
NORETURN void duplaberr(void);     /* "Duplicate Label" */
NORETURN void nolaberr(void);      /* "Label not Found" */
NORETURN void typeiferr(void);     /* "Can't Compare Types" */
NORETURN void noiferr(void);       /* "End Condition without Condition" */
NORETURN void bigiferr(void);      /* "If Block too Large" */
NORETURN void badenderr(void);     /* "Illegaly nested Condition" */
NORETURN void dupmacerr(void);     /* "Macro already Defined" */
NORETURN void macnesterr(void);    /* "Can't create Macro inside Macro" */
NORETURN void macenderr(void);     /* "Macro without End Macro" - sets linenumat=macline first */
NORETURN void macbigerr(void);     /* "Macro too Big" */
NORETURN void tmmacerr(void);      /* "Macros Nested too Deep" */
NORETURN void nomacerr(void);      /* "Macro not Found" */
NORETURN void comerr(void);        /* "Too many comma's in Let" */
NORETURN void nocomerr(void);      /* "Can't use comma in Let" */
NORETURN void conintype(void);     /* "Can't use a Constant in New Type" */
NORETURN void alincerr(void);      /* "Already Included" */
NORETURN void noincerr(void);      /* "Can't open Include" */
NORETURN void readerr(void);       /* "Error Reading File" */
NORETURN void toifmacerr(void);    /* "If Macro's Nested too Deep" */
NORETURN void noenderr(void);      /* "If Without End If" */
NORETURN void dupprocerr(void);    /* "Duplicate Procedure name" */
NORETURN void duplocerr(void);     /* "Duplicate parameter variable" */
NORETURN void noprocerr(void);     /* "Procedure not found" */
NORETURN void toovarerr(void);     /* "Too many parameters" */
NORETURN void illvarerr(void);     /* "Illegal Parameter Type" */
NORETURN void nopenderr(void);     /* "Unterminated Procedure" */
NORETURN void illprocerr(void);    /* "Illegal Procedure Call" */
NORETURN void functypeerr(void);   /* "Illegal Function Type" */
NORETURN void illprocperr(void);   /* "Illegal Parameter Type" */
NORETURN void extlaberr(void);     /* "Can't Access Label" */
NORETURN void sreterr(void);       /* "Illegal Procedure return" */
NORETURN void sharederr(void);     /* "Shared outside of Procedure" */
NORETURN void dupsherr(void);      /* "Variable already Shared" */
NORETURN void badforerr(void);     /* "Bad Type for For...Next" */
NORETURN void noforerr(void);      /* "Next without For" */
NORETURN void bigforerr(void);     /* "For...Next Block to Long" */
NORETURN void nonexterr(void);     /* "For Without Next" */
NORETURN void zprocerr(void);      /* "Can't Nest Procedures" */
NORETURN void nodimerr(void);      /* "Can't Dim Globals in Procedures" */
NORETURN void noconerr(void);      /* "Can't Convert Types" */
NORETURN void baddaterr(void);     /* "Bad Data" */
NORETURN void nociferr(void);      /* "CEND without CNIF/CSIF..." */
NORETURN void cnferr(void);        /* "Constant Not Found" */
NORETURN void badconerr(void);     /* "Illegal Constant Expression" */
NORETURN void badpenderr(void);    /* "Illegal End Procedure" */
/* Fixed (#16): not in blitz2.s - a path or file name longer than the
   buffer it is copied to */
NORETURN void pathlongerr(void);   /* "Path too Long" */

/* ================================================== char classes */
/* all: TRUE == asm Z=1 */
BOOL tstalpha(UWORD c);     /* 7681 A-Z a-z */
BOOL tstnum(UWORD c);       /* 7694 0-9 */
BOOL tstnum2(UWORD c);      /* 7697 same */
BOOL tstlab(UWORD c);       /* 7704 alpha, digit, '_', loclabch (') */

/* ================================================== names */
/* Result: name length (asm d2.w; 0 == asm Z=1 "no name"), name + NUL in the
   buffer.  *c (may be NULL) = following char (asm d0; spaces after the name
   skipped).  A name longer than NAMEMAX characters is a syntax error
   (Fixed (#29)): the buffers (namebuff, namebuff2) have 128 bytes. */
#define NAMEMAX 127
UWORD makename(UWORD *c);               /* 7716 into namebuff */
UWORD makename2(UWORD *c);                    /* 7715 bakup() first */
UWORD makename3(char *buf, UWORD *c);         /* 7722 into buf */
/* 7723 append at `at`, counting on from `len` (asm a1/d2 preset);
   returns the new total length */
UWORD makename4(char *at, UWORD len, UWORD *c);

/* ================================================== name lists */
/* Nodes: next at 0, size byte at nameoff-1, name\0 at nameoff (asm d7, see
   NAMEOFF_xxx in structs.h).  `link` is the address of a next field: a list
   head variable cast to struct node *, or a node (findlab continues after
   it).  Name to look for = namebuff, length `len` (asm d2).
   All of them end with reget() like the asm (lastchar unchanged): callers
   that used d0 afterwards use lastchar / reget().

   findlab: returns the node (asm Z=1, a2=node) or NULL (asm Z=0); if NULL
   and last != NULL, *last = last node of the list (or `link` itself if
   empty) - pass it to addhere to append.  len 0 finds an entry with an
   empty name without comparing (Fixed (#30)). */
struct node *findlab(struct node *link, UWORD nameoff, UWORD len,
                     struct node **last);
/* wrappers, blitz2.s 7789-7820 (same NULL/last semantics) */
struct include  *findinc(UWORD len, struct node **last);   /* &firstinc,   19 */
struct xinclude *findxinc(UWORD len, struct node **last);  /* &firstxinc,  5 */
struct constant *findconst(UWORD len, struct node **last); /* &firstconst, 9 */
struct macro    *findmac(UWORD len, struct node **last);   /* &firstmacro, 13 */
struct label    *findadd(UWORD len, struct node **last);   /* &firstlabel, 19 */
struct type     *findtype(struct node *link, UWORD len, struct node **last); /* 11 */
struct proc     *findproc(UWORD len, struct node **last);  /* &firstproc,  33 */
struct fornode  *findfor(UWORD len, struct node **last);   /* &firstfor,   17 */
struct var      *findvar(struct node *link, UWORD len, struct node **last);  /* 15 */

/* blitz2.s:7746 findlast - last node of the list at link (link if empty) */
struct node *findlast(struct node *link);
/* blitz2.s:7761 addhere - new node of nameoff+len+1 bytes (doallocmem,
   MEMF_PUBLIC|MEMF_CLEAR: fixed, #970) with next=0, size byte, namebuff
   copied;
   stored into link->next (rest of a list after link is dropped - asm).
   dirmode -> illdirerr.  Returns the node (asm a2; d2=size = node len). */
struct node *addhere(struct node *link, UWORD nameoff, UWORD len);
/* blitz2.s:7754 addhere2 - same, but inserts after link (keeps old next) */
struct node *addhere2(struct node *link, UWORD nameoff, UWORD len);

/* ================================================== freeing */
void freeprocvs(void);              /* 7850 alllocals records + var lists */
void freevars(struct var *v);       /* 7866 var/field list (len @14) */
#define freevars2 freevars          /* 7867 same entry */
void freelabels(void);              /* 7878 labels after lastrescon + refs */
void freepends(void);               /* 7906 firstpend */
void freeprocs(void);               /* 7918 firstproc */
void freetypes(struct type *t);     /* 7933 NewType list + fields */

/* ================================================== object file / relocs */

/* blitz2.s:6740/6746 calcstatic(2) - pokedata1 (offset.w, size.w) for every
   plain NewType var of the list (calcstatic: firstvar); uses staticslib
   (65335) and memlib (65530) if any.  Returns count (asm d4.w); *tab =
   data1 at entry (asm d3.l). */
UWORD calcstatic(ULONG *tab);
UWORD calcstatic2(struct var *list, ULONG *tab);
/* blitz2.s:6770 datastart - emit move.l #tab,a2 (reloc) / move #n-1,d4
   (patches endarray+2 in place) */
void datastart(ULONG tab, UWORD n);
/* blitz2.s:6800 savefile - write the executable (hunks, relocs, symbols,
   icon).  Own error context: an error inside just returns. */
void savefile(void);
/* blitz2.s:6790 mywrite - Write, on short write Close(fh) + execerr */
void mywrite(BPTR fh, APTR buf, LONG len);
/* blitz2.s:6997 addoff - reloc entry for the current pc (pc-pcat) */
void addoff(void);
/* blitz2.s:7006 addoff2 - reloc entry for offset off (push front firstoff,
   numoffs++) */
void addoff2(ULONG off);
/* blitz2.s:7022 killoffs - free firstoff, numoffs=0 */
void killoffs(void);

/* ================================================== variables */

/* blitz2.s:7953 fetchvar - find/create the variable named namebuff (len)
   using flagmask, procmode, nonew, defaulttype (optional `.type` / `$`
   suffix is parsed).  Sets added (-1 = created) and varmode (0 global /
   -1 local).  Returns the var (asm a3); *type = its type (asm a2);
   *c (may be NULL) = following char (asm d0). */
struct var *fetchvar(UWORD len, struct type **type, UWORD *c);

/* blitz2.s:8170/8172 calcvar / calcvar2 - generate code addressing a var
   (array subscripts, `\field` chains, `[n]`, pointer derefs).
   in/out: *v (asm a3: var, out: var or last field), *t (asm a2: type),
           *c (asm d0: char after the name, out: next char).
   returns asm d2: TF_ADDRINA2 (bit15: address in a2 generated), TF_POINTER
   (bit14), bit13 (pointer field without flag bit2), low byte = type code
   (T_xxx, 0 = struct).  *offset = asm d3: offset from a5/a4 for a simple
   var (only valid without TF_ADDRINA2).  Also sets lastoffset. */
#define TF_PTRNOAT 0x2000
UWORD calcvar(struct var **v, struct type **t, UWORD *c, WORD *offset);
#define calcvar2 calcvar

#endif
