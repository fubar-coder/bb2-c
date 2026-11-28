# Design: type-system-rework

## Context

The typing pass `programs/blitz2/src/exprtyping.c` types every operation by
rule P+ (#1216, archived change `expr-tree-5-operand-typing`) and its
follow-ups (#1221-#1228): the wanted type (destination, parameter, index)
sets the format of intermediates (16.16, 48.16, whole at once for a `/`
under a Long), a constant subtree is typed by `const_flow` (the original's
"type so far"), and the code generator (`eval.c`) keeps the first build's
code for a constant when `p5consteq`/`p5constapply` see the same type.
`exprfold.c`/`exprarith.c` fold constants, `libs/compilerlib/compilerlib.c`
holds the 48.16 routines (D-027: any change there is acceptable, named).
The matrix model `tools/lib/exprmatrix/model.c` is the independent oracle of
the matrix; its port side is wrong for several constant shapes (#1232).

The user's decisions that this change writes down (rounds/tree/decisions.md,
quoted there; read by heading, not reworded here):

- "2026-11-27 Rework of the whole type system (direction)" (active,
  direction; details open)
- "2026-11-27 Type system rework, round 1: width, constants, whole division,
  conversion" (active; the width rule, said as a leaning, is confirmed by
  "type-system-rework Q9, Q12, Q6, Q8", Q6)
- "2026-11-27 Type system rework, round 2: bit operators, shifts, overflow,
  no destination" (the no-destination rule confirmed in round 3)
- "2026-11-27 Type system rework, round 3: comparisons, point constants, the
  shift exception, AB3"
- "2026-11-27 Type system rework, round 4: the shift exception's reach;
  planning"
- "2026-11-27 Type system rework, round 5: LSR vs LSL in the shift
  exception"
- The answers to the planning questions: "2026-11-27 type-system-rework
  Q9, Q12, Q6, Q8", "... Q1, Q2, Q4, Q7", "... Q3, Q5, Q10, Q13-Q15, Q17",
  "... Q11, Q16, Q18, Q19, Q20" and "2026-11-27 type-system-rework: shift
  widths" (all active), and the answers to the Q12 classes:
  "2026-11-27 type-system-rework Q12 classes" (A, B, C1, C3, D, E, J
  adopted, each a named exception replacing the answer named with it; F
  not adopted) and "2026-11-27 type-system-rework N1, N2".
- The answers to the interaction questions: "2026-11-27 type-system-rework
  I1–I3", "... I4" to "... I8", "... I9 and exception warnings", the
  answer to conflict item 29: "2026-11-27 type-system-rework conflict item
  29 (destination influence)", and "2026-11-27 type-system-rework
  warnings: scope, names, volume" (all active).

Issues: #1235 "[blitz2] The operand-only type system" (enhancement,
compiler) tracks the change; #1236 is the constant shift beside a Quick
(bug-port, shift-print/report.md option A, closed by task 3.4).

## Goals / Non-Goals

**Goals:** one type system for constants and variables, independent of the
destination and of the order of operands; the rules as a spec with values;
every replaced decision, requirement, doc line and test named; the cost
measured and kept low by value-keeping optimizations; AB3 mode the same.

**Non-Goals:** FFP arithmetic itself, string operators, functions (`Int`,
`Abs`, `Frac`, library commands take their argument by its type), new
types, the expression limits, the runtime error checks of indexes.

## Decisions

### 1. The rules (spec delta, requirement by requirement)

| Rule | Source (heading, 2026-11-27) | Spec requirement |
|---|---|---|
| destination only converts | direction | "The destination does not change the computation" |
| whole ops one step wider, cap Long; unary minus too | round 1 (width), Q6 "Bestätigen, beides" | "A whole-number operation is one step wider than its operands" |
| Quick ops in 48.16, cap 48.16 | round 1 (width) | "An operation with a Quick is computed in 48.16" |
| Float operand -> Float | direction (upcasting) | "An operation with a Float is computed in Float" |
| whole constant smallest type, no DEFTYPE; `-8` a negation (a Word) | round 1 "(c) kleinster Typ", N2 "Negation" | "A whole constant takes its smallest type" |
| point constant a Quick only when exact or beside a Quick, else Float | round 3, Q8 "Quick nur wenn exakt" | "A constant with a point is read by its operands" |
| whole `/` towards zero | round 1 "Auf null hin" | "A whole division rounds towards zero" |
| whole `/` at the top or reached through `+ - * /` and unary minus under Quick/Float keeps its fraction, every operand combination; under a Float a Float quotient | Q9; Q12 C1 "Ja, für alle"; I3 A; I4 B | "The whole-division exception under a Quick or Float destination", "A divisor 0 of a quotient that keeps its fraction" (derived from C1 and Q4) |
| whole `/` beside a Quick-class operand (variable, Quick subtree, Quick constant; not a Float) keeps its fraction | Q12 C3; I1 A | "A whole division beside a Quick keeps its fraction" |
| under a Float destination the top operation with a Quick runs in Float; a point constant there read as a Float | Q12 D "Float reicht oben hinein"; I2 A | "The top operation under a Float destination runs in Float" |
| 48.16 `*`, `/` cut down | Q1 "Immer abrunden" | "An operation with a Quick is computed in 48.16" |
| floor Mod | Q2 "Floor" | "Mod takes the sign of the divisor" |
| `/` and `Mod` by a run-time 0: the division trap for every type; a constant 0 the compile error | D1/D7 "Trap für alle Typen" (replaces Q4's run-time values) | "A division or Mod by a run-time 0 raises the division trap" |
| D only at an arithmetic top operation; D and E not through a unary minus | D2 | "The top operation under a Float destination runs in Float", "A shift at the top under a Quick destination" |
| a unary minus does not change a point constant's reading; a Quick reading is the FFP value cut down | D3; planner's P1 (section1-decisions.md) | "A constant with a point is read by its operands" |
| C3 at arithmetic operators, Mod, `^`, comparisons, not at bit operators and shifts; with C1 under a Float the Float quotient | D4; I3 | "A whole division beside a Quick keeps its fraction", "A whole division reached by two exceptions under a Float destination" |
| Quick and Float parameters take the exceptions like a destination | D5 | "Only the named exceptions depend on the destination" |
| `NOT` one step wider, of a Quick a Long | D6 | "A whole-number operation is one step wider than its operands" |
| conversion: a Quick rounds down, a Float towards zero (SPFix); B/W/L wrap, a Float beyond the Long range too; Quick saturates; `Data.b` with a point as a Float | round 1, Q12 B "Float auf null hin", round 2 "Umbrechen", direction, I7 A, I8 A+C | the two conversion requirements |
| bit operators whole; Float refused | round 2 (option 1), Q17 "Weiter verweigern" | "Bit operators take whole operands" |
| Quick: LSL/ASL/ASR in 48.16, LSR on 16.16; whole: LSR/ASR own width, LSL/ASL a Long; count whole | round 2, Q15 "48.16", Q14 and F "Eigene Breite", Q12 A "LSL immer Long" | "A shift keeps the type of its left operand" |
| shift exception under a Quick: at the top or through `+ -`; right shifts on the 16.16 bits, beyond the range in 48.16; left shifts a Long; a left shift that is the whole expression (whole or Quick operand) wraps, one in a sum saturates | round 3-5, Q13, N1 "16.16, außerhalb 48.16", Q12 E "Shift durch + -", Q12 J "LSL unter Quick umbrechen", I5 A, I6 B, I9 A | "A shift at the top under a Quick destination", "A left shift at the top under a Quick destination wraps" |
| the destination reaches in only through the named exceptions (a closed list) | conflict item 29 "Richtung + Ausnahmeliste" | "Only the named exceptions depend on the destination" |
| each exception place warns, one name per exception; `longwrap`, `datafrac`; editor at most 8 per name, console all | I8, I9 (warnings), "warnings: scope, names, volume" | compiler/warnings delta |
| no destination: Quick / Long / Float | round 2 (counter-proposal), round 3 "Ja, so" | "Without a destination ..." |
| comparisons on the full value; result a Word | round 3, Q5 "Word, wie Original" | "Comparisons compare the full value" |
| folding = variable form | direction; "Constant shift beside a Quick ..." | "A constant expression gives the value of its variable form" |
| AB3 the same | round 3 "Ja, dieselbe Regel" | "Both compiler modes follow the rework's rules" |

The coordinator's equivalence note (round 1) holds and is used: with the
cap at Long, the whole `+ - *` give the exact value modulo 2^32, so every
whole `+ - *` can be computed as a Long operation with wrap; `/`, `Mod`,
comparisons and the shifts need the true width (right shifts in the
operand's own width, left shifts one step wider: "shift widths").

### 2. Implementation shape

- **One rule table, three users.** The typing pass (`exprtyping.c`) gives
  each node its class (whole width, 48.16, Float) from its operands only; a
  node gets the wanted type only at the root (conversion) and for the named
  exceptions (the whole-division exception, the shift exception with J, the
  top operation under a Float); where an exception changes a node's type,
  the pass raises its warning (`ext_warning`, compiler/warnings delta;
  editor at most 8 per name, console all). `const_flow`, `const_fetch`'s "type so far" cases, `has_long`,
  `frac_side`, the wanted-type parameters of `typed()` and the A1/E
  special cases go. The fold (`exprfold.c`, `exprarith.c`) computes a
  constant subtree with the same table (whole as int32 wrap, 48.16 as
  int64 with the cut of Q1, Float as today's FFP fold), so folding is the
  variable form by construction (closes #1225, #1229, #1230, #1233, the
  constant shift). `p5consteq`/`p5constapply` compare the value, not only
  the type (shift-print/report.md, point 1).
- **Code.** Whole operations: the narrow 68000 code where "Optimizations"
  below allows it, else `ext` + Long code. 48.16: the existing pair
  representation (low 16.16 in one register, the upper 32 bits in the
  other, the user's remark of 2026-11-24 on the C7 format), the existing
  compilerlib routines (`q48add` .. `q48trunc`), new: a saturating 48.16 ->
  Quick conversion, a 48.16 -> no-destination conversion (Quick / Long /
  Float at run time); a Float -> whole conversion stays the library's
  `SPFix` towards zero (B) except beyond the Long range, where the value
  cut towards zero wraps (I8). 16.16 code stays only where it is
  value-identical (below).
- **Print and untyped parameters** take the converted value in the form
  the run-time test chose (a Quick, a Long or a Float), by the existing
  print routines of each type.

### 3. Optimizations that keep the value

A narrow form is used only when it gives the rules' value for every operand
value (spec "Optimizations keep the value"):

- **Narrow whole code.** `+`, `-`, `*`, `NOT`, `AND`, `OR`, `EOR`, `LSL`/
  `ASL` modulo 2^n give the low n bits of the exact value. So under a
  wrapping destination of n bits (Byte 8, Word 16) a chain of these
  operators on operands of at most n bits runs in n-bit code:
  `x.b=b1+b2+b3` in Byte code (44 for 100+100+100). Not for `/`, `Mod`,
  `LSR`, `ASR`, comparisons, a Quick/Float destination or no destination;
  there a range bound may still allow it (`NPrint b1+b2+b3`: at most 381,
  Word code; `w*v` one `MULS.W` gives the Long).
- **Reordering whole and Quick terms.** For a sum of whole terms W and Quick
  terms Q under a whole destination, `floor(W+Q) = W + floor(Q)` because W
  is whole, and the wrap is modulo 2^n: `b1+q1+q2+b2+b3` under a Byte is
  `b1+b2+b3 + floor(q1+q2)` in Byte code, and `q1+q2` may even run in 16.16
  (its integer part modulo 2^16 is enough for n <= 16). Conditions: only
  `+` and `-` between the groups; whole destination with wrap of n <= 16
  bits for the 16.16 Quick part (n = 32 needs the 48.16 Quick part); not
  under a Quick destination (saturation is not modular), not without a
  destination, not across `*`, `/`, `Mod` or a comparison.
- **16.16 with an overflow test under a Quick destination.** One `+`/`-`
  of two Quicks whose result is converted to a Quick: the 16.16 add with a
  `BVS` to the saturation gives the 48.16-then-saturate value (+6 cycles).
  A chain needs the exact intermediates (`q*q-q*q` may come back into
  range), unless every intermediate is shown in range; the user's 64-bit
  pair with the fast path "upper part = sign extension" applies there.
- **Whole-valued Quick at run time** (the 4.3 fast paths: `q48divw`,
  `qqmult` with a whole factor, a whole Quick divisor): kept where they
  return the exact rule value; a fast path that gives a less exact value
  than the rule is changed or removed (Q20 "Immer exakt").

### 4. Plan and Monitor choices (the planning agent's, for the coordinator-decisions file)

- **P1 (Plan):** "untyped parameters" are the places where the pass has no
  wanted type today other than the comparison places: `Print`, `NPrint`,
  `Str$` and library parameters typed "any"; `If`, `While`, `Until`,
  `Select`/`Case` and comparison operands compare the full value. Reason:
  the counter-proposal names "bei unbekanntem Datentypen", round 3 names
  comparisons and If; the code's no-type places are exactly these.
  Point: the list of no-type places — L1 Unlikely / I3 Observable / E2
  Reasoned → Plan.
- **P2 (Plan):** saturation gives $7FFFFFFF above and $80000000 below the
  Quick range (the largest and smallest 16.16 values). Point: saturation
  bounds — L1 Unlikely / I3 Observable / E2 Reasoned → Plan.
- **P3 (Plan):** the old tests of the replaced rules are rewritten to the
  new values in the commit of the rule they test, with the before value in
  a comment (expr-top-div-trunc, expr-q48-cut-down, expr-q48-paths,
  compiler-quick-long-arith, compiler-operand-order, compiler-pow-negative-
  base, expr-constmod-right/-larger/-exact, compiler-array-later-index's
  shift case, the `Mod` tests of #1197/#1198, and the matrix by
  regeneration). Point: test rewrite instead of new
  tests beside them — L2 Possible / I2 Medium / E2 Reasoned → Plan.
- **P4 (Monitor):** the measurements below come from a one-off evaluator of
  the new rules (D-002 addition: scratch analysis), not from the model; task
  1.3 repeats them with the model's new rule set. Point: evaluator
  differences to the model — L2 Possible / I1 Low / E3 Proven (spot-checked
  on the emulator, table below) → Monitor.
- **P5 (Monitor):** requirements of `compiler/expression-types` whose values
  do not change stay untouched (the exact Long/Quick compare, booleans in
  both modes, AB3 power, the index low word and index code size,
  comparisons as right operands, NOT/AND under a
  Quick, negative constants under a Quick); `testing/expression-matrix` has
  no requirement change (its rule "the expected value comes from a recorded
  rule" is met by the new rule set). Point: spec scope — L2 Possible / I1
  Low / E2 Reasoned → Monitor.
- **P7 (Plan):** the folding commit (task 3.4) fixes #1225, #1229, #1230,
  #1233 and #1236 (the constant shift beside a Quick) together (one `Fixes` line and one
  `fixed = N` test each): one rule table makes all of them the variable
  form at once; round 4 folded the series ("one commit per issue") into
  the change. Point: one commit for five bug-port issues — L2 Possible /
  I2 Medium / E2 Reasoned → Plan.
- **P6 (Monitor):** the precedence of `MOD` (above `*`/`/`) and of `NOT`
  (the rest of the expression) is unchanged; the measurements use it.

## Measurements

Method: a one-off evaluator of the decided rules (all answers of
2026-11-27) over (a) the typed CD expression dump of the expr-tree-5
analyses (27,899 expressions of 724 sources, 60 value sets each, 20 of
them non-negative; `cd.values` of f3-narrowing with the model's `P+` =
the current port and `orig` = 1.95), (b) the committed matrix sources,
(c) the hand-written tests. Data and scripts:
`~/.local/share/bb2-c-work/type-system-rework/` (`scripts/tsr2.py`,
`cdrun2.py`, `classes.py`; `data/cd2.pkl`, `data/q12-places.tsv`).
Before-values confirmed on the emulator (port at 82ee59f73 with the C
libraries, 1.95 with the CD's; `data/emu-values.txt`):

| # | Line (values) | Port now | 1.95 | New (before the Q12 classes) |
|---|---|---|---|---|
| P01 | `x.q=1/16` | .0625 | .0625 | .0625 (top `/`) |
| P02 | `x.q=w/9` (w.w=200; TRIPLANES.BB2) | 22.2222 | 22.2222 | 22.2222 |
| P03 | `x.q=(2*f)*(4/200)` (f.f=108; LINE_TORUS2.BB2:33) | 4.32 | 4.32 | 0 (class C) |
| P04 | `x.q=q-(w/2)` (10.5, 7; ISOBLASTER.BB:80) | 7 | 7 | 7.5 (class C) |
| P05 | `x.w=(q+(w/2))-2` (149.75, 135; INTUITOOLS2.BB:69) | 215 | 215 | 214 (class C) |
| P06 | `x.f=q/100` (20.875) | .20875 | .20875 | .2087402 (class D) |
| P07 | `x.f=(q/360)*(2*f)` (194, 85) | 91.6111 | 91.6111 | 91.6095 |
| P08 | `x.l=q ASL 4` (139.375) | 2224 | 2224 | 2230 |
| P09 | `x.w=q/2` (-139.125; PLAYBACK.BB2:13) | -69 | -70 | -70 |
| P10 | `x.b=b*0.8` (b.b=-29; EXPLORER1.4.BB:1540) | -23 | 0 | -24 (class B) |
| P11 | `x.l=-1.5` | -1 | -1 | -2 (class B) |
| P12 | `x.l=f` (f.f=-2.5) | -2 | -2 | -3 (class B) |
| P13 | `x.q=(q+w)-((l LSR 3)-m)` (QUIKASM.BB2:3063) | 182.875 | 182.875 | 183 (class E) |
| P14 | `x.q=-1/w` (w.w=7; AGA_FADE.BB2:47) | -.1429 | -.1429 | -.1429 ($FFFFDB6D, cut down) |
| P15 | `NPrint 100000+q` (1.5) | 100001 | 100001 | 100001.5 |
| P16 | `NPrint w LSR q` (3, 2.5) | .75 | .75 | 0 |
| P17 | `x.l=l*0.1` (100) | 10 | 10 | 10 |
| P18 | `x.q=b+100000` (3) | $86A30000 | $86A30000 | $7FFFFFFF |
| P19 | `If 1.5/l` (100000) | true | true | false |
| P20 | `x.l=(l/3>3.2)` (10) | -1 | 0 | 0 |
| P21 | `x.q=100000 LSR 2` | 25000 | 25000 | 25000 |
| P22 | `x.q=q+q` (20000) | $9C400000 | $9C400000 | $7FFFFFFF |
| P23 | `NPrint w+w` (20000) | -25536 | -25536 | 40000 |
| P24 | `x.l=q LSR 2` (-8) | 1073741822 | 1073741822 | 16382 |
| P25 | `x.q=f/10` (f.f=-144) | $FFF1999A | $FFF1999A | $FFF19999 |
| P26 | `x.l=q/2*2` (7.5) | 6 | 6 | 7 |
| P27 | `x.l=i/2*2` (i.l=7) | 6 | 6 | 6 |

The model's `P+` and `orig` values agreed with the emulator on every line.
The column "New" holds the values of the rules before the Q12 classes and
the answers I1-I9; P03-P06 and P10-P12 change with them (classes C, B, D;
P12 is -2 by B). The final values are those of the spec scenarios; task
1.3 recomputes this table with the model.

Final numbers, with every answer including the adopted Q12 classes and
N1, N2 (`scripts/tsr3.py`, `cdrun3.py`; `data/cd3.pkl`,
`data/final-away.tsv`, `data/final-classes.txt`):

**CD against the port today.** 1,668,952 (expression, value set) pairs
evaluated; 21,070 change (1.3 %) at 468 places, 205 places at values that
do not overflow, 83 at non-negative values.

**CD against 1.95** (the user's measure for Q12, "gemessen an den uns
vorliegenden Beispiel-Programmen"): 26,052 pairs differ from 1.95. 188
places move away from 1.95 where the port agrees with it today (74 at
non-negative values, in 45 files; before the Q12 classes: 337 and 169);
149 of the 337 come back. 102 places differ from 1.95 today and still do.
The 188 by class (section "Q12"): G (the 48.16 cut, one 1/65536 step) 80
fully and 55 partly (40 at non-negative values), F (not adopted) 17 (none
at non-negative values), C1/C2 partly 11 (8; their quotient's last step,
class G), H 7 (3), J 4 (4; see I6), B partly 5 (3), K 2 (0), I 1 (0), C
partly 1 (0), other 5 (3).

**With I4 B** (the C1 reach through `/` and unary minus;
`i4-option-b-impact.md`): 26 more pairs evaluable (an inner whole quotient
no longer gives a `/0`), 147 fewer pairs differ from the port and from 1.95
(20,923 and 25,905), at three places under a Float destination
(FILEREQUESTER.BB2:11, AGAFADESDEMO.BB2:356 and :380); none moves away.
The matrix and the hand-written checks do not change. The place counts
above are recounted by task 1.3. Emulator: `(7/2)/2` 1.75, `7/(1/2)` 14,
`-(7/2)` -3.5 under Quick and Float, port and 1.95 alike.

**Matrix.** 30,591 cells of the committed sources evaluated (7,130 string
or index cells and cells without a scalar expectation skipped); 2,233
change. By operator: `Mod` 536 (floor Mod), `^` 452 (the evaluator's exact
power differs from `SPPow` in the last bit: many of these are the
evaluator's, to be recounted with the model in task 1.3), `+` 196, `LSR`
192, `-` 180, `*` 160, `/` 106, `LSL`/`ASL` 156, bit operators 224, `ASR`
42, unary 21; 32 newly refused (a shift count with a point). List:
`data/matrix-changed3.tsv`. The named cells and classes are regenerated by
the model's new rule set (task 6.1).

**Recounted with the model (task 1.3).** `exprmatrix candidates` with the
column `TSR` (`EM_RULES_TSR`, model.c "the rules of type-system-rework",
written from the spec delta), against the model's `P+` and `orig` columns,
with the comparison tolerances of `cdrun3.py`:

* *The table above:* every line P01-P27 by the rules, the spec's values where it names the line (P03 4.3176
  and P07 91.6095: the C1 quotient and the Quick subtree below the top are
  cut down to 1/65536, class G; P10 -23 by B; P12 -2; P13 182.875 by E).
* *CD* (the same input as above: `cdall.exprs.tsv` of div-const-rounding,
  which agrees with `expr-tree-5-rules/data/cd.cases`; the 3.7 copy of
  `cd.cases` has other address constants): 1,668,858 pairs evaluated
  (5,080 not known to the model: a `^` whose result the model cannot give,
  2 a division trap). 20,558 pairs change against the port (above, with
  I4 B: 20,923), 25,554 differ from 1.95 (25,905). The difference is four
  causes, each a change of the evaluator or of a later answer, none of
  the rules: J -390 pairs (the planner's set before I6 B saturated a Quick
  left shift; I6 B wraps it), I4 B -147 (as counted above), `^` beyond
  FFP's range -155 (the evaluator wrapped `2^71`, the model does not know
  `SPPow`'s overflow), and two the evaluator's tolerances hid: a Float
  division by 0 under a Float destination +118 pairs at 39 places (value
  sets with a divisor 0: the model gave the saturated infinity
  $FFFFFF7F/$FFFFFFFF and 0 for the port and 1.95, which both raise the
  division trap there; see task 1.5 below) and a 48.16 quotient printed one
  step lower +62 pairs (class G, -2.8323 for 1.95's -2.8322, within the
  evaluator's print tolerance). Places changed against the port: 501
  (above: 468), 240 at values that do not overflow (205), 84 at
  non-negative values (83); without the 39 places of the Float division by
  0: 465 and 204.
* *Places that move away from 1.95 where the port agrees today:* 226 (78
  at non-negative values, 50 files; above: 188, 74, 45). By class, 184 of
  them are the evaluator's places with the same class: G 80, part G 55, F
  17, part C1+C2 11, H 7, other 5, K 2, part B 2, part B+G 2, part B+C2 1,
  part C1+C2+C3 1, I 1. The class J (4) comes back to 1.95 (I6 B). 42 are
  new: the 38 places of a Float division by 0 under a Float destination
  (crashes in both compilers, see task 1.5 below) and 4 places of class G
  under Print (ABALL2.BB2:137, BSPREAD.BB2:27, :434, :536). Places that
  differ from 1.95 today and still do: 109 by the model (no place moves
  away, a non-overflowing pair differs from 1.95 under both the port and
  the rules); the evaluator counted 102.
* *Matrix* (`tests/corpus/tests-exprmatrix/matrix.txt`, every cell without
  a string; the model's `P+` against `TSR`): by operator `Mod` 536, `+`
  196, `LSR` 192 (8 of them newly refused), `-` 180, `*` 160, `/` 116, `LSL`
  and `ASL` 156 (16 newly refused, 28 index cells), bit operators 224,
  `ASR` 42 (8 newly refused), unary 21: the evaluator's counts except `/`
  (+10: `1.5/100000` prints 0 and `1000/1.5` 666.6667, within the
  evaluator's print tolerance, and two `If` cells it skipped). `^`: 58 cells
  change, 871 are not known to the model (a power converted to a whole
  number or a Quick is known only far from a step). Not counted above: the
  Float-destination cells of the bit operators and shifts that P+ refuses
  and the rules compile (1,696 cells: a whole operation converted to
  Float), and 156 more index cells (`LSR`, `Mod`, `^`, the later index).

**Recounted after the answers D1-D7 and P1 (task 1.5).** The model's `TSR`
set follows the answers (section "Answers to the readings of the model
(task 1.1)"); its `P+` and `orig` columns give the emulator's values of
section1-decisions.md (#1232): a Float division by 0 raises the division
trap in both compilers (the model gave 0), and a point constant read as a
Quick is the FFP value of its text cut down in both (`x.q=q+30000.3`
$75314D00; P+ gave $75314CCC). A Long division by 0 of a negative dividend
stays as it was: +2147483647 by the CD's library (`orig`; the model gives
the same for the port with the CD's libraries, the planner's probes ran
mostly as port/cd), -2147483647 by the port's C library (`P+`; #200, as
`exprarith` and its host test `blitz2_exprarith`, which fails if P+ gives
+2147483647). The committed matrix tests do not change. Same input, tolerances and place counts as task 1.3; a pair
where one side raises the division trap and the other has a value is
counted apart from "away from 1.95" (D1/D7: a crash that becomes a value,
or a value that becomes the trap):

| Measure | Task 1.3 | Task 1.5 |
|---|---|---|
| Pairs evaluated (5,080 `^` not known to the model) | 1,668,858 (2 traps left out) | 1,668,860 |
| ... the rules raise the division trap | 2 | 272 (149 where 1.95 and the port trap too, 123 where both have a value) |
| Pairs changed against the port | 20,558 | 20,512 (123 a value that becomes the trap) |
| Pairs differing from 1.95 | 25,554 | 25,472 (123 a value that becomes the trap, none a crash that becomes a value) |
| Places changed against the port | 501 | 459 (9 only by a value that becomes the trap) |
| ... at values that do not overflow | 240 | 204 |
| ... at non-negative values | 84 | 77 |
| Places moving away from 1.95 | 226 / 78 / 50 files | 186 / 68 / 44 files |
| ... a 1.95 value that becomes the trap, at a value that does not overflow | (not counted) | 1 (GORNY3DLIB.BB2:1670, a Long `/` by 0) |
| Places differing from 1.95 today and still | 109 | 109 |

* *By cause.* The 149 pairs at 54 places with a Float division by 0
  (under a Float destination the 118 pairs at 39 places of task 1.3, the
  rest under other destinations and Print) are a crash in 1.95, the port
  and the rules: they leave every count. The 123 pairs at 65 places that
  become the trap are divisions by 0 of whole, Quick and 48.16 values (64
  places) and BIO.BB2:43, D7's Quick-class Mod; all but one pair
  (GORNY3DLIB.BB2:1670) are overflowing pairs (the largest value of a
  division by 0), which the place counts at values that do not overflow
  leave out.
  D2, D3, D4, D6 and P1 change no pair of the CD's value sets
  (ANDREWSDEMOS/SCRATCH.BB:26 is D3's one place, `v1>-(0.1)`, and gives the
  same truth value), nor any cell of the matrix.
* *Away from 1.95, by class:* G 80, part G 55, F 17, part C1+C2 11, H 7,
  other 5, G under Print 4 (the four places above), K 2, part B+G 2, part
  B 1, part C1+C2+C3 1, I 1: class G 139 places, 4 of them under Print, as
  Monitored. Against the evaluator's 188: J 4 back (I6 B), G under Print 4
  more, and 2 of the evaluator's places (FILEREQUESTER.BB2:11 "part
  B+C2", 3DLIB.BB2:1653 "part B") differ only at a Float division by 0,
  a crash on every side. No cause differs from its expectation by more
  than 1 %.

**Nesting depth under the new rule (task 1.4).** The method of
expr-tree-5 task 3.7 (`~/.local/share/bb2-c-work/expr-tree-5-3.7/`: the
port's dump of the CD with the register of every operator, 171,426 E lines,
`depth.py`), with the typing of the model's `TSR` set (`exprmatrix
candidates --typing`, its rule set recording the class of each operation;
a scratch build) in place of P+'s, and every operation of Quick class (a
48.16 value: a Quick operand, C1, C3, the shift exception) a register pair;
whole and Float operations and comparisons with a Float one register.
27,898 E lines typed (1 not matched, 1,322 strings and other elements taken
as today, as in 3.7). 6,518 E lines have an operation of Quick class (P+:
545), at 2,318 places in 203 files, 10,192 such operations.

| | today | pairs, leaves in one register (A) | every operand of a pair operation widened in place (B, bound) |
|---|---|---|---|
| most registers one E line uses | 6 (2 E lines) | 6 (9 E lines) | 8 (37 E lines) |
| E lines that reach D7 (D6 saved) | 22 | 22 (the same) | 140 |
| E lines beyond D7 ("Expression too Complex") | 0 | 0 | 13, at 7 places |

A raises the highest register of 1,527 E lines (1,510 by one, 17 by two);
the 22 E lines at D7 are today's (operators in a 7th or later argument,
none of Quick class). Nested expressions (an element inside a pending pair
operation, the E lines of one source line and pass with a higher base
moved up by the largest leaf move at their base, recursively): A still
reaches D7 with the same 22 E lines and none beyond; B 159 at D7, the
same 13 beyond. So no CD source reaches "Expression too Complex" when the
emitter keeps a leaf in one register until its operation (A, the
compile-time fast path of 3.7). With every operand widened in place (B),
STARTREK.BB2:884, HYPERHELP2.BB2:103, AGADUAL_PF.BB2:92, BEZIER1.BB2:47
and :69, TEXTVIEW.BB2:40 and INTUITOOLS.BB:274 would need registers
beyond D7 (expressions with a Quick operation that start at D3 to D6).

**Hand-written tests.** 593 checks of `tests/blitz2`, `tests/bug-orig/
compiler` and `tests/bug-port` evaluated by the scan (Print-output tests
and checks inside loops not evaluated); 163 change in 12 files (with the
final rules; compiler-pow-negative-base no longer changes):
expr-q48-paths (85), expr-top-div-trunc (37, all A1 and #1222 checks),
expr-q48-cut-down (8), compiler-quick-long-arith (6), expr-constmod-right
(3), expr-constmod-larger (2), expr-constmod-exact (1),
compiler-operand-order (`l1/3>3.2`), compiler-pow-negative-base (`vf^nw`
-16), expr-leaves, compiler-byte-const-compare, compiler-operand-beyond-d7
and compiler-array-later-index (the last four to be checked by hand).
Plus every scenario the spec delta marks "before:".

**Cost** (68000 cycles; budgets of `tests/perf/budgets.txt`, estimates
marked ~):

| Path | Today | New | Note |
|---|---|---|---|
| `q3=q1+q2` | ~45 (inline 16.16) | ~51 | 16.16 add + `BVS` saturation (one operation) |
| `x=l+q+p` under a Quick | 96 (bq48.xladdqp) | ~125 | + saturation test |
| `q3=q1*q2` | 394 (bmath.qmul) | ~540 | qqmult 512 + saturation; the largest cost |
| `q3=q1/q2` | 1988 (bmath.qdiv) | ~1830 | q48div 1795 + saturation: faster |
| `c=q1*q2` under a Long | 512 (bmath.qqmul) | 512 | unchanged |
| `w3=w1*w2` under a Word | 89 (bmath.wmul) | 89 | narrow code allowed |
| `NPrint w+v` | Word add | +8 | `ext.l` x2, `add.l` |
| Print of a 48.16 value | FIT | +~25, Float path +~1900 | run-time test; Float only beyond the Quick range |
| `If q1+q2>q3` | 16.16 compare | ~+100 | 48.16 compare (bq48.ifladdq 222) unless shown in range |
| `c=f1` under a Long | 230 (bmath.f2l) | ~260 | rounding down: a test of the sign and the fraction |
| `f3=q1/100` | Float path | ~500 (q48div by a whole constant, then q2f) | 

The budgets that rise are named in the tasks (bmath.qmul, bmath.f2l, the
bq48 Quick-destination lines); D-007 asks for the user's view of the hot
path; Q16 "Annehmen, optimieren (Recommended)" accepts it.

## Conflict list (CLAUDE.md rule 2: named, not resolved)

Recorded decisions (rounds/tree/decisions.md unless named) that the rules
replace or contradict, with the quote:

1. "2026-11-24 C7: the wanted type sets the intermediate format, a
   candidate for Q4" — "Was wäre mit der Idee, dass der Ziel-Typ Einfluss
   auf den Typen des Zwischenergebnisses hat?" and "2026-11-24 expr-tree-5
   Q4: direction C7" — "C7-Familie (Recommended)": replaced by the
   direction ("Der Ziel-Typ sollte keinen Einfluss auf die Berechnung des
   Ausdrucks haben").
2. "2026-11-24 C7K-long as working assumption; goals and fixed points" —
   "Im Grunde möchte ich so nahe am Original bleiben wie möglich ...": the
   rework moves 337 CD places away from 1.95 (Q12, its classes below); the fixed points
   "Symmetrie" and "Quick als Ganzzahlig (Bit-Operatoren, On/Off)" hold,
   "Genauigkeit von Quick-Operationen" improves except before a Float (Q10).
   The quote "`3.0` ist **nie** ein integraler Typ" holds (#1231).
3. "expr-tree-5 Q4 (P+ round 1)" F1 — "Ja, außer unter Long-Ziel
   (Recommended)": replaced (`q+w/2` 1.5, `l1/3>3.2` false).
4. Same entry, F3 — "Option 1: \"Ja, sofort ganzzahlig\" ..." and "P+ F3
   narrowed (N1)" — "N1 (Recommended)": N1's fraction kept with a Quick
   operand holds; "a `/` at the top still cuts towards zero (A1 ...
   stands)" is replaced by rounding down (`r.l=q/2` -2).
5. Same entry, F5 — "16.16 wie Original (Recommended)": replaced, every
   Quick operation is 48.16 (`NPrint i*300` 90000, `NPrint q+20000` 40000).
6. "expr-tree-5 Q4 (P+ round 2)" F2 — "Float wie Original, sonst Quick
   (Recommended)" and "P+ part 3 under Byte/Word (A1) and F2 ..." (b) —
   "Option 1, accept (Rec.)": replaced by round 3's point constants (Quick
   by default; `x.w=-1.5+z` -2).
7. Same entry, F7 — "Lassen wie entschieden (Recommended)" (a Quick
   rounded down before a shift): replaced by round 2 "Bleibt Quick, 16.16"
   (`r.l=q LSL 2` 6; the 4 ISOBLASTER places change).
8. Same entry, F4 — "Heutige Regel, exakt (Recommended)", and rounds/quick
   "2026-11-22 Matrix questions Q-A, Q-C, Q-D, Q-E", Q-D — "Wert des
   Originals (Recommended)": replaced by "type-system-rework Q1, Q2, Q4,
   Q7" Q2 "Floor (Recommended)" (floor Mod; #1197/#1198 close with the
   rework).
9. rounds/quick "2026-11-21 #1178 rounding rule decided" — "Zwischenlösung:
   / gegen Null (Recommended)": the `/` towards zero of a Quick quotient is
   replaced by rounding down at the conversion (`r.l=l/n` with `n=-1.5`
   -7); the floor of `+ - *` and negation holds.
10. "2026-11-26 Quick rounding: down stays; the 48.16 cut in the final
    direction (E)" — Q2 "E: as the final rounding (Recommended)": the cut by
    the destination contradicts the direction; replaced by Q1 "Immer abrunden (Recommended)"; Q4 "Keep, fix wording
    (Recommended)" (FIT rounded down) is replaced by the no-destination rule.
11. "2026-11-26 Division under Byte/Word; ..." (a) and "P+ part 3 under
    Byte/Word (A1) ..." (a) — "A1, deeper / named (Rec.)": replaced.
12. "2026-11-25 expr-tree-5: division by 0 per format ..." — "Nur 48.16,
    16.16 wie heute (Recommended)" and "Größter Wert des Ziels
    (Recommended)": there is no 16.16 division any more; the largest value
    of the destination is a conversion rule now (Q4 "±unendlich, gesättigt (Recommended)"); the
    run-time values of Q4 are replaced in turn by D1/D7 (item 30).
13. "2026-11-26 #1222 overflow on the longdiv path; constant Mod exact" —
    "Accept (Recommended)" (the low bits of an overflowing quotient):
    replaced by saturation under a Quick, exact otherwise.
14. "2026-11-26 Optimisations may lose accuracy within limits" (this case
    only) — "Wenn sich durch die Optimierung die Genauigkeit
    verschlechtert, dann ist es kein Problem ...": a less exact fast path
    contradicts "Das Ergebnis des Ausdrucks muss identisch sein"; Q20 "Immer exakt (Recommended)" settles it.
15. "2026-11-27 Shift under Print; the matrix model's port-side errors" —
    "Eigenes Issue (Recommended)" (the model fixed before it serves again):
    Q11 "P+ bis Abschluss (Recommended)": a new rule set as the oracle, the P+ sets removed at the end.
16. "2026-11-27 Constant shift beside a Quick; the series of constant-typing
    fixes" — "Eine Serie, #1231 mit (Recommended)": replaced by round 4
    "Planer übernimmt (Recommended)" (the series folded into the change).
17. "2026-11-26 Division under Byte/Word; ..." (b), the proposed "-1 ist die
    richtigere Antwort": lapsed by round 1 (recorded there).
18. "2026-11-25 rb.b=1.5 LSL 2 stays 6": holds; `r.l=1.5 LSL 2` follows it
    now (6, before 4).
19. D-003 — "funktionale Kompatibilität ohne sichtbare Seiteneffekte
    (Bug-Fixes ausgenommen)": the rework changes visible results that are
    not original bugs (Q9, Q12): a conflict to be weighed by the user.
20. D-007 — "Also alles, was in einer \"hot loop\" ist, sollte ganz genau
    geprüft werden": Quick arithmetic under a Quick destination gets slower;
    accepted by Q16 "Annehmen, optimieren (Recommended)".
21. "2026-11-27 type-system-rework: shift widths" — "LSR/ASR eigen, LSL/ASL
    breiter (Recommended)": its left-shift part replaced by Q12 class A "LSL
    immer Long" (`1 LSL 17` 131072).
22. Q9 "Ausnahme für `/` - aber wie bei der anderen Ausnahme nur auf der
    obersten Ebene; das sollte ausreichend sein.": its reach replaced by
    Q12 class C1 "Ja, für alle (Recommended)" (through `+ - *`).
23. Round 1 "Immer abrunden (Recommended)" ("for Quick and Float alike")
    and Q7 "Exakt abrunden, umbrechen (Recommended)": for a Float replaced
    by Q12 class B "Float auf null hin" (towards zero as SPFix; a Quick
    keeps rounding down). For a Float beyond the Long range settled by I8
    A+C (cut towards zero, then wrap, as Q7's wrap; a warning when known
    at compile time).
24. Round 1 "Auf null hin (Recommended)" (a whole `/` is whole): replaced
    beside a Quick by Q12 class C3 "`/` neben Quick".
25. Q10 "Annehmen (Recommended)" (a Quick subtree in Quick precision until
    it meets a Float): replaced for the top operation under a Float
    destination by Q12 class D "Float reicht oben hinein".
26. Round 4 "Nur der Shift ganz oben (Recommended)" (`x.q=qa+3 LSR 3` is
    1.25): replaced by Q12 class E "Shift durch + -" (1.625; without a
    destination still 1.25).
27. Round 5 (a left shift in the exception "saturated at the conversion"):
    replaced by Q12 class J "LSL unter Quick umbrechen".
28. Q13 "In 48.16 schieben" against Q15 for a negative right shift in the
    exception: settled by N1 "16.16, außerhalb 48.16 (Recommended)".
29. The direction ("Der Ziel-Typ sollte keinen Einfluss auf die Berechnung
    des Ausdrucks haben - außer am Ende bei der Konvertierung"): the named
    exceptions (shift through `+ -`, whole `/` through `+ - *`, the top
    operation under a Float) make the destination reach into the
    expression by the user's choice. Settled by "type-system-rework
    conflict item 29 (destination influence)", "Richtung + Ausnahmeliste
    (Recommended)": the direction stays the rule; C1, D, E and J are the
    closed list of exceptions to it, each warns where it changes the type;
    a further destination-dependent exception needs a decision of its own.
30. Q4 "±unendlich, gesättigt (Recommended)" (shown as "= bisherige Werte",
    which a Float division did not have: both compilers trap) and
    "2026-11-25 ... division by 0 per format ..." "Wie heute je Typ
    (Recommended)" (Long `/` $7FFFFFFF, Long/Float Mod the dividend; "a
    change would be its own issue"): replaced for this change by "2026-11-28
    type-system-rework D1/D7" "Trap für alle Typen (Recommended)", the
    user's counter-question "Ist der Absturz ein TRAP? ... Und vielleicht
    auch Quick entsprechend umstellen?" answered first.

Spec requirements of `compiler/expression-types` replaced or changed: the
twelve REMOVED and six MODIFIED requirements of the delta, each with its
reason; values change in the scenarios marked "before:".

Doc statements: `docs/blitz2/expression-types.md` sections "The port: a
Quick keeps its fraction (#1178)" (its table: "`/` ... towards zero";
"`^` ... towards zero (`SPFix`)"; "shifts ... the Quick rounded down
first"), "The operand typing (rule P+, #1216)", "A constant with a fraction
under a Byte type so far (#1188)", "Converting to a whole number: Float and
Quick differ", "A comparison starts again"; `expression-type-tables.md`
(all tables); `docs/blitz2/deviations.md` rows of #1178, #1186, #1188,
#1216, #1221, #1222-#1228; site `language/types.md` "A Float becomes a
whole number by cutting off the fraction (-1.5 gives -1)" and "A number
with a fraction or an exponent is a Float constant, unless the expression
it stands in is computed in Quick"; site `language/expressions.md` "The
type of an expression" points 1-4 ("A Quick with a Long, or where a whole
number is wanted, is computed exactly ..."; "its result, a truth value,
takes the type of the context") and the paragraph "Two parts of the rule
need care"; the site pages `compared-with-original/expressions/type-system`
and `how-blitz2-works/compiling`. A contradiction already there, outside
this change's rules: site `language/expressions.md` says "Of a Quick or a
Float the remainder is a whole number: `7.5 MOD 2` is 1", the main spec
("Mod keeps today's sign rule, exact with fractions") gives `x.q=q Mod 2`
1.5; with Q2 the site sentence is rewritten (exact, 1.5; task 7.3).

Tests: the lists in "Measurements"; `tests/perf/bench/bq48.bas` and
`bmath.bas` budgets; the named-cells file and the `compiler-expr-1216-*`
tests (their deviations are 1.95's values, which the regenerated tests
take from the new classes).

## Answers to the planning questions

All twenty questions are answered (rounds/tree/decisions.md, 2026-11-27,
the five "type-system-rework" entries; quoted there). Where the answer is
not the recommendation: Q9 (a top-level exception for a whole `/` under a
Quick or Float destination), Q12 (proposals by class, below), Q2 (floor
Mod), Q5 (a Word), Q14 with its follow-up ("shift widths": right shifts
in the operand's own width, left shifts one step wider), Q11 (the P+ rule
sets stay until the rework ends, then they are removed). The spec delta
carries every answer; no working assumption is left in it.

## Q12: the classes of places that move away from 1.95

Measured with the decided rules (Q9's exception included) on the CD dump:
337 places move away from 1.95 where the port agrees with it today (169 at
non-negative values). Each class is one cause and shape; each proposal is
a candidate rule measured alone ("restores": places whose every changed
value set comes back to 1.95; "part": some value sets; "breaks": places
that agree with 1.95 now and would not). Answered in "2026-11-27
type-system-rework Q12 classes": A, B, C1 (for every operand combination),
C3, D, E and J adopted, each a named exception in the spec delta that
replaces the answer named in its row (conflict list 21-27); F not adopted
("Eigene Breite"); G, H, I, K not proposed. The table keeps the
measurements of each proposal alone; the result of all of them together
is in "Measurements".

| Class | Shape and examples | Proposal (kind) | Restores (non-negative) / part | Breaks | Conflicts with |
|---|---|---|---|---|---|
| A | a small whole constant shifted left wraps one step wider: `#x=1 LSL 17` 0 (INTBITS.BB2:26, ADKBITS.BB2:22, DOSHUNKS.BB2:55-57, the AREXX examples, REQTOOLSEZDEMO.BB:46-52, STARFIELD2.BB2:302-303 `v*(1 LSL n)`) | A: a left shift of a whole operand gives a Long (width rule, not a top-level exception) | 25 (25) / 25 | 1 (GREP.BB:105, a count beyond 31) | "shift widths" ("LSL/ASL breiter") |
| B | a negative Float converted to a whole destination: `x.w=20+f`, `x.l=f*l` (GORNY3DLIB.BB2:1088-1729, LHAFRONT.BB2:72, CHECKBOARD.BB2:33) | B: a Float converted to Byte/Word/Long is cut towards zero, as the original's `SPFix` (a conversion rule) | 79 (21) / 87 | 0 | round 1 "Immer abrunden (Recommended)" (Quick and Float alike), Q7 |
| C | a whole `/` below the top under a Quick or Float destination: `x.q=15-(v/16)` (SQUIRT1-3_TO.BB2), `x.q=1-(v/15)` (PALETTE.BB2:21-23), `x.q=v1-(v2/2)` (DUNGEON.BB:110-111), `x.q=(2*f)*(4/200)` (LINE_TORUS1-6), AGAFADESDEMO.BB2:356/380, LHAFRONT.BB2:70 | C1: Q9's exception reaches from the top through `+ - *` (a whole `/` reached from the root only through `+ - *`) | 14 (14) / 26 | 0 | Q9 "nur auf der obersten Ebene" |
| | | C2: every whole `/` under a Quick or Float destination (destination influence) | 14 (14) / 27 | 0 | direction ("Der Ziel-Typ sollte keinen Einfluss ..."), Q9 |
| C3 | a whole `/` whose other operand is a Quick: `x.w=(q+(w/2))-2` (INTUITOOLS2.BB:69-71), DEFENDER.BB2:386, ISOBLASTER.BB:80, PLOT_SOME_TEXT.BB2:81, TOWERGEN.BB2:70 | C3: a whole `/` beside a Quick keeps its fraction (an operand rule like Q8's point constants, any destination) | 7 (7) / 8 | 0 | round 1 "Auf null hin", F1's removal |
| D | a Quick computed before a Float destination: `x.f=q/100` (CATHWHEEL.BB2, RCANDLE.BB2), `x.f=q*0.002` (ABALL2.BB2:130-131), `x.f=v1/v2` (3DLIB.BB2:1503-1504) | D: under a Float destination the top operation with a Quick operand is computed in Float (a point constant there read as Float) | 19 (18) / 22 | 0 | Q10 "Annehmen" (its places to be checked by these classes) |
| E | a whole shift in a `+`/`-` chain under a Quick destination: `x.q=(v LSR 3)+14` (DEFENDER.BB2:1970), DEFENDER.BB2:1866, QUIKASM.BB2:3063/3133/3162 | E: the shift exception reaches from the top through `+ -` | 5 (5) / 5 | 0 | round 4 "Nur der Shift ganz oben" (`x.q=qa+3 LSR 3` would be 1.625, round 4 says 1.25) |
| F | a right shift of a negative Word/Long at the top under a Long destination or in Print: `NPrint v LSR 4` (BLITZMAN.BB2, STARTREK.BB2:194/216), `x.l=v LSR 6` (FAST21.BB2:566) | F: a right shift at the top under a Long destination or without a destination shifts in 32 bits | 17 (0) / 17 | 0 | Q14 "Eigene Breite", "shift widths" |
| G | the 48.16 cut of `*` and `/` (negative quotients, one 1/65536 step; the original's qdivide cuts towards zero with its last bit): DEFENDER.BB2:1596-1632, STARTREK.BB2:458, FUN_VECTORS, SPHERE | G1: cut towards zero everywhere | 80 (13) / 156 | 80 (3) | Q1 "Immer abrunden" |
| | | G2: towards zero only at the top under a Quick or Print | 36 (6) / 81 | 49 (3) | Q1 |
| H | a Quick shifted left under a Long: `x.l=q ASL 4` (ISOBLASTER.BB:82-106), CCK.BB2:423-429 | H: a Quick rounded down before a top shift under a whole destination (old F7) | 7 (3) / 7 | 45 (40) | round 2 "Bleibt Quick, 16.16" |
| I | a top `/` of a negative Quick under a Long (BSPREAD.BB2:550) | I: a top `/` towards zero (old A1) | 1 (0) / 1 | 17 (9) | round 1, Q1 |
| J | a left shift beyond the Quick range under a Quick destination: DEFENDER.BB2:456, STARTREK.BB2:457/688, CHECKERSCAPE.BB2:74 | J: a top left shift under a Quick destination wraps instead of saturating | 4 (4) / 4 | 0 | round 5 ("saturated at the conversion") |
| K | `Mod` with a negative divisor (ZOMBIES.BB2:703, BIO.BB2:43) | K: today's sign rule | 2 (0) / 2 | 0 | Q2 "Floor" |

Combined (alone counts do not add up; measured together): A+B+D (with D's
division part) leave 211 of the 337 places away from 1.95 (101 at
non-negative values); adding C1 and E 192 (78); adding C3 and F 171 (74).
The rest is mostly class G (one 1/65536 step, negative values), which no
simple rule restores: the original's qdivide cuts towards zero with its
last bit. Not proposed: G1, G2, H, I (they break more places than they
restore), K (signed values only, against Q2).

The planner's view (not a recommendation to adopt, Q12 asks to discuss):
A is the strongest class (AmigaOS include constants compute 0, every value
set); B and D are one-line rules with no place broken; C1 and E extend the
two top-level exceptions by the same reach and break nothing; F restores
only negative values.

## Answers to the interaction questions (I1-I9)

Each one was a reading where two answers meet; all are answered
(rounds/tree/decisions.md, "2026-11-27 type-system-rework I1–I3" to "...
I9 and exception warnings"; quoted there):

- **I1 A:** C3 applies beside any operand of Quick class: a Quick variable,
  a subexpression with a Quick result (`(q*n)+w/2`), a constant with a
  point that is a Quick (`0.5+w/2`); also with a whole variable divisor
  (`q+w/d`). A Float does not (`f+w/2` 1.5).
- **I2 A:** under D a constant with a point beside a Quick is read directly
  as a Float once the top operation's Float type is final (`x.f=q*0.002`
  0.184, as the original).
- **I3 A:** a quotient reached by C1 under a Float destination is computed
  directly in Float (`z.f=(7/2)*2` 7).
- **I4 B:** C1 reaches through a unary minus and through an enclosing `/`,
  at any depth (`(a/b)/c` 1.75, `a/(b/c)` 14); measured in "With I4 B"
  above. Runtime cost not measured yet (task 5.1). A divisor 0 of such a
  quotient raises the division trap, as every division by 0 (D1/D7).
- **I5 A:** J applies only when the left shift is the whole expression; a
  left shift reached through `+ -` is a Long (A) and the sum saturates at
  the conversion (`x.q=(v LSL 4)+p`).
- **I6 B:** J also wraps a Quick left operand when the left shift is the
  whole expression (`z.q=q LSL 2`, restores the four class-J places).
- **I7 A:** convert by the established type: a Quick rounds down, a Float
  cuts towards zero (`x.l=-1.5` -2, `x.l=-1.4` -1). The uneven
  distribution of the Float conversion (a double-wide interval around 0)
  is GitHub #1234, outside this change.
- **I8 A+C:** a Float beyond the Long range is cut towards zero and wraps
  (`r.l=f` with `f=1e10` 1410065408); `Data.b` with a point cuts as a
  Float (-1.5 gives -1). Two warnings: `longwrap` (a wrap known at compile
  time) and `datafrac` (a nonzero fraction of a Data.b value discarded).
- **I9 A:** E reaches through `+ -` only under a Quick destination
  (`x.q=qa+3 LSR 3` 1.625, `NPrint qa+3 LSR 3` 1.25).
- **Warnings** (I9, "warnings: scope, names, volume"): every place where
  an exception gives another type than the standard rules warns, also
  with the same value (`x.q=4/2`): `divfrac` (Q9/C1), `divquick` (C3),
  `floattop` (D), `shiftfrac` (the shift exception and E), `shiftwrap`
  (J); A, B and Q8 are the standard rules and do not warn. In the editor
  at most 8 places per name and compile, then one `NAME: N more`; on the
  console every place (compiler/warnings delta).

## Answers to the readings of the model (task 1.1)

Task 1.1 named twelve readings the delta left open; the planner took them
through D-011 with emulator probes (section1-decisions.md); answered in
rounds/tree/decisions.md "2026-11-28 type-system-rework D2, D3, D4", "...
D1/D7 ..., D5" and "... D6", the precedents and Plan choices in
coordinator-decisions.md "2026-11-28 ... task 1.3":

1. A point constant under a unary minus beside a Quick is a Quick (D3):
   `x.q=q+(-0.1)` $0002E667.
2. D applies only to an arithmetic top operation, not to a comparison
   (D2): `x.f=(q=0.1)` -1.
3. D and E do not reach through a unary minus; C1 does (D2, I4 B).
4. A quotient reached by C1 under a Float and by C3 is the Float quotient
   (I3, precedent; emulator .8333334).
5. and 11. Every `/` and `Mod` by a run-time 0 raises the division trap
   (D1/D7).
6. `NOT` is one step wider; of a Quick a Long (D6).
7. A point constant beyond the Quick range is a Float (round 3,
   precedent); read as a Quick it is the FFP value of its text cut down
   (planner's P1).
8. Quick and Float parameters take the exceptions (D5); an index is a Long
   destination (spec).
9. A Float left operand of a shift is refused (round 2, Q17, precedent).
10. C3 applies at arithmetic operators, Mod, `^` and comparisons, not at bit
    operators and shifts (D4).
13. (task 1.5) C3 reaches through a unary minus ("2026-11-28 type-system-rework
    C3 through a unary minus"): `NPrint q+-(w/2)` -1. A constant with a
    point left of a shift with no Quick beside it stays a Float and is
    refused (`-0.1 LSL 2`; the model's 1.5 reading made it a Quick).
12. A Float division by 0 raises the trap (D1/D7).

## Findings of task 4.3 (the conversions in compilerlib)

- The routines: `q48sat` ($D612, 48.16 -> Quick saturated; d0/d1 in, d0
  out, d1 kept), `f2l` ($D613, Float -> Long towards zero, wrapped beyond
  the Long range; a Word or Byte the low word, ext.l) and `f2q` ($D614,
  Float -> Quick rounded down, saturated); `f2l` and `f2q` change d1
  (qtof2's contract of a conversion). Oracle: `exprarith_q48_quick`,
  `exprarith_ffp_whole`, `exprarith_ffp_quick` (exprarith.c), checked
  against compilerlib in `libtest_q48.c` (emulator, `libs_emulator`
  filter `libtest compilerlib`) and against the exact double value in
  `test_exprarith`.
- `f2l` computes the value from the FFP bits itself instead of calling
  mathffp's `SPFix`: the libtest compares it with the ROM's `SPFix` for
  every Float in mathffp's form in the Long range, no difference. Outside
  that form the ROM differs: a Float whose low byte is 0 (exponent 0,
  a mantissa not 0: below 2^-64) comes back from `SPFix` unchanged (the
  ROM takes a low byte 0 as the value 0), and a mantissa 0 with a large
  exponent saturates; `f2l` gives 0 for both.
- Speed (bcompilerlib, the call and one load included): `f2l` 183 cycles
  for -2.5, 135 for 1e10; `f2q` 186 for -14.4, 131 saturated; `q48sat` 87
  and 89. Today's `c=f1` through SPFix has the budget 230, `q3=f1` through
  `fltoqu` 1784 (bmath, with the loads and stores).
- A Float -> Quick for a mantissa with bit 31 clear and exponent 16 or
  more (not mathffp's form) saturates in `f2q`; `exprarith_ffp_quick`
  gives the exact value. mathffp never gives such a Float (a program
  can only poke it).

## Risks / Trade-offs

- [Many results change at once] -> one rule per commit where possible, the
  prev comparison names every changed executable; the CD places listed
  per cause; the user decides Q9/Q12 first.
- [The evaluator is not the model] -> task 1 builds the model's rule set
  and repeats the measurements; differences stop the work.
- [Speed of Quick code] -> the optimizations of Decisions 3, the perf
  budgets re-measured and every raised budget named.
- [Register pressure of 48.16 everywhere] -> the nesting count of #1185
  (expr-tree-5 3.7) is repeated for the new rule (every Quick operation a
  register pair); "Expression too Complex" must not appear for a CD source.
- [AB3 differences] -> rows in docs/ab3/variant.md; the AB3 matrix cases
  keep AmiBlitz3's values as REF_EXPECT.
- [The warnings are enabled by default] -> a command-line compile of a
  source with an exception place ends with return code 5 instead of 0;
  the test runners that compile corpus, CD and matrix sources handle it
  (task 3.6), and the warning tests check the lines themselves.
- [The cost of I4 B is not measured] -> more inner divisions run in 48.16
  or Float; the CD has three such places, all under a Float destination;
  D-007 asks for an A500 cycle comparison of the nested forms (task 5.1).

## Findings of the warning scaffold (tasks 3.5, 3.6)

- **The call for the typing pass.** `typewarn(W_NAME)` (`parse.h`) raises
  a type warning at `linenumat`; the names come from one list,
  `shared/ab3job/warnlist.h` (core `W_*`, job `AB3JOB_W_*`,
  `ab3args_warnings`). Only the first pass of a compile reports (a second
  pass for the smallest code or after more memory repeats every place).
  `exprtyping.c` is also a host-compiled file without `parse.h`: tasks
  3.1/3.2 need a way out of it to `typewarn` (a callback or a flag the
  caller reads), not a direct call.
- **The editor's text.** The spec asks that the requesters "name the
  places"; the requester reads `Warning: line N: TEXT [NAME]` (N the
  editor's line, the INCLUDE line for a place in an include), then
  `Warning: NAME: K more`. The requesters are kept and shown when the
  pass has ended, after the progress requester closed (docs/architecture.md
  "Warnings": one during the compile would sit under it). On the console
  every place is printed at once, inside an include with the include's
  name and line (as an error's place).
- **`datafrac`** is raised in `dodata` after the value is accepted (no
  warning before a "Bad Data" error); the value stays the one of today:
  the Byte keeps the value cut towards zero, `Data.b -1.5` -1, as the
  scenario "Data.b with a point" gives.
- **`longwrap` has no place yet.** Today no compile-time conversion of a
  Float constant to a Long exists: `r.l=1e10` converts at run time
  (`floattolong`, SPFix, saturating), `Data.l` refuses a Float as bad
  data. The place comes with task 3.4 (folding) and the conversion of
  task 4.3.
- **Task 3.6: RETURN_WARN accepted, the warnings not disabled.** The
  runners of corpus, CD, matrix and site examples compile in the editor
  path (tedstub): it took any message during the compile for the error;
  it now logs a message `Warning: ...` as a line `WARNING ID MESSAGE` and
  the compile goes on (the result is the `RESULT` line as before). The
  command-line runner of the test model (`caserun`, `cr_cli_compile`)
  already judges the compile by the executable, so rc 5 is a compile
  "ok". Disabling the warnings for the runners would hide what the
  warning tests check through the same runner (`warn-datafrac-limit`); a
  warning in the error set stays an error in both paths.
- An executable with runtime error checking holds values that depend on
  where the compile had the source in memory (the `TRAP #1` records of
  its lines, observed: they differ between two compiles), so two compiles of
  one source in one session differ; the byte comparison with the warning
  disabled is made with `-r` (`ab3_cli` `datafrac_r`/`datafrac_off`).

## Findings of tasks 3.1, 3.2 (the classes) and the warning sites of 3.5

- **The classes are their own function.** `exprtyping_classify`
  (`exprtyping.c`, "the operand-only classes") gives every node its class
  by the rules of the spec delta (`cls`: a whole width, 48.16, Float) and
  the exception that changes it (`exc`, `ETX_*`), and the set of
  exceptions of the expression. It agrees with the model's
  `EM_RULES_TSR` node by node (the model's `sig` under `EM_R_TSR`, its
  `tsr_sig`): every matrix cell in every context including the indexes,
  the chains of the analysis, every chain of two operators with unary
  minuses under a whole, Quick and Float destination and none, the spec's
  scenarios, and the CD's expressions (`cd.cases`, with
  `EXPRTYPING_CD_CASES`; not in the repository).
- **The cut.** The code still follows `exprtyping_type` (rule P+): its
  decisions (path, operand types, 16.16 or 48.16, the consumer) are what
  eval.c's emitter writes. The new classes cannot be handed to the emitter
  without the code of section 4: a whole operation one step wider is
  4.1's `ext` + Long code, every Quick operation in 48.16 is 4.2 (today
  16.16 with qmul, qdivide where P+ chose it), the saturating conversion
  to a Quick, the shift widths (4.4), the run-time form without a
  destination (4.5) and the comparison of the full value (4.6). So
  `const_flow`, the wanted-type cases of `typed()`, `has_long`,
  `frac_side`, A1 and the cut by direction stay until the emitter takes the
  classes; tasks 3.1 and 3.2 stay open (their removal and the constant
  typing in the code go with section 4).
- **The warning sites.** `p5end` (eval.c) raises `typewarn` once per name
  and expression for each exception of `exprtyping_classify` under the
  expression's wanted type (a typed parameter its type), in both modes;
  the executables do not change. The exceptions keep the original's
  behaviour, which P+ keeps at these places, so today's code does what the
  warning says at the scenarios of the warnings spec (emulator:
  `x.q=w/9` 22.2222, `y.q=4/2` 2, `a.q=1 LSR 7` .0078, `b.q=1 LSL 16` 0,
  `f1.f=q/100` .20875, `pq{1/16}` .0625, `NPrint q+w/2` 120.875 with
  q=20.875 and w=200). `divquick` warns wherever C3 applies, also without
  a destination (`NPrint q+w/2`): the decision "I9 and exception warnings"
  asks for every place an exception applies, C3 does not depend on the
  destination ("conflict item 29"); the warnings spec says "at a
  destination or a typed parameter". Where C1 and C3 reach the same `/`,
  the warning is `divfrac` (C1 decides its type). A run of minus signs is
  one node of the compiler's tree (`p5neg`) and is classed as one unary
  minus.

## Findings of task 4.1 (the whole-number operations)

- **Where the rule sits.** `exprtyping_type` takes the classes of
  `exprtyping_classify` (blitz2 runs it first, `p5end`; without them, as
  in the host test of rule P+, the pass is P+ alone). For a node of whole
  class without an exception (not Mod, not a shift, not a bit test) the
  type the operand typing chose stays where its code gives the class value
  for every operand value, else the node is computed in the narrowest
  whole type whose code does (`wb_type`; the bounds of `wb_need`: Byte 8,
  Word 16, Long 32 bits, + - one bit more, * the sum, / one more unless the
  divisor is a constant of at least 2). The kept code is the design's
  "Optimizations": under a destination that wraps at n bits through + - *,
  the unary minus, NOT and the bit operators (a / of two Byte or Word
  operands in its Byte or Word too: DIVS.W keeps the dividend on -32768 /
  -1, whose low 16 bits are the quotient's), and range bounds (a Word add
  of values of 15 bits, MULS.W of two Words, a 16.16 operation within 16
  bits, a Float one within the 24 bits of the mantissa). Elsewhere: the
  operands extended and the Long code; a minus sign wider than its operand
  (`wide`: the operand converted first); a Long / of two values in the
  Word range by #1179's inline DIVS.W (`o.fast`), not ldivlib.
- **What changes.** `NPrint w+v` (Long add), `x.w=(w+v)/2` (the sum
  Long), a whole operation under a Quick destination (`x=a+1` with a Word
  `a` and the default type Quick: Long code, then today's conversion; the
  value changes only with the saturating conversion of 4.2), a Long product
  under a Float destination (`f.f=l*l` wraps: 1410065408, the matrix cells
  df/pf `l*l`, `l*cl`, `cl*l`), a whole / below another operator under a
  Quick or Float destination without the exception. Of the CD's 27,898
  typed expressions 507 decisions change in 74 sources; 51 executables
  change (each of them has a changed decision; the other sources' changed
  decisions give the build's code already).
- **What of P+ stays.** The wanted-type cases of `typed()` stay as the
  candidate the rule keeps where it holds the value (that is the narrow
  code under a wrapping destination and the range bounds); `cmplong`
  (whole operations in a comparison at least Long) stays for the same
  reason. Not this task's: the fractional world (`frac_value`,
  `frac_side`'s whole arithmetic subtree in 16.16 or 48.16, part 7:
  `has_long`, the 16.16 operands; 4.2), Mod's type and sign rule (4.7),
  the shifts (4.4), the comparisons (4.6), folding (3.4) and the reading
  of constants (3.3). A whole-class operand that P+ computes in 16.16
  counts as 16 bits (its value as computed), so the parent changes only
  when 4.2 makes it 48.16.
- **Resolved (2026-11-28, round tree decisions, "Compilieren").** A bit
  operator or NOT with whole operands under a Float destination
  (`rf.f=b AND 1`) compiles: the operator is typed by its own operands
  and its whole result converts to Float at the end; a Float operand stays
  refused. The build's early refusal is left to the pass for these
  operators (`exprtype_bypasswhole`, `exprtype_bypassnotwhole`, as for a
  constant with a point). Test: expr-tsr-bitop-float (1.95 refuses it);
  the refused matrix cells that compile now are regenerated by task 6.1.

## Findings of task 4.2 (the Quick operations)

- **Where the rule sits.** Under the classes (`c->tsr`) every `+ - * /`
  and unary minus of Quick class (`q48_class`) is computed in 48.16
  (`ETY_Q48`, `ETF_Q48`) wherever it stands: at the root
  (`exprtyping_type`), inside an operation of the operand typing
  (`typed`: beside a Float taken as a Float, by a comparison exactly), and
  in the fractional world (`frac_value`, `frac_side`): an operand is typed
  by its class, a whole operand as the whole number its class is (P+'s
  part 7, a whole subtree in a fractional format, no longer applies under
  the classes; no destination reaches it). Mod keeps rule P+'s format
  choice (16.16 or 48.16; with a Long in it 48.16 in every context, task
  4.7) and so do the shifts (task 4.4); a bit operator, shift or NOT takes
  a 48.16 operand as P+ takes its operand there (the floor as a Long
  where P+ computes in 48.16, else the low 32 bits), so their values stay
  P+'s until 4.4.
- **The consumers.** A wanted Quick saturates (`ETC_SAT`: the inline test
  of q48sat in `p5sat`, D(r+2) its scratch); a wanted Byte, Word or Long
  rounds down (`ETC_FLOOR`; #1222's cut towards zero, `ETC_TRUNC`, and the
  towards-zero code at a / under a wanted Long no longer occur under the
  classes); a wanted Float takes the 48.16 value (`ETC_TOFLOAT`,
  `p5q48tof`, which now converts the magnitude and negates: -0.2087402
  lost its fraction in -65536 + 65535.79 before); no destination as
  before (`ETC_FIT`, the Float form is task 4.5). The conversion of a Long
  or a Float to a Quick at a destination or a Quick parameter
  (`evalexit`, `quicksat`) saturates: a Long by q48sat's test inline
  (D(regat+1) its scratch; q48sat itself in D7), a Float by f2q; a left
  shift at the top (J, `EVF_QWRAP`) keeps the wrapped Long. A Long or
  Float root under a wanted Quick takes the pass's code instead of the
  build's (which converts a Long variable as it fetches it, `zq.q=l`),
  except a constant (#1070's literal near 32768: the reading of constants
  is task 3.3).
- **The cut.** Every product and quotient is cut down (`p5down`: q48mulf,
  q48divw's cut; the fast paths P5F_NDIV and P5F_NMULF in their down
  form). The value of a division by 0 stays the wanted type's largest
  (task 4.7): under a wanted Quick, Long or none an inline test of the
  divisor calls q48divq / q48div only for 0 and q48divw otherwise.
- **Kept as value-identical.** One `+`/`-` at the top under a wanted Quick
  of two values held exactly in 16.16 (a Quick, Byte or Word variable, a
  constant of the Quick range, a whole operation of a Byte or Word result)
  is the 16.16 operation with `BVC` around the saturation (`P5Y_SAT`,
  `p5satmark`); `p5mixfloor` (mixmult, qqmult, mixedop under ETC_FLOOR:
  the exact floor), q48mull (exact), the whole-divisor and whole-Mod paths
  and P5F_WMUL (exact). `P5Y_LOW` (the low 32 bits under a wanted Quick)
  no longer applies: saturation needs the high part.
- **The cut with task 3.3.** Where rule P+ reads a constant with a point
  in such an operation as a Float (`is_floaty`: every constant with a point
  under a wanted Byte, Word, Long or Float, one beside whole numbers
  only), the operation stays P+'s Float one (`r.l=q*0.5`,
  `w=(qt+lz)/2.5`); it becomes 48.16 when the reading of constants
  switches. The expected failure `quick48__cut_under_long`
  (`p=1/65536`) is the folding of a whole division of constants (3.4).
- **What of P+ remains.** For the host test of rule P+ (no classes) the
  pass is P+ alone; `has_long`, `frac_side`'s whole part, the force and
  A1, `ETC_TRUNC`/`P5F_LDIV`/`p5qdivt`, `P5Y_LOW`/`p5lowmark` and the 16.16
  paths stay reachable there (and in blitz2 only where the classes refuse
  an expression that P+ accepts). Under the classes they are used by Mod
  (`has_long`'s force), by the bit operators', shifts' and NOT's operand
  (`frac_operand`'s P+ form) and by the comparisons' P+ routing (4.6).
- **Cost.** bmath and bq48 (one run, cycles per call, against
  `tests/perf/budgets.txt`): `q3=q1*q2` 652 (394), `x=l*q` 397 (220),
  `x=lx*q` 557 (224), `x=l+q+p` 201 (96), `q3=Int(q1)` 97 (67), `Int(q6)`
  113 (81), `c=a4/q7` 492 (329), `r=lx/qw` 634 (462), the divisions under
  a wanted Word or Byte at the top (`rw=n/q` and the like) about 2000
  (1150-1214: longdiv on the 16.16 bits towards zero is no longer the
  value), and seven loops 3-11 % over; faster: `q3=f1` 181 (1784, f2q
  instead of fltoqu), `q3=q1/q2` 1926 (1988). The CD's executables grow
  (DEFENDER.BB2 92148 to 98684 bytes): the 48.16 code of every Quick
  operation.

## Findings of task 4.4 (the shifts)

- **Where the rule sits.** Under the classes a shift of Quick class (a
  Quick left operand, or the shift exception E of a whole one) is typed by
  `frac_shift` (exprtyping.c), in the fractional world and at the top as
  an operation of Quick class (`q_shift`); a shift of whole class by
  `whole_shift`. The new code is the path `ETY_SHIFT` (eval.c,
  `p5shiftop`): a whole `LSR` of a Byte or Word class in its own width
  (`lsr.b` + `ext.w`, the Byte a sign-extended Word in the register;
  `lsr.w`), and the 48.16 shift of a register pair, one bit per step
  (`lsl/roxl`, `asr/roxr`, `lsr/roxr`, `dbra`) by the count modulo 64 in
  D(r+2), the left operand widened first; the `LSR` of a Long under E is
  the 16.16 `lsr.l` when the Long is in the Quick range (its result
  extended to the pair) and the 48.16 one beyond it (N1), decided at run
  time. The count is whole: a Quick count is rounded down by its
  conversion (`quicktoword`), a 48.16 count is taken as its whole part.
- **Kept as the build's code (value-identical).** A Quick's `LSR` (on the
  16.16 bits; of a 48.16 operand its low 32 bits) and the `ASR` of a 16.16
  operand (exact); a left shift of a Quick at the top under a Quick
  destination (J: the low 32 bits of the 48.16 value) or under a Byte or
  Word one (its bits 16-31 depend on the low 32 bits only); E of a Byte or
  Word class (in the Quick range: the 16.16 shift); a whole `LSL`, `ASL`,
  `ASR` and a Long's `LSR` as the build's Long shift (the low bits of
  `asr.l` of the extended value are those of the shift in the operand's
  width), in the build's type (the wanted whole one and the left operand's, not
  the count's; a Quick count at least a Word); the left
  shift at the top under a Quick destination (J) of a whole operand as
  the build's 16.16 shift (its low 32 bits are those of the Long shifted
  and wrapped).
- **The bit operators, NOT and a shift count** take a 48.16 operand as its
  whole part, a Long rounded down, in every context (`frac_operand`; 4.2
  took the low 32 bits where rule P+ did): the spec's "Bit operators take
  whole operands". `wb_need` of a Byte or Word shift is its width, so a
  parent's whole code follows the class.
- **Measured** (emulator, an ad-hoc case set beside the expr-tsr tests):
  `l.l=-3 : x.q=l LSR 3` $1FFFA000, `l=100000` 25000, `l=-100000` LSR
  $7FFFFFFF (the 48.16 shift saturated), ASR -25000, a count 0 -3,
  `(v LSR 2)+0.5` with `v.l=100000` 25000.5; `q=1.5 : rl.l=q LSL 20`
  1572864, `q=-30000 : rl=(q+q) ASR 1` -30000, `(q+q) LSR 1` 2768,
  `xw.w=q LSL 2` 6, `zq.q=q ASL 2` with q=10000 $9C400000, `(q LSL 1)+1`
  40001 with q=20000; `b.b=-8 : rw.w=b LSR c` -8, 0, 124 for c 0, 9, 1;
  `w.w=-1 : rl.l=w LSR 4` 4095; `NPrint (q+q) OR 1` 60001 and `Not (q+q)`
  -60001 with q=30000, `NPrint x1 LSL (q2+q2)` 4 with q2=1.25.
- **What changes.** Every executable with a shift of a Quick (under a
  Long or no destination, in 48.16; under a Long also the LSR and ASR on
  the 16.16 bits instead of the Quick rounded down first), with a whole
  `LSR` of a Byte or Word value, with a Quick count beside a whole
  operand, with E of a Long, or with a bit operator, NOT or shift count
  of a 48.16 value outside a wanted Long. Of the CD's sources one more than
  the earlier tasks' (EXTRAS/LIBSDEV/ACIDLIBSRC/DEFAULTDBUG.BB2: a Word
  `LSR` in its own width, same size). The matrix cells of shifts with a
  Quick (bug-orig `compiler-expr-1216-*-now` dl, i1, i2, pl and
  `compiler-expr-1179-i2-i-u-01`) now give the classes' values (`-1.5 LSL
  3` -12, not P+'s -16; `1.5 LSL 2` in an index 6, not 4); their
  expectations are regenerated by task 6.1, as the dq/pq cells of 4.2.
- **What of P+ remains.** For the pass without classes (the host test of
  rule P+) the P+ shift typing in `typed()` (a Quick count makes the shift
  a Quick one, the Quick rounded down under a Long) and `frac_other` for
  shifts; under the classes no shift goes through them. The `plong` choice
  of 4.2 is gone.
- **Not this task.** A constant shift is folded as before (3.4: `y.q=-(1
  LSR 7)` stays -1/128, `x.q=qa+3 LSR 3` keeps the build's fold, the CD
  idiom `hsp=1 LSR 7` 1/128 as the build folds it). A shift under a Float
  destination (`rf.f=b LSR 1`) stays refused as before, although the
  classes accept it: the decision of 4.1 ("Compilieren") names bit
  operators and NOT only (open question to the coordinator).
- **Cost.** No loop of the benchmark has a shift other than the timer's
  `th LSL 8` (Long, the build's code unchanged), so the perf loops do not
  change. A 48.16 shift costs about 30 cycles per bit (`lsl.l #1`,
  `roxl.l #1`, `dbra`) plus about 50 for the widening and the count.
