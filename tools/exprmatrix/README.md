# exprmatrix

The expression matrix of blitz2 (OpenSpec change `expression-type-matrix`,
spec `testing/expression-matrix`): every cell - a context, an operator,
the operand kinds in order and a value set - is one line of a generated
Blitz source that prints the cell's key and its value. `generate` writes
the sources from a description of the matrix; `classify` gives every
measured cell its class from the model of the expected values (design
decisions 6 and 7 of the change). The logic is `tools/lib/exprmatrix`
(`exprmatrix.h`: the description and the sources in `exprmatrix.c`, the
model in `model.c`, the classes in `classify.c`), tested by
`tools/tests/test_exprmatrix.cpp`, `test_exprmatrix_model.cpp` and
`test_exprmatrix_classify.cpp`.

## Usage

```
Usage: exprmatrix [OPTIONS] SUBCOMMAND

The expression matrix of blitz2: every cell (context, operator, operand kinds in
order, value set) a keyed line of a Blitz source.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  generate                    the Blitz sources of the cells from the description
  classify                    the class of every measured cell, and the questions
```

### generate

```
Usage: exprmatrix generate [OPTIONS] DESC

the Blitz sources of the cells from the description

POSITIONALS:
  DESC FILE REQUIRED          the description of the matrix

OPTIONS:
  -h,     --help              Print this help message and exit
  -o,     --out OUTDIR REQUIRED 
                              the directory the sources are written to (it exists)
          --cells N           at most N cells per source, 1 to 100000 (default:
                              cells-per-source)

Writes OUTDIR/NAME.bas for every source (NAME is BLOCK-CTX-SET-NN) and prints
"NAME CELLS" per source and the totals. Exit status 0, 1 the description is
malformed (one message per error, FILE:LINE: ...) or a file cannot be read or
written, 2 usage error.
```

`generate` reads the description `DESC`, makes its cells and writes one
source per block, value set and context, split into sources of at most
`N` cells (`--cells`, else the description's `cells-per-source`). A
malformed description writes nothing: every error is reported, each
once.

### classify

```
Usage: exprmatrix classify [OPTIONS] DESC TABLE

the class of every measured cell, and the questions

POSITIONALS:
  DESC FILE REQUIRED          the description of the matrix
  TABLE FILE REQUIRED         the measured cell table

OPTIONS:
  -h,     --help              Print this help message and exit
  -o,     --out CELLS REQUIRED 
                              the cell table with a class per cell (written)
  -q,     --questions FILE    the questions, one per class (written)

Prints the counts per class and question class and the cells where the model
differs from the measurement (the original's rules against 1.95, the port's
against the port). Exit status 0, 1 a file cannot be read or written or is
malformed (one message per error), 2 usage error.
```

`classify` reads the description `DESC` the cells were generated from
and the measured cell table `TABLE`, computes every cell by the model
(below) and writes the cell table `CELLS` and, with `-q`, the questions.
Every cell of the description must be measured once; a broken table
writes nothing.

## Inputs and outputs

### The description

One directive per line; `#` starts a comment; words are separated by
blanks.

| Directive | Meaning |
|---|---|
| `cells-per-source N` | at most `N` cells per source (default 100) |
| `dim N` | the last element of the index arrays (default 300) |
| `values SET KIND=VALUE...` | the operand values of the value set `SET` (letters and digits); may be given on several lines |
| `counts SET KIND=VALUE...` | the right operand of the shifts and of `^` in `SET` |
| `block NAME` | starts a block, an operator group (letters, digits, `_`) |
| `contexts CTX...` | the block's contexts |
| `binary OP...` | its operators of two operands |
| `unary OP...` | its operators of one operand |
| `kinds KIND...` | its operand kinds, for both sides |
| `sets SET...` | its value sets (defined above) |
| `cell SET CTX OP KIND [KIND]` | one cell of the block, given by itself |

A block makes the product of its sets, contexts, operators and kinds (two
operands: every ordered pair of its kinds, so both orders), in that
order, unary operators first; then its `cell` lines. The product leaves
out what BASIC gives no meaning: a string with a number, a string under
an operator other than `+` and the comparisons, a string `+` outside
`print`, a string under a unary operator; a `cell` line with one of them
is an error.

* Kinds: `b` `w` `l` `q` `f` (Byte, Word, Long, Quick, Float variables),
  `s` (a string variable), `cb` `cw` `cl` (literal constants of Byte,
  Word and Long size), `cf` (a constant with a fraction).
* Operators: `+ - * / MOD ^ AND OR EOR LSL LSR ASL ASR = <> < <= > >=`
  (binary; also by their key names), `-` and `NOT` (unary); words in any
  case.
* Contexts: `db` `dw` `dl` `dq` `df` (a destination Byte to Float), `pl`
  `pw` `pq` `pf` (a Long, Word, Quick or Float parameter), `if`, `i1`
  (the first array index), `i2` (a later array index), `print`.
* Values: decimal numbers (`-` and a fraction allowed where the kind
  takes them): a variable's value inside its type's range, `cb` of Byte
  size, `cw` of Word but not Byte size, `cl` beyond the Word range, `cf`
  with a `.`; a string without `"`. A count is a whole number for the
  whole-number kinds and any number for `q`, `f` and `cf`.

Errors, each `FILE:LINE: message`: an unknown directive, kind, operator,
context or value set; a duplicate key (the same cell made twice, also by
two blocks); a name or entry given twice; a block without contexts,
kinds, operators or value sets; a value outside its kind; a value set
without the value (or count) a cell needs; no cells at all.

### The cells and their keys

A key is `CTX-OP-KIND[-KIND]-SET`, `OP` one of `add sub mul div mod pow
and or eor lsl lsr asl asr eq ne lt le gt ge` (binary), `neg not`
(unary): `dl-mul-q-l-A` is `r.l=q*l` with the values of set `A`.

### The sources

`OUTDIR/BLOCK-CTX-SET-NN.bas` (`NN` from `01`; a name is at most 25
characters, an error otherwise), plain text with LF line ends: a header
of two comment lines, the variables the cells use (`vb.b`, `vw.w`,
`vl.l`, `vq.q`, `vf.f`, `vs$`, the counts `nb.b` ... `nf.f`), the
context's destination, procedure or array, one line per cell and `End`.
Per context (design decision 8 of the change):

| Context | Line of a cell |
|---|---|
| `db` `dw` `dl` | `rl=EXPR : NPrint "KEY ",rl` (`rb`, `rw`; a comparison in brackets: `rl=(a < b)`) |
| `dq` `df` | `rq=EXPR : NPrint "KEY ",Hex$(Peek.l(&rq))` (`rf`; the raw bits) |
| `pl` `pw` | `Print "KEY " : pl{EXPR}`; the statement prints its argument |
| `pq` `pf` | `Print "KEY " : pq{EXPR}`; the statement copies its argument to a global and prints its raw bits |
| `if` | `If EXPR Then NPrint "KEY T" Else NPrint "KEY F"` |
| `i1` `i2` | `NPrint "KEY ",ia(EXPR)`, `NPrint "KEY ",ja(1,EXPR)`; element `i` holds `i*16+1` |
| `print` | `NPrint "KEY ",EXPR` |

`EXPR` is `LEFT OP RIGHT` with blanks around the operator, or `-LEFT`
and `NOT LEFT` (`- -3` for a negative constant); the right operand of a
shift and of `^` is the count of its kind.

On stdout: `NAME CELLS` per source, then `TOTAL cells in N sources`.

### The measured cell table (classify)

One line per cell, tab separated: `key`, `source`, then the values of
`c210` (the CD's 2.10), `c195` (blitz2 1.95, the build of `blitz2.s`),
`portcd` (the port with the CD's libraries), `clib` (the port with its C
libraries), `ab3` (AmiBlitz3 v3.14.0), all with runtime error checking
off, and `c210_on`, `c195_on`, `portcd_on`, `clib_on` with it on (empty
for a cell that is no index cell). A first line starting with `key` is
skipped. A value is what the cell printed, or `!noncompile[:MESSAGE]`
(refused), `!runtime[:MESSAGE]` (a runtime error), `!outside` (left out:
outside the array in the checked build), `!garbage[:VALUE]` (no element
printed); the measurement of task 1.3 writes this form.

### The model

`em_model` computes a cell as the code blitz2 emits for it: the type so
far, `varcont`, `consttype2` and the operator handlers of `eval.c` with
the m68k semantics of each instruction, the library routines of the
CD's libraries and of the port's C libraries (`lmullib`, `ldivlib`,
`modlib`, `floatquicklib`, `printlib`), and Motorola FFP for mathffp.
Rule sets (flags in `exprmatrix.h`): the original's (`EM_RULES_ORIG`),
the port as measured (`EM_RULES_PORT`, `EM_RULES_PORTCD`) and the fixed
rules (`EM_RULES_FIXED`: #1177, #1186, #1178 variant C, #1179, #1187,
#1188, the C libraries); each rule in `model.c` names its entry in
`task-decisions.md`. Not modelled: mathtrans SPPow (`^`) where its bits
matter, and the original's `If` on a Float, which tests d1 (#1187);
those values are `?`. Beside the value the model raises the flags of the
question classes (overflow, bit operators with a Quick or Float, `MOD`
with negative or fractional operands, `^`).

### The cell table and the questions (classify)

`CELLS`: a header line, then per cell `key`, `class`, `issues`,
`question`, `expected`, `expected_on`, `c210`, `c195`, `ab3`, `clib`,
`note`, tab separated. The class is `agree`, `fixed-N`, `open-N`
(`N` the issues whose fixed rules change the value, `+` between them),
`port`, `new`, `question` (with its class `Q-A` overflow, `Q-B` the two
orders differ, `Q-C` bit operators, `Q-D` `MOD`, `Q-E` `^`, `Q-F` the
originals disagree) or `noncompile`. The expected value is the fixed
rules' value, or 1.95's where the model cannot compute a Float power
that both rule sets compute alike.

The questions file has one section per question class with the count,
the issues of its cells, the options and three example cells with the
values of 2.10, 1.95, AmiBlitz3 and the port.

On stdout: `cells N`, `class NAME N` per class, `question Q-X N`, the
number of values not modelled, and the cells where the original's rules
differ from 1.95 and where the port's rules differ from the port's
measurement (`mismatches ... N`, then one line per cell).

## Exit codes

* 0 the sources (`generate`) or the tables (`classify`) are written;
* 1 the description or the measured table cannot be read or is
  malformed (messages on stderr, nothing written), a source name is too
  long, or a file cannot be written;
* 2 usage error (no or an unknown subcommand or option, a missing
  argument, `--cells` outside 1 to 100000).

Messages go to stderr as `exprmatrix SUBCOMMAND: [FILE: ]message`, the
errors of a description or a table as `FILE:LINE: message`.

## Callers

* Tests: `help_exprmatrix`, `readme_exprmatrix`
  (`tools/exprmatrix/CMakeLists.txt`); the library: `exprmatrix`,
  `exprmatrix_model`, `exprmatrix_classify` (`tools/tests/`).
* Task 2.3 of the change: `classify` on the measurement of task 1.3.

## Example

```
$ cat m.txt
values A w=1000 q=1.5
block g
contexts dl if
binary *
kinds q w
sets A
$ exprmatrix generate --cells 3 -o out m.txt
g-dl-A-01 3
g-dl-A-02 1
g-if-A-01 3
g-if-A-02 1
8 cells in 4 sources
$ cat out/g-dl-A-01.bas
; g-dl-A-01 - expression matrix: block g, context dl, value set A, cells 1 to 3 of 4
; generated by exprmatrix from m.txt (tools/exprmatrix/README.md); do not edit
vw.w=1000 : vq.q=1.5
rl.l=0
rl=vq * vq : NPrint "dl-mul-q-q-A ",rl
rl=vq * vw : NPrint "dl-mul-q-w-A ",rl
rl=vw * vq : NPrint "dl-mul-w-q-A ",rl
End
$ exprmatrix generate -o out bad.txt
bad.txt:3: unknown context "dz"
bad.txt:4: unknown operator "%"
bad.txt:5: unknown kind "z"
```
