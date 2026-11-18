# riintuilib (15): text and pens on a RastPort

Reflective Images, 1995. An object library: "Rast", 128 bytes per
object (a RastPort), default maximum 7. IPBitMap initialises a Rast
(InitRastPort) and gives it a Blitz bitmap; IPLocate and IPColour work
on the used Rast (Move, SetAPen/SetBPen); IPOutput makes the Rast the
output channel of inputoutputlib, so that Print draws on it with
PrintIText. init opens graphics.library and intuition.library (any
version, not checked), finit closes both.

* **Source:** `rwelibsrc/riintuilib2.bb` (readable:
  `build/amiga/rwelibsrc/riintuilib2.bb`). The older `riintuilib.bb`
  had its own `IPPrint` and `IPRastPort` instead of the Rast objects and
  IPOutput.
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`, stage D: no binary exists). The declarations equal it
  (ctest `rwe_declarations_riintuilib`).
* **C file:** `libs/userlibs/rwe/riintuilib.c`; test
  `libs/tests/rwe/riintuilib.c` (snitch case "rwe riintuilib").

## The output handler

IPOutput calls inputoutputlib's dosetout (`ALibJsr #inputoutputlib
LSL 7+4`) with the handler ipprint:

* −8: a null terminated string at a0 (printed directly);
* −4: d0 (a word) characters at a0, copied to a 1024 byte buffer with
  a 0, in pieces of at most 1023 characters, each printed after the
  previous one (a 0 among the characters ends the text, as PrintIText
  does);
* 0: one character (the low byte of d0), copied to the buffer; a line
  feed (10, the only single character printlib sends: NPrint's) is not
  printed but moves the position to the start of the next line (x 0,
  y + the Rast's text height).

The text is printed by PrintIText with the library's IntuiText:
FrontPen and BackPen are the low bytes of the pens of the last IPColour
(1 and 0 at first), DrawMode 0 (JAM1), with the Rast's pen position
(IPLocate; 0,0 after IPBitMap) as the top left corner of the text.
Then the position moves right by the text's width (TextLength in the
Rast's font, the font PrintIText takes); its y stays. PrintIText itself
leaves the pen at the end of the text on the baseline (the original's
observed pen position after every output: the text's width, 6), so the
handler sets the position after it.

The table ends twice, as in the source: `!libfin{...}` after the object
entries, then the init/finit nullsubs and a plain `!libfin`.

## C and assembler

All C behind thunks that keep d0/d1/a0/a1. The handler's two `BRA.w`
in front of its entry are a short assembler block (as in fontlib).
IntuiText, buffer and pens lie one after the other as in the original.
Every handler puts the pens into the IntuiText first, before it copies
a string, as the original (in the original an overlong string itself is
still drawn with the old pens, only the next text with the ones it
overwrote: #935, #698).

## Fixed original bugs

Intended deviations (`docs/libs/deviations.md`, section 3a, kind F):

* **IPColour fgpen,bgpen with a negative fgpen** calls SetBPen on the
  Rast. The original took a2 (the caller's, the compiler's argument
  stack) as the RastPort, which it set to the Rast only together with
  SetAPen: the Rast kept its background pen and SetBPen wrote into the
  memory at a2 (#696).
* **IPOutput prints at the pen position** and moves it on. The original
  printed every text at 0,0 (+ the baseline): IPLocate had no effect on
  the output, and each text was drawn over the previous one. The
  library's first version (`riintuilib.bb`: IPPrint's x,y and IPLocate
  in the IntuiText's LeftEdge/TopEdge) took the position as the top left
  corner of the text, as the fix does (user decision W3 Q3 (a), #697).
* **The single character handler** takes the character in d0; a line
  feed starts a new line. The original read the character from the
  address d0 (`MOVE.l d0,a0:MOVE.b (a0)+,(a1)`), i.e. from low memory:
  every NPrint drew the byte at address 10 (user decision W3 Q3 (a),
  #699).
* **The a0/d0 handler** prints a string of 1024 characters and more
  whole, in pieces of 1023. The original copied it without a limit into
  the 1024 byte buffer: it overwrote the pens behind it (the pens of the
  next text) and, from 1028 characters on, the library behind them (user
  decision W3 Q3 (a), #698).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riintuilib.txt` recorded from the
original. The set has its inputoutputlib, a Rast object and a BitMap
(320 × 32, 2 planes).

Observed:

* the Rast (every field but the BitMap pointer, which must be the set's
  BitMap, and the pen position);
* the pen position on its own (where the next output starts);
* both planes after every output (and whether anything was drawn);
* 128 bytes around the caller's a2 (the original's IPColour calls
  SetBPen there, #696);
* the kept registers; the memory back at the end.

Covered:

* init, IPBitMap, IPLocate (with upper words, negative words);
* IPColour in both forms: pens ≥ 0, a negative fgpen (the original's
  SetBPen on a2, the port's on the Rast: #696), a
  negative bgpen;
* IPOutput and all three handler entries through inputoutputlib's
  dooutsingle/dooutstring1/dooutstring2: a character ("Q", 65, and the
  line feed 10, then a text in the next line; the original reads them
  from the low memory addresses: #699), a0/d0 strings
  (also length 0), null terminated strings (also empty), with the pens
  of IPColour, after IPLocate and after the previous output (#697);
* finit.

Not covered: a0/d0 strings over 1024 characters (such a text is drawn
far beyond this BitMap, which a RastPort without a layer does not clip;
the bug-orig test `riintuilib-output-buffer-overflow` prints one on a
bitmap large enough, #698).
