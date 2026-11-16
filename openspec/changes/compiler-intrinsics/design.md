## Context

See proposal.md for the motivation. Facts of the current state (checked in the
sources of this commit):

- `Int` is `BLITZ_AFUNCTION(BL_LONG)` with `BLITZ_ARGS(BL_UNKNOWN)`,
  `BLITZ_SUBS(doint, intchk, 0)` (`libs/acidlibs/vallib.c:65-68`). The compiler
  evaluates the argument, pushes its type as a word on the a2 stack and calls
  `doint`, which reads the word and branches (`vallib.c:461-`). For a Quick it
  truncates towards zero: positive `clr.w d0 : swap d0 : ext.l d0`, negative
  `neg.l d0 : clr.w d0 : swap d0 : neg.w d0 : ext.l d0`. `bmath.int` measures
  196 cycles.
- `Frac` is `BLITZ_AFUNCTION(BL_FLOAT)` with `BLITZ_ARGS(BL_FLOAT)`
  (`vallib.c:76-80`), so a Quick argument is converted by `qutofl` first and
  the result is a Float; `q3=Frac(q1)` converts it back with `fltoqu`
  (`maineval2`, `eval.c:~735`: `convtype(t, asked)`). `bmath.frac`: 3751
  cycles. Task 10.9 of libs-performance (#1064) changes the library to
  `BL_UNKNOWN` with a direct Quick path and keeps the Float result.
- The compiler is told the wanted type: `maineval2` passes the asked type
  into `eval3`, `function` keeps it as `old = *type` and hands it to
  `libfunction(oldtype, ...)` (`eval.c:2265-2290`). A call as the whole
  right-hand side of an assignment to a Quick therefore sees `old` = Quick;
  whether a nested context (an operand, a parenthesis, an argument) can also
  show `old` = Quick is not yet known (open question 1).
- Library calls are emitted by `dolibtoke`/`userjsr` (`compile.c:1443-`); the
  runtime-error-checking mode (`DEBUGGA_B`) adds a JSR to the check routine of
  the sub before the call (`makelsub3`, `libcall.c:878-910`). That is
  compile-time: a program compiled without checking has no check call.
- `Len` and `Asc` take `BL_STRING`: the compiler builds the string in the
  work area at a3 (`makesbase`, `copstring`) and pushes the length for a2
  (`maineval2`, `eval.c:~750`). `lenstring` is the inline code
  `move.l d0,a3 : move.l -(a2),d0` (`libs/acidlibs/stringfunclib.c:1159`);
  `ascii_c` returns -1 for an empty string, else `*s` as a long
  (`stringfunclib.c:873-882`) and sets a3 to the string. Issue #1062 says
  "0 for ''"; the library returns -1, and the intrinsic must match the library.
- Both compilers (`blitz2`, `blitz2-ab3`) compile from the same files in
  `programs/blitz2/src`.

## Goals / Non-Goals

**Goals:** the generated code of the listed calls is the few instructions the
result needs; the value, the registers the compiler relies on (a3 to a7) and
the error-check behaviour equal the library call; the cases outside the
conditions compile byte for byte as before.

**Non-Goals:** no new language feature; no change of any library; no
intrinsic that needs a different result type than the call has today except
the one defined for `Frac`; no decision about the candidates.

## Decisions

### 1. Where the decision is made

An intrinsic is a branch in `libfunction` (`eval.c`), taken before
`dolibtoke`, keyed by the identity of the library entry (library and sub, as
`findtoke` returns them), not by the name, so that a user library with a
function called `Int` is not touched. The table is small and in one place
(`intrinsics[]`: entry, argument test, emitter). The branch looks at the
shape of the argument first (static type, plain variable) and, if the
conditions fail, rewinds the source pointer (`bakup`) and takes the existing
path, so that every case outside the conditions compiles exactly as before.
Alternative: a peephole pass over the emitted code. Rejected: the code is
poked word by word while the source is parsed (`pokewd`); there is no
representation to match on.

### 2. Int of a Quick

The argument is evaluated as an expression with `BL_UNKNOWN` as today; its
type is known when it has been evaluated. If the type is Quick, the compiler
emits the inline integer part (about 6 instructions, the library's
truncating code without the type dispatch) instead of the push of the type
word, the a2 set-up and the JSR. Any Quick expression qualifies (a
variable, an element, an expression), because the value is in d0 either
way. Truncation towards zero, as the library: `Int(-1.75)` is -1, not the
floor -2. The result type stays Long. The `intchk` JSR is not emitted in
the checking mode: it only tests that the type word is below 6, and a
Quick is 4.

The registers: `Int` as a library call keeps a3 to a7 and changes d0 and d1
(and a0/a1 through mathffp on the Float path). Inline code must change no
register but d0 (the register rule of libs-performance), and uses no a2
(nothing is pushed, so the stack accounting `stackuse` must not count the
type word: check `stackuse` after the emitted code).

Alternative: leave Int to the library. The gain of Int is smaller than the
gain of Frac (196 cycles of which about 70 are call, push and set-up), but
the code is the same kind and it shares the test, so it is done together.

Implemented (task 2.1): `tst.l d0 : bpl.s 1f : addi.l #$FFFF,d0 : 1: swap
d0 : ext.l d0` (14 bytes; 22 cycles for a positive value, 36 for a
negative one). Adding $FFFF to a negative value before the shift rounds
the integer part towards zero unless the fraction is 0 (q + 65535 cannot
overflow for q < 0; -32768.0 = $80000000 gives $8000FFFF, -32768); the
library's negate-shift-negate gives the same value for every Quick (the
sweep of `intrinsic-int`: all 65536 integer parts at four fractions).
The way through the compiler (Decision 1): libfunction arms the entry,
getap passes push flag 2 for the argument, maineval2 pushes no type word
for a Quick (so `stackuse` stays 0 and dolibtoke sets up no a2), and
dolibtoke emits the entry's code in place of the JSR (`userjsr`, which
also makes the `intchk` JSR in the checking mode). Every other type
compiles exactly as before.

### 3. Frac of a Quick, used as a Quick

`Frac` returns a Float, so the type of `Frac(q1)` in an expression is Float,
and `Frac(q1)*100000` has Float range. An intrinsic that returned a Quick
everywhere would change such programs (Quick range -32768..32767.99998,
Quick arithmetic). Therefore the Quick result is allowed only where the
Float result would be converted to Quick at once and unchanged: the call is
the whole right-hand side of an assignment to a Quick (variable, element,
field), `old` is Quick at entry (see Context) and the character after the
closing parenthesis ends the statement (`:`, end of line, a comment). The
conversion `fltoqu` of the exact fraction is exact (a 16-bit fraction fits
in the 24-bit mantissa), so the inline Quick equals the library's Float
converted back. The inline value: the low 16 bits of the magnitude with the
sign of the argument (positive: `and.l #$FFFF,d0`; negative: the two's
complement fraction, 0 when the low word is 0). `Frac(-1.25)` = -0.25 =
$FFFFC000; `Frac(-3.0)` = 0.

All other contexts keep the library call: a Float or Long target, a Float
argument, an operand (`Frac(q)*2`, `1+Frac(q)`), an argument of another
function, a condition (`If Frac(q)>0.5`), `Print Frac(q)`. For these the
fast Quick path of #1064 is the gain. Making the inline code also produce a
Float for a Float context (building the FFP value in line) is possible but
is the library path's job (10.9) and is not done here; a measurement after
10.9 may justify it, as a later change.

Dependency: the exact fraction is #1064's result. Before #1064 lands, the
library loses fraction bits for integer parts of 256 and more, and the
equality test of the spec (intrinsic against `f=Frac(q1) : q2=f`) fails for
those values. Order: #1064 first. If this change must land first, the
equality test keeps only integer parts below 256 until #1064 is in.

Runtime error checking: since #1064 `Frac` has the error check of `Int`
(`BLITZ_SUBS(doffrac, intchk, 0)`, `libs/acidlibs/vallib.c:80-84`: a string
is the runtime error "Illegal Type"); for a Quick it cannot fail, and the
intrinsic, like `Int`'s, makes no `intchk` call. The checking-mode tests
check that the result is the same and the code has no call.

Implemented (task 3.1). The condition, from the probe of task 1.1 (below):
the call is the first element of the right side of an assignment to a
number (`dolet2` calls `intrinsic_let` before its `eval`, which records the
`evaldepth` of that evaluation and the number of its first element;
`getelement2` counts the elements it begins), the asked type is Quick
(`old`), the argument's type is Quick, and the source behind the closing
parenthesis is, after spaces, a 0, `:` or `;` (read from `srcptr`
without `getchar_`, so that no CNIF, `~constant~` or UsePath marker is
acted on; anything else, also `Else`, keeps the library). The code:
`tst.l d0 : bpl.s 1f : neg.l d0 : swap d0 : clr.w d0 : swap d0 : neg.l d0
: bra.s 2f : 1: swap d0 : clr.w d0 : swap d0 : 2:` (22 bytes; a positive
value 26 cycles, a negative one 46). `intrinsic-frac` compares it with
the library (`f=Frac(q1) : q3=f`) for 11 integer parts at 264 fraction
words each, and with the definition for every fraction word of 0, -1,
255 and -32768.

The probe (task 1.1: `old`, `evaldepth`, the depth of the precedence stack
`precsp - precstack` and `regat` at the entry of `libfunction`, printed
by a temporary `Write` in `function`, runtime error checking on):

| Source | old | evaldepth | prec | regat | intrinsic |
|---|---|---|---|---|---|
| `q2=Frac(q1)` | 4 | 1 | 1 | 0 | yes |
| `q2=Frac(q1)*2` | 4 | 1 | 1 | 0 | no (`*` behind) |
| `q2=1+Frac(q1)` | 4 | 1 | 2 | 1 | no (second element) |
| `q2=(Frac(q1))` | 4 | 1 | 2 | 0 | no (second element) |
| `Poke.q a,Frac(q1)` | 4 | 1 | 1 | 2 | no (not an assignment) |
| `If Frac(q1)>0.5` | 0 (unknown -1) | 1 | 1 | 0 | no |
| `NPrint Frac(q1)` | 0 (unknown -1) | 1 | 1 | 0 | no |
| `f=Frac(q1)` | 5 | 1 | 1 | 0 | no (Float target) |
| `q2=-Frac(q1)` | 4 | 1 | 1 | 0 | no (second element) |
| `c=Int(q1)` | 3 | 1 | 1 | 0 | (Int: always) |
| `c=c+Int(q1)` | 3 | 1 | 2 | 1 | (Int: always) |

### 4. Len and Asc of a plain string variable

Condition: the argument is a bare string variable name (global or local),
with no index, no field and nothing after the name but the closing
parenthesis. The compiler loads the address of the variable's memory and
reads the length from the header (`-4(var)` per the issue; check against the
string layout in `docs/` and `strings1lib.c` before use), 0 for a variable
without memory; for `Asc` it reads the first byte if the length is not 0,
else -1. No string is built, so `makesbase`/`sbasegot`/`sbgot` are not
touched for this argument (the bookkeeping that decides whether a3 is saved
and the work area started), and no length is pushed for a2. a3 stays as it
was; the library call sets a3 to the string, and the compiler does not rely
on that after `Len`/`Asc` (to verify: nested string expressions such as
`b$=a$+Chr$(Len(a$))`, where a3 is the end of the string being built).

The string variable layout (task 1.2, read from the sources of this
commit):

- A string variable is a long that holds the address of the characters,
  0 while the variable has no memory (`libs/acidlibs/strings2lib.c:11-15`:
  "The variable holds the address of the characters (0 = no memory
  yet)"). The memory begins with a header of two longs in front of the
  characters: `-8` the allocated length, `-4` the length of the string;
  the characters follow, then a 0 byte (`strings2lib.c:12-15`, the
  structure `struct bb_strmem` in `libs/include/blitzobj.h:327-331`). The
  assignment (`lastring`/`astring`, `strings2lib.c:83-117`) sets `-4` to
  the new length and reuses the memory when it fits, else frees it and
  allocates length + 9 bytes; so the length at `-4` is always the length
  of the characters, also after a shorter string or "" was assigned (the
  memory stays, the length is 0).
- The compiler reads a string variable this way in `stvar`
  (`programs/blitz2/src/eval.c:2784-2799`, blitz2.s:10617) with the
  template `stvarget2` (`programs/blitz2/src/main.c:357-366`): `lea
  off(a5),a2` (a global; `pokela5s` makes it `off(a4)` for a local, as
  `varmode` says), `moveq #0,Dn : move.l Dn,-(a7)` (the length 0),
  `tst.l (a2) : beq.w skip` (no memory: length 0, no copy), `move.l
  (a2),a0 : subq #4,a0 : move.l (a0)+,(a7)` (the length from `-4`, a0 on
  the characters), `jsr copstring` (`libs/acidlibs/strings1lib.c:45-63`:
  copies the characters to a3 and ends them with 0).
- A global variable: the long at `off(a5)`; a local variable (a
  procedure's): the long at `off(a4)` (the same code with a4, `varmode`
  set); a field of a NewType (or an array element): `calcvar` leaves the
  address of the long in a2 (`TF_ADDRINA2`, `structs.h:114`), and `stvar`
  emits no `lea` (`eval.c:2787`) - the address calculation differs, and
  the long it finds is the same kind (address of the characters or 0); an
  unassigned variable holds 0 (globals start cleared; a procedure clears
  its locals with `clrloc` at its start, `tokens2.c:395-406`, in AB3 mode
  as `ext_procclrloc` says, `ab3proc.c:295`): the length is 0, `Len`
  gives 0, `Asc` -1 (`stringfunclib.c:931-939`: -1 for the length 0, else
  the first byte zero-extended, and a3 set to the string; `Len` is the
  inline `move.l d0,a3 : move.l -(a2),d0`, `stringfunclib.c:1262-1272`).

Alternatives: also an array element or a field (the issue: "unless easy").
Rejected for now: the address calculation differs, and with checking on the
index check must still run; list it under the candidates.

### 5. Candidates (listed and measured, not decided)

A task measures each in `tests/perf` with a loop of 2000 calls, the current
cost and the cost an intrinsic could reach (estimated from the instruction
count, then checked by a hand-written assembler loop in the benchmark
program if the estimate says it is worth it):

- `Sgn(q)` and `Abs(q)` of a Quick (`bmath.sgn` 218, `bmath.abs` 747: Abs
  returns a Float and converts through `qutofl`; the same type-and-context
  analysis as Frac).
- `Left$`, `Right$`, `Mid$` with a plain variable as source (the copy into the
  work area is needed for the result anyway; the gain would be the second copy).
- `Instr(a$,b$)` with plain variables (both strings are copied today).
- `Len` and `Asc` of an array element or a field.
- `Val(a$)`, `Cvi`/`Cvl`/`Cvq(a$)` with a plain variable.
- `Peek`/`Poke` with a constant or a simple variable address (already
  assembler entries; the saving is call and set-up only).
- `QAbs`, `QFrac`, `QWrap` are inline already: nothing to do.

Each measured candidate gets a line in the report with the cycles before,
the estimate and the risk; the decision is the coordinator's.

## Risks / Trade-offs

- [The context test for `Frac` accepts a case that is not the whole
  right-hand side] -> the nested cases are tests (`Frac(q)*2`, `1+Frac(q)`,
  `(Frac(q))`, an argument, a condition); the implementer checks `old` and
  `evaldepth` at entry first (open question 1) and restricts the condition
  until every such test compiles as before.
- [An intrinsic changes a register a following instruction relies on] ->
  inline code changes d0 only; a test with a following statement that uses
  d1..d7 values the compiler holds in registers (`regat`) around the call
  (an expression `a=b+Int(q)` that keeps a partial result in d1).
- [The header layout of a string variable is assumed] -> task 4.1 reads it
  from the sources and tests it for a global, a local, a `NewType` field and
  a variable that was never assigned.
- [The two compilers diverge] -> the code is shared; the tests run for
  `blitz2` and `blitz2-ab3`.
- [A third-party library defines a function with the same name] -> the table
  is keyed by library and sub identity (Decision 1).

## Migration Plan

Each intrinsic is its own commit with its tests and budget line. Every
commit names the changed results (`-DPREV_INTENDED`, the programs that use the
call) and the measured cycles before and after. No flag is added: the
change is the default; rollback is reverting the commit. Land after #1064
(Frac) or with the reduced equality test.

## Open Questions

1. Does `old` (the asked type) at the entry of `libfunction` identify only the
   top-level right-hand side, or can an operand, a parenthesis or an argument
   also show Quick there? Answered by the first task of group 2 (a probe of the
   compiler with the nested cases). The answer decides the exact test of the
   condition, not the behaviour in the spec.
   Answer (task 1.1, the table in Decision 3): no. `q2=Frac(q1)*2`,
   `q2=-Frac(q1)` and `Poke.q a,Frac(q1)` show the same `old` (Quick) and
   `evaldepth` (1) as `q2=Frac(q1)`, and the first two the same precedence
   depth; a parenthesis and a left operand show only a deeper precedence
   stack. The condition is therefore the position (the first element of
   an assignment's right side, `intrinsic_let`) and the end of the
   statement behind the call, not `old` and `evaldepth` alone.
2. Does `Int` with an argument that holds a partial result in a register (the
   `regat` mechanism) need the value moved to d0 first? Same task group.
   Answer (task 2.1): no. libfunction saves the registers below `regat`
   (movem) and sets `regat` to 0 before the argument, so the argument is
   in d0; after the call (and after the inline code) it moves d0 to
   D(regat) and restores the registers. The intrinsic keeps that frame
   and replaces only the push, the a2 set-up and the JSR (`intrinsic-int`:
   `c=Int(q)*3+Int(q1)`, `c=c+Int(q)`).
