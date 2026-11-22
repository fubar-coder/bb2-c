# exprmatrix

The expression matrix of blitz2 (OpenSpec change `expression-type-matrix`,
spec `testing/expression-matrix`): every cell - a context, an operator,
the operand kinds in order and a value set - is one line of a generated
Blitz source that prints the cell's key and its value. `generate` writes
the sources from a description of the matrix. The logic is
`tools/lib/exprmatrix` (`exprmatrix.h`), tested by
`tools/tests/test_exprmatrix.cpp`.

## Usage

```
Usage: exprmatrix [OPTIONS] SUBCOMMAND

The expression matrix of blitz2: every cell (context, operator, operand kinds in
order, value set) a keyed line of a Blitz source.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  generate                    the Blitz sources of the cells from the description
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

## Exit codes

* 0 the sources are written;
* 1 the description cannot be read or is malformed (messages on stderr,
  nothing written), a source name is too long, or a source cannot be
  written;
* 2 usage error (no or an unknown subcommand or option, a missing
  argument, `--cells` outside 1 to 100000).

Messages go to stderr as `exprmatrix generate: [FILE: ]message`, the
errors of a description as `FILE:LINE: message`.

## Callers

* Tests: `help_exprmatrix`, `readme_exprmatrix`
  (`tools/exprmatrix/CMakeLists.txt`); the library: `exprmatrix`
  (`tools/tests/test_exprmatrix.cpp`).

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
