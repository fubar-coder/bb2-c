# rithreadlib (44): threads

Reflective Images 1995. Blitz code as a separate DOS process:

* `CreateThread name$,address` – takes 10240 bytes (`MEMF_CLEAR`)
  through memlib's `globalloc` (freed by memlib's finit), saves d0–d7/
  a1–a6 as they are after that call (d0 = the memory), the address and
  the memory as the thread's start data behind the 10240 bytes and
  starts a process (`CreateNewProc`: `NP_Entry` the library's start
  code, `NP_Name` the string, `NP_ExitCode` the library's exit code),
  whose `tc_UserData` gets the start data between `Forbid` and `Permit`
  around the call (the process cannot run before). The start code
  takes the start data from its `tc_UserData` (and clears it), loads
  the saved registers, sets a2 = memory + 1024, a3 =
  memory + 3072 (after the 2048-byte string buffer), a4 = a3 + 4096 and
  a0 = `address`, and jumps there; the thread shares the program's
  globals (a5). When its code returns, DOS calls the exit code: the
  count goes down, at 0 the parent task gets the library's signal.
* `ActiveThreads` – the number of threads not yet ended.
* init: the highest free signal of 31..17 of the calling task, the task,
  dos.library V36 (none if no signal is free: then `CreateThread` does
  nothing). finit: waits for the signal until no thread runs, closes
  dos.library and frees the signal.

* **Source:** `rwelibsrc/threadlib.bb` (as TED shows it:
  `build/amiga/rwelibsrc/threadlib.bb`).
* **Reference:** the reference build of the source (`refbuild`,
  `build/amiga/libs/userlibs/rwe/ref/rithreadlib.obj`); no binary exists
  (`docs/rwelibs/README.md` §3.3). Declarations equal (ctest
  `rwe_declarations_rithreadlib`).
* **C file:** `libs/userlibs/rwe/rithreadlib.c`; test
  `libs/tests/rwe/rithreadlib.c` (snitch case "rwe rithreadlib").

## C and assembler

C behind `BLITZ_THUNK`s; `CreateThread`'s thunk passes all registers of
the call to the C function (and keeps them all; the original changes
d0/d1/a0/a1/a6). Assembler where C cannot do it:

* the start code (it sets every register and jumps);
* the `ALibJsr` to `globalloc`: the registers after it (d0, d1, a1, a6)
  are the thread's, so the call is an inline asm that returns them; d2–
  d7/a2–a5 are the call's (memlib keeps them);
* the count's `ADDQ.l`/`SUBQ.l` on memory: the parent and the threads
  change it, one instruction each as in the original (a load/add/store
  of C could lose a change).

The variables keep the source's order in one structure (`th`), whose
`stringbuffersize` the start code reads by name; the start data of each
thread are a structure of their own (`thstart`).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* finit waits until the count is 0. The original waited only once: a
  signal left from an earlier time the count was 0 let it return while
  threads still ran, in the program's code and with their exit code in
  this library, both freed when the program ended (#837; `libs_emulator`
  cases `[fixed #837]` of `rwe_rithreadlib`).
* finit frees the signal (cleared first), also when dos.library V36 did
  not open, and clears dos.library's base. The original never freed it:
  every run in the same task - from the editor, or a CLI program, which
  runs in the shell's process - took one more of the signals 31..17;
  after 15 runs `CreateThread` did nothing (#839; `libs_emulator` cases
  `[fixed #839]` of `rwe_rithreadlib`).
* The registers, the memory and the address are start data of each
  thread. The original kept them in single variables, which the process
  read only when it first ran; `CreateNewProc` returns before that, so a
  second `CreateThread` before the first thread had started gave both
  threads the second call's registers, memory and code (#838; test
  `rithreadlib-shared-start-state`). The scheduling is unchanged: the
  new process still runs when the scheduler picks it.

## Original behaviour that is not a defect

The memory is not checked; that is no defect: memlib's `globalloc`
never returns 0 (it raises "Out of Memory!").

`ActiveThreads` (`#word`) returns the count as a long; that is no defect
(the caller takes the low word, which is the count).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases (rwecmp.h), compared with the
expected data `libs/tests/expected/rwe_rithreadlib.txt` recorded from the
original. The thread code
is the test's: it records the registers it starts with, its process
name and whether its memory is cleared, signals the test task, waits for
CTRL-F and returns; thread D also returns by itself after 2 seconds (a
finit that waits for it would otherwise wait for the test task it
blocks). Every thread is started and ended before the next
step, and the test waits until the process is gone (finit returns on
the last thread's signal, the process ends right after) – nothing runs
when the test ends.

* phase 0: no free signal 17..31 (the test takes them): init gets no
  dos.library, `CreateThread` does nothing, finit nothing;
* phase 1: init, threads A and B (`ActiveThreads` 1, 2), A ends (1, no
  signal), B released, finit waits for its signal (0);
* phase 2: init again, thread C ends (0, the signal stays pending),
  thread D, finit waits until D has ended (`ActiveThreads` 0; the
  original returns at once while D runs, `ActiveThreads` 1: fixed #837),
  D ends.

Observed: the registers of every call (the kept ones stated per
routine, `KEEPS`),
`ActiveThreads`, the signal of each init and whether it is pending, the
thread's registers (d1–d7/a1/a5/a6 the same, a0 = the code, a2/a3/a4 =
memory + 1024/3072/7168, the memory cleared) and its name, and whether
the signal of each init is still allocated after its finit (the test
frees what the original leaves).

Two `CreateThread` before the first thread runs (#838) are the
bug-orig test `rithreadlib-shared-start-state`; in this scenario every
thread runs before the next step.
