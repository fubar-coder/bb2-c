# Proposal

## Why

blitz2 (like the original compiler) hands the type an expression is wanted in (the "type so far") to every operand, also to the operands of a comparison, and `varcont` cuts a Quick that is fetched while that type is Long (`blitz2.s:10491-10498`; `programs/blitz2/src/eval.c:2893`). For a comparison the result is a wrong boolean that depends on the order of the operands (issue #1177, measured with blitz2 1.95, AmiBlitz3 v3.14.0, the CD's 2.10 and the port), with `q=1.5`, `l=1`:

* `r.l=(q>1)` is 0 (expected -1), `r.l=(q=1)` is -1 (expected 0), `NPrint Hex$(q>1)` gives 00000000 (a Long parameter);
* `If q>l` is true, `If l<q` is false (the same comparison); `r.l=(q>1.4)` is 0, `r.l=(1.4<q)` is -1;
* a Word or Byte destination is not affected; a Quick or Float destination converts whole-number operands: `rq.q=(lw>1)` with `lw=65537` is 0 (the Long keeps its low word), `rf.f=(l2=l3)` with `l2=100000001`, `l3=100000000` is -1 (both become the same FFP value, 24-bit mantissa), also under a Float parameter (`tf{l2>l3}`, `Cos(l2=l3)`) and as the right operand of a Float (`If fz+(l2>l3)`); measured with the CD's 2.10, 1.95 and the port (`~/.local/share/bb2-c-work/tr-1177-float-context.md`).

The manual says the comparison operators "all generate a boolean result" (`BLITZ2_MANUAL:1166-1171`, `:1180-1185`). `docs/blitz2/expression-types.md` has the rule and a measured table.

Decided (`~/.local/share/bb2-c-work/task-decisions.md`, 2026-11-21): the comparison is fixed as an original bug (D-021 class (a), severity 1, topic Compiler), parts 1 and 2 exact; the rule is the same in AB3 mode; D-007 is checked with the benchmark; this is a change of its own, done before `quick-long-arithmetic` (#1178), which depends on it.

## Dependency and order

* `quick-long-arithmetic` depends on this change: both need the operand types at the operator; this change provides them for the comparison operators (design, decision 2). No parallel work with `quick-long-arithmetic` or `compiler-function-optimizer` (same file, `eval.c`; user decision 2026-11-21).
* The deviation entry goes into `docs/blitz2/deviations.md`, which another task creates by moving section 5 of `docs/libs/deviations.md`; this change starts after that file exists.

## What Changes

* **Part 1: the context type no longer reaches the operands of a comparison.** The operands are evaluated with their own types; only the boolean (a Word, `cpoke`, `eval.c:1087-1093`) is converted to the type of the context. `r.l=(q>1)` is -1, `r.l=(q=1)` is 0, `Hex$(q>1)` is FFFFFFFF, `q>l` and `l<q` agree, `rq.q=(lw>1)` is -1, `rf.f=(l2=l3)` is 0.
* **The right operand starts at type 0** (design 2 as written; user decision 2026-11-21 "#1177: right operand as planned"): it is evaluated in its own types. Accepted consequences until the expression tree (#1180): about 173 of the CD's 496 executables change (a constant on the right is typed by its value: `If x>1` with a default-typed `x` is `move.w #1 / swap / clr.w` instead of `move.l #$10000`, 2 instructions and 8 cycles more), `If q>1.4` is a Float comparison (a library call; `1.4` is a Float constant), and a compound right operand is computed in its own types, so `If base<i*4+base` (`base.l=$40000`, a default-typed `i=20000`) is false through a Quick overflow of `i*4` (true today).
* **The Float and Quick contexts join part 1** (user decision 2026-11-21 "#1177: Float context measured wrong"): whole-number operands of a comparison under a Float destination or parameter, as the right operand of a Float, or under a Quick destination are compared as whole numbers; only the boolean is converted. Comparisons with a Float operand keep their code (design, decision 3).
* **Partial fix (user decisions, `task-decisions.md` 2026-11-21 "quick-compare-types (#1177): open questions", "#1177 Float constant left; new bugs as issues", "#1177: `-q` left limit accepted"):** the left operand is covered when it is one element (variable, field, array element, function value; `&var` and a constant need nothing), by a patch with a marker from `varcont`; a left operand in `d7` (eight nesting levels) keeps the original's code, because its right operand lies beyond the registers, an original bug of its own (design 2). Named limits: a compound left operand (`r.l=(q*2>3)`, `ok.l=(i*4+adr>lim)`) and `-q` on the left still get the context type and stay wrong until the expression tree (#1180; issue #1183); a Long constant on the left under a Float context (`rf.f=(100000001>l3)`) is still emitted as an FFP value. The limits are stated in the spec, the deviation entry, `expression-types.md`, the site and the issue comment.
* **Part 2: a Long or Word against a Quick is compared exactly.** The Long (a Word is first extended) is compared with `floor(q)` (the high word of the Quick, sign-extended); on equality the fraction (the low word) decides. No float conversion, a few instructions, no library call, no change for two Quicks, two whole numbers or a Float (a Float comparison is a library call as today). Checked against the benchmark (D-007).
* **What stays:** the type so far flows as today for everything that is not an operand of a comparison, also the cut in arithmetic (`r.l=q*2` is 2 until `quick-long-arithmetic`); comparisons of whole numbers (including the byte-constant rule of #978, `eval.c:880-905`), of Quicks, of Floats and of strings keep their code.
* **Results change:** the executables of programs with a comparison whose right operand was typed by the left one (most comparisons with a constant: about 173 of the CD's 496 executables, measured by the spike), that compare a Quick in a Long context or a Long or Word with a Quick, or that compare whole numbers under a Float or Quick context. No library file changes (part 2 is inline). The 12 comparison places on the CD named in the #1177 comment (8 distinct, all whole numbers) give the same results; their code may differ. Named for the previous-commit comparison.
* **Not in this change:** #1182 (a comparison as the right operand lowers the left operand to Word: `rl.l=lw AND (l1=l1)`), a change of its own later; #1184 (the pad word of the code hunk).
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
