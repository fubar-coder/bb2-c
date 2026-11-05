# Design

## Context

**Research.** The behaviour was read from AmiBlitz3's compiler at
`58a11658` (`AmiBlitz3.ab3`) and measured with probes in the emulator.
The notes from 2026-11-04/05 are in the session scratchpad, `g4/notes.md`
and `g4b/notes.md`. The probe sources become the test cases (task 6).

**The procedure code in `programs/blitz2/src`:**

| Part | Code |
|---|---|
| definition | `tokens2.c` `dofunc`/`dofunc2` (blitz2.s:13504) |
| parameter loop | tokens2.c:311-351 (limit `npars >= 7` at :351, register stores :334-349) |
| string parameters | the 'stloop at :364-372 |
| prologue | `linkput` (`link a4` + `jsr clrloc`) |
| end of definition | `procfixer` |
| epilogue | tokens2.c:479-496 (removes 4 bytes per string length) |
| statement call | `compile.c` `handleproc` |
| function call | `eval.c` `afunction` |
| shared argument loop | `asmlibs.c` `fetchpees_p` (`notparerr` on a missing `,`) |
| argument pushes | `eval.c` `maineval2`: `move.w d6,-(a7)` for byte/word, `move.l` for the rest, counted in `stackuse` |

**`struct proc`** (`structs.h:129`) already has `params[10]`. The
parameter count is a nibble. The node size `len` is BB2's, so new fields
would change the chunk layout.

**`@` today.** In BB2's `getvname` (eval.c:524) `@` is a variable-name
prefix, the pointer flag.

**Extension points.** AB3 mode works through the extension points in
`ext.h`. `ext_bb2.c` implements them with BB2's behaviour, the AB3
modules are `ab3*.c`.

## Goals / Non-Goals

**Goals:**
- AmiBlitz3's visible behaviour for optional parameters, `FAST` and the
  NewType return type.
- AmiBlitz3's code for everything about parameters 1–6, including the
  omitted-string code. G8's byte equality for procedures with up to 6
  parameters stays possible.
- The stack convention for parameters 7–10.

**Non-Goals:**
- Byte-equal code for procedures with 7–10 parameters. The stack
  convention differs from AmiBlitz3's static temps by decision.
- FPU parameters (G6).
- The `SYNTAX` argument type checks and the `Function Return` NewType
  check (G8).
- The function optimizer (G8).

## Decisions

### D1 Procedure data in a side table, not in `struct proc`
`ab3proc.c` keeps, per `struct proc *`, these entries:
- the index of the first optional parameter (none = 31, as AmiBlitz3's
  byte `$20`);
- the FAST flag;
- the NewType node of a NewType return type.

The table is a small array or list indexed by the proc pointer, filled
by the definition's extension points and read by the calls. The chunk
layout and `NAMEOFF_PROC` stay unchanged, so the BB2 code paths are
untouched.

**Alternative:** new fields in `struct proc`. Rejected: that is a chunk
change in shared code, and the BB2 node size would change.

### D2 Definition: `FAST`, `@`, limit, return type
New extension points, implemented as no-ops (BB2's behaviour) in
`ext_bb2.c`:

- **`ext_procfast()`**, after the return type and before the name: in
  AB3 mode it consumes the text `FAST` followed by a space, as
  AmiBlitz3.ab3:22045-22056 does. It is upper case only and not a token.
- **`ext_procoptional()`**, at the start of each parameter: in AB3 mode
  it consumes `@` and records the first optional index. This happens
  before `getvname`, so the `@` never reaches BB2's pointer-flag path.
- **`ext_procmax()`**: the parameter limit, 7 in BB2 mode and 11 in AB3
  mode, replacing the constant at tokens2.c:351.
- **`ext_procrettype()`**, in `dofunc` where `sizespec2` refuses a
  NewType: in AB3 mode it accepts `Function.nt`. The return type
  becomes long, and the NewType node is kept for the right side of a
  pointer assignment (`ab3type.c`'s value type), so `*p.nt = mk{}`
  passes the rule of `ab3-compat/type-rules`.

### D3 Stack slots of parameters 7–10
**Caller:** the arguments are evaluated in order.
- Parameters 1–6 go to d0–d5 as today.
- For parameter 7 and up, `maineval2`'s push path is used, as for
  library commands with more than 6 arguments. A byte or word is pushed
  as a word (2 bytes); long, quick and float as a long (4).
- A string at any position gets AmiBlitz3's 8-byte slot: the length,
  then the filler or the pointer. For a register below 6 that is
  `sbtolong`'s `pea 0` filler; from the 7th on it is the pointer
  (`move.l d6,-(a7)`).
- Every push is counted in `stackuse`, so later string expressions in
  the same statement address the stack correctly (`movea7dn`,
  eval.c:575-584).

**Callee:**
- The frame offset of each slot is computed from `params[]`. A slot's
  offset is 8 plus the sizes of all slots pushed after it.
- At entry, after the register stores, each parameter from 7 up is
  copied from `off(a4)` to its local: `move.w` for byte and word (the
  byte from `1+off`), `move.l` for the rest.
- Strings use the 'stloop with the slot offsets. For a register above 5
  the pointer comes from the slot, as AmiBlitz3.ab3:22297-22307 does.

**Epilogue:** `ext_procstackbytes(p)` returns the slot bytes N. The
epilogue removes N bytes below the return address with BB2's forms
(tokens2.c:479-496). `afunction`/`handleproc` subtract N from `stackuse`
after the call, as they do today for the string lengths.

**Nested calls:** the outer slots lie below the `movem` save of
`afunction`, so an inner call cannot touch them (g4b §2.2).

**Alternatives:**
- AmiBlitz3's static temps (`staticslib` `savetemp`/`loadtemp`).
  Rejected by the user: a nested call overwrites them, and nothing in
  the corpus depends on them (g4b §1).
- Long slots for byte and word. Rejected: it needs a special push mode,
  while BB2's word push already exists for library commands.

### D4 Omitted arguments
- **Tail path:** `fetchpees_p` reaches a missing `,`. If the call is a
  procedure call and the argument count has reached the first optional
  index, `ext_procdefaults(p, n)` writes the defaults of parameters n
  and up. Otherwise it is `notparerr`.
- **Empty path:** in `handleproc`/`afunction`, when the first parameter
  is optional and the braces hold only spaces, the same routine is
  called with n = 0, before `fetchpees_p`.
- **Default code**, AmiBlitz3's for registers below 6:

  | Type | Register < 6 | Parameter 7–10 |
  |---|---|---|
  | byte, word, long | `move.l #-1,dN` | `move.w #-1,-(a7)` for byte/word, `pea -1.w` for long |
  | quick | `move.l #$ffff0000,dN` | `move.l #$ffff0000,-(a7)` |
  | float | `move.l #$800000c1,dN` | `move.l #$800000c1,-(a7)` |
  | string | tail: `move.l #-1,dN : pea 0 : subq.w #4,a7`; empty path: `moveq #-1,dN` and two `move.l #0,-(a7)` | `pea 0 : move.l #-1,-(a7)` |

  - The -1 pointer is the marker AmiBlitz3's `lastring` treats as
    "leave empty" (AB3 mode links AmiBlitz3's libraries).
  - The empty path also fills parameters 7–10 with the pushes above.
    AmiBlitz3 gives a syntax error there (D-row in variant.md).
  - Every push is counted in `stackuse`.

### D5 FAST
- `linkput` skips `jsr clrloc` when the side table marks the procedure
  FAST.
- `procfixer` gives "FAST directive is not allowed if local strings or
  newtypes are used" when it is FAST and `p->nstatics != 0` (bit 15
  included). This is AmiBlitz3's rule (22361-22420), which also counts
  string literals.

### D6 Errors
These keep blitz2's own texts:
- "Not Enough Parameters";
- "Too many parameters";
- "Syntax Error" for gaps and too many arguments.

The FAST error is new in blitz2-ab3. The tests compare outputs, not
error texts, against AmiBlitz3. That is how G2 and G3 handled it.

### D7 BB2 mode
Every new extension point returns BB2's behaviour in `ext_bb2.c`. The
BB2 path through the chunk files is unchanged, and the `bb2-equivalence`
checks (corpus with both compilers, `ab3_bb2_equivalence`) cover it.

## Risks / Trade-offs

- [The stack accounting is wrong after pushes, and a later string
  expression in the same statement reads a wrong offset] → Count every
  push in `stackuse`, including the default pushes. Test cases with a
  string argument after 7+ arguments, and a nested call (p11).
- [Byte/word slots of 2 bytes leave a7 at 2 mod 4] → The 68000 accepts
  it, and BB2's library calls do the same today. Offsets come from
  `stackuse`, not from alignment assumptions.
- [The callee's slot table and the caller's pushes disagree] → One
  function computes the slot layout from `params[]`, and both sides use
  it. A test covers every type at positions 7–10.
- [A source relies on AmiBlitz3's syntax error for `name{}` with 7+
  only-optional parameters, or on `Function Return` checks] → Intended
  deviations, recorded in variant.md "Differences".
- [The `@` of a BB2 source compiled in AB3 mode changes meaning] → AB3
  mode follows AmiBlitz3, which has the same rule. BB2 sources compile
  in BB2 mode.

## Open Questions

None. The decisions are the user's of 2026-11-04/05.
