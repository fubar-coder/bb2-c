/*
 * literal.h - literal characters (NEW, not in ted.s; spec
 * editor/literal-characters, design of the change literal-characters).
 *
 * A Latin-1 character other than the six umlauts of the germs table is
 * stored in a line as the token pair $90 cc (cc = the character, $80 to
 * $FF): the word $9000 + cc of library 33, which the library literallib
 * names with the one character (design D1-D3).  The editor shows it as
 * one character, in the colour of ordinary text.
 *
 * Inside the editor a line is also held as a "column row": one byte per
 * shown column, a token as its name (detok, detoka5), and in the
 * character map chmap the token cells with bit 7 set.  A literal takes
 * one column there.  Its byte cc cannot tell it from a token cell (bit 7)
 * or from a byte of a token pair copied raw (tokenise off), so a column
 * row carries the literal characters beside it:
 *   - the cursor row of chmap: chmap holds TED_LITGLYPH at the column,
 *     litrow the character (litrow_sync builds it from thisitem, the line
 *     editor gl_* shifts it with the row);
 *   - the line buffers charbuff, charbuff2, charbuff3 and the scratch
 *     row of this module: the byte is cc and a parallel byte holds cc too.
 * litat() reads it; every byte that has no such mark is what it was
 * before this change (a token cell or a raw byte).
 */
#ifndef TED_LITERAL_H
#define TED_LITERAL_H

#include "tedcore.h"

/* the first byte of a literal character (library 33, design D2) */
#define TED_LITHI 0x90
/* the character $80 is no literal character: library 33 has no word $9080
   (blitz2 numbers a user library from libnum * 128 + 1); a byte $80 that
   enters the editor is stored as TED_NOLITCHAR (decision of 2026-11-13) */
#define TED_NOLIT 0x80
#define TED_NOLITCHAR '?'
/* c as a character entering the editor ($80 -> TED_NOLITCHAR); c is
   evaluated twice */
#define TED_LITIN(c) ((UBYTE)(c) == TED_NOLIT ? TED_NOLITCHAR : (UBYTE)(c))
/* the pair hi lo is a literal character */
#define TED_ISLIT(hi, lo) ((UBYTE)(hi) == TED_LITHI && (UBYTE)(lo) > TED_NOLIT)
/* the word w (bit 15 set or not) is a literal character */
#define TED_ISLITWORD(w) (((UWORD)(w) & 0x7F80) == 0x1080 && ((UWORD)(w) & 0x7F))
/* the glyph a literal character is drawn with (and its byte in chmap):
   TED's fonts have glyphs for 0-127 only (keymap.h fontcalc), so every
   literal character is drawn with the same glyph, glyph 0 of the font
   (spec: "the font's default glyph"); no line holds a 0 */
#define TED_LITGLYPH 0
/* getbchar2's result for a literal character: TED_BLIT | cc */
#define TED_BLIT 0x100
/* **p = the block character c (getbchar2), a literal character marked;
   *p one further */
void litstore(UBYTE **p, WORD c);
/* bytes of a stored line without its 0 that a literal character may end
   at (charbuff2 holds 129 and the 0, design D4a) */
#define TED_LITLIMIT 129

/* set by charstobuff when a literal character did not fit the line
   (TED_LITLIMIT) and the line ended in front of it */
extern BOOL littrunc;
/* the name of a literal word from the built-in table: the one character
   (TED_ISLITWORD(tok)); findtoke uses it when the token list has no entry */
const char *litname(UWORD tok);

/* the literal character at p of a column row (cursor row of chmap, a line
   buffer or the scratch row), 0 if p holds none */
UBYTE litat(const UBYTE *p);
/* *p = cc, marked as a literal character (p in a line buffer or the
   scratch row; elsewhere only the byte is written) */
void litput(UBYTE *p, UBYTE cc);
/* forget the marks of the line buffers and the scratch row */
void litclear(void);

/* the cursor row of chmap: litrow_sync builds its literal characters from
   thisitem as chmap_showa2item shows it; litrow_shift moves the marks from
   column `from` on by n columns (n > 0 right, the last ones lost; n < 0
   left, blanks behind), litrow_set sets or clears one */
void litrow_sync(void);
void litrow_shift(UWORD from, WORD n);
void litrow_set(UWORD col, UBYTE cc);

/* the scratch row (chcols + 2 bytes): a copy of a column row with its
   marks (litrow_copy: n columns from row), or a stored line made a column
   row (litrow_expand: $90 cc -> cc marked, other bytes as they are; the 0
   copied; returns the index of the 0) */
extern UBYTE litscratch[];
void litrow_copy(const UBYTE *row, UWORD n);
UWORD litrow_expand(const UBYTE *line);

/* the code 26-31 of an umlaut as the Latin-1 umlaut (germs backwards);
   any other byte unchanged */
UBYTE unmapgerms(UBYTE c);
/* the stored line holds a literal character */
BOOL haslit(const UBYTE *line);
/* the stored line src as text with its literal characters marked
   (tokenise off: $90 cc -> cc, every other byte as it is; dst up to end - 1
   and the 0, as detok); returns dst on the 0 */
UBYTE *detolit(const UBYTE *src, UBYTE *dst, const UBYTE *end);
/* detok (fileio.h) that keeps a literal character as its pair: the text
   detokallz2 stores when tokenise goes off (design D6: one stored form) */
UBYTE *detokpairs(const UBYTE *src, UBYTE *dst, const UBYTE *end);
/* the stored line src as the text a text save writes (design D6): tokens
   as their names, $90 cc as cc, 26-31 as the umlauts of germs; dst up to
   end - 1, no 0 written; returns the end of the text */
UBYTE *textline(const UBYTE *src, UBYTE *dst, const UBYTE *end);

#endif
