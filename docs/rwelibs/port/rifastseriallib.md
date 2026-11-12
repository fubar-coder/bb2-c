# rifastseriallib (FastSerial, library 36)

**What it does:** byte transfers straight through the serial port
hardware (polling SERDATR, writing SERDAT/SERPER, acknowledging RBF in
INTREQ; no serial.device), for links between two machines.
`InitSerialIO baud,numbytes,inbuf,outbuf` sets the baud rate (SERPER =
3546895 / baud − 1) and switches the TBE/RBF interrupts off;
`DoSerialIO [numbytes-1]` exchanges the buffers (master: write then read
per byte, slave the other way round, `SetSerialMode`); `PutSerial`/
`GetSerial` send/receive one byte or a block; `CheckSerial` = received
byte or -1; `SyncSerial syncbyte` decides who is master (-1) and slave
(0); `SyncPut`/`SyncGet` blocks in lockstep; `SetIOBuffers`,
`SetIOSpeed`, `SetIOSize`, `FlushSerialIO` (acknowledge RBF),
`EndSerialIO` (nothing). finit = `FinitSerialIO`.

**Source and reference:** `rwelibsrc/FASTSerial/fastseriallib3.bb`,
reference BSS2 `RIFASTSERIALLIB.OBJ` (1920 bytes) = the blitz2 2.10
build of the source. The C build has the same declarations
(`rwe_declarations_rifastseriallib`); the code is not byte-identical.

**C:** everything except one instruction
(`libs/userlibs/rwe/rifastseriallib.c`). The custom chip registers are
volatile accesses of the original's width (the TBE/RBF tests read the
high byte of SERDATR like `BTST #5/#6,$dff018`, the data its low byte;
SERDAT, SERPER, INTENA, INTREQ are words); the source's macros
`!writebyte`/`!readbyte` are the C functions `writebyte`/`readbyte`, the
polling loops plain C loops (no timing beyond the hardware flags).
The state (mode, numbytes, inbuf, outbuf, baud, inten, temp: 18 bytes)
keeps the original's order and sizes, including `mode` and `temp` as the
first byte of a word. Every entry is a thunk that keeps those of
d0/d1/a0/a1 that are not the result.

**Assembler:** `DIVU.L d0,d1` in InitSerialIO/SetIOSpeed. The source
writes this 68020 instruction as data (`!divul {0,1}` = `Dc.w
$4c40,$1001`); the library is built for the 68000, where C would divide
with a libgcc routine that works on every CPU, while the original takes
an illegal instruction exception on a 68000 (BUG(orig) (#647), kept). So
it is the same `.word` in a small inline asm.

**BUG(orig)** (kept):

* `InitSerialIO`/`SetIOSpeed` use DIVU.L, a 68020 instruction (above);
  `SetIOSpeed` divides by all of d0, also the upper word of its #word
  argument (#647, #654).
* `InitSerialIO` stores the immediate `#$dff000+intenar` (the word
  `$F01C`) to `inten` instead of reading INTENAR, and `FinitSerialIO`
  writes to the absolute address `$9A` (`intena` without `custom+`). With
  `$F01C` (bits 11 and 0 clear) `FinitSerialIO` never writes: TBE and
  RBF stay off (#651, #648).
* `InitSerialIO` (#long result) returns its baud argument, also when it
  does nothing because it was already initialised (#650); a baud rate that
  gives SERPER 0 (≥ 1773448) leaves the library "not initialised"
  (#653).
* Only `FinitSerialIO`, `InitSerialIO`, `PutSerial` and `SetIOSpeed`
  check the initialisation (#652); a block of 0 bytes
  (`PutSerial`/`GetSerial` address,0, `SyncPut`, `SyncGet`) is 2^32
  bytes (`SUBQ.l: BNE`) (#646).
* `GetSerial address,bytes` (#byte result) returns its address argument
  (#649).
* `SyncSerial`: two machines starting at the same time both become
  master (each takes the other's sync byte for the echo) (#655).

Not a defect (kept): `SyncGet` sends the byte in `temp` (whatever was
last written there) for every byte received; `SyncPut` only waits for
the answer, its value does not matter.

**Test** (`libs/tests/rwe/rifastseriallib.c`, snitch case "rwe
rifastseriallib"): each run loads one library set (the C build or the CD
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_rifastseriallib.txt` recorded
from the original. The emulator has
no serial connection: SERDATR always shows TBE and RBF never comes
(Amiberry does not let a program set RBF through INTREQ either; probed),
so what sends can run and nothing that waits for a byte. Observed: d0 of
the functions, the kept registers, INTENAR (each call is made inside
`Disable()` with the TBE/RBF interrupts switched on, InitSerialIO must
switch them off, the others must not touch them), the word at `$9A`, and
the 18-byte state block of the library (found after the first
InitSerialIO by its inbuf/outbuf values; it holds the SERPER value
written, which cannot be read back from the chip). Cases: CheckSerial
without a byte, FlushSerialIO, SetSerialMode, EndSerialIO,
SetIOBuffers, SetIOSize, SetIOSpeed (also with garbage in the upper
word), PutSerial byte and block before and after InitSerialIO,
InitSerialIO twice and with a baud rate giving SERPER 0, FinitSerialIO,
init/finit. Not covered: GetSerial (both forms), DoSerialIO, SyncPut,
SyncGet, SyncSerial, CheckSerial with a byte, and the bytes on the line
(they need a remote station).
