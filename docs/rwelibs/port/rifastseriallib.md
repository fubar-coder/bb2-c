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

**C:** everything (`libs/userlibs/rwe/rifastseriallib.c`). The custom chip registers are
volatile accesses of the original's width (the TBE/RBF tests read the
high byte of SERDATR like `BTST #5/#6,$dff018`, the data its low byte;
SERDAT, SERPER, INTENA, INTREQ are words); the source's macros
`!writebyte`/`!readbyte` are the C functions `writebyte`/`readbyte`, the
polling loops plain C loops (no timing beyond the hardware flags).
The state (mode, numbytes, inbuf, outbuf, baud, inten, temp: 18 bytes)
keeps the original's order and sizes, including `mode` and `temp` as the
first byte of a word. Every entry is a thunk that keeps those of
d0/d1/a0/a1 that are not the result.

**Fixed** (intended deviations, `docs/libs/deviations.md` 3a, kind F):

* `InitSerialIO`/`SetIOSpeed` divide in C (bit by bit), which works on every
  CPU; the source writes `DIVU.L d0,d1`, a 68020 instruction, as data
  (`!divul {0,1}` = `Dc.w $4c40,$1001`), an illegal instruction exception
  on a 68000 (#647; test `rifastseriallib-divul-68000` on the A600).
* A block of 0 bytes (`PutSerial`/`GetSerial` address,0, `SyncPut`,
  `SyncGet`) transfers nothing; the original sent or received one byte
  and counted on with `SUBQ.l: BNE`, 2^32 bytes (#646; test
  `rifastseriallib-block-zero-bytes`).

* `SetIOSpeed` divides by its #word argument alone; the original divided
  by all of d0, also its undefined upper word (#654; D-019, `libs_emulator`
  cases `[fixed #654]` of `rwe_rifastseriallib`: `SetIOSpeed $A5A52580`).
* `InitSerialIO` saves INTENAR's value, and `FinitSerialIO` switches the
  RBF and TBE interrupts that were on back on in INTENA. The original
  stored the immediate `#$dff000+intenar` (the word `$F01C`, bits 11 and
  0 clear) to `inten`, so `FinitSerialIO` never wrote and RBF and TBE
  stayed off (#651), and its write went to the absolute address `$9A`
  (`intena` without `custom+`) (#648) (test
  `rifastseriallib-finit-interrupts`: RBF of serial.device on again;
  `libs_emulator` cases `[fixed #651]` of `rwe_rifastseriallib`: the
  saved inten).
* `InitSerialIO` (#long result) returns -1 when it set the port up, 0
  when it did nothing because it was already initialised; the original
  returned its baud argument in both cases (#650; the result of a
  command that works, as the other RWE fixes; test
  `rifastseriallib-initserialio-result`; `libs_emulator` cases
  `[fixed #650]` of `rwe_rifastseriallib`).
* The library keeps a flag of its own for "initialised", behind the
  18-byte state; the original took SERPER 0 (3546895 baud) for "not
  initialised", so after it `PutSerial`, `SetIOSpeed` and `FinitSerialIO`
  did nothing. `InitSerialIO` and `SetIOSpeed` refuse a baud rate outside
  109–3546895, whose SERPER does not fit the register (`InitSerialIO`
  returns 0); the original divided by 0 for baud 0 (a rate from a
  settings file or menu, D-006) (#653; test `rifastseriallib-serper-zero`;
  `libs_emulator` cases `[fixed #653]` of `rwe_rifastseriallib`).
* Every command that works on the port returns at once without
  `InitSerialIO`: `DoSerialIO`, `GetSerial` (both forms), `SyncSerial`,
  `SyncPut`, `SyncGet`, `CheckSerial` (`CheckSerial` -1, `GetSerial` and
  `SyncSerial` 0), as `PutSerial`, `SetIOSpeed` and `FinitSerialIO`
  already did; the original ran them on the port, `DoSerialIO` with the
  buffers 0 (reading into and sending from address 0) (#652; R10
  decision P2-2 (a); test `rifastseriallib-no-init-check`).

* `GetSerial address,bytes` (#byte result) returns -1 when it received
  the block, 0 when it did nothing (not initialised, 0 bytes); the
  original never set d0, the result was the low byte of its address
  argument (#649; R10 decision P2-3 (a); test
  `rifastseriallib-getserial-result`).
* `SetIOSize n` stores n − 1 for `DoSerialIO`'s `DBRA` loop, as
  `InitSerialIO` stores its numbytes; the original stored n as it is, so
  `DoSerialIO` transferred n + 1 bytes, one past buffers of n (#1158;
  test `rifastseriallib-setiosize-count`; `libs_emulator` cases
  `[fixed #1158]` of `rwe_rifastseriallib`). A program that compensated
  with `SetIOSize n-1` now transfers one byte less.

**BUG(orig)** (kept):

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
