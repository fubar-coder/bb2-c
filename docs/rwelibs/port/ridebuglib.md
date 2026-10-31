# ridebuglib (51): talking to the debugger

Leading Edge Software (Steven Matty), V2.1 of 27/2/95. Commands that
send messages to the Blitz debugger front end: `AddVarTrace`,
`DelVarTrace`, `VarTraceWindow`, `DisAsmWindow`, `CopperTrace`,
`ProcControl`, `MemoryWindow`. A message is command.b, data1.b, data2.b,
pad.b, size.l (an address since 5/1/94) and 252 characters; it goes to
the debugger through debuglib's `docall` (token `$D509`, a0 = the
message), which calls handler 6 of the front end. Bytes a command does
not set keep what the previous message left.

* **Source:** `rwelibsrc/Debuglib/Debuglib4.bb` (as TED shows it:
  `build/amiga/rwelibsrc/Debuglib/Debuglib4.bb`).
* **Reference:** OL `RIDEBUGLIB.OBJ`; declarations equal (ctest
  `rwe_declarations_ridebuglib`). BSS2's `RIDEBUGLIB.OBJ` is another build.
* **C file:** `libs/userlibs/rwe/ridebuglib.c`; test
  `libs/tests/rwe/ridebuglib.c` (snitch case "rwe ridebuglib").

## How it works

Every command's routine is an `RTS` (`_dummybranch`, Amiga mode only, also
init and finit); the work is done in its **error check** routine. So the
commands act only when the program is compiled with the debugger
(runtime errors on); without it the compiler cuts the error area away
and they do nothing. `$D509` is a token of the system library debuglib
(header tokens `$D500`-`$D510`), not `libnum<<7 + n`.

## C and assembler

The checks are C functions in the error check area (`BLITZ_ERRCHK`)
behind `BLITZ_ERRCHK_THUNK`s; the header's error field is
`BLITZ_ERROR_AREA`. One thunk is assembler: DisAsmWindow's check needs
its caller's return address (`MOVE.l (a7),a0`), which lies above the
thunk's saved registers – a C function only sees the thunk's return
address. The original's `error` routine (`MOVE.l #mess1,d7:TRAP #1`,
called by nothing) is left out. The message block `mess1` is ordinary C
data (bss), not in the error check area as in the original: 260 bytes
that stay in the program without the debugger, no other difference. The
thunks keep d0/d1/a0/a1; the original changes a0/a1 (AddVarTrace also
d0, d2, d3 and a2, ProcControl d0.b) and whatever docall changes. The
type word is read from -2(a2) without changing a2 (DIFF as in all C
ports, `docs/libs/cmdlibs-port.md` "General").

## BUG(orig)

* AddVarTrace/DelVarTrace copy the name with a loop that tests the byte
  *after* the one copied: an empty name copies its 0 and then whatever
  follows up to the next 0. Nothing limits the copy to the 252
  characters of `mess1` (#612, #613).
* AddVarTrace converts the type word with an 8 byte table indexed without
  a check: a type outside 0..7 reads the code behind the table in the
  original, other bytes in the C build (DIFF, unavoidable) (#609).
* DelVarTrace writes the name at +12 instead of +8 (`ADD.l #11` after the
  command byte), its 0 without advancing; +1..+11 keep the previous
  message's bytes (#610).
* DisAsmWindow searches backwards from the return address for the word
  `$4E41` (the TRAP #1 the debugger's code has in front of the call)
  without a limit (#611).
* Not a bug: ProcControl clears the top byte of `size` (`MOVE.b #0,4(a0)`)
  where the other commands clear `chars[0]` at +8; the debugger
  (`rwelibsrc/Debuglib/defaultdbug50.bb2`, `_CheckTrace`) uses only data1
  and data2 for `_proccontrol` (250).

## Test

docall is replaced by a stub (`sysload_stubs`) that records the whole
message; the checks of every form run in one sequence on both sets and
the messages are compared after each (so the bytes left from the
previous message too): AddVarTrace with the types 0..7, output 0/1/2 and
the empty name, DelVarTrace (also empty), VarTraceWindow, DisAsmWindow
(called from a small code block with a TRAP #1 word in front: the address
must be that word), CopperTrace with 0, an address, an object with the
offsets 0, 4 (copper list 0), 8 and -4, and object 0, ProcControl toggle,
on, off, $101, MemoryWindow; the number of messages; the routines, init
and finit for the kept registers. Not covered: the real debugger front
end.
