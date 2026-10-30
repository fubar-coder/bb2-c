# riseriallib (22): the serial port with its own interrupts

"Serial-Library v1.0 (7.3.1994)", no author in the source. The serial
port through the hardware, without serial.device:

* `SerialOpen [baudrate]` writes its receive handler into the level 5
  autovector (`$74`, RBF) and its transmit handler into the level 1
  autovector (`$64`, TBE), keeping the old ones (they get the interrupts
  that are not the library's), sets CIA-B PRA to `$3F` (RTS/DTR),
  remembers INTENAR and switches RBF, TBE and the master bit on.
  `SerialClose` (and finit) puts the vectors back, switches all
  interrupts off (`INTENA $7FFF`), sets PRA to `$FF` and switches on what
  was on at `SerialOpen`. `SerialBaud rate`: SERPER = 3546895 / rate − 1
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
* `SerialReceiveBlock`'s end after `SerialStop`, and `SerialReadString`'s
  end with the port open: both return to a value on the stack
  (BUG(orig) below), the same way as the original;
* the thunks.

## BUG(orig), kept

* The vectors are written at `$64`/`$74` whatever the VBR of a 68010+
  is; the old ones go into the code (two `JMP` operands) without
  clearing the instruction cache. `SerialClose` switches every interrupt
  off and then on only those that were on at `SerialOpen`.
* The open flag is a word; `SerialSendBlock`, `SerialReceiveBlock`,
  `SerialFlush`, `SerialReadLong` and `SerialWriteLong` test it as a
  long, with the first instruction of `SerialClose` (`$4A79`) behind it:
  never 0. So the first three never open the port, the other two always
  call `SerialOpen`'s code (opening it if it is closed).
  `SerialSendString` and `SerialReadString` test the word the other way
  round: they call that code only when the port is open (it then only
  clears the stop flag).
* `SerialRead`/`SerialWait` return the byte with the upper word of d0 at
  the call (`#long`); a byte `$FF` with `$FFFF` there and a ring index
  with `$FF` in its high byte reads as "empty" (−1).
* `SerialSendBlock` with a length of 0 or less sends one byte.
* `SerialReceiveBlock` throws away what is in the ring first, then
  waits; a length of 1 receives without end (the count goes below 0).
  `SerialStop` during it ends it through the `abort` code of
  `SerialSendBlock`, which pops the saved d7 – here the return address –
  into d7, so it returns to the long above it on the stack.
* `SerialSendString` reads the string length from the a2 stack (`MOVE.l
  -(a2),d1`) for every byte and sends the low byte of the string's
  address (d0) with the stop bit each time, until a long with the low
  byte 0. DIFF: a2 stays unchanged (as in all C ports, `docs/libs/
  cmdlibs-port.md`); the original leaves it below the longs it read.
* `SerialReadString` stores up to and including the 0 and leaves a3
  behind it; d0 is the count − 1, and the count is reset only when the
  port is open – where the routine returns to the d0 of the call (it
  pushed d0 and never takes it back). The loop ends at a 0 only if the
  upper word of d0 at the call is 0 and no wait for a byte (−1) came
  before.
* `SerialReadLong` never returns: its loop takes the counter back from
  the stack every time and reads bytes without end.
* `SerialWriteLong` sends the low byte of its argument and then three
  times `$01` (it rotates the register `SerialWrite` changed).
* `SerialGot` returns −1/0, not a count; `SerialProgress` is the
  remaining count of the last send or receive block.
* `SerialBaud 0` divides by 0 (ldivlib).

## Test

C build and reference build side by side (rwecmp.h). Nothing reaches
the machine:

* every absolute address `$DFF000`–`$DFF1FF` and `$BFD000` in the code of
  both loaded libraries is patched to a RAM copy per set (13 and 2 in
  each build); the copy is filled before each call (SERDATR with TBE,
  INTENAR `$2A35` – the master bit off, so `SerialClose`'s `$C000` shows
  –, INTREQR 0) and compared after it;
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
thrown away and those received are the same in both builds. The test
removes the task (`RemTask`) when it is done or, for `SerialReadLong`,
when every byte is read (the port is open then, so the task does not
touch the vectors). The stopped `SerialReceiveBlock` returns to a landing routine
passed as the long above its return address.

Compared after every call: the fake registers, CIA-B PRA, the vectors,
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
