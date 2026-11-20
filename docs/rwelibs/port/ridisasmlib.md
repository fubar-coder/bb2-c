# ridisasmlib (Disasm, library 50)

**What it does:** a table driven 680x0 disassembler of Leading Edge
Software (1994): `DisAssemble$(address)` returns one line of at least 34
columns (name with size and condition in columns 0-8, operands from
column 9, padded with spaces to 34; the original cut it after 34, #616),
`InstrSize` the bytes of the last
instruction. The first instruction word is matched against 126 table
entries (mask, compare value, name, source/destination `<ea>` mask and
shift or a special operand code, size mask and shift, a flag for a
second instruction word); an unknown word gives `?????` (length 5) and
InstrSize 2. 68000 to 68030 instructions, the 68020 addressing modes
(brief and full extension words, memory indirect), MOVEM register lists
("d0-d3/a0"), MOVEC control registers. Numbers: hex (`$XXXXXXXX`, `#$XX`
...) for addresses and immediates, signed decimal for displacements.
No library at runtime (init and finit are RTS; the port's init clears
InstrSize's size, #1170).

**Source and reference:** `rwelibsrc/Disasmlib/Disasm-library.bb2`, the
last original version; the AmiBlitz3 `RIDisAsm.obj` is an AmiBlitz3
derivative of it that calls THOR's disassembler.library (1999) first and
falls back to this code (README 3.2), not an original. The reference is
the reference build of the source by blitz2 2.10 (`ref/ridisasmlib.obj`,
ctest `rwe_refbuild`); the C build has its declarations
(`rwe_declarations_ridisasmlib`), the code is not byte-identical. Library
number `LIB_ridisasmlib` (50).

**C:** everything (`libs/userlibs/rwe/ridisasmlib.c`). The table
`DisData` is a C array of `struct disdata` generated from the source's
`Dc` lines (symbolic special codes kept); its 3024 bytes equal the
reference build's (checked once by comparing the library files; the four
7-byte `"move",0,0,0` names get the pad byte blitz2 inserts). The data
around LineBuff keep the original's order in one packed struct: the 4
code bytes in front of it (`MOVEQ #34,d0 : RTS`, read by the condition
code check of a name starting with `b*`), LineBuff (128 bytes, the
original's 80, #616), opsize/opextra/extrainstr/isdestop/docomma and the
condition tables. The operand
pointer a5 of the original is a variable (`opstr`).
The instruction stream is read with the original's widths (volatile
word/long reads at the given address); the string is written to a3
byte by byte (the original: 17 word writes, #617). The thunks keep d1/a0/a1; the
original changes d1-d7, a0, a1 (it saves a2-a6 itself).

**Fixed** (intended deviations, `docs/libs/deviations.md`, kind F):

* A decimal 0 is printed as "0". The original's `DecPrint` has
  `LEA .l8,a0` for its local label `'l8` (`"0",0`), which blitz2 2.10
  assembles as the absolute address 8 (no relocation, the bus error
  vector): every displacement 0 (d16(an), brief extension, bd/od 0 or of
  size "null") showed the bytes at address 8 up to a 0 byte - nothing on
  the emulator's Kickstart 3.1, where the vector points to ROM
  (`move.l   (a0),d0` for `0(a0)`), other bytes where it lies in RAM
  (`@J(` ...) (#614; test `ridisasmlib-decprint-zero-address-8`, the
  `libs_emulator` blocks with a displacement 0).

* The string is written to a3 byte by byte: the original's 17 word
  writes (`MOVE.w (a0)+,(a3)+`) were an address error on a 68000 when a3
  was odd, after a string of odd length in an expression
  (`"x"+DisAssemble$(a)`) (#617; test `ridisasmlib-odd-a3-address-error`
  on the a500). The word and long reads of the code at an odd address
  stay: disassembling there means nothing.
* The comma between the operands is set when the source operand wrote
  something. The original's `docomma` was "a2 <> a5" after the source,
  but a5 is also the pointer the operand routines use for numbers and
  register lists: MOVEM with an empty register list (mask 0) as the
  source got a comma, `movem.l  ,-(a7)` (#615; test
  `ridisasmlib-docomma-empty-movem-list`, the `libs_emulator` blocks of
  the masks 0 of `MOVEM -(a7)` and `MOVEM (a0)`).
* init clears InstrSize's size: the original's init is RTS, so a second
  run of the program with the library still loaded (from the editor)
  got the run before's size from `InstrSize` before its first
  `DisAssemble$` (#1170; `libs_emulator`, the second run of
  `rwe_ridisasmlib`).
* `DisAssemble$` returns the whole line, at least 34 columns (a shorter
  line padded with spaces as before). The original put a 0 at column 34
  and returned the first 34 columns ("woz 39!!!" in the source), cutting
  the operands of long 68020 instructions in the middle; its 80-byte
  LineBuff took the longest line (86 bytes, two memory indirect `<ea>`s
  with 32-bit displacements) only by overwriting opsize, opextra and
  extrainstr. LineBuff has 128 bytes (#616; test
  `ridisasmlib-line-cut-34`, the `libs_emulator` blocks with a line
  longer than 34 columns).

**Test** (`libs/tests/rwe/ridisasmlib.c`, snitch case "rwe ridisasmlib"):
each run loads one library set (the C build or the original) and
disassembles a code buffer at a fixed address (so PC relative targets
are absolute and every run gives the same text) into a string area;
observed after every call are d0, how far a3 moved, 100 bytes of the
128-byte string area, InstrSize and the kept registers - about 380000 calls, 3
million observations (about 3 minutes in the emulator
with the C build). The expected data
`libs/tests/expected/rwe_ridisasmlib.txt` (recorded from the original)
holds 777 cases: the sweeps over the instruction words are 747 block
cases (a block of consecutive observations is one case with a hash,
`LIBS_CAP_DUMP=BLOCK:N` writes its observations), init, the first
InstrSize, the odd address, the odd a3, finit and the second run are 30
plain cases.
Cases: InstrSize before the first call;
every first word $0000-$FFFF with brief and again with full format
extension words behind it; the extension words of MOVE.l
(bd,a0,xn),(bd,a0,xn) (source every one, destination every third) and
of MOVE.l (pc,xn),d0 forms (every second); MOVEM lists (every mask for
-(a7), every third for (a7)+ and (a0)); MOVEC control register words
(every third: all 4096 codes); an odd code address and an odd a3 (fine
on the emulator's 68020); init and finit; a second run (init after
finit, then InstrSize before the first DisAssemble$). Not covered: the 68000
address error of an odd a3 (the bug-orig test on the a500) and of an
odd code address.
