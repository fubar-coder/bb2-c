/*
 * libcall.h - chunk 07, blitz2.s 10684-12135:
 * number fetch (fetchdata/fetchnum), code push/pull, library call / init
 * code generation, compile-time constant evaluation, include file reader,
 * Data poking, CNIF/CSIF compare helpers.
 *
 * Type codes (asm d2) are the T_xxx enum of structs.h: 0 = any/unknown,
 * 1 b, 2 w, 3 l, 4 q (16.16 fixed), 5 f (FFP), 7 string.
 * Floats are raw Motorola FFP bit patterns (ULONG), computed with ffpcall().
 */
#ifndef BLITZ2_LIBCALL_H
#define BLITZ2_LIBCALL_H

#include <exec/types.h>
#include "structs.h"

/* ---------------------------------------------------- register results */

/* DIFF: the asm leaves results in d3/d6 that callers in other chunks read
   after a number fetch or evalconst (Data checks d3 < 65536, docnif/numcom
   tests d3, evalconst callers test d6 != 0 = "was a float" -> error).
   C hands them over explicitly: fetchdata's *raw (d3: the value before
   the type conversion and before Data '-'), evalconst's return value (d3)
   and *isfloat (d6: 0 = integer, != 0 = float).  Inside libcall.c the two
   registers stay (numval, numfloat; see there). */

/* ------------------------------------------------------- number fetch */

/* The fetch routines below share one result convention (asm: they all end
   in fetchnum3):
     *type in: wanted type (d2), out: final type (may be promoted: b->w->l,
           q with int out of +-32767 -> l, q with bad fraction -> f)
     data:   TRUE only for Data (asm: the flag dfetch, 10854);
     return: data -> the value (asm d0: int, q as int<<16|frac, or FFP);
             !data -> `move.w/.l #value,D(regat)` has been emitted and
             the next char/token is returned (asm reget: use eos()).
   Errors: overerr, syntaxerr, mismatcherr (never return).
   A float literal in a type w or type 0 expression is a float constant
   (f), as in the original 1.95, whose jump to `.quick2` without makeint
   always ended there (#12, a port regression: the port made some of them
   a Quick by chance).
   Fixed (#13): a quick with a fraction of 0 after *65536 is the quick
   integer (the original made it a float), the integer part is rounded
   down so that the fraction is never negative. */

/* blitz2.s:10686 fetchdata - fetch a (optionally negative) number for Data:
   reads its first char itself (getchar3) and fetches with data = TRUE, so
   it always returns the value.  Negative: int neg.l, FFP sign bit flipped
   (*raw = asm d3: the value before the conversion, un-negated).  Hex ($) /
   binary (%) go to fetchhex/fetchbin (chunk 06), which end in fetchnum2. */
ULONG fetchdata(UWORD *type, ULONG *raw);

/* blitz2.s:10709 fetchfrac - number starting with '.', c = '.'
   (a point alone is a syntax error; "1." is the float 1, Fixed (#10);
   the E exponent counts after a zero fraction too, "1.0e5", Fixed (#11)). */
ULONG fetchfrac(UWORD c, UWORD *type, BOOL data);

/* blitz2.s:10712 fetchnum - decimal number, c = first digit; handles '.'
   and 'e'/'E' exponent (0..18) */
ULONG fetchnum(UWORD c, UWORD *type, BOOL data);

/* blitz2.s:10902 fetchnum2 - integer constant num is ready (asm d6 := 0):
   size it to *type and emit/return it.  Common tail of fetchhex, fetchbin,
   fetchconst, fetchsize, fetchon/off, domaximum, fetchqasm, fetchasm. */
ULONG fetchnum2(ULONG num, UWORD *type, BOOL data);

/* Fixed (#978), no asm counterpart: TRUE if the code from `from` to `to`
   is exactly the integer constant the fetch routines emitted last
   (move.w/.l #x,Dn, type w, l or q) and its value is 128..255. */
BOOL isbyteconst(ULONG from, ULONG to);

/* Fixed (#978): rewrite the constant isbyteconst accepted at `at` as the
   sign-extended byte (128..255 -> -128..-1, of the same type). */
void byteconstext(ULONG at);

/* blitz2.s:10906 constotype - evalconst, then size the constant to *type
   (consttype2).  Returns the value (asm d0); no callers in the asm. */
ULONG constotype(UWORD *type);

/* --------------------------------------------- code push/pull (motion) */
/* Non-reentrant: one static save area (pushpc/pushoff/pushdooff/pushclen/
   pushat). Sequence: pushstart ... pushdo ... (other code) ... pushput. */

/* blitz2.s:11022 pushstart - remember pc and firstoff */
void pushstart(void);
/* blitz2.s:11028 pushdo - cut the code emitted since pushstart out of the
   object buffer (pc rewinds) into an AllocMem'ed save buffer */
void pushdo(void);
/* blitz2.s:11057 pushput - re-emit the saved code at the current pc and
   relocate its offset nodes by the move distance; frees the buffer.
   Fixed (#14): also when the object buffer is full (nomemleft). */
void pushput(void);
/* Fixed (#994): not in blitz2.s - frees a save buffer of pushdo that no
   pushput freed (a compile error between them); called by qfree */
void freepush(void);

/* ---------------------------------------------------- code generation */

/* blitz2.s:11106 muld1 - emit D(regat) *= #mul: lsl.l for a power of two,
   else mulu.w #mul (x256 is one lsl #8, Fixed (#15)).  (asm clobbered
   d1-d4) */
void muld1(UWORD mul);

/* blitz2.s:11140 makeinits - emit the init calls of all used libs
   (highest lib# first) as a subroutine; patches a `jsr` to it at pcat and
   sets noinits low word (8 = no init code, 0 = there is some) */
void makeinits(void);
/* blitz2.s:11327 makefinits - emit the finit calls (lowest lib# first),
   including the per-item free loops of "maximum" libs */
void makefinits(void);

/* blitz2.s:11416 chkstak - if debugga && !makeexec: tokejsr(stchk) */
void chkstak(void);

/* blitz2.s:11434 tokejsr - make a jsr (or inline code / register gets) for
   lib token `token` (findtoke + makelibsub).  Returns the lib (asm a2).
   tokejsr2 (11427) = lasta6 = -1, then tokejsr. */
struct blitzlib *tokejsr(UWORD token);
struct blitzlib *tokejsr2(UWORD token);

/* blitz2.s:11466 makelibsub - generate a call of lib sub `off` (entry =
   (UBYTE*)lib + 6 + off): parameter register loads (fetchregs/toreg),
   error-check routines (debugga), blitz/amiga mode checks, then jsr or
   inline copy.  The lib is passed down (asm: global libisat).  Returns lib
   (asm a2).  Uses uselib (asm a1 clobbered).
   Original: toreg's max-count/used forms (spec low byte 3/4) test bit 7
   of the register byte for "address register" while makeregop tests
   bit 4, so the a6 check there is unreachable (no library needs it).
   Original: doajsr2's global `inline` length is cleared when a nested
   makebtst (debug mode check) recurses into doajsr2 (at worst jsr instead of inline). */
struct blitzlib *makelibsub(struct blitzlib *lib, ULONG off);

/* blitz2.s:11444 userjsr - like makelibsub, `params` = sub entry starting
   with a word whose low byte is the number of parameter format bytes to
   skip (then word aligned) */
struct blitzlib *userjsr(struct blitzlib *lib, UBYTE *params);

/* blitz2.s:11459 cutejsr - like makelibsub, `sub` = the sub's register
   list directly (asm a3).  Also used as a callback by chunk 04 (6100). */
struct blitzlib *cutejsr(struct blitzlib *lib, UBYTE *sub);

/* blitz2.s:11619 getbbase - a5 offset of the runtime blitzmode flag word;
   allocates it (word aligned, 2 bytes at varoff) when blitzoff == -1 */
WORD getbbase(void);

/* blitz2.s:11636 blitzoff - var offset of the blitzmode flag, -1 = none
   (setcvars sets -1) */
extern WORD blitzoff;
/* blitz2.s:11637 blitzmode - >= 0 Amiga mode, < 0 Blitz mode */
extern WORD blitzmode;

/* -------------------------------------------------- code templates */
/* Writable copies of the original bytes (defined in libcall.c, verified
   against the shipped binary). */

/* blitz2.s:11263-11267 (16 bytes):
   prepd1 222D 0000  move.l 0(a5),d1        (unused)
   putidata1 23CD 0000 0000  move.l a5,abs.l (opcode word used; long =
                                             intdata1, relocated)
   prepint 247C 0000 0000  move.l #0,a2     (unused) */
extern UWORD prepd1[];
#define putidata1      (&prepd1[2])
#define prepint        (&prepd1[5])

/* blitz2.s:11407-11414 (16 bytes): free loop of a max lib (makefinits)
   freemax  3F3C 0000       move.w #max,-(a7)
   freemax2 5357 6600 FFFC  subq #1,(a7) / bne.w (disp computed)
   freemax4 544F            addq #2,a7
   freemax3 47EB 0000       lea size(a3),a3 */
extern UWORD freemax[];
#define freemax2       (&freemax[2])
#define freemax4       (&freemax[5])
#define freemax3       (&freemax[6])

/* blitz2.s:11633 tstmode 4A6D 0000  tst.w blitzoff(a5) */
extern UWORD tstmode[];

/* blitz2.s:11829-11839 (20 bytes):
   lsldn  E148                     lsl.w #8,d0 (OR count<<9 | Dn)
   maxchk 6500 0008 4EB9 0000 0000  bcs.w +8 / jsr 0  (only the first long
          is poked, the jsr comes from tokejsr(maxerr); also used by 08)
   maxdo  D0C0                     adda.w d0,a0 (OR Dn | An<<9)
   stackopuse 0000                 dc 0 (unused data word)
   stackop 2F2D 0000               move.l 0(a5),-(a7) */
extern UWORD lsldn[];
#define maxchk         (&lsldn[1])
#define maxdo          (&lsldn[6])
#define stackopuse     (&lsldn[7])
#define stackop        (&lsldn[8])

/* ----------------------------------------------------- misc parsing */

/* blitz2.s:11861 ifchar - asm: Z=1 if d0 is '<','=','>' and then d0-=60.
   Returns TRUE and *c = 0/1/2 for those, else FALSE (*c unchanged). */
BOOL ifchar(UWORD *c);

/* blitz2.s:11880 evalconst2 / evalconst3 (11881, same address) - evaluate
   a constant expression at srcptr: compiles it (arreval, cfetchmode = -1)
   into the constant buffer at constpcat, appends `move.l d0,d3 / rts`,
   flushc() and runs it with callregs() (d3/d6 of the last number fetch
   in, d3/d6 out; a5 = srcptr as in the asm).  pc, libat, regat, nomemleft,
   lasta6, cfetchmode are restored afterwards (nested calls continue in the
   const buffer).  Returns the value (asm d3); *c (may be NULL) = next
   char/token from reget (eos(*c) = asm Z); *isfloat (may be NULL) = asm
   d6, != 0 if the constant is a float.
   Errors: conmemerr and anything from the evaluator; the error handler
   restores libat from oldlibat. */
ULONG evalconst2(UWORD *c, WORD *isfloat);
ULONG evalconst3(UWORD *c, WORD *isfloat);
/* blitz2.s:11871 evalconst - bakup, then evalconst2 */
ULONG evalconst(UWORD *c, WORD *isfloat);

/* blitz2.s:11934 oldlibat - libat saved by the outermost evalconst2 */
extern ULONG oldlibat;

/* ---------------------------------------------------------- includes */

/* blitz2.s:11936 doincdir - IncDir "path" statement handler: copies the
   chars (without '"') into incdir up to end of line; token -> syntaxerr.
   Fixed (#16): a path longer than 95 characters is "Path too Long".
   Fixed (#17): a path that starts with '"' ends at the closing quote;
   after it only ':' (next statement), a comment or the line end.  An
   unquoted path is the rest of the line (also after ':'), as in blitz2.s. */
void doincdir(UWORD c);

/* blitz2.s:11947 makeiname - namebuff = incdir + file name from the
   source (bakup first, '"' skipped); returns the total length (asm d2,
   used by addhere for the node); 0 -> syntaxerr.
   Fixed (#325): more than 127 characters -> "Path too Long".
   Fixed (#341): a name with a device or assign (':') is used without
   incdir (blitz2.s puts it in front of every name, e.g. "BlitzLibs:" +
   "blitzlibs:libnums.bb"); AB3 mode keeps it (ext_incdevice).
   Fixed (#17): a name that starts with '"' ends at the closing quote;
   after it a comment or the line end, with colon (IncBin) also ':' that
   starts the next statement; anything else is a syntax error.  An
   unquoted name is the rest of the line (also after ':'), as in
   blitz2.s. */
WORD makeiname(BOOL colon);

/* blitz2.s:11972 readinc - next line of the current include (firstinc).
   asm: Z=0 -> a5 = line.  Returns TRUE with srcptr = the 0-terminated line,
   FALSE when the last include got closed (numincs == 0; srcptr unchanged).
   Closes finished includes and continues with the outer one. */
BOOL readinc(void);

/* blitz2.s:12037 freexincs / 12050 freeincs - free the XInclude list /
   close and free all open includes */
void freexincs(void);
void freeincs(void);

/* ------------------------------------------------------------- data */

/* blitz2.s:12074 pokedata2l / 12081 pokedata2 / 12094 pokedata2b - append
   a long / word / byte to the Data area (data2); past allat only data2 is
   advanced and nomemleft = -1 */
void pokedata2l(ULONG l);
void pokedata2(UWORD w);
void pokedata2b(UBYTE b);

/* --------------------------------------------------- CNIF / CSIF */

/* blitz2.s:12104 getsp - c must be '"' (else syntaxerr); copies the quoted
   string to buf, NUL terminated; end of line inside -> syntaxerr */
void getsp(UWORD c, UBYTE *buf);

/* compare callbacks for `compare` (chunk 08); asm: return CCR tested by
   signed Bcc (cifeq..cifge).  Return <0 / 0 / >0. */
/* blitz2.s:12118 numcom - sign of (LONG)value (asm: tst.l d3 after
   evalconst) */
int numcom(ULONG value);
/* blitz2.s:12121 strcom - namebuff vs namebuff2, signed byte compare;
   shorter first string = less */
int strcom(void);

#endif
