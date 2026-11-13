/*
 * lineedit.h - chunk 06 core (ted.s 5253-6285): block edge rules
 * (inbloks/inblokf/blokfix), horizontal cursor logic (horizpos/cursleft/
 * cursrite) and the chmap line operations of the line editor getline.
 * UI part of chunk 06: src/input.h (getline loop, cursor, props, refresh,
 * getinput/handled0, jumptable/menutable).
 *
 * The line editor works on the chmap row a2 = chmap + chloc (chcols bytes,
 * independent of cursoff); "abs. column" = cursoff + cursx.  The gl_*
 * functions below are the data parts of getline's local labels (survey-06
 * H): they change chmap and the block, the caller (getline, input.c) then
 * shifts the bitmap / prints / moves the cursor exactly as the asm does.
 */
#ifndef TED_LINEEDIT_H
#define TED_LINEEDIT_H

#include "tedcore.h"

/* ted.s:5360 inbloks: line == btopy and abs. column <= btopx (unsigned).
   asm: Z=1 -> TRUE (d2/d3 side outputs unused by callers) */
BOOL inbloks(void);
/* ted.s:5372 inblokf: the same with bboty/bbotx */
BOOL inblokf(void);
/* ted.s:5384 blokfix: btopx >= chcols -> next line; may end in blokdis;
   continues in blokfixb */
void blokfix(void);
/* ted.s:5393 blokfixb: second entry (blokfix, char127): bbotx < 0 -> end
   of the previous line, bbotx >= chcols -> chcols-1.  Fixed (#349): the
   original compares btopy instead of bbotx with chcols for a multi-line
   block (survey-06 I.1). */
void blokfixb(void);

/* ted.s:5742 horizpos: abs. target column col (asm d1) -> cursoff/cursx
   (hscmarg, edcols/2 rules); refresh() if cursoff changed.  No cursor
   drawing. */
void horizpos(WORD col);
/* ted.s:5775 cursleft: one column left (cursoff -= 2 + scrollrite at the
   margin) */
void cursleft(void);
/* ted.s:5789 cursrite: one column right (cursoff += 2 + scrollleft) */
void cursrite(void);

/* ------------------------------- getline data parts (NEW names) */

/* .isasc (ted.s:5450ff, inside getline3): insert c at the abs. column:
   anyalt = c, block edges (inbloks -> btopx++, inblokf -> bbotx++,
   blokfix), chmap row from the column one to the right (the byte at
   chcols-1 is lost, survey-06 I.2).  The caller shifts the bitmap, prints
   c, cursrite. */
void gl_isasc(UBYTE c);
/* NEW (literal characters, literal.h): gl_isasc for the literal character
   cc ($A0-$FF typed, not an umlaut of germs): the cell holds
   TED_LITGLYPH, the literal map (the cursor row's part, litrow_*) the
   character.  The caller shifts the bitmap, prints the character
   (printlit: its glyph), cursrite.  (The line editor keeps the map in
   step with the row: every gl_* moves it as the row.) */
void gl_islit(UBYTE cc);
/* char127 (ted.s:5665): delete under the cursor: anyalt = c, block edges
   (blokfixb only if inblokf), chmap row left, chmap[chcols-1] = ' '.
   c (asm d7) = 127 (Del) or 8 (Backspace after cursleft). */
void gl_char127(UBYTE c);
/* charnot8 without Shift (ted.s:5600ff): insert spaces up to the next
   multiple of tabstop.  Returns the abs. target column for reline, or -1
   if nothing to do (>= chcols).  Pushes nothing: the caller passes the
   result to reline(). */
WORD gl_tab(void);
/* charnot8 with Shift (ted.s:5560ff, "delete tab"): delete back to the
   previous multiple of tabstop (any characters, survey-06 I.3).  Returns
   the target column or -1 (< 0). */
WORD gl_untab(void);
/* charnotgad (ted.s:5726): menu $f8e1 "delete right": spaces from the abs.
   column to chcols (no block adjust, survey-06 I.4); returns the column */
WORD gl_delright(void);
/* charlow .tababove (ted.s:5500ff): Shift-left at column 0: indentation of
   the previous line; -1 = no move (line 0 or empty previous line).  At a
   column != 0 the target is 0 (no call needed). */
WORD gl_tababove(void);
/* charnot31 with Shift (ted.s:5521ff): 1 + index of the last non-space of
   the chmap row (max chcols-1, 0 for an empty row) */
WORD gl_lastcol(void);

#endif
