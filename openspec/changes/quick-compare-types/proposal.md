# Proposal

## Why

blitz2 (like the original compiler) hands the type an expression is wanted in (the "type so far") to every operand, also to the operands of a comparison, and `varcont` cuts a Quick that is fetched while that type is Long (`blitz2.s:10491-10498`; `programs/blitz2/src/eval.c:2893`). For a comparison the result is a wrong boolean that depends on the order of the operands (issue #1177, measured with blitz2 1.95, AmiBlitz3 v3.14.0, the CD's 2.10 and the port), with `q=1.5`, `l=1`:

* `r.l=(q>1)` is 0 (expected -1), `r.l=(q=1)` is -1 (expected 0), `NPrint Hex$(q>1)` gives 00000000 (a Long parameter);
* `If q>l` is true, `If l<q` is false (the same comparison); `r.l=(q>1.4)` is 0, `r.l=(1.4<q)` is -1;
* a Word, Byte, Quick or Float destination is not affected.

The manual says the comparison operators "all generate a boolean result" (`BLITZ2_MANUAL:1166-1171`, `:1180-1185`). `docs/blitz2/expression-types.md` has the rule and a measured table.

Decided (`~/.local/share/bb2-c-work/task-decisions.md`, 2026-11-21): the comparison is fixed as an original bug (D-021 class (a), severity 1, topic Compiler), parts 1 and 2 exact; the rule is the same in AB3 mode; D-007 is checked with the benchmark; this is a change of its own, done before `quick-long-arithmetic` (#1178), which depends on it.

## Dependency and order

* `quick-long-arithmetic` depends on this change: both need the operand types at the operator; this change provides them for the comparison operators (design, decision 2). No parallel work with `quick-long-arithmetic` or `compiler-function-optimizer` (same file, `eval.c`; user decision 2026-11-21).
* The deviation entry goes into `docs/blitz2/deviations.md`, which another task creates by moving section 5 of `docs/libs/deviations.md`; this change starts after that file exists.

## What Changes

* **Part 1: the context type no longer reaches the operands of a comparison.** The operands are evaluated with their own types; only the boolean (a Word, `cpoke`, `eval.c:1087-1093`) is converted to the type of the context. `r.l=(q>1)` is -1, `r.l=(q=1)` is 0, `Hex$(q>1)` is FFFFFFFF, `q>l` and `l<q` agree.
* **Partial fix (user decision, `task-decisions.md` 2026-11-21 "quick-compare-types (#1177): open questions"):** part 1 covers the right operand and a left operand that is one element (variable, field, array element, function value, `&var`, constant), by a patch with a marker from `varcont`. A compound left operand (`r.l=(q*2>3)`, `ok.l=(i*4+adr>lim)`) still gets the context type and stays wrong until the expression tree (#1180). The limit is stated in the spec, the deviation entry, `expression-types.md`, the site and the issue comment.
* **Part 2: a Long or Word against a Quick is compared exactly.** The Long (a Word is first extended) is compared with `floor(q)` (the high word of the Quick, sign-extended); on equality the fraction (the low word) decides. No float conversion, a few instructions, no library call, no change for two Quicks, two whole numbers or a Float (a Float comparison is a library call as today). Checked against the benchmark (D-007).
* **What stays:** the type so far flows as today for everything that is not an operand of a comparison, also the cut in arithmetic (`r.l=q*2` is 2 until `quick-long-arithmetic`); comparisons of whole numbers (including the byte-constant rule of #978, `eval.c:880-905`), of Quicks, of Floats and of strings keep their code.
* **Results change:** the executables of programs that compare a Quick variable or function value (a) in a Long context (destination, parameter) or (b) with a Long or Word operand; the compiled code of those comparisons. No library file changes (part 2 is inline). The 12 comparison places on the CD named in the #1177 comment (8 distinct, all whole numbers) give the same results; their code may differ. Named for the previous-commit comparison.
* **AB3 mode** gets the same rule (`blitz2` and `blitz2-ab3` link one core); AmiBlitz3 v3.14.0 keeps the old rule. A deviation row in `docs/ab3/variant.md`; the affected oracle tests in `tests/ab3` are marked.
* **Documentation:** the kind-F entry in `docs/blitz2/deviations.md`, `docs/ab3/variant.md`, `docs/blitz2/expression-types.md`, the site section "The type of an expression" (`site/content/docs/how-blitz2-works/compiling.md:155-170`), the comment at `eval.c:2893`.

## Capabilities

### New Capabilities

- `compiler/expression-types`: the typing of operators in an expression. This change adds the requirements for comparisons; `quick-long-arithmetic` adds the requirements for arithmetic to the same capability (requirement names do not collide, design decision 1). Whichever change is archived first creates the spec, the other one finds it.

### Modified Capabilities

(none)

## Impact

* `programs/blitz2/src/eval.c` (`eval5`, `doop`, the comparison handlers `doeqb`...`dogef`, `cpoke`, `varcont`), code shared with `blitz2-ab3` (`programs/blitz2/CMakeLists.txt`).
* Tests: a new bug-orig test `tests/bug-orig/compiler/compiler-quick-compare.{bas,expect,orig}` with `; @fixed #1177`, marked oracle tests in `tests/ab3`, the corpus and the prev comparison (`PREV_INTENDED`), the benchmark for the comparison of a loop condition.
* Docs and site as listed; issue #1177 (label `fixed-in-port`, comment: coordinator).
* Related: #1178 (`quick-long-arithmetic`, depends on this), #1179 (array index type), #1180 (expression tree; the tests of this change are its regression suite).
