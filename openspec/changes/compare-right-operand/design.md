# Design

## Context

`eval5` (`programs/blitz2/src/eval.c:883`, main c336cf473) evaluates the right operand with the type so far (for a comparison operator with type 0, #1177), then for every non-comparison operator runs `convtypef(left, *type)` (`eval.c:947`) and `doop(cur, *type)`. The type so far only rises while an expression is read (a fetched value is converted up to it, `varcont`), so `*type` after the right operand is normally the left type or higher. Two things lower it:

* a comparison: `compareop` (after #1177) and `cpoke` (`eval.c:1128`) return Word;
* the late cut of `quick-long-arithmetic`, decision 4: a Quick left operand and a Long right operand with no type so far (`q*100000`): the Long turns the type so far Long, below Quick in the type numbering, and the Quick is converted to Long on purpose (`+ -` as today, `* /` by the new routines).

The measurement (`tr-1177-float-context.md` section 5, `fc4`, 30 cases, 2.10, 1.95 and the port the same, runtime error checking off and on) shows the first: the left operand is converted down to Word. The analyst's guess (not built): convert the right result up instead. Not measured: AmiBlitz3 v3.14.0 and AB3 mode.

## Goals / Non-Goals

**Goals:** the left operand keeps its type and value when the right operand is a comparison; every other conversion in `eval5` stays as it is (after #1178).

**Non-Goals:** see proposal, "Not in scope". No look-ahead, no new patching mechanism (the tree, #1180, must replay this change's bytes in its steps 1-4: a local rule in `eval5` is easy to replay, a code patch is not).

## Decisions

### 1. How the right operand is recognised as a comparison's boolean

* **B1, a flag (recommended):** `cmpres` ("D(regat) holds a comparison's boolean"), set where `compareop` returns, cleared by `getelement2` with `negnext` and `rawq` (#1178), kept by a bracket (the inner `eval5` leaves it), by `NOT` and by unary minus of the boolean (`-(a=b)` is 1 or 0, still a whole number of Word size), cleared by every other operator. In `eval5`, after the right operand: if the operator is not a comparison, `left` is set and higher than Word and `cmpres` is set, the boolean in D(regat+1) is converted up to `left` (`regat++; convtype(*type, left); regat--`, the form `dopow` uses) and `*type = left`; then `convtypef(left, *type)` emits nothing.
* **B2, a type rule:** convert up whenever `*type` is lower than `left`, except the late-cut pair Quick/Long. Shorter, but it reads every future lowering as a comparison and must name the #1178 exception explicitly.

Choice (mine, planning): B1. Rating: `Point: the flag misses a path that yields a boolean (NOT, negation, a bracket) and the left operand is still cut there — L2 Possible / I2 Medium / E2 Reasoned → Plan`; the test covers each path (task 1.1) and a missed one is a correction inside this change.

### 2. The conversion of the boolean

`ext.l` for Long (the boolean is -1 or 0, also `$FFFF`/0 extended), Word to Quick (`swap`/`clr.w`, -1.0 or 0) for Quick, `SPFlt` (after `ext.l`) for Float, with `lasta6` handled as `convtype` does today. No new code form: `convtype` already has each conversion.

### 3. What changes for whole-number programs

`If flag AND x=5` with a default-typed (Quick) `flag`: today `flag` is converted to Word (its whole part) and `AND.w` with the boolean; after the fix the boolean becomes -1.0 (`$FFFF0000`) and `AND.l` with the raw Quick: the result is the whole part of `flag` or 0, the same truth value for every `flag`. `OR` and `EOR` keep the fraction: `If q OR (a<>b)` with `q=0.5` is true after the fix (0.5 is not 0), false today. Long left operands: values beyond 16 bits survive. The executables of all such programs change (about 4 to 8 bytes per place); the prev comparison counts them (task 3.3).

### 4. With #1178's raw-Quick flag

After `quick-long-arithmetic`, a Quick fetched under a Long type so far is held raw with `*type` Long and `rawq` set. With a comparison on the right (`rl.l=q+(l1=l1)`), `left` is Long (raw): the boolean is converted to Long and #1178's mixed path computes `floor(q + -1)` = 0 for `q=1.5`. So this change needs no case of its own for a raw left operand; the test has the case (task 1.1), its expected value by #1178's rounding rule.

### 5. Float left operand of a bit operator (decided by the user)

With decision 1, `rf.f=fh AND (l1=l1)` reaches `doop(AND, Float)`, which has no handler: "Illegal Operator for Type", as `rf.f=fh AND w` is today. The alternative, keeping the old down-conversion for these operators when the left is a Float (a special case that keeps a wrong value), is not taken. Decision (the user, 2026-11-22, "Last answers of the session"): "Ablehnen wie unter Word (Recommended)". `Point: a Float left of AND/OR/EOR/a shift with a comparison on the right becomes a compile error — L2 Possible / I3 Observable / E2 Reasoned → decided by the user` (task 3.2: the CD corpus compiled with the fix shows whether any CD source has the form; reported before the merge).

### 6. AB3 mode (decided by the user)

One core, no mode condition. Decision (the user, 2026-11-22, "Coordinator session (handoff-2026-11-22-quick.md), answers", D2): "Beide Modi (Recommended)" -> the fix in BB2 and AB3 mode. Whether AmiBlitz3 has the same bug is measured in task 1.3; if it has, `docs/ab3/variant.md` gets a row (as #1177).

### 7. Cost (D-007)

Per place one conversion of the boolean instead of one of the left operand: `ext.l` (4 cycles) instead of nothing for a Long; `swap`+`clr.w` instead of `swap` for a Quick (4 cycles more); `SPFlt` instead of `SPFix` for a Float. No library routine, no loop. A condition such as `If flag AND x=5` inside a game loop costs about 4 cycles more. `Point: a hot loop with many such conditions gets measurably slower — L1 Unlikely / I1 Low / E2 Reasoned → Monitor` (speed alone is not I3; the commit names the extra instruction).

## Risks / Trade-offs

* [The flag is lost or kept across an operator by mistake] → the test has brackets, nesting, `NOT`, negation, two operators after the comparison (`fh+(l1=l1)+fz`, `fz+fh+(l1=l1)`, `fh+(l1=l1)*1`).
* [Many CD executables change] → named in `PREV_INTENDED` from the prev comparison; outputs checked by the CD corpus and `corpus`.
* [#1178's mixed paths meet a boolean of Long type they were not tested with] → task 1.1's raw cases.
* [The tree must reproduce the bytes] → decision 1 is a rule local to `eval5`; noted for #1180's step 0.

## Planning choices (mine, not the user's)

* Decision 1, B1: L2 / I2 / E2 → Plan (above).
* Decision 7: L1 / I1 / E2 → Monitor.
* Test form: a hand-written bug-orig test in the old form (`tests/bug-orig/compiler/compiler-compare-right-operand`), as the matrix's open tests are ("A: Alte Form, später umziehen (Recommended)" was given for the matrix; this test follows the same form only because the BASIC test model is not usable yet: prescribed by the state of `basic-test-runner`, not rated).
