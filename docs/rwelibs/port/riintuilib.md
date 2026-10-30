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
  a 0;
* 0: one character, copied to the buffer.

The text is printed by PrintIText with the library's IntuiText:
FrontPen and BackPen are the low bytes of the pens of the last IPColour
(1 and 0 at first), DrawMode 0 (JAM1), always at 0,0.

The table ends twice, as in the source: `!libfin{...}` after the object
entries, then the init/finit nullsubs and a plain `!libfin`.

## C and assembler

All C behind thunks that keep d0/d1/a0/a1. The handler's two `BRA.w`
in front of its entry are a short assembler block (as in fontlib).
IntuiText, buffer and pens lie one after the other as in the original,
so that an overlong string runs into the pens the same way. Every
handler puts the pens into the IntuiText first, before it copies a
string, as the original: the overlong string itself is still drawn with
the old pens, only the next text with the overwritten ones (#935, the
bug-orig test of #698).

## BUG(orig), kept

* **IPColour fgpen,bgpen with a negative fgpen** calls SetBPen with the
  caller's a2 (the compiler's argument stack) as the RastPort: a2 is
  set to the Rast only together with SetAPen (#696).
* **The single character handler** reads the character from the
  address d0 (`MOVE.l d0,a0:MOVE.b (a0)+,(a1)`), i.e. from low memory,
  not d0 itself (#699).
* **The a0/d0 handler** copies without a limit into the 1024 byte
  buffer; the pens follow it (#698).
* **PrintIText always prints at 0,0** (+ the baseline): IPLocate has no
  effect on the output of IPOutput (#697).

## Test

Each set has its own inputoutputlib, Rast object and BitMap (320 × 16,
2 planes).

Compared:

* the Rast (every field but the BitMap pointer, which must be the set's
  BitMap);
* both planes after every output (and whether anything was drawn);
* 128 bytes around the caller's a2 (IPColour's SetBPen there);
* the kept registers; the memory back at the end.

Covered:

* init, IPBitMap, IPLocate (with upper words, negative words);
* IPColour in both forms: pens ≥ 0, a negative fgpen (SetBPen on a2), a
  negative bgpen;
* IPOutput and all three handler entries through inputoutputlib's
  dooutsingle/dooutstring1/dooutstring2: a character (from the address
  d0: a string's address and the low memory address 65), a0/d0 strings
  (also length 0), null terminated strings (also empty), with the pens
  of IPColour, after IPLocate;
* finit.

Not covered: a0/d0 strings over 1024 characters (they overwrite the
pens, and the text would be drawn far beyond the BitMap, which a
RastPort without a layer does not clip).
