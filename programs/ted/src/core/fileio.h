/*
 * fileio.h - chunk 04 core (ted.s 3063-4197): option file, number output,
 * file reading (doopen/doread/fillbuff/readline/allocline/linka0), token
 * lookup and detokeniser, search (isithere2), line list (freeupa5,
 * makeline, calcitem, afirst2), block cursor (bloksetup/getbchar), block
 * state (bloken/blokdis/blokadj/sortblox/orderem), charstobuff.
 * UI part of chunk 04: src/reqs.h (option requester helpers, erase, error,
 * openreq/closereq, file requester bridge, showall & co., invertarea).
 */
#ifndef TED_FILEIO_H
#define TED_FILEIO_H

#include "tedcore.h"
#include "state.h"

/* ------------------------------------------------------- option file */

/* ted.s:3063 loaddefs: Open(defname, OLDFILE), Read 50 bytes into defstart
   (result unchecked: a short read keeps the rest), Close; then
   setdefs(&defstart) - also if Open failed. */
void loaddefs(void);
/* ted.s:3078 savedefs: Open(defname, NEWFILE), Write 50 bytes, Close;
   errors ignored */
void savedefs(void);
/* NEW: defstart <-> 50 byte big-endian file image (identity on the Amiga,
   byte swap on a little-endian host).  Used by loaddefs/savedefs. */
void defs_frombytes(struct defblock *d, const UBYTE *b);
void defs_tobytes(UBYTE *b, const struct defblock *d);

/* NEW: `divu.w src,d0` of the 68000 as a register result: (remainder <<
   16) | quotient; quotient > $FFFF -> d0 unchanged (V=1).  src = 0: the
   68000 zero divide trap.  Amiga: defined in src/glue.c as the real
   DIVU.W instruction (exact semantics incl. the trap; no libgcc/libnix
   division - their __udivsi3 needs UtilityBase).  Host: plain C in
   fileio.c, aborts with a message on src = 0 (decision O3).  Every DIVU
   of ted.s goes through this function. */
ULONG divu_w(ULONG d0, UWORD src);

/* ted.s:3140 numtoascii2: numtoascii + 0 byte; returns a0 ON the 0 */
UBYTE *numtoascii2(UWORD d0, UBYTE *a0);
/* ted.s:3144 numtoascii: d0 (unsigned word) in decimal, no leading zeros,
   NO 0 byte; returns a0 behind the last digit */
UBYTE *numtoascii(UWORD d0, UBYTE *a0);
extern const UWORD powersof10[5];   /* ted.s:3165 10000,1000,100,10,1 */

/* ------------------------------------------------------ reading files */

/* Register state of a load (asm: d7 = file handle, a5 = read pointer in
   dosbuff, d6 = bytes left), kept between doopen and doclose (survey-04 G.4). */
struct rdstate {
    tedfh fh;                   /* d7 (before doopen: the name) */
    UBYTE *a5;                  /* read pointer */
    LONG d6;                    /* bytes left in dosbuff */
};
extern struct rdstate rd;

/* ted.s:3307 doopen: Open(d7, d2) -> rd.fh, filehand.  Failure: error(error1)
   and FALSE (asm: addq #4,a7 + bra error = the CALLER returns). */
BOOL doopen(const UBYTE *d7, LONG d2);
/* ted.s:3302 doclose: filehand = 0, Close(rd.fh) */
void doclose(void);
/* ted.s:3337 fillbuff: rd.a5 = dosbuff, rd.d6 = Read(doslen); error ->
   error(error2), d6 = 0.  Returns d6 (asm: Z=1 if 0). */
LONG fillbuff(void);
/* ted.s:3325 doread: next byte (0..255) or -1 at EOF (asm: N=1) */
WORD doread(void);

/* readline results (asm: d0 + N/Z) */
#define RL_EOF   (-1)           /* N=1: nothing read */
#define RL_NOEOL 0              /* Z=1: line without end (EOF/overflow) */
#define RL_OK    1              /* normal line */
/* ted.s:3375 readline: one line into charbuff2 (token pairs copied
   unchecked, LF -> 0 + "ASCII", TAB expanded with divu tabstop - 0 traps
   in the original, overflow at charbuff3), tokenised via mytoke if ASCII
   and tokenise.  *d5 = length incl. 0.  BUG(orig) list: survey-04 I.1-6, I.22 (#374-#379, #385). */
int readline(UWORD *d5);
/* ted.s:3447 mytoke: tokit2 in place on charbuff2 (d1 = d5 = length
   without 0); returns the new d5 (incl. 0) */
UWORD mytoke(UWORD d5);
/* ted.s:3349 allocline: new item from charbuff2, length d5 (alloc2: no
   memory -> outmem); nextitem = 0, lastitem NOT set (every caller
   sets it); label line -> remember.  asm: a0 = item. */
struct item *allocline(UWORD d5);
/* ted.s:3277 linka0: link a0 after a4, numlines++, enditem if last (does
   NOT clear a0->nextitem).  Returns the new a4 (= a0 = thisitem). */
struct item *linka0(struct item *a0, struct item *a4);
/* ted.s:3293 bloadadd: append charbuff2 (token pair aware, incl. 0) at a3;
   returns a3 ON the 0 byte (blokload only) */
UBYTE *bloadadd(UBYTE *a3);

/* ---------------------------------------------- tokens / detokeniser */

/* ted.s:3455 detok: tokenised line a0 -> text at a1 (0-terminated, no
   length limit, survey-04 I.8).  Returns a1 ON the 0 byte.  (asm: a0 also
   behind the source - not needed by any caller) */
UBYTE *detok(const UBYTE *a0, UBYTE *a1);
/* ted.s:3472 findtoke: name of token word d0 (bit 15 cleared) in the list
   `tokens`; unknown -> tokeerr.  Needs tokens != 0 (else reads at 0). */
const char *findtoke(UWORD d0);
/* ted.s:3759 detoka5: detok chars(a5) to charbuff3+1 WITHOUT 0 byte,
   charbuff3[0] = length + 1; writes beyond charbuff3 for long lines
   (allocation end, BUG(orig) #355, ORIG_HAZARD) */
void detoka5(struct item *a5);

/* ------------------------------------------------------------- search */

/* ted.s:3523 isithere: isithere2(a4, 0) */
WORD isithere(struct item *a4, UBYTE **a1);
/* ted.s:3530 isithere2: search findreqla16 in line a4 from column d5
   (tokenised lines: detok to charbuff first), case-insensitive unless
   findcase().  Returns the column (asm: d0, N=0) or -1.  *a1 (if not
   NULL) = the line text that was searched (charbuff or chars(a4)).
   Destroys a5 in the asm (irrelevant). */
WORD isithere2(struct item *a4, UWORD d5, UBYTE **a1);

/* --------------------------------------------------------- line list */

/* ted.s:3684 freeupa5: unlink a5 (first/end), numlines--, label ->
   mforget2, FreeMem(numchars+lenitem), memalt = size.  numlines == 1:
   afirst2 first (new empty line), numlines stays 1.  thisitem is NOT
   adjusted (caller). */
void freeupa5(struct item *a5);
/* ted.s:3783 calcitem: thisitem = firstitem + (curstop+cursy) steps (no
   NULL check); returns it (asm: a4) */
struct item *calcitem(void);
/* ted.s:3794 makeline2: d5 = a3 - charbuff + 1, makeline */
struct item *makeline2(UBYTE *a3, struct item *a4);
/* ted.s:3799 makefirst: charstobuff(charbuff2), makeline */
struct item *makefirst(struct item *a4);
/* ted.s:3803 makeline: replace a4 (MUST be thisitem) by a new item from
   charbuff/d5; freethis frees THISITEM; returns the new a4 = thisitem */
struct item *makeline(struct item *a4, UWORD d5);
/* ted.s:4043 afirst2: allocnull -> firstitem = enditem = thisitem; NULL
   if it failed (asm: Z=1) */
struct item *afirst2(void);
/* ted.s:4041 allocfirst: afirst2, then cls() */
void allocfirst(void);

/* ------------------------------------------------------- block cursor */

/* Block walk state (asm: a5 = item, d6 = line, d5 = column), survey-04 G.4 */
struct bcur {
    struct item *a5;
    UWORD d6;
    WORD d5;
};
extern struct bcur bc;

/* ted.s:3727 bloksetup2: walk from thisitem (line curstop+cursy) to line
   btopy: bc.a5, bc.d6 = btopy.  Returns d7 = curstop+cursy.  `found`
   (3750) is its end label (only rts). */
UWORD bloksetup2(void);
/* ted.s:3752 bloksetup: bloksetup2, bc.d5 = btopx, tokenise -> detoka5 */
void bloksetup(void);
/* ted.s:3849 getbchar: next block character: -1 block end (asm N=1), 0 end
   of line (Z=1; bc already on the next line - caller re-runs detoka5 when
   tokenise), else the character.  Non-token mode returns bytes >= $80 as
   NEGATIVE (move.b sets N: umlauts end the block, BUG(orig) (#362) survey-04
   I.9) - keep the sign extension.  `newline` (3884) is a label inside. */
WORD getbchar(void);
/* ted.s:3865 getbchar2: second entry without the block end check */
WORD getbchar2(void);

/* ------------------------------------------------------- block state */

/* ted.s:3890 bloken: blokmenus(TRUE), bltext = "block" (lower case:
   move.l #'bloc' / move.b #'k'), blokprnt2 */
void bloken(void);
/* ted.s:3903 blokdis: bstx = -1, bboty = -1, blokmenus(FALSE), bltext =
   5 spaces, blokprnt2.  (asm saves d0/a0-a2) */
void blokdis(void);
/* ted.s:3919 blokadj: line d0 inserted: block active and d0 <= bboty ->
   bboty++, and d0 <= btopy -> btopy++ (unsigned) */
void blokadj(UWORD d0);
/* ted.s:3947 sortblox: order btop <= bbot (rows unsigned, columns
   signed).  *r (may be NULL) = d0-d3 as the asm leaves them: the ORDERED
   values (the asm swaps the registers with exg before storing them; with
   no swap they are btopx/btopy/bbotx/bboty unchanged). */
void sortblox(struct quad *r);
/* ted.s:3965 orderem: the same on registers only (*r in/out) */
void orderem(struct quad *r);

/* ------------------------------------------------ line output (chmap) */

/* ted.s:4104 (showa2item, core part - NEW name, decision O4): everything
   showa2item/endofline/normprint/normprint2/tokprint do except the
   drawing, for item a2 at screen row cursy (chloc): writes chmap[chloc +
   column] (token characters with bit 7, long token expansions run on into
   the next chmap row), the chmap write of print2 for every printed
   character (bit 7 only if color != normcol - may clear the bset of
   tokprint when tokcol == normcol), the block padding of endofline (spaces
   through print), color = tokcol/normcol, cursx saved/restored.  The row
   is NOT cleared first.  For every character that the asm prints, `draw`
   (may be NULL) is called with cursx/color set: on the Amiga printa0 with
   print4 (inverse + bitmap), on the host nothing. */
void chmap_showa2item(struct item *a2, void (*draw)(UBYTE d0));
/* ted.s:4050/4073/4088 (showall/showtopitem/showbotitem, core part - NEW
   names): the walks of the asm (storepos, cursy steps back from thisitem,
   scloc/chloc reset or godown, restorepos) around chmap_showa2item.
   chmap_showall calls the cls() hook first (only showall clears).  No
   NULL checks where the asm has none (showall: cursy predecessors must
   exist, survey-04 I.19). */
void chmap_showall(void (*draw)(UBYTE d0));
void chmap_showtopitem(void (*draw)(UBYTE d0));
void chmap_showbotitem(void (*draw)(UBYTE d0));

/* --------------------------------------------------------- chmap line */

/* ted.s:4173 charstobuff: chmap line a2 (chcols bytes) -> charbuff:
   tokenise -> tokit (d1 = chcols - 1); else copy, trailing spaces cut,
   0 appended (`enddo` 4190 = common end with tokit).  Returns d5 = length
   incl. 0. */
UWORD charstobuff(const UBYTE *a2);
/* ted.s:4195 tokit2: tokenise a2 (d1 = LENGTH) to a1; returns d5.
   *a1end (may be NULL): asm a1 after the call. */
UWORD tokit2(UBYTE *a1, const UBYTE *a2, WORD d1, UBYTE **a1end);

#endif
