# ridisasmlib (Disasm, library 50)

**What it does:** a table driven 680x0 disassembler of Leading Edge
Software (1994): `DisAssemble$(address)` returns one line of 34 columns
(name with size and condition in columns 0-8, operands from column 9,
padded with spaces, cut after 34), `InstrSize` the bytes of the last
instruction. The first instruction word is matched against 126 table
entries (mask, compare value, name, source/destination `<ea>` mask and
shift or a special operand code, size mask and shift, a flag for a
second instruction word); an unknown word gives `?????` (length 5) and
InstrSize 2. 68000 to 68030 instructions, the 68020 addressing modes
(brief and full extension words, memory indirect), MOVEM register lists
("d0-d3/a0"), MOVEC control registers. Numbers: hex (`$XXXXXXXX`, `#$XX`
...) for addresses and immediates, signed decimal for displacements.
No library at runtime (init and finit are RTS).

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
around LineBuff keep the original's order and sizes in one packed
struct: the 4 code bytes in front of it (`MOVEQ #34,d0 : RTS`, read by
the condition code check of a name starting with `b*`), the 80-byte
LineBuff, opsize/opextra/extrainstr/isdestop/docomma and the condition
tables - a line up to 86 bytes overwrites the same words. The operand
pointer a5 of the original is a variable, because `docomma` compares it.
The instruction stream is read with the original's widths (volatile
word/long reads at the given address) and the string is written to a3
with 17 word writes, as in the original. The thunks keep d1/a0/a1; the
original changes d1-d7, a0, a1 (it saves a2-a6 itself).

**BUG(orig)** (kept):

* A decimal 0 is not printed as "0": `DecPrint` has `LEA .l8,a0` for
  its local label `'l8`, which blitz2 2.10 assembles as the absolute
  address 8 (no relocation, the bus error vector): every displacement 0
  (d16(an), brief extension, bd/od 0 or of size "null") shows the bytes
  at address 8 up to a 0 byte (Kickstart 3.2 in the emulator: `@J(`)
  (#614).
* `docomma` is "a2 <> a5" after the source operand, but a5 is also the
  pointer the operand routines use for numbers and register lists: a
  source that only moves a5 (MOVEM with an empty register list) still
  gets a comma (#615).
* The string is cut after 34 columns; two memory indirect `<ea>`s with
  32-bit displacements make a line of 86 bytes, over the end of the
  80-byte LineBuff (opsize, opextra, extrainstr) (#616).
* Word writes to a3 and word/long reads of the code: an odd a3 or address
  is an address error on a 68000 (#617).

**Test** (`libs/tests/rwe/ridisasmlib.c`, snitch case "rwe ridisasmlib"):
each run loads one library set (the C build or the original) and
disassembles a code buffer at a fixed address (so PC relative targets
are absolute and every run gives the same text) into a string area;
observed after every call are d0, how far a3 moved, 40 bytes of the
string area, InstrSize and the kept registers - about 380000 calls, 3
million observations (3058392 checks, about 3 minutes in the emulator
with the C build). The expected data
`libs/tests/expected/rwe_ridisasmlib.txt` (recorded from the original)
holds 770 cases: the sweeps over the instruction words are 747 block
cases (a block of consecutive observations is one case with a hash,
`LIBS_CAP_DUMP=BLOCK:N` writes its observations), init, the first
InstrSize, the odd address, the odd a3 and finit are 23 plain cases.
Cases: InstrSize before the first call;
every first word $0000-$FFFF with brief and again with full format
extension words behind it; the extension words of MOVE.l
(bd,a0,xn),(bd,a0,xn) (source every one, destination every third) and
of MOVE.l (pc,xn),d0 forms (every second); MOVEM lists (every mask for
-(a7), every third for (a7)+ and (a0)); MOVEC control register words
(every third: all 4096 codes); an odd code address and an odd a3 (fine
on the emulator's 68020); init and finit. Not covered: the 68000
address errors.
