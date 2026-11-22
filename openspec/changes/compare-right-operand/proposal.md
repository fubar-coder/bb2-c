# Proposal

## Why

When a comparison is the right operand of an operator, blitz2 lowers the type so far to Word (the type of the comparison's boolean, `cpoke`, `programs/blitz2/src/eval.c:1128`) and then converts the **left** operand down to Word before the operation (`eval5`, `convtypef(left, *type)`, `eval.c:947`; `blitz2.s:8998`). A Long loses its high word, a Float or Quick is cut: `rl.l=lw AND (l1=l1)` gives 1 for `lw=65537`, `rl.l=lb+(l1=l1)` -31073 for `lb=100000`, `rf.f=fh+(l1=l1)` -1 for `fh=0.5`, `If $10000 AND (l1=l1)` is false. blitz2 1.95, the CD's 2.10 and the port do the same, with runtime error checking off and on; no error is raised (issue #1182, measured in `~/.local/share/bb2-c-work/tr-1177-float-context.md` section 5, 30 cases `fc4`).

User decision (2026-11-21, `task-decisions.md`, "#1177 Float constant left; new bugs as issues"): "Issue, eigener Fix später" — a new issue (bug-orig, compiler, wrong-result) and a fix as a change of its own after #1177 and #1178.

## Dependency and order

* After `quick-long-arithmetic` (#1178) is merged; that change adds a raw-Quick flag to `eval5` whose left operand this change converts against (design, decision 4).
* Recorded order (2026-11-22, "Order of the Quick work; matrix test": "A: Fixes zuerst (Recommended)"): #1178, #1179, #1182, then the tree; and "Go for steps 1-7": "#1182" is step 5, "#1188, #1189, #1190" step 6, changes that edit `eval.c` one after another. So this change starts after #1179's change is merged, before `byte-fraction-constant` (#1188) and `power-negative-base` (#1189). Starting it before #1179 would change the recorded order (the user's).
* Not in parallel with any change that edits `programs/blitz2/src/eval.c` (`quick-long-arithmetic`, the #1179 change, `compiler-function-optimizer`, `byte-fraction-constant`, `power-negative-base`).

## What Changes

- **The boolean of a comparison is converted up to the left operand's type**, not the left operand down to Word: the left operand keeps its type and value, the operation is done in the left operand's type. `rl.l=lw AND (l1=l1)` gives 65537, `rl.l=lb+(l1=l1)` 99999, `rf.f=fh+(l1=l1)` -0.5, `rq.q=q+(l1=l1)` 0.5, `If $10000 AND (l1=l1)` is true. This holds with the comparison in brackets, bound by precedence (`lw AND l1=l1`), nested in a bracket with other operators (`fh+(0+(l1=l1))`), and after `NOT` or unary minus of the comparison.
- **Unchanged:** a comparison as the left operand (`(l1=l1)+lb`, already right), a comparison passed through a function, a Byte or Word left operand (the boolean is already a Word), the late conversion of a Quick by a Long right operand (#1178, decision 4 of `quick-long-arithmetic`).
- **A new compile error where the left operand is a Float and the operator has no Float form** (`AND`, `OR`, `EOR`, the shifts): `rf.f=fh AND (l1=l1)` is refused with "Illegal Operator for Type", as `rf.f=fh AND w` is today (open question 1).
- **Results change** (rule 3, named with `PREV_INTENDED`): the executables of every program with a Long, Quick or Float left operand of an operator whose right operand is a comparison; this includes the common `If flag AND x=5` with a default-typed (Quick) `flag`, whose truth value does not change for `AND` (design, decision 3). Program outputs change where the left value has more than 16 bits, a fraction, or (`OR`, `EOR`) a fraction below 1.
- **Documentation:** `docs/blitz2/expression-types.md` (the #1182 sentence, `:136`), the kind-F entry in `docs/blitz2/deviations.md` (and the #1177 row's "Not part of it: #1182"), `docs/ab3/variant.md` if AmiBlitz3 keeps the old behaviour (task 1.3), the `BUG(orig) (#1182)` comment at `eval5` (added with the red test) becomes `Fixed (#1182)`.

## Not in scope

- A compound left operand of a comparison (#1183), `-q` on the left (#1177's named limit), order independence (#1180).
- Comparisons themselves (#1177 is done) and their operands' types.
- The expression matrix: it holds two-operand cells only and has no #1182 cell; the matrix is not extended (its proposal excludes three operands).
- Array indexes (#1179), Quick arithmetic (#1178).

## Open questions

1. **A Float left operand of `AND`/`OR`/`EOR`/a shift with a comparison on the right** (`rf.f=fh AND (l1=l1)`): refused as any Float there (recommended), or the old conversion kept for these operators (the left operand cut to Word, as today). The user's (asked together with `byte-fraction-constant`'s question 1; worked with: refused).
2. **AB3 mode:** the fix in both modes, as decided for #1178 ("Neue Regel auch in AB3 (Recommended)") — that answer was given for #1178; for this case the user's (worked with: both modes).

## Capabilities

### New Capabilities

- `compiler/expression-types`: a comparison as the right operand of an operator. (The capability is also added by `quick-compare-types` and `quick-long-arithmetic`; whichever change is archived first creates the spec, the later ones add to it.)

### Modified Capabilities

(none in `openspec/specs/`)

## Impact

- `programs/blitz2/src/eval.c` (`eval5`, `compareop`, `getelement2`, the bracket and `NOT`/negation paths that carry the flag of decision 1), shared by `blitz2-ab3`.
- Tests: new bug-orig test `tests/bug-orig/compiler/compiler-compare-right-operand` (`.bas`, `.expect`, `.orig`), the prev comparison (`PREV_INTENDED`), the CD corpus, the matrix passes (unchanged).
- Docs as listed above; site: the fixed-bugs entry and overview row (coordinator).
- Related: #1177, #1178, #1180, #1183.
