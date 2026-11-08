/*
 * structs.h - heap / list node layouts of the Blitz Basic 2 v1.95 compiler.
 *
 * The original code accesses all of these with raw offsets.  The layouts are
 * kept byte-identical (m68k-amigaos-gcc aligns int/pointers to 2 bytes, like
 * Devpac's `even`), and every field offset is checked with OFFS() below, so a
 * struct can be used wherever the asm used `n(aX)`.
 *
 * Named nodes all share one scheme (see findlab/addhere, blitz2.s 7754-7865):
 *   +0 next.l ... size.b at NAMEOFF-1, name\0 at NAMEOFF
 * where NAMEOFF is the value the asm passes in d7.  The NAMEOFF_xxx constants
 * below are those d7 values.
 *
 * Fields marked "?" were not fully understood in the survey; fix the comment
 * (not the offset!) when you learn more.
 */
#ifndef BLITZ2_STRUCTS_H
#define BLITZ2_STRUCTS_H

#include <exec/types.h>
#include <dos/dos.h>
#include <stddef.h>

#define OFFS(type, field, off) \
    static_assert(offsetof(struct type, field) == (off), #type "." #field " must be at " #off)
#define SIZE(type, size) \
    static_assert(sizeof(struct type) == (size), "sizeof " #type " must be " #size)

/* generic list node: every list in the compiler has `next` at offset 0, and
   the asm often treats the address of a list head variable as a node */
struct node {
    struct node *next;
};

/* ------------------------------------------------------------------ labels */

/* reference to a label not yet defined (12 bytes, makealab2 ~10352) */
struct labref {
    struct labref *next;    /* 0 */
    ULONG addr;             /* 4  address of the long to patch (pc+2 of the
                                  emitted instruction); bit0 set = Restore/data
                                  reference: real address = addr-1, patched
                                  with the label's data2 value */
    WORD procnum;           /* 8  proc context of the reference */
    WORD line;              /* 10 linenumat of the reference */
};
SIZE(labref, 12);

/* program label / #constant (firstlabel, d7=19).  Labels and constants share
   this list: refs==1 marks a constant whose value is in `pc`. */
#define NAMEOFF_LABEL 19
struct label {
    struct label *next;     /* 0 */
    struct labref *refs;    /* 4  pending reference list; ==1: #constant;
                                  bit0 set: used as constant (illegal label) */
    ULONG pc;               /* 8  address (0 = not yet defined) / const value */
    ULONG data;             /* 12 data2 at definition (for Restore) */
    WORD procnum;           /* 16 proc context; before definition: line of
                                  first reference */
    UBYTE len;              /* 18 node size */
    char name[];            /* 19 */
};
OFFS(label, refs, 4); OFFS(label, pc, 8); OFFS(label, data, 12);
OFFS(label, procnum, 16); OFFS(label, len, 18); OFFS(label, name, 19);

/* ------------------------------------------------------ variables / types */

struct type;

/* variable, shared/global/local var, or NewType field (d7=15)
   lists: firstvar, firstlocal, firstglob, type->fields */
#define NAMEOFF_VAR 15
struct var {
    struct var *next;       /* 0 */
    WORD offset;            /* 4  offset from a5 (globals) / a4 (locals), or
                                  field offset inside a NewType */
    UWORD flags;            /* 6  flagmask: low byte bit0 pointer ('*'),
                                  bit1 array, bit2 '@'; high byte bit0 = List
                                  array (vars) */
    WORD ndims;             /* 8  number of dimensions (array var) / max index
                                  for a `[n]` field (0 = no bounds check) */
    struct type *type;      /* 10 type node (resident file: high word = type
                                  index, see fixtypes) */
    UBYTE len;              /* 14 node size */
    char name[];            /* 15 */
};
OFFS(var, offset, 4); OFFS(var, flags, 6); OFFS(var, ndims, 8);
OFFS(var, type, 10); OFFS(var, len, 14); OFFS(var, name, 15);

/* type node (alltypes: bytetype..stringtype built in, then NewTypes from
   firsttype; d7=11) */
#define NAMEOFF_TYPE 11
#define TYPE_FORWARD 0xff   /* fields value of a forward declared type */
struct type {
    struct type *next;      /* 0 */
    union {
        ULONG code;         /* 4  < 256: primitive type code 1..7
                                  (b w l q f a s); TYPE_FORWARD = no fields yet */
        struct var *fields; /* 4  >= 256: NewType field list */
    } u;
    WORD size;              /* 8  size in bytes (forward type: line number) */
    UBYTE len;              /* 10 node size */
    char name[];            /* 11 */
};
OFFS(type, u, 4); OFFS(type, size, 8); OFFS(type, len, 10); OFFS(type, name, 11);
#define TYPE_IS_NEWTYPE(t) ((t)->u.code >= 256)
#define TYPE_CODE(t)       ((UWORD)(t)->u.code)    /* asm: 6(aX).w */

/* type codes as used in d2 (low byte) by eval/calcvar */
enum {
    T_STRUCT = 0, T_BYTE = 1, T_WORD = 2, T_LONG = 3, T_QUICK = 4,
    T_FLOAT = 5, T_ADDR = 6, T_STRING = 7
};
#define TF_ADDRINA2 0x8000  /* d2 bit15: address already in a2 */
#define TF_POINTER  0x4000  /* d2 bit14: pointer */

/* procedure-local var list record (droplocals, 12 bytes) */
struct alllocal {
    struct alllocal *next;  /* 0 */
    struct var *locals;     /* 4 firstlocal list */
    struct var *globs;      /* 8 firstglob list */
};
SIZE(alllocal, 12);

/* ------------------------------------------------------------ procedures */

/* Statement / Function (firstproc, d7=33) */
#define NAMEOFF_PROC 33
struct proc {
    struct proc *next;      /* 0 */
    UBYTE nparams;          /* 4  low nibble: #params, high nibble: #string
                                  params (longs on stack) */
    BYTE rettype;           /* 5  -1 ($ff) = Statement, else return type 1..7 */
    UWORD linksize;         /* 6  link size (locvaroff & ~3) */
    UWORD nstatics;         /* 8  #static struct locals; bit15 = needs
                                  newmem/freelast */
    UWORD unk10[2];         /* 10 ? */
    ULONG pc;               /* 14 entry pc */
    ULONG unk18;            /* 18 ? */
    UBYTE params[10];       /* 22 parameter type bytes (<= 6 used) */
    UBYTE len;              /* 32 node size */
    char name[];            /* 33 */
};
OFFS(proc, nparams, 4); OFFS(proc, rettype, 5); OFFS(proc, linksize, 6);
OFFS(proc, nstatics, 8); OFFS(proc, pc, 14); OFFS(proc, params, 22);
OFFS(proc, len, 32); OFFS(proc, name, 33);

/* ---------------------------------------------------------------- macros */

/* macro (firstmacro, d7=13) */
#define NAMEOFF_MACRO 13
struct macro {
    struct macro *next;     /* 0 */
    UBYTE *text;            /* 4  macro text (textlen bytes) */
    UWORD textlen;          /* 8 */
    UWORD usecnt;           /* 10 use counter (for `@) */
    UBYTE len;              /* 12 node size */
    char name[];            /* 13 */
};
OFFS(macro, text, 4); OFFS(macro, textlen, 8); OFFS(macro, usecnt, 10);
OFFS(macro, len, 12); OFFS(macro, name, 13);

/* macro parameter record in macrobuff (expandmax, 6 bytes) */
struct macparam {
    UBYTE *start;           /* 0 */
    UWORD len;              /* 4  excludes terminator */
};
SIZE(macparam, 6);

/* ------------------------------------------------------------- constants */

/* #constant list firstconst (d7=9) ? see findconst / checkclash */
#define NAMEOFF_CONST 9
struct constant {
    struct constant *next;  /* 0 */
    ULONG value;            /* 4 ? */
    UBYTE len;              /* 8  node size */
    char name[];            /* 9 */
};
OFFS(constant, len, 8); OFFS(constant, name, 9);

/* -------------------------------------------------------------- includes */

#define INCLEN 1024         /* asm: inclen, how much of an include to load */

/* open include file (firstinc, d7=19) */
#define NAMEOFF_INC 19
struct include {
    struct include *next;   /* 0 */
    UBYTE *buf;             /* 4  INCLEN byte buffer */
    BPTR fh;                /* 8 */
    UWORD nbuf;             /* 12 chars in buffer */
    UBYTE *ptr;             /* 14 current read pointer */
    UBYTE len;              /* 18 node size */
    char name[];            /* 19 */
};
OFFS(include, buf, 4); OFFS(include, fh, 8); OFFS(include, nbuf, 12);
OFFS(include, ptr, 14); OFFS(include, len, 18); OFFS(include, name, 19);

/* XINCLUDEd file (firstxinc, d7=5) */
#define NAMEOFF_XINC 5
struct xinclude {
    struct xinclude *next;  /* 0 */
    UBYTE len;              /* 4 node size */
    char name[];            /* 5 */
};
OFFS(xinclude, len, 4); OFFS(xinclude, name, 5);

/* ------------------------------------------------- control structures */

/* If / While (firstif, 14 bytes) */
#define IFK_WHILE   1       /* low byte of kind: 0 = If, 1 = While */
#define IFK_ONELINE 0x8000  /* one-line If..Then (bset #7,12(a2)) */
struct ifnode {
    struct ifnode *next;    /* 0 */
    ULONG startpc;          /* 4  pc at If/While start (Wend back-branch) */
    ULONG brapc;            /* 8  pc after the bcc.w (disp word at brapc-2) */
    UWORD kind;             /* 12 */
};
SIZE(ifnode, 14);

/* Repeat (firstrep, 8 bytes) */
struct repnode {
    struct repnode *next;   /* 0 */
    ULONG pc;               /* 4 loop top */
};
SIZE(repnode, 8);

/* pending branch / patch address node (8 bytes): Select's end-of-case bra
   list, firstpend (End Statement jmp longs), tokeslist offsets */
struct addrnode {
    struct addrnode *next;  /* 0 */
    ULONG addr;             /* 4 address of the word/long to patch */
};
SIZE(addrnode, 8);

/* Select (firstsel, 14 bytes) */
struct selnode {
    struct selnode *next;   /* 0 */
    struct addrnode *bras;  /* 4  pending end-of-case bra list */
    ULONG bnepc;            /* 8  address of pending bne disp word (0 = none) */
    UWORD type;             /* 12 */
};
SIZE(selnode, 14);

/* For (firstfor, d7=17) */
#define NAMEOFF_FOR 17
struct fornode {
    struct fornode *next;   /* 0 */
    ULONG toppc;            /* 4  pc of the compare code (loop top) */
    ULONG brapc;            /* 8  pc after bgt.w */
    UWORD vartype;          /* 12 */
    UWORD unk14;            /* 14 ? */
    UBYTE len;              /* 16 node size */
    char name[];            /* 17 */
};
OFFS(fornode, toppc, 4); OFFS(fornode, brapc, 8); OFFS(fornode, vartype, 12);
OFFS(fornode, len, 16); OFFS(fornode, name, 17);

/* ---------------------------------------------------- object code relocs */

/* relocation list firstoff (addoff2, 8 bytes); numoffs = count */
struct offnode {
    struct offnode *next;   /* 0 */
    ULONG offset;           /* 4 offset from pcat (or resident file offset) */
};
SIZE(offnode, 8);

/* token jsr list (maketjsr, 12 bytes) */
struct tokejsrnode {
    struct tokejsrnode *next;   /* 0 */
    UWORD token;                /* 4 */
    UWORD form;                 /* 6 bit14 = Blitz mode form */
    struct addrnode *offs;      /* 8 addresses to patch */
};
SIZE(tokejsrnode, 12);

/* ------------------------------------------------------------- assembler */

/* expression to resolve at end of pass (firstasm, insasm) */
struct asmnode {
    struct asmnode *next;   /* 0 */
    ULONG addr;             /* 4  patch address (host) */
    UWORD mode;             /* 8  patch mode 5-11,16,17 (see asmfixer) */
    UWORD flag;             /* 10 asm entry flag (0 if negative) */
    UWORD line;             /* 12 linenumat */
    UWORD size;             /* 14 allocation size (Fixed (#1): a word; the
                                  original's byte at 14 truncated the size
                                  of a text over 239 bytes) */
    UBYTE type;             /* 16 0 = indirect (text is a pointer into the
                                  source), else direct */
    UBYTE pad;              /* 17 (the text stays even: a pointer) */
    UBYTE text[];           /* 18 text\0 or pointer */
};
OFFS(asmnode, addr, 4); OFFS(asmnode, mode, 8); OFFS(asmnode, flag, 10);
OFFS(asmnode, line, 12); OFFS(asmnode, size, 14); OFFS(asmnode, type, 16);
OFFS(asmnode, text, 18);

/* registers handed to an asm checker routine (asmmac abcdchk, movechk ...,
   called by asmit via asment.chk).  The checker may modify every field. */
struct asmregs {
    UWORD opcode;           /* opcode being built */
    UWORD srcreg;           /* src reg / value (movem: reg mask) */
    UWORD srcmode;          /* src ea mode (0..15, see asmit) */
    UWORD destreg;          /* dest reg */
    UWORD destmode;         /* dest ea mode (-1 = none) */
    UWORD size;             /* asmsize: 0 .b, 4 .w, 8 .l */
};
/* asm: result in d0 + CCR: -2 (szerror) illegal size, -1 (eaerror) illegal
   ea, 0 (noerror) ok, 1 (swapem) ok and swap src/dest */
typedef WORD asmchkfn(struct asmregs *r);

/* asm mnemonic data (asmmac macro `a`), pointed to by asmpnts[i]: lives in
   the token list after name\0 help\0, even */
struct asment {
    UWORD opcode;           /* 0 */
    UWORD srcmask;          /* 2 allowed src modes, bit per ea mode */
    UWORD destmask;         /* 4 allowed dest modes */
    UBYTE srcshift;         /* 6 (regshift<<4)|modeshift, 15 = none */
    UBYTE destshift;        /* 7 */
    UWORD size;             /* 8 (fieldshift<<12)|(.l<<8)|(.w<<4)|.b,
                                 code 15 = illegal, fieldshift 15 = no field */
    asmchkfn *chk;          /* 10 checker (NULL = none) */
    WORD flag;              /* 14 <0: bit0 An->entry+2, bit1 #imm->entry+1;
                                 1 quick, 2 Bcc, 3 moveq, 4 trap, 5 DBcc */
};
OFFS(asment, srcmask, 2); OFFS(asment, destmask, 4); OFFS(asment, srcshift, 6);
OFFS(asment, destshift, 7); OFFS(asment, size, 8); OFFS(asment, chk, 10);
OFFS(asment, flag, 14); SIZE(asment, 16);

/* chunk 03 called the same data `struct asmentry` (asmpnts[] element) -
   one layout only (decided by chunk 01, asmmac owner) */
#define asmentry asment

/* token list node (tokens, blitz2.s 283; lib tokens appended after
   lasttoken).  Variable length: name\0 help\0, then even.  For asm
   mnemonic tokens (fnum..tnum-1) a struct asment follows at the next even
   address (makeasmtab).  The editor walks this list directly. */
struct token {
    struct token *next;     /* 0 */
    UWORD num;              /* 4 token number (source: $8000|num) */
    char name[];            /* 6 name\0 help\0 (help may be empty) */
};
OFFS(token, num, 4); OFFS(token, name, 6);

/* edstruct (blitz2.s 274): handed to the editor's init in a1; ted copies
   these 28 bytes into its own `tokens..remark` block */
struct edstruct {
    struct token *toklist;  /* 0  token list (asm: tokens) */
    const char *menus;      /* 4  menu text (see main.c menus) */
    APTR loadxtra;          /* 8  .xtra load callback (register based stub) */
    APTR savextra;          /* 12 .xtra save callback */
    APTR menu;              /* 16 menu callback (menu_stub) */
    APTR cleanup;           /* 20 cleanup callback (0) */
    char chars[4];          /* 24 colon, quote, label, remark chars ':".;' */
};
SIZE(edstruct, 28);

/* -------------------------------------------------------------- libraries */

/* user library directory list (libslist, 38 bytes, makeblitzlibs) */
struct libsnode {
    struct libsnode *next;  /* 0 */
    WORD used;              /* 4 -1 = load it */
    char name[32];          /* 6 dir name under blitz2:userlibs/
                                 (Fixed (#28): a longer name is skipped) */
};
SIZE(libsnode, 38);

/* Blitz library, loaded image of hunk 0 of a lib file (firstlib list).
   The layout is fixed by the lib file format. */
struct blitzlib {
    struct blitzlib *next;  /* 0 */
    WORD libnum;            /* 4  signed: <0 = system lib (65535..), >=0 user */
    ULONG size;             /* 6  struct size in bytes (set by loader) */
    WORD varoff;            /* 10 a5 offset of the lib base var (-1 none yet,
                                  0 none) */
    UWORD flags;            /* 12 bit15 used, bit14 allowed in const mode */
    UWORD firsttoken;       /* 14 first token (user: libnum*128+1) */
    UWORD lasttoken;        /* 16 last token + 1 */
    ULONG maxinfo;          /* 18 offset to end of max-info block = code start */
    ULONG initsub;          /* 22 init sub offset / dependency list 1 */
    WORD varkind;           /* 26 base var kind: 0 none, <0 word, >0 long */
    ULONG finitsub;         /* 28 finit sub offset / dependency list 2 */
    ULONG nrelocs;          /* 32 */
    ULONG *relocs;          /* 36 */
    ULONG outaddr;          /* 40 address of the lib code in the output */
    ULONG errchkoff;        /* 44 offset where errchk-only code begins (0 none) */
    UBYTE subs[];           /* 48 sub table (byte stream, see fixlibs) */
};
OFFS(blitzlib, libnum, 4); OFFS(blitzlib, size, 6); OFFS(blitzlib, varoff, 10);
OFFS(blitzlib, flags, 12); OFFS(blitzlib, firsttoken, 14);
OFFS(blitzlib, lasttoken, 16); OFFS(blitzlib, maxinfo, 18);
OFFS(blitzlib, initsub, 22); OFFS(blitzlib, varkind, 26);
OFFS(blitzlib, finitsub, 28); OFFS(blitzlib, nrelocs, 32);
OFFS(blitzlib, relocs, 36); OFFS(blitzlib, outaddr, 40);
OFFS(blitzlib, errchkoff, 44); OFFS(blitzlib, subs, 48);
#define LIBF_USED    0x8000
#define LIBF_CONSTOK 0x4000

/* max-info block: negative offsets from E = (UBYTE*)lib + lib->maxinfo */
#define LIBMAX_E(lib)       ((UBYTE *)(lib) + (lib)->maxinfo)
#define LIBMAX_USESUB(lib)  (*(ULONG *)(LIBMAX_E(lib) - 14))  /* Use sub offset */
#define LIBMAX_FREESUB(lib) (*(ULONG *)(LIBMAX_E(lib) - 10))  /* Free sub offset */
#define LIBMAX_VAROFF(lib)  (*(WORD  *)(LIBMAX_E(lib) - 6))   /* a5 offset of max array */
#define LIBMAX_COUNT(lib)   (*(WORD  *)(LIBMAX_E(lib) - 4))   /* maximum */
#define LIBMAX_SHIFT(lib)   (*(WORD  *)(LIBMAX_E(lib) - 2))   /* log2 item size, 0 = no max */

/* --------------------------------------------------------------- residents */

#define MOSTRES 8           /* asm: mostres */
struct resentry {           /* resbuff entry, 68 bytes */
    char name[64];          /* 0 */
    BPTR seg;               /* 64 LoadSeg'ed resident */
};
SIZE(resentry, 68);

/* --------------------------------------------------------- expression eval */

/* forth stack entry (eval5, 18 bytes):
   "pc.l,firstoff.l,optat.l,lasta6.w,0,constgot.w" */
struct forthent {
    ULONG pc;               /* 0  code start of the element */
    struct offnode *firstoff; /* 4 */
    ULONG lasta6q;          /* 8  long read of lasta6 (= lasta6.w,quoteflag.w);
                               not read back (#343) */
    UWORD lasta6w;          /* 12 lasta6 (renamed: lasta6 is a macro in data.h) */
    UWORD zero;             /* 14 */
    UWORD constgot;         /* 16 0 = constant, $ffff = not */
};
SIZE(forthent, 18);

/* type conversion routine (convtable entry, chunk 06): converts D(regat) in
   place by emitting code; from/to types are implied by the table slot */
typedef void convhandler(void);

/* operator code generator (operators table entry, chunk 06): in: type = d2
   (common operand type), regat = left operand Dn, regat+1 = right operand;
   emits code, returns the result type (new d2) */
typedef UWORD ophandler(UWORD type);

/* operators table entry (blitz2.s 14704, 32 bytes):
   "dc operator ascii / dc.l ..b,.w,.l,.q,.f,.a,.s / dc prec" */
struct opentry {
    UWORD op;               /* 0  ascii char, char pair ('<>') or token; 0 ends */
    ophandler *h[7];        /* 2  handler per type 1..7 (NULL = illoperr) */
    UWORD prec;             /* 30 precedence */
};
OFFS(opentry, h, 2); OFFS(opentry, prec, 30); SIZE(opentry, 32);

/* precedence stack entry (pushprec/popprec, 6 bytes) */
struct precent {
    struct opentry *op;     /* 0  (eval4's sentinel: garbage) */
    UWORD prec;             /* 4 */
};
SIZE(precent, 6);

/* ------------------------------------------------------------- editor */

/* line item of the editor's text list (ted.s rsreset) */
struct editem {
    struct editem *next;    /* 0 */
    struct editem *prev;    /* 4 */
    UBYTE numchars;         /* 8 */
    UBYTE chars[];          /* 9 tokenised, NUL terminated */
};
OFFS(editem, numchars, 8); OFFS(editem, chars, 9);

/* --------------------------------------------------------------- typedefs */

/* statement handler (atokens table etc.): entered after getchar3 with the
   current char/token; asm: d0 + Z flag (use eos(c)) */
typedef void tokhandler(UWORD c);

/* ---------------------------------------------------- raw memory access */

/* big-endian raw accesses into object code / loaded images, asm style.
   `a` may be a ULONG address or any pointer. */
#define B_(a) (*(UBYTE *)(a))
#define W_(a) (*(UWORD *)(a))
#define L_(a) (*(ULONG *)(a))

#endif
