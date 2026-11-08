# Design

## Context

State of main `8392d208`.

- **Errors today.** `err(msg)` (`parse.c:545`): in direct mode (`dirmode`)
  silent; otherwise it records the line (`menuret = linenumat`), closes
  the progress requester (`endstop`), calls `ext_errpos()` and
  `ed_error(msg)` (the editor's modal requester, waits for OK;
  `main.h:188`), then `longjmp`s to `errjmp` (`parse.c:564-568`).
  `menu` returns `menuret` and TED puts the cursor on the line
  (`docs/architecture.md` 3.6). The only editor service for messages is
  `comdata+12`; TED 1.1's `comdata` table (`ted.s:6030`, mirrored in
  `shared/edstub/edstub.c:47`) has no spare function slot, and the
  original TED must keep working.
- **Command line.** Only `blitz2-ab3` has one (`ab3main.c:1-20`,
  `shared/ab3job/ab3args.c`): `-s SOURCE -e EXE [-r] [-mode ab3|bb2]`,
  single-dash long options as in AmiBlitz3, usage errors rc 20. It does
  not compile itself: it loads `PROGDIR:ab3cli` as the "editor"
  (`ext_editor`, `ab3main.c:163`), which loads the source and its options
  and calls the compiler's menu entry; the error is printed by `ab3cli`
  on the console (`Output()`) as `SOURCE:LINE: MESSAGE` and the return
  code is 10 (`programs/ab3cli/src/main.c:700-722`, spec
  `ab3-compat/command-line`). `blitz2` (BB2 mode) has no command line.
- **Where `loadres` runs.** Only when options are loaded or changed:
  `loadxtra_c` (`data.c:1481`, after TED loaded a source with its
  `.xtra`) and `optsdone` (`data.c:1623`, the options requester's OK
  after a change of the residents). Not at the start of a compile. A
  clash there is an `err`: `loadres` catches it with its own `setjmp`,
  drops all residents (`freeres`) and returns (`stmts.c:1301-1305`), the
  requester having been shown by `err`. In the `ab3cli` run the editor's
  message service is `error()` -> `ab3cli_loaderror` (`hooks.c:88`,
  `ab3cli/src/main.c:93`).
- **Options storage.** The option gadgets' `Flags` are the storage;
  `loadxtra`/`savextra` read and write them through the pointer table
  `optspnts` (`data.c:1401-1430`), 14 switch words at `.xtra` +2..+28
  (`docs/formats.md` 1.1; `shared/xtra/xtra.h:66-85`). Word +24 is
  `optreq2ga5`: not in the gadget chain (`ga4->next = ga8`,
  `data.c:740`), "meaning open", always `$0080` in all 575 `.xtra` of the
  CD (`docs/formats.md` 1.2), used by nothing but saved and restored by
  `runerropts` (`stmts.c:1329/1345`); `xtra_init` defaults it to ON
  (`shared/xtra/xtra.c:145`). The second requester (`optreq2`, 240x132)
  has four check boxes at y=24..60; `ga5` has a position (y=72), border
  and text slot already.
- **AmiBlitz3.** AB3's CLI has no `-Werror` (the options we refuse by
  name are listed in `ab3args.c:27`). The one AB3 warning documented in
  this repository ("detected unknown const" at an unknown escape
  sequence, `docs/ab3/variant.md:813`) is not reproduced: this port
  gives an error. So there is no AB3 behaviour to follow; AB3 mode takes
  the same mechanism.

## Goals / Non-Goals

**Goals:** warnings that never change a result; one place that decides
"warning or error"; the user always sees a warning; `-Werror` from the
shell and from the editor; no change of any result file.

**Non-Goals:** more warnings than #36; a changed `comdata` table;
silencing options; per-warning selection.

## Decisions

### D1. `warn(msg)` beside `err(msg)`

`void warn(const char *msg)` (`parse.c`, declared in `parse.h`):

```
if (dirmode) return;                 /* direct mode: silent, as err */
if (werror()) err(msg);              /* never returns */
else report the warning (D3)
```

`err` is unchanged, so a warning under "warnings are errors" is
byte-for-byte what the error would have been: same requester, same
`menuret`, same abort. Callers that find a warning-worthy condition call
`warn` and continue with their normal path, which is why `warn` returns.

### D2. The option as a predicate

`werror()` is `!(optreq2ga5.Flags & GFLG_SELECTED)` (D5). It is read at
the moment of the warning, so no state needs saving. In the command line
run `ab3cli` clears the flag after loading the options when `-Werror`
was given (D4), before the first `loadres`.

### D3. Reporting a warning: three paths

| Path | When | What the user sees |
|---|---|---|
| immediate | outside a compile (all of #36: `loadres`) | editor: the modal requester through `ed_error("Warning: MSG")`; the load goes on after OK |
| queued | during a compile (no such warning yet) | collected (a small fixed table: line, message), shown after the compile through `ed_error`, one requester per warning |
| console | `blitz2-ab3 -s ...` run | printed, see D4 |

Why the existing requester: it is the only service every editor has
(TED, `ab3cli`, the test stub `shared/edstub`), works without touching
TED or `comdata` (the original TED stays usable), and the user cannot
miss it. Its modal OK is acceptable for a rare warning; it does not
abort anything (unlike `err` there is no `longjmp`). The queue exists
because `ed_error` during a compile would sit under the progress
requester (`err` calls `endstop` first, `parse.c:555`); the first
compile-time warning needs it, #36 does not. Recommendation: implement
the immediate path and the console path now; keep the queued path as the
rule for later warnings and implement it with the first warning that
needs it (no code without a test). Open question Q4.

Rejected: a new `comdata` slot "warning" (breaks the original TED
binary, which must keep working with blitz2, and `docs/architecture.md`
3.2 treats the table as TED's contract); writing warnings to the console
from TED mode (the editor owns the screen); the title bar of the
progress requester (gone when the compile ends).

### D4. Console format and the extension point

New extension point in `ext.h` (group "errors, tokens"):

```
/** warn (parse.c): a warning in a command line run.
 *  @return TRUE if printed (the editor's requester is not used) */
BOOL ext_warning(const char *msg, const char *detail /* or NULL */);
```

`ext_bb2.c` returns FALSE (blitz2 has no command line). `ab3main.c`
returns TRUE in a command line run (`ab3job` set) and prints one line to
`Output()`, the stream the errors use:

```
SOURCE: warning: Clash in Residents: #c (r1.res, r2.res)
SOURCE:LINE: warning: MESSAGE            (a warning with a line, later)
```

The form follows gcc's `file:line: warning: text` and the existing error
line `SOURCE:LINE: MESSAGE`; "warning:" is what a script greps for. A
load-time warning has no line. `Output()` and not a separate stderr:
the errors use it too, so a script redirects both together. Outside a
command line run `ext_warning` returns FALSE (the `blitz2-ab3` editor
session behaves as `blitz2`).

Option: `-Werror` in `shared/ab3job/ab3args.c` (single dash,
case-insensitive like the other options, listed in the usage text).
`struct ab3job` gets a flag byte out of `pad` (`uint8_t werror`); its
size is unchanged and both sides are built from the same header.
`ab3cli` applies it: the options' switch for `optreq2ga5` is cleared
after `load_options`, before the resident load. In AB3 mode the same
applies; AB3's embedded header has no such option, so only the flag sets
it.

### D5. The option in the editor: the spare gadget `optreq2ga5`

Alternatives considered:

| | Option | `.xtra` effect | Verdict |
|---|---|---|---|
| A | make the hidden `optreq2ga5` visible (word +24) | none: byte-identical files | recommended |
| B | new field appended to `.xtra` | new format; the original TED/compiler and AB3 read garbage or reject; `shared/xtra`, `formats.md`, the AB3 conversion change | rejected |
| C | an option in the source (directive or comment) | a new language element; BB2 compatibility says no new commands | rejected |
| D | only the command line, no editor option | no way to get it from TED | rejected |
| E | environment variable | invisible, not per source | rejected |

For A the old files all hold `$0080` in +24 (575 files; the default of
`xtra_init`). If selected meant "warnings are errors", every existing
source would silently turn on `-Werror`. So the gadget's sense is the
old default: selected = warnings stay warnings, label "Warnings only";
deselected = warnings are errors. `savextra` keeps writing the gadget's
flags: the saved `.xtra` is byte-identical for every source whose option
is untouched. A box labelled the other way round would need an inversion
in `loadxtra_c`/`savextra_c` and in `shared/xtra`; rejected for the risk
to a result file (Q2 lets the user choose).

Layout: chain `ga4 -> ga5 -> ga8` (`data.c:740-741`); `ga5` has its
border (`optreq2bo7`) and text slot (`optreq2in13`/`optreq2la34`, 3
bytes today); the label text follows the pattern of the other four rows
(`optreq2in1..5`, `optreq2la7..11`; the fifth row at y=74). Whether the
window height (132) holds the row above OK/Cancel is checked on the
emulator in the implementation (the hidden `ga6/ga7` at y=84/96 stay
hidden).

### D6. Return codes and result

| Situation | Editor | `blitz2-ab3 -s/-e` |
|---|---|---|
| no warnings | as today | rc 0 |
| warnings, option off | warning shown, compile/load goes on | rc 0, executable written |
| warnings, `-Werror` / box deselected | the warning is an error (D1) | rc 10; a load-time warning (#36) drops the residents like the existing clash error for types, and the compile then fails on the missing constants or runs without them |

A warning never changes the executable. rc 0 follows gcc; AmigaDOS has
`RETURN_WARN` (5), and a script with `failat 5` would stop on it.
Open question Q3.

### D7. The #36 warning

**What clashes.** After the loop in `loadres` the list `firstlabel`
holds exactly the residents' constants (`freeres` sets `lastrescon =
&firstlabel`, `fixconsts` appends, nothing else writes `firstlabel`
during `loadres`). A new `checkconstclash()` (`stmts.c`) walks it with
the label layout (name at offset 19, value in `pc`, `structs.h:52`) and,
for every name that occurs twice, warns when the values differ. Same
name and same value is the same definition included twice (typical for
residents made from the same header): harmless, and warning for it would
make the check noisy. Types and macros stay errors for any repeat: their
bodies cannot be compared cheaply. Open question Q1: warn for equal
values as well?

**Which message.** One warning per clashing name, not per pair:
`Clash in Residents: #NAME`. The editor shows that text only; the command
line run appends the two resident files (`resbuff` names plus the range
each `fixconsts` call appended, recorded in `loadres`'s loop as the
`lastrescon` before and after each entry). More than 8 clashing names:
the first 8, then one warning "Clash in Residents: N more". No line:
`loadres` runs when the options load.

**Which wins.** As today the first resident's definition (the compiler's
lookup finds the first match): the executable does not change with the
fix. Documented in the deviations entry.

**Where `warn` is called.** After `checkclash` for types and macros in
`loadres`, inside the same error context (`errjmp = &jb`): with the
option on, `warn` becomes `err`, the existing handler `freeres`es and
returns, exactly as for a clashing type. Without it `loadres` returns
with all residents loaded.

**Check of the CD (task 6.1).** Run `corpus_cd` (724 sources, their
`.xtra` residents) before and after: no executable may change (the table
`tests/data/blitz2-cd.tsv` holds result and hash), and every source whose
residents now warn is listed (commit message and `TODO.md`). If many
sources warn with differing values, Q1 and the severity are reconsidered
before the merge.

### D8. Tests

| Test | What |
|---|---|
| `compiler-loadres-clash-offset` (bug-orig) | `; @fixed #36`; `.expect` has the requester line `Warning: Clash in Residents: #c` and the program prints `1` (the first resident's value); `.orig` stays as the record |
| `compiler-loadres-clash-werror` (new) | the same residents with the option on: the error "Clash in Residents" and no `#c`; needs a job-line key `werror=on` in `tests/corpus/runner` that clears `optreq2ga5` in the start options |
| `compiler-loadres-clash-same` (new) | same name and same value in two residents: no message |
| `ab3_cli` cases | `-Werror` accepted and listed by `-h`; the console line of a warning; rc 0 without and rc 10 with the option |
| host test `ab3job` (`tools/lib/ab3job`) | `-Werror` parse, case, combination with the other options; a deliberately broken input |
| `.xtra` identity | `prev_results`, `ted_ab` `P5`/`Pc` and the `xtra` host tests: a saved `.xtra` is unchanged; toggling the gadget changes exactly the byte at +25 |
| options requester | the check box exists and toggles (the existing way requesters are tested is looked up in the implementation) |

### D9. Documentation

`docs/ab3/usage.md` (option list), the `-h` text, `docs/v2.10/blitz2.md`
(second options requester), `docs/formats.md` 1.1 (+24 has a meaning
now), `docs/architecture.md` 3.6 (warnings), `docs/*/deviations.md`
(kind F, #36), `tests/README.md` (the job-line key), `TODO.md`.

## Risks / Trade-offs

- The modal requester for a warning at `.xtra` load time pops up when a
  source with clashing residents is opened; several such sources give one
  requester per load. Acceptable: the error did the same.
- Reusing `ga5` gives the "meaning open" word a meaning: a tool that wrote
  something else than `$0000`/`$0080` there was not found in the CD files;
  the compiler tests the SELECTED bit only.
- `-Werror` with a load-time warning drops all residents (existing
  behaviour for an error): follow-up errors may confuse; the first
  message names the clash.
- Equal-value constants do not warn (D7); a user may want them to.

## Open Questions

1. Q1, equal values: warn only when the values differ (recommended), or
   for any repeated name like types and macros?
2. Q2, gadget sense: "Warnings only", selected by default and `.xtra`
   byte-identical (recommended), or "Warnings are errors" with an
   inversion at load and save?
3. Q3, return code with warnings: 0 as gcc (recommended) or 5
   (`RETURN_WARN`)?
4. Q4, the queued path: with the first compile-time warning
   (recommended) or now, without a user?
5. Q5, option name: `-Werror` (recommended: gcc's, the user's wording) or
   an AmiBlitz3-style `-werr`?
6. Q6, if `corpus_cd` finds sources that warn: keep the warning for them
   (recommended, they compile unchanged) or list them as known clashes?
