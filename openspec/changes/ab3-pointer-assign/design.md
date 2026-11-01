# Design

## Context

See proposal.md (Why). AmiBlitz3's rule (`AmiBlitz3.ab3` 23531-23572 at
`58a11658`, in its assignment routine after `JSR eval` of the right
side): left side pointer (`leftsideflagmask = 1`, or a NewType left side
– not taken over, decision 2026-11-01) → with a cast (`typetocast`) the
`SYNTAX 4` check only; right side without a type → ok; right pointer
(`rightsideflagmask = 1`) → ok; right NewType (type ≥ $100) → `SYNTAX 4`
check only; right long (type 3) → ok; any other basic type → the error
`error_convert_types` ("Can't convert types"), before and independent of
`syntax_typecheck`. Our assignment is `dolet` (`blitz2.s` 14xxx, C in
`programs/blitz2/src`), where the left side's variable and the right
side's type (`eval`'s result type) are known; extension points as in
README "blitz2-ab3".

## Goals / Non-Goals

**Goals:** the rule of spec `ab3-compat/type-rules` in AB3 mode, the
same error text as AmiBlitz3, nothing else changed.

**Non-Goals:** NewType left sides, NewType-against-NewType checks, casts
checks, every other `SYNTAX` check (G8); the `+` prefix's
`notypecheck` (it only skips the `SYNTAX 4` part in AmiBlitz3, so it has
no effect on this rule).

## Decisions

### D1: One extension point after the right side
`ext_assigncheck(lefttype, leftisptr, righttype, rightisptr, cast)`
called in `dolet` after the right side is evaluated; `ext_bb2.c`
returns "no error"; the AB3 module raises the compiler's existing error
for "Can't convert types" (BB2 has the message, `noconerr`) if the rule
applies. *Why:* the core keeps BB2's code; one call site.

### D2: Which "type" counts
The right side's type is the result type of the expression as our
`eval` reports it (byte, word, long, quick, float, string, pointer,
NewType, none for constants); the mapping to AmiBlitz3's
`rightsidetype`/`rightsideflagmask` is checked against `ab3_cli_ref`
for each scenario (a constant has no type in AmiBlitz3; to verify: a
typed constant like `#c=5` used on the right side).

## Risks / Trade-offs

- [Our `eval` types an expression differently from AmiBlitz3 (e.g. a
  constant, `Peek.l`, a function without a declared type)] → each
  scenario is compared with AmiBlitz3; differences adjust D2's mapping.
- [AB3 programs that BB2-style rely on the silent conversion] →
  AmiBlitz3 refuses them too; the rule only matches AmiBlitz3.
