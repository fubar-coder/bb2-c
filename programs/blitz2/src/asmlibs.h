/*
 * asmlibs.h - chunk 04 (blitz2.s 5000-6739): inline assembler back end
 * (asmconst, asmit, asmfixer, doea, insasm), library parameter fetching
 * (countpees, fetchlibps, fetchpees*, handlep ...), library usage and
 * token jsrs (uselib*, fixjsrs, tokecode, maketjsr, findlib, findtoke),
 * library loading / freeing, pokedata1*.  See notes/survey-04.md.
 *
 * Registers: srcptr (asm a5) and the current char/token (asm d0) as in
 * CONVENTIONS.md.  Output addresses (pc, lib, data1, patch addresses) are
 * ULONG host addresses.  All err stubs called here never return.
 */
#ifndef BLITZ2_ASMLIBS_H
#define BLITZ2_ASMLIBS_H

#include <exec/types.h>
#include "structs.h"

/* ============================================================ globals */

extern struct tokejsrnode *tokeslist;/* 6227 token jsr fixup list */
extern APTR deflibsat;               /* 6318 deflibs group buffer (deflibs bytes) */
extern APTR acidlibsat;              /* 6319 acidlibs group buffer */
extern struct node *blitzlibsat;     /* 6320 "node" whose next field starts the
                                        per-dir blitz libs (last group lib or
                                        (struct node *)&firstlib) */
extern struct blitzlib *ffplib;      /* 6616 lib 64935 */

/* ======================================================= code templates */
/* writable, exact bytes of the original; only the opcode words are read */

extern UWORD ptype[];                /* 5617 move.w #0,-(a7)   $3F3C,$0000 */
extern UWORD fbase[];                /* 5818 move.l 0(a5),d0   $202D,$0000 */
#define pushadd  (&fbase[2])         /* 5819 add.l (a7),d0     $D097 */
#define pushlast (&fbase[3])         /* 5820 add.l d0,d0       $D080 */

/* ===================================================== inline assembler */
/* EA modes (asm d3 src / d5 dest): 0 Dn 1 An 2 (An) 3 (An)+ 4 -(An)
   5 d(An) 6 d(An,Xi) 7 abs.w 8 abs.l 9 d(PC) 10 d(PC,Xi) 11 #imm 12 CCR
   13 SR 14 USP 15 movem reglist; asmnode-only: 16 imm word/dc.w, 17 dc.b */

/* 5000 store the text of the eval into asmbuff (+0 flag.w, +2 text\0 or
   +2 source pointer.l), sets asmlen2.  in: srcptr, asmbuff.
   returns the terminating char ('(' ',' or end of statement, test eos()) */
UWORD asmconst(void);
/* 5028 copy const text to dst till '(' / ',' / eos; sets asmlen2 and *lc
   (local label char seen, 0 if none; the asm's global lc, 5026, was only
   read by asmconst right after the call).  returns the terminating char */
UWORD copyconst(UBYTE *dst, UWORD *lc);
/* 5072 assemble one instruction; c = asm token ($8000+fnum..), entered
   from handletoken (asm `bra asmit`).  asm d0 on return is garbage. */
void asmit(UWORD c);
/* 5278 add a relocation for host address addr (pc temporarily = addr) */
void asmoff(ULONG addr);
/* 5286 end of pass asm filler-inner: resolve all firstasm nodes */
void asmfixer(void);
/* 5386 or the ea bits for (reg, mode) into *opcode, write extension
   words at *at (advanced).  extra = extraword, size = asmsize (0/4/8 =
   .b/.w/.l), mask = allowed mode mask,
   shifts = shift byte (lo nibble mode shift, hi nibble reg shift, 15 = none).
   illeaerr if mode not allowed. */
void doea(UWORD *opcode, UWORD reg, UWORD mode, UWORD extra, WORD size,
          UWORD mask, UBYTE shifts, const struct asment *e, ULONG *at);
/* 5474 extension words / asmnodes for mode at *at.
   returns TRUE for mode 15 (movem list): DIFF: asm `addq #4,a7` returned
   past doea to doea's caller; doea returns at once when TRUE. */
BOOL makemode(UWORD reg, UWORD mode, UWORD extra, WORD size,
              const struct asment *e, ULONG *at);
/* 5423/5429 range checks on a value (illdiserr) */
void chkbyte(LONG val);
void chkword(LONG val);
/* 5435 add (asmbuff, asmlen, linenumat, mode, patch address addr, flag
   of entry e: only e->flag is read, e.g. (struct asment *)dummyasm) to
   firstasm.  Preserves everything in the asm. */
void insasm(const struct asment *e, ULONG addr, UWORD mode);

/* ================================================ parameter fetching */

/* 5519 count the parameters following (nesting ( { ) }).  c = current
   char (asm d0, saved by storeloc); srcptr/lastchar restored on return
   (resloc), current char is c again.  returns count (asm d1). */
UWORD countpees(UWORD c);
/* 5563 fetch library parameters.  spec: lo byte = # parameters, hi byte
   bits 0-2 = # repeatable, (spec >> 12) = position of first repeat.
   types = type byte table.  Sets numreps.  returns char after params. */
UWORD fetchlibps(UWORD spec, const UBYTE *types);
/* 5620/5623/5624/5626 fetch n parameters of types[] (0 = userp).
   fetchpees4: always push; fetchpees2: push only when told by the type
   byte; fetchpees3: n & 15, pushmode given; fetchpees: n as is.
   returns char after the last param (notparerr if ',' missing). */
UWORD fetchpees4(UWORD n, const UBYTE *types);
UWORD fetchpees2(UWORD n, const UBYTE *types);
UWORD fetchpees3(UWORD n, const UBYTE *types, UWORD pushmode);
UWORD fetchpees(UWORD n, const UBYTE *types, UWORD pushmode);
/* pmode (handlep .. highadd): the asm's global push mode (5618), now a
   parameter.  Byte/word aliasing: the asm writes the whole word in
   fetchpees (pushmode 0/1) and the HIGH byte with move.b (handlep: $40
   push bit, highadd: 1); tested as a word.  In C:
   pmode = (pmode & 0x00ff) | (b << 8). */
/* 5653 get one parameter: type bit7 address of var (low 3 bits type),
   bit6 push, bit5 array info (bit4 high end, bit3 type); pmode = the push
   mode of fetchpees.  returns char */
UWORD handlep(UWORD type, UWORD pmode);
/* 5675 result in D(regat) of type: push (pmode != 0) or regat++ */
void putap(UWORD type, UWORD pmode);
/* 5679 emit move.l/.w Dregat,-(a7), stackuse += 4/2 */
void pushap(UWORD type);
/* 5689 regat++ */
void putapincreg(void);
/* 5695 putap(T_LONG) */
void putalp(UWORD pmode);
/* 5698 address of a var, expected type (0 = any: type word pushed too) */
UWORD handlevara(UWORD type, UWORD pmode);
/* 5730 array info parameters; returns next char (getchar3) */
UWORD handlearr(UWORD type, UWORD pmode);
/* 5742 moveq #type,Dregat + push; v/t from baseadd (asm a3/a2) */
void sendtype(struct var *v, struct type *t);
/* 5759 array base address: parses "name()", out *v (a3), *t (a2) */
void baseadd(struct var **v, struct type **t, UWORD pmode);
/* 5781 array high end */
void highadd(struct var *v, UWORD pmode);
/* 5822 evaluate a parameter of type (*type & 7, 0 = any -> evalu2, else
   eval); *type = result type (asm d2).  returns char */
UWORD getap(UWORD *type);

/* ============================================================ libraries */

/* 5826 mark all libs unused (varoff -1, LIBF_USED off) */
void resetlibs(void);
/* 5835 uselib(allocvars) */
struct blitzlib *usevars(void);
/* 5837 lib number -> use it in this program (copy code, relocate, var
   slot, dependencies).  noliberr if unknown; badconerr / dirliberr.
   returns the lib (asm a1); recursive. */
struct blitzlib *uselib(UWORD libnum);
/* 5841 uselib for a known lib (libnum = lib->libnum, asm d1) */
struct blitzlib *uselib3(struct blitzlib *lib, UWORD libnum);
/* 6237 use every lib of the dependency list at lib+off+6 except self.
   off == 0: nothing (asm: Z flag from the caller's move.l) */
void uselib2(struct blitzlib *lib, ULONG off, UWORD self);
/* 6003 create code and fill in offsets for all token jsrs (end of compile) */
void fixjsrs(void);
/* 6047 token tok, form number form (bit14 = Blitz form): returns the host
   address to jsr (asm d7): the lib routine itself or a stub generated at
   pc (cutejsr / makelibsub + rts).  tokeerr on bad tokens. */
ULONG tokecode(UWORD tok, UWORD form);
/* 6138 free tokeslist */
void freetlist(void);
/* 6158 token-jsr relocs of lib: relocs = first token-jsr reloc entry,
   count = entries left from there */
void handlejsrs(struct blitzlib *lib, const ULONG *relocs, ULONG count);
/* 6189 remember a token jsr: addr = host address of the long to patch,
   tok = token, form = form number (bit14 = Blitz).  Ends with findtoke
   (lib gets used).  returns sub (asm a3), *libp = lib (a2) if non-NULL.
   asm d1 on return (= lib's first token, used by 3606 as the placeholder
   long) is not returned: the long is overwritten by fixjsrs anyway. */
UBYTE *maketjsr(ULONG addr, UWORD tok, UWORD form, struct blitzlib **libp);
/* 6253 lib by number or NULL (asm: Z=1 if not found, a1) */
struct blitzlib *findlibnoerr(UWORD libnum);
/* 6264 lib by number, noliberr if missing */
struct blitzlib *findlib(UWORD libnum);
/* 6277 token -> sub node (asm a3) of lib (*libp, asm a2); marks the lib
   used (uselib3).  notokerr if none.  asm d1 is changed (first token of
   the lib) - no caller uses it. */
UBYTE *findtoke(UWORD tok, struct blitzlib **libp);

/* 6322 load a group file (len bytes, name), link its libs after *tail
   (asm a3, updated to the last lib).  returns the buffer, 0 if len == 0
   or Open failed */
APTR loadlibgroup(ULONG len, STRPTR name, struct node **tail);
/* 6374 load acidlibs, deflibs and the blitz libs, then fixlibs */
void loadlibs(void);
/* 6392 load the libs of every enabled libslist dir, then fixlibs */
void loadblitzlibs(void);
/* 6470 load one lib file and link it after *tail (silently ignored if
   Open fails) */
void loadalib(STRPTR name, struct node **tail);
/* 6525 fill in token numbers, sub links, code offsets; append lib tokens
   after lasttoken; nummaxs; memlib, ffplib (missing ones ignored) */
void fixlibs(void);
/* 6618 skip a lib reg list ({w,w}*, 0.w) */
UBYTE *skiplibreg(const UBYTE *p);
/* 6624/6629 align up to even */
UBYTE *aligna0(const void *p);
UBYTE *aligna1(const void *p);
/* 6635 free the per-dir blitz libs and libslist (tail frlibslist) */
void freeblitzlibs(void);
/* 6653 free all memory used by libs */
void freelibs(void);
/* 6680 free the non-resident macros (after lastresmac) */
void freemacs(void);
/* 6701 free firstasm */
void freeasms(void);

/* ============================================================ data1 */
/* poke into data1 (limit data2at: nomemleft, data1 still advanced) */
void pokedata1b(UBYTE b);
void pokedata1(UWORD w);
void pokedata1l(ULONG l);            /* high word first */

#endif
