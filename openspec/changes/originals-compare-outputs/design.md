## Decisions

**Where an executable still counts.** Only where the comparison partner is the
port's own earlier state: the `prev` build (corpus configuration `prev`,
`prev_results`, `ted_ab` `P5`/`Pc`), and the AB3 compiler against blitz2
(`tests/ab3`, both are the port). Everything that names the original or the
CD (`NAME_originals`, the CD table, `ted_ab_originals`) compares outputs and
compile results.

**Corpus.** The observation `exe` is removed from the expected data
(`tools/lib/corpusexpect`): a `NAME.exe`, `NAME.orig.exe` or `NAME.fixed-N.exe`
is "unknown observation", so a stale file is refused before any session. The
recording and update targets no longer write executables. The configuration
expectation `exe` keeps its meaning for `exe:BASE` (prev) and for the compile
only passes (ab3). A new expectation `exeinfo` (run passes) means: the output
is judged as for `output`; the executable is compared with the reference's
and a difference printed as `exe differs (informational)`; a missing
executable (the port does not compile what the original does) is a failure.
`port` uses it.

**CD table.** Fields `corpusrun cd` compares with a reference: result, line,
message. Size and hash are printed for information (`exe differs`) and not
compared. `results cd-update`: the row of a source whose result is the old
row's (ok to ok, or the same error) keeps the old size and hash, because the
table holds the original's executable; a changed result is written and needs a
note as before. A note naming an executable hash (`(2.10: HEX)`) for a row that
the port still compiles is obsolete: its row result is the CD's again, the note
is dropped. `results cd-reference` is unchanged: the original's run is exact
data, and a run of `blitz2-cd-reference` restores the original's size and hash.

**ted_ab_originals.** `@expect corpus NAME TEST` compares TED-driven compile
output with the executable of the corpus ref run. For the combinations that
use the port's compiler (`Cc`, `Rc`) in the pass whose corpus output is the
original's (`corpus_out_info=`), a difference is reported (informational); the
same expectation for `R5` and `C5` stays strict (the original compiler's
executable must be the corpus' one). In the pair comparison, a file that an
`@expect corpus` names is an executable; when the two combinations use
different compilers (`R5-Rc`, `C5-Cc`) its difference is informational. Other
files, dumps, requesters and return codes stay strict.

**Removal.** The code that reads exe pairs is removed, not kept dormant.

## Risks

A port compiler that emits a wrong executable which runs correctly is not
found by `*_originals` any more; it is found by the output tests (`clib`),
the CD compile (compile results), and `prev` for any change.
