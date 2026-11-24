# Proposal

## Why

The spec `compiler/expression-types` has no rule for the bit operators
under a Quick type so far, and its requirement "Rounding happens once per
operation" says that a Quick OP Quick keeps the fraction, which is no
longer true for them. Two fixes are merged on main without a spec:

- #1209 (`bc7b6614d`): `NOT`, `AND`, `OR`, `EOR`, `&` and `|` under a Quick
  type so far take the whole part of a Quick operand and give a Quick.
  It also took back the part of #1182's fix that made
  `If qh OR (l1<>l1)` true for `qh=0.5` (`compiler-compare-right-operand`
  cells c33, c34, c44).
- #1208 (`95bd46e62`): a negative whole constant under a Quick type so far
  is the Quick of its value.

The user's decisions of 2026-11-24 (`rounds/tree/decisions.md`: "Bit
operators on a Quick: ..." and "Spec for #1209/#1208: a small OpenSpec
change").

## What Changes

- MODIFIED "Rounding happens once per operation": the first sentence is
  limited to `+`, `-`, `*`, `/` and the shifts and refers to the new
  requirement; the scenarios stay.
- ADDED "NOT, AND, OR and EOR under a Quick type take the whole part"
  (#1209).
- ADDED "A negative whole constant under a Quick type is the Quick of its
  value" (#1208).

## Impact

Spec only: no code, test or build result changes; the code and the tests
(`tests/bug-orig/compiler/compiler-quick-bit-operators`,
`compiler-quick-negative-const`) are on main. The spec has no scenario
for `If q OR (a<>b)` with a Quick left operand and a fraction between 0
and 1, so no scenario of #1182 changes; the Quick case is covered by the
new requirement.
