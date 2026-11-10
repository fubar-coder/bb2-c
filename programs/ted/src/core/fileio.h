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
   (result unchecked: a short read keeps the rest), Close, the tab size
   clamped to 1..79 (fixed #385); then setdefs(&defstart) - also if Open
   failed. */
void loaddefs(void);
/* ted.s:3078 savedefs: Open(defname, NEWFILE), Write 50 bytes, Close;
   errors ignored */
void savedefs(void);
/* NEW: defstart <-> 50 byte big-endian file image (identity on the Amiga,
   byte swap on a little-endian host).  Used by loaddefs/savedefs. */
void defs_frombytes(struct defblock *d, const UBYTE *b);
void defs_tobytes(UBYTE *b, const struct defblock *d);

/* NEW: `divu.w divisor,dividend` of the 68000 as a register result:
   (remainder << 16) | quotient; quotient > $FFFF -> dividend unchanged
   (V=1).  divisor = 0: the 68000 zero divide trap.  Amiga: defined in
   src/glue.c as the real DIVU.W instruction (exact semantics incl. the
   trap; no libgcc/libnix division - their __udivsi3 needs UtilityBase).
   Host: plain C in fileio.c, aborts with a message on divisor = 0
   (decision O3).  Every DIVU
   of ted.s goes through this function. */
ULONG divu_w(ULONG dividend, UWORD divisor);

/* ted.s:3140 numtoascii2: numtoascii + 0 byte; returns buf (a0) ON the 0 */
UBYTE *numtoascii2(UWORD num, UBYTE *buf);
/* ted.s:3144 numtoascii: num (d0, unsigned word) in decimal, no leading
   zeros, NO 0 byte; returns buf (a0) behind the last digit */
UBYTE *numtoascii(UWORD num, UBYTE *buf);
extern const UWORD powersof10[5];   /* ted.s:3165 10000,1000,100,10,1 */

/* ------------------------------------------------------ reading files */

/* Register state of a load or save (asm: d7 = file handle, a5 = read
   pointer in dosbuff, d6 = bytes left), kept between doopen and doclose
   (survey-04 G.4).  A local of the file operation (fileopen2, filesave,
   bloksave, blokload) passed down by pointer; the global filehand (for
   outmem) is set by doopen as in the asm. */
struct rdstate {
    tedfh fh;                   /* d7 (before doopen: the name) */
    UBYTE *ptr;                 /* a5: read pointer */
    LONG left;                  /* d6: bytes left in dosbuff */
    WORD carry;                 /* NEW (#375): a byte readline read but did
                                   not store (the line was full), returned
                                   first by the next doread; -1 = none
                                   (from doopen) */
    BOOL textfile;              /* NEW (#376): a line of the file ended
                                   in LF (text format); FALSE from doopen */
    BOOL rawnoeol;              /* NEW (#1013): a line without end (EOF) is
                                   left untokenised (blokload tokenises it
                                   with the cursor line's tail); FALSE from
                                   doopen */
};

/* ted.s:3307 doopen: Open(name (d7), mode (d2)) -> rd->fh, filehand.
   Failure: error(error1) and FALSE (asm: addq #4,a7 + bra error = the
   CALLER returns). */
BOOL doopen(struct rdstate *rd, const UBYTE *name, LONG mode);
/* ted.s:3302 doclose: filehand = 0, Close(rd->fh) */
void doclose(struct rdstate *rd);
/* ted.s:3337 fillbuff: rd->ptr = dosbuff, rd->left = Read(doslen); error ->
   error(error2), rd->left = 0.  Returns rd->left (asm: Z=1 if 0). */
LONG fillbuff(struct rdstate *rd);
/* ted.s:3325 doread: next byte (0..255) or -1 at EOF (asm: N=1) */
WORD doread(struct rdstate *rd);

/* readline results (asm: d0 + N/Z) */
#define RL_EOF   (-1)           /* N=1: nothing read */
#define RL_NOEOL 0              /* Z=1: line without end (EOF) */
#define RL_OK    1              /* normal line */
#define RL_FULL  2              /* NEW (#375): line full (128 columns or
                                   charbuff2), the next call goes on */
/* ted.s:3375 readline: one line into charbuff2 (token pairs copied
   unchecked, LF -> 0 + "ASCII", TAB expanded with divu tabstop (never 0:
   fixed #385) from the column the line shows (fixed #377) up to the end
   of charbuff2 (fixed #378); a line wider than 128 shown columns (or
   longer than 129 bytes) ends in front of the character, token pair or
   TAB that does not fit, which starts the next line (RL_FULL, rd->carry;
   fixed #375: the original lost 3 bytes and returned RL_NOEOL),
   tokenised via mytoke if ASCII
   and tokenise: a line ended by LF, and one without end (EOF/full)
   in a file an earlier line of which ended by LF or without a byte
   >= $80 (fixed #376); the one without EOL not when rd->rawnoeol
   (fixed #1013); a token start byte right before EOF is dropped
   (fixed #379).  *length (d5)
   = length incl. 0.  BUG(orig) list: survey-04 I.1-6, I.22 (#374). */
int readline(struct rdstate *rd, UWORD *length);
/* ted.s:3447 mytoke: tokit2 in place on charbuff2 (d1 = len (d5) =
   length without 0); returns the new length (d5, incl. 0) */
UWORD mytoke(UWORD len);
/* ted.s:3349 allocline: new item from charbuff2, length len (alloc2: no
   memory -> outmem); nextitem = 0, lastitem NOT set (every caller
   sets it); label line -> remember.  asm: a0 = item. */
struct item *allocline(UWORD len);
/* ted.s:3277 linka0: link item (a0) after prev (a4), numlines++, enditem
   if last (does NOT clear item->nextitem).  Returns the new a4 (= item =
   thisitem). */
struct item *linka0(struct item *item, struct item *prev);
/* ted.s:3293 bloadadd: append charbuff2 (token pair aware, incl. 0) at dst
   (a3); returns dst ON the 0 byte (blokload only) */
UBYTE *bloadadd(UBYTE *dst);

/* ---------------------------------------------- tokens / detokeniser */

/* ted.s:3455 detok: tokenised line src (a0) -> text at dst (a1)
   (0-terminated).  end (NEW, Fixed (#355) survey-04 I.8: the original has
   no length limit) = the end of the buffer: the text stops silently at
   end - 1, the 0 goes there.  Returns dst ON the 0 byte.  (asm: a0 also
   behind the source - not needed by any caller) */
UBYTE *detok(const UBYTE *src, UBYTE *dst, const UBYTE *end);
/* ted.s:3472 findtoke: name of token word tok (d0, bit 15 cleared) in the
   list `tokens`; unknown or no list (tokens == 0, Fixed (#359)) ->
   tokeerr. */
const char *findtoke(UWORD tok);
/* ted.s:3759 detoka5: detok chars(line) to charbuff3+1 WITHOUT 0 byte,
   charbuff3[0] = length + 1; the text stops silently at charbuff4 - 1
   (at most chcols characters: the byte behind them is bloksave's LF;
   the original wrote behind charbuff3, the allocation end: Fixed
   (#355)) */
void detoka5(struct item *line);

/* ------------------------------------------------------------- search */

/* ted.s:3523 isithere: isithere2(line, 0) */
WORD isithere(struct item *line, UBYTE **text);
/* ted.s:3530 isithere2: search findreqla16 in line (a4) from column col (d5)
   (tokenised lines: detok to charbuff first), case-insensitive unless
   findcase().  Returns the column (asm: d0, N=0) or -1.  *text (asm a1,
   if not NULL) = the line text that was searched (charbuff or chars(line)).
   Destroys a5 in the asm (irrelevant). */
WORD isithere2(struct item *line, UWORD col, UBYTE **text);

/* --------------------------------------------------------- line list */

/* ted.s:3684 freeupa5: unlink line (a5) (first/end), numlines--, label ->
   mforget2, FreeMem(numchars+lenitem), memalt = size.  numlines == 1:
   afirst2 first (new empty line), numlines stays 1; if that fails, outmem
   and the line stays (fixed #361).  thisitem is NOT adjusted (caller). */
void freeupa5(struct item *line);
/* ted.s:3783 calcitem: thisitem = firstitem + (curstop+cursy) steps (no
   NULL check); returns it (asm: a4) */
struct item *calcitem(void);
/* ted.s:3794 makeline2: length (d5) = end (a3) - charbuff + 1, makeline */
struct item *makeline2(UBYTE *end, struct item *line);
/* ted.s:3799 makefirst: charstobuff(charbuff2), makeline */
struct item *makefirst(struct item *line);
/* ted.s:3803 makeline: replace line (a4, MUST be thisitem) by a new item
   from charbuff/len (d5); freethis frees THISITEM; returns the new a4 =
   thisitem */
struct item *makeline(struct item *line, UWORD len);
/* ted.s:4043 afirst2: allocnull -> firstitem = enditem = thisitem; NULL
   if it failed (asm: Z=1) */
struct item *afirst2(void);

/* ------------------------------------------------------- block cursor */

/* Block walk state (asm: a5 = item, d6 = line, d5 = column), survey-04
   G.4.  A local of the block operation, passed down by pointer. */
struct bcur {
    struct item *item;          /* a5 */
    UWORD line;                 /* d6 */
    WORD col;                   /* d5 */
};

/* ted.s:3727 bloksetup2: walk from thisitem (line curstop+cursy) to line
   btopy: bc->item, bc->line = btopy.  Returns d7 = curstop+cursy.  `found`
   (3750) is its end label (only rts). */
UWORD bloksetup2(struct bcur *bc);
/* ted.s:3752 bloksetup: bloksetup2, bc->col = btopx, tokenise -> detoka5 */
void bloksetup(struct bcur *bc);
/* ted.s:3849 getbchar: next block character: -1 block end (asm N=1), 0 end
   of line (Z=1; bc already on the next line - caller re-runs detoka5 when
   tokenise), else the character (zero-extended in both modes; the
   original's non-token path returned bytes >= $80 negative, so umlauts
   ended the block: Fixed (#362) survey-04 I.9).  `newline` (3884) is a
   label inside. */
WORD getbchar(struct bcur *bc);
/* ted.s:3865 getbchar2: second entry without the block end check */
WORD getbchar2(struct bcur *bc);

/* ------------------------------------------------------- block state */

/* ted.s:3890 bloken: blokmenus(TRUE), bltext = "block" (lower case:
   move.l #'bloc' / move.b #'k'), blokprnt2 */
void bloken(void);
/* ted.s:3903 blokdis: bstx = -1, bboty = -1, blokmenus(FALSE), bltext =
   5 spaces, blokprnt2.  (asm saves d0/a0-a2) */
void blokdis(void);
/* ted.s:3919 blokadj: line (d0) inserted: block active and line <= bboty
   -> bboty++, and line <= btopy -> btopy++ (unsigned) */
void blokadj(UWORD line);
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
   drawing, for item line (a2) at screen row cursy (chloc): writes
   chmap[chloc + column] (token characters with bit 7, long token
   expansions run on into the next chmap row), the chmap write of print2
   for every printed character (bit 7 only if color != normcol - may clear
   the bset of tokprint when tokcol == normcol), the block padding of
   endofline (spaces through print), color = tokcol/normcol, cursx
   saved/restored.  The row is NOT cleared first.  For every character
   that the asm prints, `draw` (may be NULL) is called with cursx/color
   set: on the Amiga printa0 with print4 (inverse + bitmap), on the host
   nothing. */
void chmap_showa2item(struct item *line, void (*draw)(UBYTE c));
/* ted.s:4050/4073/4088 (showall/showtopitem/showbotitem, core part - NEW
   names): the walks of the asm (storepos, cursy steps back from thisitem,
   scloc/chloc reset or godown, restorepos) around chmap_showa2item.
   chmap_showall calls the cls() hook first (only showall clears).  No
   NULL checks where the asm has none (showall: cursy predecessors must
   exist, survey-04 I.19). */
void chmap_showall(void (*draw)(UBYTE c));
void chmap_showtopitem(void (*draw)(UBYTE c));
void chmap_showbotitem(void (*draw)(UBYTE c));

/* --------------------------------------------------------- chmap line */

/* ted.s:4173 charstobuff: chmap line row (a2, chcols bytes) -> charbuff:
   tokenise -> tokit (d1 = chcols - 1); else copy, trailing spaces cut,
   0 appended (`enddo` 4190 = common end with tokit).  Returns d5 = length
   incl. 0. */
UWORD charstobuff(const UBYTE *row);
/* ted.s:4195 tokit2: tokenise src (a2; len = d1 = LENGTH) to dst (a1);
   returns d5.  *dstend (may be NULL): asm a1 after the call. */
UWORD tokit2(UBYTE *dst, const UBYTE *src, WORD len, UBYTE **dstend);

#endif
