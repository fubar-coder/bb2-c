# Design

## Context

See proposal.md for the motivation. What exists:

- **Per-statement records:** with runtime error checking on,
  `dostatement` calls `debugstuff` (`compile.c:1066`, `blitz2.s:2871`)
  before each statement. It writes `trap #1` and a 10-byte record: the
  address of the editor's line item (`thisitem`), the column
  (`srcptr - thisitem`) and a link to the previous record (`cont_pc`).
  `undodebug` (`compile.c:1099`) withdraws it for labels and assembler
  directives; macros (`macmask`) and direct mode are skipped. Both only
  act if `DEBUGON` (`debugga` high byte).
- **Source lines:** the compiler reads the source as a list of editor
  line items (`nextline`, `compile.c:355`; `linenumat` counts them from
  0). In a command-line run (`ab3cli`, `ab3job`) the main source is
  such a list too, so `linenumat` is the line in the main source (minus
  the empty lines of an embedded header, `ab3hdrlines`). Includes are
  read by `readinc`; `ab3inc.c` counts the lines read of every open
  include (`incs[].line`, `ext_incline`) and already uses that for error
  places (`ext_errpos`, `ab3inc.c:327`); `ab3_incheaderline` knows the
  empty lines of an embedded header. `\\__CODELINE` (`ab3str.c:134`)
  counts a program-wide line and is not a file line.
- **Procedures:** `thisproc` names the `Statement`/`Function` being
  compiled (`ab3str.c`).
- **Switches:** the options of `blitz2-ab3` are parsed by `ab3args_parse`
  into the job (`ab3main.c:106`); `-r` clears the debug flag. `blitz2`
  itself starts the editor and has no command line.
- **Normal output:** whatever the compilers write without the mode must
  stay byte-identical (CLAUDE.md, acceptance rule).

## Goals / Non-Goals

**Goals:**
- Statement execution counts for programs compiled by `blitz2-ab3` in
  both modes, and an lcov report with lines and procedures.
- A mechanism that does not depend on runtime error checking.
- Programs with the mode run under Kickstart 1.3.

**Non-Goals:**
- Branch or condition coverage; coverage of the Blitz libraries'
  machine code (that is the later stage of `test-coverage`).
- A TED menu or `blitz2` (editor) switch; coverage of programs run
  from the editor.
- Counting inside macros expansions separately from the use site.

## Decisions

### D1 Switch: `-coverage` of `blitz2-ab3`
The option is an extension of `ab3args` (like `-mode`), stored in the
job (`ab3job`). It sets a compiler flag `covmode` that is false by
default. The compile fails with a message if the option is combined
with `-run`, `-d` (refused anyway) or given without `-e`.

**Alternatives:** an `.xtra` switch or a TED compiler option would put
the setting into the editor's files and need TED changes, with no use
for the test runners; a source directive would change sources of the
corpus. `blitz2` has no command line, and `blitz2-ab3 -mode bb2` covers
BB2 mode.

### D2 Counting: one counter per statement, own table
Instead of the `trap #1` record, `dostatement` calls a new `covstatement`
next to `debugstuff` (same place; a `covundo` withdraws the statement for labels and
directives like `undodebug`) that:

1. appends the statement to the compile-time map (file, line, column,
   procedure) and assigns the next index;
2. emits `addq.l #1,<table + 4*index>` with a hunk relocation (6 bytes)
   into the program.

The table is a block of longs in the program's data, one per statement,
sized after the last statement (the table is allocated like the
program's other global data). The two mechanisms are independent:
`debugstuff` still writes its records when error checking is on.

**Alternatives:**
- *Reuse the debug records:* they exist only with error checking, run
  only through the debugger trap path (no count), and the trap is
  taken per statement, which is slow; the table is simpler and counts
  in release builds.
- *A `jsr` to a counting routine:* smaller code but needs registers
  and a library; `addq.l` to memory touches no register. The condition
  codes are dead between BASIC statements: the compiler generates
  compare and branch within one statement (verified in task 2.2).
- *A flag-neutral counter* (`pea (a0)`, `movea.l cnt,a0`, `addq.l #1,a0`,
  `movem.l a0,cnt`, `movea.l (a7)+,a0`: none of these changes the
  condition codes; about 22 bytes): the fallback if task 2.2 finds a BASIC
  statement whose code passes condition codes to the next one. `move
  sr,<ea>` is privileged from the 68010 on (a privilege violation in a
  user-mode program on the A1200), `move ccr,<ea>` does not exist on the
  68000, so saving the flags is not an option.

**Inline assembler is not counted.** Each assembler line is a statement
of its own, and it may pass condition codes (`CMP` on one line, `Bcc` on
the next), the X bit (`ADDX` chains) or rely on distances (`Bcc.s`
ranges, `DC.w l2-l1` tables, `?l2-?l1`). Any inserted code could break
it, so assembler lines get no counter and generate the same code as
without the option; the map lists them without counts. Counting at
assembler labels (the starts of basic blocks) is a possible later
extension.

**Statements treated like assembler.** `ALibJsr`, `BLibJsr`, `TokeJsr`,
`SysJsr`, `GetReg` and `PutReg` are assembler in BASIC form (a `jsr` or a
register move, mostly between assembler lines). They get no counter
either. `ALibJsr` and `TokeJsr` already get none, because they are in
`undodebug`'s list (`compile.c:1379-1383`), which `covundo` mirrors. A
line with a macro use gets no counter at all (`expandmax` sets `macmask`
before `debugstuff`, `compile.c:436`, `1073`).

### D3 Where counts are written
The generated program saves the table at its end. `finit` routines of
the system libraries run at program end (`End`, errors, window close),
so a new system library `covlib` has a finit that writes the table, and
the compiler links it (like `rundebuglib` in `compile.c:791`) only with
`-coverage`. Writing uses `dos.library` (`Open`/`Read`/`Write`/`Close`),
which exists in Kickstart 1.3, with the process window pointer set to -1
so a missing volume raises no requester.

The file `<exe>.covcnt` (the executable's name as given to `-e`, baked
into the program) has a header (magic, version, a 32-bit id of the
compile, the number of statements) and then the counts. If the file
exists with the same id and count the new counts are added; otherwise
it is replaced. Write errors are ignored.

### D4 The statement map
`<exe>.covmap` is text, written when the compile ends successfully (and
not at all after an error), one line per record, tab-separated:
`S index file line column` for a statement, `P name file line` for a
procedure, a first line `id` with the same compile id. File names are
as the compiler opened them (the source as given to `-s`, includes as
`readinc` resolved them). Text keeps the tool simple and diffable; the
size is irrelevant.

The line is taken as follows: main source `linenumat` plus 1 less
`ab3hdrlines`; inside an include `incs[].line` less its header lines
(`covline()` next to `ab3_incline`-style functions in `ab3inc.c`).
Statements of macro expansions are not counted (`macmask`, as
`debugstuff`).

### D5 Report: tool `blitzcov`
A C tool `tools/blitzcov` (logic in `tools/lib/blitzcov`, host tests
with inputs from memory) reads `.covmap` and `.covcnt` files (checks the
id), merges, and writes lcov. Per line the count is the largest count of
its statements (a loop line `For i=1 To 9 : x=1 : Next` is 1, 9, 1:
reported 9). A `--strip PREFIX` option rewrites the source paths, so that
the build's temporary paths give repository-relative `SF` names.

### D6 Test runners
Both runners get a switch (CMake variable) that adds `-coverage` to the
compile, copies `.covcnt` files from `TEST:` (a host directory) next to
the maps and runs `blitzcov` into
`<build>/coverage/lcov/<test>.info`. The compile uses the build's own
`blitz2-ab3`; no other test changes. A run of a measured program on
Kickstart 1.3 is part of the corpus' existing KS variants.

## Risks / Trade-offs

- **[Code size and speed]** `addq.l` adds 6 bytes and one memory
  access per statement → acceptable for a test and a user opt-in.
- **[Table size over 64 KB]** A baserel-style small data model could
  break with a large table → `covtable` uses absolute addresses with
  relocations, not a4 offsets (task 2.3 checks a program with 20000
  statements).
- **[A program that never ends normally]** (crash, reset): counts are
  lost → documented; a flush on `Delay`-free loops is not attempted.
- **[Changed generated code]** Branch distances shift by 6 bytes per
  statement → the compiler's existing relative-branch handling covers
  it; the corpus runs with the mode prove it.
- **[Line accuracy in joined lines]** `@@` joins lines (`ab3syn.c`);
  the statement is mapped to the first line of the joined line, as
  error places do.

## Open Questions

- The editor (TED) and `blitz2` could get a compiler-options switch
  later; it would reuse D2-D4 unchanged.
- Whether `covlib` should get its own library number or reuse a free
  slot is decided in task 2.1; the specs do not depend on it.

## Findings

### Condition codes across BASIC statements (corpus scan, 2026-11-05)
The scan looked for assembler that sets condition codes, then BASIC
statements, then assembler that reads the codes set before the BASIC
code. Such code would break with a counter before the BASIC statement.

**Searched:** 2 366 sources, 663 of them with inline assembler:
- the CD and Aminet, detokenised with `teddetok`;
- `bblibsrc` and `rwelibsrc`;
- the AmiBlitz3 repository's `.ab3` sources.

They hold 264 225 assembler statements and 26 878 consumers of condition
codes (Bcc, DBcc with a condition, Scc, ADDX/SUBX/NEGX/ROXL/ROXR, MOVE
from SR, TRAPV).

**Result:**
- No case carries condition codes, or the X bit, across a BASIC
  statement. 2 435 sequences of assembler, 1-3 BASIC statements and
  assembler exist, mostly `ALibJsr`, `BLibJsr` and `GetReg`, but none
  carries the flags across.
- 100 of the 111 raw reports branch on the result of a library call
  (`MOVE.l #65536,d1 : ALibJsr AllocMem : BEQ _initerror`). The flags
  come from the callee, so a counter before the call does no harm.
- 1 report reads flags set by a library's caller on entry.
- 10 are scanner false positives: FPU instructions, macro labels and
  disassembly lines.

**The generator:** no BASIC statement passes condition codes to the next
one. Compare and branch are always in one statement (`doif2`,
`tokens1.c:1005-1035`). So `addq.l` stays, and the flag-neutral counter
is not needed. Task 2.2 confirms it dynamically with a corpus run.

