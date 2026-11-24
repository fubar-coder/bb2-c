# exprtables

Readable tables of the type rules of an expression in `blitz2` (OpenSpec
change `expr-tree-5-operand-typing`, design decision 3): for one operation
of two values, keyed by operator, left type, right type and type so far,
the type each operand is taken in and the result type, with the exceptions
as notes naming their issues; the elements and the constants under each
type so far. `page` writes them as the page
[`docs/blitz2/expression-type-tables.md`](../../docs/blitz2/expression-type-tables.md)
or checks the committed page. The logic is `tools/lib/exprtype_tables`
(`exprtype_tables.h`: the rule set in `rules.c`, the rows of `exprtype.tsv`
as decisions in `tsv.c`, the page and its comparison in `page.c`), which
links `programs/blitz2/src/exprtype.c`; tested by
`tools/tests/test_exprtables.cpp`.

## Usage

```
Usage: exprtables [OPTIONS] SUBCOMMAND

Readable tables of the type rules of an expression in blitz2, from the decisions
of programs/blitz2/src/exprtype.c.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  page                        the page of the tables, written or checked
  candidate                   the tables of a candidate rule of expr-tree-5 beside today's
```

```
Usage: exprtables page [OPTIONS]

the page of the tables, written or checked

OPTIONS:
  -h,     --help              Print this help message and exit
  -o,     --out FILE          write the page to FILE (default: stdout)
          --check FILE        compare the committed page FILE with the generated one
          --tsv FILE          take the decisions from the rows of exprtype.tsv FILE, not from
                              exprtype.c

Without --out and --check the page goes to stdout. With --check it is written
only to --out, and only if FILE equals it (a file at --out is removed
otherwise). Exit status 0, 1 the pages differ (one message per differing line,
FILE:LINE: ..., at most ten, and their number), a file cannot be read or
written, or a decision has no row in the --tsv table, 2 usage error.
```

`page` makes the page by today's rules: the type so far flows from left
to right through the decisions of `exprtype.c`. `--tsv` takes the
decisions from the rows of the committed table
`programs/blitz2/tests/host/exprtype.tsv` instead (each decision the page
needs must have its row there). Without `--out` and `--check` the page
goes to stdout; `--out` writes it to a file; `--check` compares the
committed page with it line by line and writes `--out` only if they are
equal.

```
Usage: exprtables candidate [OPTIONS] NAME

the tables of a candidate rule of expr-tree-5 beside today's

POSITIONALS:
  NAME TEXT REQUIRED          the candidate, C1 to C7 (exprmatrix candidates)

OPTIONS:
  -h,     --help              Print this help message and exit
          --scope-b           the left operand of a comparison by scope (b), not (a)
          --marks FILE        the cells of the CD places to mark: OP WANT LEFT RIGHT per line
  -o,     --out FILE          write the page to FILE (default: stdout)
          --c7k VARIANT       C7 as C7K: own, long or dest (a comparison's whole-number
                              operations)

With --out prints the cells that differ from today's and the marked ones per
operator group. A line of --marks is the cell as exprmatrix candidates --typing
writes it ("mul L W Q"), anything after a tab ignored, # starts a comment. Exit
status 0, 1 a file cannot be read or written, 2 usage error or no such
candidate.
```

`candidate` prints a candidate rule of the decision round of the OpenSpec
change `expr-tree-5-operand-typing` (open question 1; the rule sets
`EM_R_CAND` of the expression matrix's model, `tools/lib/exprmatrix`,
`exprmatrix candidates`) as the same tables as `page`, beside today's: its
own heading and text in place of the introduction and of today's
exceptions, a column being the wanted type at the operation; a cell that
differs from today's is marked ◆, a cell named in `--marks` (the cells of
the CD operations the candidate retypes, from the count of the decision
round) ‡, and a table of the constants beside an operand of each type is
added. With `--scope-b` the left operand of a comparison follows scope (b)
of the open question (the cells, two variables each, are the same); with `--c7k` C7
is printed as C7K (constants by their smallest type, comparisons typed by their
operands; `own`, `long`, `dest` the width of a comparison's whole-number operations). The
logic is `ett_rules_cand` (`cand.c`) and `ett_page_opts` (`page.c`).

## Inputs and outputs

The page is Markdown (GitHub tables). `--tsv` reads `exprtype.tsv` (its
format is in its header, written by `test_exprtype`). The messages of a
failed check go to stderr: `FILE:LINE: differs from the generated page:`
with the committed line and the generated one, `FILE:LINE: missing:` (the
committed page ends earlier), `FILE:LINE: not in the generated page:`, at
most ten, then `FILE: lines that differ from the generated page: N`.

## Exit codes

0 success; 1 the pages differ, a file cannot be read or written, a
malformed `--tsv` table, or a decision without its row there; 2 usage
error.

## Callers

* `tools/exprtables/CMakeLists.txt`: the build step of the target
  `expression_type_tables` (in the default build of the host side)
  generates the page into the build directory with `page --check`, so a
  committed page that differs fails the build; the target
  `expression_type_tables_update` writes the committed page after an
  intended change of the rules. The ctests `exprtables_page` (the
  committed page), `exprtables_page_tsv` (the same from `exprtype.tsv`)
  and `exprtables_page_broken` (`page-broken.cmake`: a copy with a changed
  cell and one without a note must fail the check).

## Example

```
$ exprtables page --check page.md
page.md:62: differs from the generated page: | B | B | B B → W | B B → W | W W → W | L L → L | Q Q → Q | F F → F |
  generated: | B | B | B B → W | B B → W | W W → L | L L → L | Q Q → Q | F F → F |
page.md: lines that differ from the generated page: 1
```
