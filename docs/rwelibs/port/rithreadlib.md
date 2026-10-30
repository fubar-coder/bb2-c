# rithreadlib (44): threads

Reflective Images 1995. Blitz code as a separate DOS process:

* `CreateThread name$,address` – takes 10240 bytes (`MEMF_CLEAR`)
  through memlib's `globalloc` (freed by memlib's finit), saves d0–d7/
  a1–a6 as they are after that call (d0 = the memory) and starts a
  process (`CreateNewProc`: `NP_Entry` the library's start code,
  `NP_Name` the string, `NP_ExitCode` the library's exit code). The
  start code loads the saved registers, sets a2 = memory + 1024, a3 =
  memory + 3072 (after the 2048-byte string buffer), a4 = a3 + 4096 and
  a0 = `address`, and jumps there; the thread shares the program's
  globals (a5). When its code returns, DOS calls the exit code: the
  count goes down, at 0 the parent task gets the library's signal.
* `ActiveThreads` – the number of threads not yet ended.
* init: the highest free signal of 31..17 of the calling task, the task,
  dos.library V36 (none if no signal is free: then `CreateThread` does
  nothing). finit: waits for the signal if threads are running, closes
  dos.library.

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

The variables keep the source's order in one structure (`th`), which
the start code reads by name.

## BUG(orig), kept

* The registers, the memory and the address are single variables: a
  second `CreateThread` before the first thread has started
  (`CreateNewProc` returns before the process runs) gives both threads
  the second call's registers, memory and code.
* finit waits only once: a signal left from an earlier time the count
  was 0 lets it return while threads still run (whose exit code is in
  this library); the signal is never freed and dos.library's base not
  cleared.
* `ActiveThreads` (`#word`) returns the count as a long.
* The memory is not checked: without it the thread's registers are
  based on 0.

## Test

C build and reference build, each set alone (rwecmp.h). The thread code
is the test's: it records the registers it starts with, its process
name and whether its memory is cleared, signals the test task, waits for
CTRL-F and returns. Every thread is started and ended before the next
step, and the test waits until the process is gone (finit returns on
the last thread's signal, the process ends right after) – nothing runs
when the test ends.

* phase 0: no free signal 17..31 (the test takes them): init gets no
  dos.library, `CreateThread` does nothing, finit nothing;
* phase 1: init, threads A and B (`ActiveThreads` 1, 2), A ends (1, no
  signal), B released, finit waits for its signal (0);
* phase 2: init again, thread C ends (0, the signal stays pending),
  thread D, finit returns at once while D runs (`ActiveThreads` 1,
  BUG(orig)), D ends.

Compared: the registers of every call (the original's kept ones kept),
`ActiveThreads`, the signal of each init and whether it is pending, the
thread's registers (d1–d7/a1/a5/a6 the same, a0 = the code, a2/a3/a4 =
memory + 1024/3072/7168, the memory cleared) and its name. The test
frees the signals the library leaves allocated.

Not covered: two `CreateThread` before the first thread runs (the
shared variables, BUG(orig)): its outcome depends on the scheduling.
