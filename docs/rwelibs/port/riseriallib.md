# riseriallib (22): the serial port with its own interrupts

"Serial-Library v1.0 (7.3.1994)", no author in the source. The serial
port through the hardware, without serial.device:

* `SerialOpen [baudrate]` writes its receive handler into the level 5
  autovector (`$74`, RBF) and its transmit handler into the level 1
  autovector (`$64`, TBE), keeping the old ones (they get the interrupts
  that are not the library's), sets CIA-B PRA to `$3F` (RTS/DTR),
  remembers INTENAR and switches RBF, TBE and the master bit on.
  `SerialClose` (and finit) puts the vectors back, switches RBF and TBE
  off, sets PRA to `$FF` and switches on those of the two that were on
  at `SerialOpen`. `SerialBaud rate`: SERPER = 3546895 / rate − 1
  (ldivlib).
* The receive handler puts SERDATR's byte into a ring of 256 bytes;
  `SerialRead` takes the next one (−1 if none), `SerialWait` waits for
  one, `SerialGot` says whether one is there (−1/0), `SerialFlush` reads
  until none is left, `SerialBuffer` is the ring's address.
* `SerialWrite byte` puts the byte with a stop bit into a ring of 128
  words and requests TBE when the ring was empty; the transmit handler
  sends one word per interrupt.
* `SerialSendBlock`/`SerialReceiveBlock address,length`,
  `SerialProgress` (the remaining count), `SerialStop` (ends a block),
  `SerialSendString`, `SerialReadString`, `SerialWriteLong`,
  `SerialReadLong`, `SerialVersion` (1).

The rings are the first 256 bytes of 1024-byte buffers: the indices are
words whose low byte alone counts (`ADD.b` on their second byte).

* **Source:** `rwelibsrc/seriallib/Serial-Library.bb` (as TED shows it:
  `build/amiga/rwelibsrc/seriallib/Serial-Library.bb`; `SerialSendFile`
  is commented out there, its code is unreachable and not in the C
  file).
* **Reference:** the reference build of the source (`refbuild`,
  `build/amiga/libs/userlibs/rwe/ref/riseriallib.obj`); no binary exists
  (`docs/rwelibs/README.md` §3.3). Number 22: the literal of the source
  (RWE's list gives 22 to `#riblitlib`, of which no source exists).
  Declarations equal (ctest `rwe_declarations_riseriallib`).
* **C file:** `libs/userlibs/rwe/riseriallib.c`; test
  `libs/tests/rwe/riseriallib.c` (snitch case "rwe riseriallib").

## C and assembler

C behind `BLITZ_THUNK`s (they keep d0–d1/a0–a1 but the result; the
original changes d0/d1/a0, d7 in some routines). The custom chip
registers and CIA-B PRA are volatile accesses of the original's width at
their absolute addresses; the vectors are written through a pointer
(`$64`/`$74`). The old vectors are variables (the original writes them
into the operands of two `JMP` instructions). The receive ring (rbig,
rend, buffer) and the transmit ring (wbig, wrend, buffer) keep the
original's order.

Assembler:

* the handlers' entries: the C function says whether the interrupt was
  the library's; then `RTE`, else on to the old vector;
* `SerialReceiveBlock`'s end after `SerialStop`: it returns to a value on
  the stack (BUG(orig) below, #803), the same way as the original;
* the thunks.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* `SerialReadString` stores up to and including the 0 and leaves a3 on
  the 0; the original left a3 behind it, so in a string expression the
  next part was written one byte too far (#795).
* `SerialClose` (and finit) switches only the library's two interrupts
  (RBF, TBE) off and those of them that were on at `SerialOpen` on
  again; the original switched every interrupt off (`INTENA $7FFF`) and
  then on those that were on at `SerialOpen`, with the master bit: an
  interrupt switched on since was lost, one switched off since came back
  (#790; test `riseriallib-close-intena`; `libs_emulator` cases
  `[fixed #790]` of `rwe_riseriallib`, the INTENA written).
* `SerialRead`/`SerialWait` return the byte alone (0–255, −1 if none);
  the original replaced only the low word of d0 (`MOVE.w rbig,d0`, then
  the byte): the upper word of d0 at the call stayed in the result
  (`#long`), a byte `$FF` with `$FFFF` there read as "empty" (−1), and
  `SerialReadString`'s loop ended at a 0 only if that upper word was 0
  and no wait for a byte came before (#797; test
  `riseriallib-serialread-upper-word`; `libs_emulator` cases
  `[fixed #797]` of `rwe_riseriallib`).
* `SerialSendBlock` with a length of 0 or less sends nothing (its
  remaining count, `SerialProgress`, −1 as after a whole block); the
  original sent the first byte before it looked at the length (#800;
  R10 precedent Q3: a loop of 0 passes; test `riseriallib-sendblock-zero`;
  `libs_emulator` cases `[fixed #800]` of `rwe_riseriallib`).
* `SerialSendBlock`, `SerialReceiveBlock` and `SerialFlush` open the
  port when it is closed (what the source means: `BSR _openserial`
  behind the test), `SerialReadLong` and `SerialWriteLong` only then. The
  original tested the word open flag as a long, with the first
  instruction of `SerialClose` (`$4A79`) behind it: never 0, so the first
  three never opened the port and the other two always ran
  `SerialOpen`'s code (#792; test `riseriallib-sendblock-opens`;
  `libs_emulator` cases `[fixed #792]` of `rwe_riseriallib`). New
  behaviour for a program that called `SerialFlush` with the port
  closed: the port is open afterwards.
* `SerialReadString` returns normally and counts the characters of every
  call from 0 (its result is the string's length); the original reset
  the count only when the port was open, and there it returned to the d0
  of the call, which it had pushed and never taken back (#796; test
  `riseriallib-readstring-open`; `libs_emulator` cases `[fixed #796]` of
  `rwe_riseriallib`). The assembler thunk of the original's end goes.
* `SerialSendString` sends the characters of the string (its length
  from the a2 stack, `MOVE.l -4(a2)`; a2 stays unchanged, as in all C
  ports, `docs/libs/cmdlibs-port.md`), without a 0 behind them: a
  program that reads with `SerialReadString` on the other side sends the
  0 itself (`SerialWrite 0`) (R10 decision P2-4 (a)). The original read
  a long from the a2 stack (`MOVE.l -(a2),d1`) for every byte and sent
  the low byte of the string's address (d0) with the stop bit each
  time, until a long with the low byte 0 (#801; test
  `riseriallib-sendstring`; `libs_emulator` cases `[fixed #801]` of
  `rwe_riseriallib`).
* `SerialSendString` and `SerialReadString` open the port when it is
  closed. The original tested the open flag the other way round and
  called `SerialOpen`'s code only when the port was open (where it only
  cleared the stop flag): with the port closed nothing was sent, and
  `SerialReadString` waited for bytes the closed port never received
  (#793; test `riseriallib-sendstring-opens`; `libs_emulator` cases
  `[fixed #793]` of `rwe_riseriallib`).
* `SerialSendBlock`, `SerialReceiveBlock`, `SerialSendString` and
  `SerialReadString` clear the stop flag at their start. The original
  left it set after a `SerialStop` outside a transfer (only opening the
  port cleared it, and before the fix of #793 `SerialSendString` with
  the port open): the next `SerialSendBlock` stopped after its first
  byte, `SerialReceiveBlock` after its second; there is no init, so the
  flag also survived into a second run (#1162; test
  `riseriallib-stop-flag`; `libs_emulator` cases `[fixed #1162]` of
  `rwe_riseriallib`).
* `SerialReceiveBlock` keeps the bytes that are in the receive ring at
  the call: they are the first of the block. The original threw them
  away first (an explicit loop), so bytes the other side sent before the
  call were lost. A program that wants an empty ring calls `SerialFlush`
  before `SerialReceiveBlock` (#798; R10 decision P2-5 (b); test
  `riseriallib-receiveblock-kept`; `libs_emulator` cases `[fixed #798]`
  of `rwe_riseriallib`).

## BUG(orig), kept

* The vectors are written at `$64`/`$74` whatever the VBR of a 68010+
  is (#804); the old ones go into the code (two `JMP` operands) without
  clearing the instruction cache (#791).
* `SerialReceiveBlock` with a length of 1 receives without end (the
  count goes below 0) (#799).
  `SerialStop` during it ends it through the `abort` code of
  `SerialSendBlock`, which pops the saved d7 – here the return address –
  into d7, so it returns to the long above it on the stack (#803).
* `SerialReadLong` never returns: its loop takes the counter back from
  the stack every time and reads bytes without end (#794).
* `SerialWriteLong` sends the low byte of its argument and then three
  times `$01` (it rotates the register `SerialWrite` changed) (#805).
* `SerialGot` returns −1/0, not a count (#802).
* `SerialWrite` (so `SerialSendBlock`, `SerialSendString`, `SerialWriteLong`)
  never tests whether the 128-word transmit ring is full and does not wait:
  byte 129 on overwrites the words of bytes 1, 2 ... that the interrupt has
  not sent yet, and `wrend == wbig` counts as an empty ring. A block of
  more than about 128 bytes loses what is in between. The library test
  sends 200 bytes without an interrupt and checks `wrend`, `wbig` and that
  the slot of byte 1 holds byte 129, in a run of either set. (#983)

No defects: `SerialProgress` gives the remaining count of the last send
or receive block, what it is meant to give; `SerialBaud 0` divides by 0
through ldivlib, whose division by zero has a defined result.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases (rwecmp.h), compared with the
expected data `libs/tests/expected/rwe_riseriallib.txt` recorded from the
original. Nothing reaches the machine:

* every absolute address `$DFF000`–`$DFF1FF` and `$BFD000` in the code of
  the loaded library is patched to a RAM copy (13 custom chip and 2
  CIA-B operands in the original's code, one long per base in the C
  build; the numbers of the run's set are checked); the copy is filled
  before each call (SERDATR with TBE, INTENAR `$2A35` – the master bit
  off, so `SerialClose`'s `$C000` shows –, INTREQR 0) and observed after
  it;
* every call runs inside `Disable()` with `$64`/`$74` set to the test's
  stubs (`RTE`, counting); what the library wrote there is recorded (a
  stub or a handler in its code), and the system's vectors are put back
  before `Enable()`. `SerialOpen` thus saves the stubs as the old
  vectors, and the handlers chain to them. At the end the system's
  vectors are checked.

The handlers are called by hand through `Supervisor()` with the RBF/TBE
bits of the fake INTREQR and the byte in the fake SERDATR – that is also
how bytes arrive. `SerialReceiveBlock` (it throws away what is there,
then waits) and `SerialReadLong` (never returns) run in a task of their
own at priority −1 (it runs only while the test task waits), fed byte
by byte: the test gives the next byte only when the task has read the
last one (the ring indices equal, then one tick more), so the bytes
thrown away and those received do not depend on timing. The test
removes the task (`RemTask`) when it is done or, for `SerialReadLong`,
when every byte is read (the port is open then, so the task does not
touch the vectors). The stopped `SerialReceiveBlock` returns to a landing routine
passed as the long above its return address.

Observed after every call: the fake registers, CIA-B PRA, the vectors,
both rings (the receive ring in front of `SerialBuffer`'s address, the
transmit ring found by the first three bytes written), the stubs
called, d0 of the functions and the registers the original keeps.
Covered: every routine, both forms of `SerialOpen`, the handlers with
and without their interrupt, the rings wrapping, the stop flag in the
block routines, the open and closed paths of `SerialSendString`/
`SerialReadString`, `SerialWriteLong` opening the port, finit.

Not covered: the stale `JMP` operands in the instruction cache (the test
clears the cache before calling a handler), `SerialBaud 0`, a ring index
with `$FF` in its high byte.
