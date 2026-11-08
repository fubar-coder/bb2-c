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
  single-dash options as in AmiBlitz3, usage errors rc 20 (message and
  usage, `AB3ARGS_FAIL`). It does not compile itself: it loads
  `PROGDIR:ab3cli` as the "editor" (`ext_editor`, `ab3main.c:163`), which
  loads the source and its options and calls the compiler's menu entry;
  a compile error is printed by `ab3cli` on the console (`Output()`) as
  `SOURCE:LINE: MESSAGE` and the return code is 10; an error outside the
  menu call (loading libraries or residents) is a set-up error: printed
  as `SOURCE: MESSAGE`, rc 20 (`programs/ab3cli/src/main.c:105`,
  `run`, spec `ab3-compat/command-line`). `blitz2` (BB2 mode) has no
  command line.
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
  has four check boxes at y=24..60 (toggle gadgets, Activation
  `TOGGLESELECT|RELVERIFY`; `runerropts` gets their ids from
  `ed_getinput` and ignores ids below 8); `ga5` has a position (y=72),
  border and text slot already. `ed_refgads` (`comdata+28`) redraws
  gadgets of the open requester.
- **AmiBlitz3.** AB3's CLI has no warning options (the options we refuse
  by name are listed in `ab3args.c:27`). The one AB3 warning documented
  in this repository ("detected unknown const" at an unknown escape
  sequence, `docs/ab3/variant.md:813`) is not reproduced: this port
  gives an error. So there is no AB3 behaviour to follow; AB3 mode takes
  the same mechanism.

## Goals / Non-Goals

**Goals:** warnings that never change a result; named warnings, each of
which can be disabled or made an error; one place that decides
"silent, warning or error"; the user always sees an enabled warning;
the settings from the shell and from the editor; no change of any result
file.

**Non-Goals:** more warnings than #36; a changed `comdata` table; the
queue for compile-time warnings (D3); per-warning settings in the editor
(D5).

## Decisions

### D1. Named warnings, two sets, `warn()` beside `err()`

Every warning has a number and a name; the first is `resclash` (number
0, the #36 check, D7). The numbers and names are one table in
`shared/ab3job/ab3job.h` (`AB3JOB_W_*`, `ab3args_warnings[]`), which the
command line parser and `blitz2-ab3` share; the compiler core has the
numbers in `parse.h` (`W_RESCLASH`), and `ab3main.c` checks with
`static_assert` that both agree.

The compiler keeps two sets (bit masks over the numbers): the
**enabled** warnings and the warnings that are **errors**. A disabled
warning is never an error (`errors &= enabled`). Default: all enabled,
none an error.

`void warn(UWORD w, const char *msg, const char *detail)` (`parse.c`,
declared in `parse.h`):

```
if (dirmode) return;                 /* direct mode: silent, as err */
warnsets(&enabled, &errors);         /* D2 */
if (!(enabled & 1 << w)) return;     /* disabled: silent */
if (errors & 1 << w) {
    ext_warning(w, msg, detail, TRUE);   /* the command line counts it */
    err(msg);                        /* never returns */
}
if (!ext_warning(w, msg, detail, FALSE))
    ed_error("Warning: " msg);       /* the immediate path, D3 */
```

`err` is unchanged, so a warning that is an error is byte-for-byte what
the error would have been: same requester, same `menuret`, same abort.
Callers that find a warning-worthy condition call `warn` and continue
with their normal path, which is why `warn` returns. `detail` (or NULL)
is shown only on the console (D4).

### D2. Where the sets come from

`warnsets()` (`parse.c`) computes the sets at the moment of the warning,
so no state needs saving:

1. the editor's setting, the word `warnopt` (`.xtra` +24, D5): off -
   none enabled; on - all enabled, none an error; errors - all enabled,
   all errors;
2. then `ext_warnsets(&enabled, &errors)`: `ext_bb2.c` changes nothing;
   `ab3main.c` applies the job's lists of a command line run (D4);
3. `errors &= enabled`.

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
requester (`err` calls `endstop` first, `parse.c:555`).

Decided (Q4, 2026-11-08): the immediate path and the console path now;
the queued path is the rule for later warnings and is implemented with
the first compile-time warning (no code without a test).

Rejected: a new `comdata` slot "warning" (breaks the original TED
binary, which must keep working with blitz2, and `docs/architecture.md`
3.2 treats the table as TED's contract); writing warnings to the console
from TED mode (the editor owns the screen); the title bar of the
progress requester (gone when the compile ends).

### D4. The command line: `-w`, `-werr`, the console form

**Options** (`shared/ab3job/ab3args.c`), in AmiBlitz3's style (short,
lowercase, single dash, case-insensitive like the other options, an
argument like `-mode bb2`):

```
-w LIST      the enabled warnings
-werr LIST   the warnings that are errors
LIST         comma-separated items, applied left to right:
             all | none | NAME (add) | -NAME (remove)
```

Examples: `-w all,-resclash` and `-w -resclash` (no clash warning),
`-werr resclash` (the clash is an error), `-w none` (silent). A list
changes the source's setting (D2 step 1; the default: all enabled, none
an error), as `-r` changes the source's other options: `NAME` and
`-NAME` add to and remove from it, `all` and `none` replace it. Given
twice, the lists are applied in order (as one list). An unknown name, an
empty item or a missing argument is an argument error: the message
(`unknown warning "NAME" in -w (known: resclash)`) and the usage,
return code 20, as the other argument errors. The usage text lists both
options and the names.

**In the job.** Left to right the items of a list compose to
`set' = (set & keep) | add` (`all`: keep 0, add all; `none`: keep 0,
add 0; `NAME`: add the bit; `-NAME`: clear it in keep and add).
`struct ab3job` gets `uint32_t warn_keep, warn_add, werr_keep,
werr_add` (default keep all, add 0: no change) and `uint8_t warnerrs`
(the warnings that became errors, counted by `blitz2-ab3`); both sides
are built from the same header. `ab3main.c`'s `ext_warnsets` applies
the masks in a command line run.

**Console form.** New extension point in `ext.h` (group "errors,
tokens"):

```
/** warn (parse.c): warning w with the text msg (detail or NULL: more
 *  for the console); error: it is an error (err follows).
 *  @return TRUE if printed (the editor's requester is not used) */
BOOL ext_warning(UWORD w, const char *msg, const char *detail, BOOL error);
```

`ext_bb2.c` returns FALSE (blitz2 has no command line). `ab3main.c`, in
a command line run (`ab3job` set): a warning is printed as one line on
`Output()`, the stream the errors use, and TRUE is returned:

```
SOURCE: warning: Clash in Residents: #c (r1.res, r2.res) [resclash]
SOURCE:LINE: warning: MESSAGE [NAME]     (a warning with a line, later)
```

The form follows gcc's `file:line: warning: text [-Wname]` and the
existing error line `SOURCE:LINE: MESSAGE`; "warning:" is what a script
greps for, the name says what to pass to `-w`. A load-time warning has
no line. For an error it counts `warnerrs` and returns FALSE: `err`
follows, `ab3cli` prints it as every error outside the compile
(`SOURCE: Clash in Residents: #c`) and, because all of its set-up errors
were warnings that are errors (`setuperr == warnerrs`), ends with return
code 10 instead of 20 (D6). Outside a command line run `ext_warning`
returns FALSE and `ext_warnsets` changes nothing (the `blitz2-ab3`
editor session behaves as `blitz2`). In AB3 mode the same applies; AB3's
embedded header has no such option.

### D5. The setting in the editor: the spare word +24

Alternatives considered:

| | Option | `.xtra` effect | Verdict |
|---|---|---|---|
| A | give the hidden `optreq2ga5`'s word +24 a meaning | none: byte-identical files | chosen |
| B | new field appended to `.xtra` | new format; the original TED/compiler and AB3 read garbage or reject; `shared/xtra`, `formats.md`, the AB3 conversion change | rejected now, see below |
| C | an option in the source (directive or comment) | a new language element; BB2 compatibility says no new commands | rejected |
| D | only the command line, no editor option | no way to get it from TED | rejected |
| E | environment variable | invisible, not per source | rejected |

Decided (Q2+Q5, 2026-11-08): one three-state setting for all warnings
in the second options requester: **off / on / errors**.

**The word.** `optspnts` points to a word of its own, `warnopt`
(`data.c`), instead of `optreq2ga5.Flags`; it holds the setting as it is
stored in `.xtra` +24:

| Word | Setting |
|---|---|
| `$0080` (bit 7 set, bit 8 clear) | on (the old default; all 575 CD files, `xtra_init`) |
| `$0000` (bit 7 clear) | off |
| `$0180` (bits 7 and 8 set) | errors |

Read by bits (any word with bit 7 clear is off, so a disabled warning is
never an error), written only when the user changes the setting (then
the normalized value). A source whose setting is untouched keeps the
word it was loaded with: its `.xtra` is saved byte-identical. The word
fits three states without a new field, so B is not needed now.
`xtra_is_ab3_xtra` (`shared/xtra/xtra.c:373`) takes a word with a bit
outside `$80` as "not AmiBlitz3's format", which is right for `$0180`
(a Blitz 2 file).

**The gadget.** `optreq2ga5` becomes a button (Activation `RELVERIFY`,
no `TOGGLESELECT`; GadgetID 5) in the chain `ga4 -> ga5 -> ga8`, 56
pixels wide at x=168, y=72; its text shows the setting (`  OFF `,
`  ON  `, `ERRORS`, the same length, drawn with JAM2), the label
`WARNINGS:` at (20, 74) as the fifth row of the other four
(`optreq2in1..5`). `runerropts` sets the text from `warnopt` before
it opens the requester; a click (id 5 from `ed_getinput`) steps the
setting off -> on -> errors -> off and redraws the gadget
(`ed_refgads`); Cancel restores `warnopt` with the other options. The
hidden `ga6/ga7` (y=84/96) stay hidden. Whether the window (240x132)
holds the row is checked on the emulator.

**Per-warning settings in the editor** come with the second warning: a
new `.xtra` field (alternative B) then holds the two sets; until then
the three-state word covers the only warning.

### D6. Return codes and result

| Situation | Editor | `blitz2-ab3 -s/-e` |
|---|---|---|
| no warnings | as today | rc 0 |
| a warning disabled | nothing | rc 0 |
| a warning enabled | warning shown, compile/load goes on | console line, executable written, rc 0 |
| a warning that is an error | the warning is an error (D1) | the error line, rc 10; a load-time warning (#36) drops the residents like the existing clash error for types; nothing is compiled |

A warning never changes the executable. Decided (Q3, 2026-11-08): rc 0
with warnings, as gcc (AmigaDOS's `RETURN_WARN` (5) would stop a script
with `failat 5`); a warning that is an error is an error, rc 10.

### D7. The #36 warning `resclash`

**What clashes.** After the loop in `loadres` the list `firstlabel`
holds exactly the residents' constants (`freeres` sets `lastrescon =
&firstlabel`, `fixconsts` appends, nothing else writes `firstlabel`
during `loadres`). A new `checkconstclash()` (`stmts.c`) checks it with
the label layout (name at offset 19, value in `pc`, `structs.h:52`): for
every name that occurs more than once it warns when a later value
differs from the first definition. Same name and same value is the same
definition included twice (typical for residents made from the same
header): harmless, no message (decided, Q1, 2026-11-08). Types and
macros stay errors for any repeat: their bodies cannot be compared
cheaply.

**How.** Big residents hold thousands of constants (the CD's
`amigalibs.res`), so the check is a hash table, not the O(n^2) walk of
`checkclash`: one `AllocMem` block for the buckets and the first
definitions, freed at the end; only when at least two residents
appended constants. If the block cannot be allocated the check is
skipped (no message).

**Which message.** One warning per clashing name, not per pair:
`Clash in Residents: #NAME`. The editor shows that text only; the
command line run gets the two resident files as detail (`resbuff` names
and the range each `fixconsts` call appended, recorded in `loadres`'s
loop). More than 8 clashing names: the first 8, then one warning
"Clash in Residents: N more". No line: `loadres` runs when the options
load.

**Which wins.** As today the first resident's definition (the compiler's
lookup finds the first match): the executable does not change with the
fix. Documented in the deviations entry.

**Where `warn` is called.** After `checkclash` for types and macros in
`loadres`, inside the same error context (`errjmp = &jb`): if
`resclash` is an error, `warn` becomes `err`, the existing handler
`freeres`es and returns, exactly as for a clashing type. Otherwise
`loadres` returns with all residents loaded.

**Check of the CD (task 6.1).** Run `corpus_cd` (724 sources, their
`.xtra` residents) before and after: no executable may change (the table
`tests/data/blitz2-cd.tsv` holds result and hash), and every source whose
residents now warn is listed (commit message, report). Decided (Q6,
2026-11-08): such sources keep the warning; there is no list of known
clashes.

### D8. Tests

| Test | What |
|---|---|
| `compiler-loadres-clash-offset` (bug-orig) | `; @fixed #36`; `.expect` has the requester line `Warning: Clash in Residents: #c` and the program prints `1` (the first resident's value); the `.orig` is deleted (the fix workflow) |
| `compiler-loadres-clash-werror` (new) | the same residents with the setting "errors": the error "Clash in Residents: #c", the residents dropped, the compile fails on `#c`; needs a job-line key `warnings=off\|on\|errors` of the test stub (`tests/corpus/amiga/tedstub.c`) that writes word +24 of the defaults |
| `compiler-loadres-clash-off` (new) | the setting "off": no message, `#c` is 1 |
| `compiler-loadres-clash-same` (new) | same name and same value in two residents: no message |
| `compiler-loadres-clash-many` (new) | 10 clashing names: 8 warnings and "Clash in Residents: 2 more" |
| `ab3_cli` cases | `-w`/`-werr` listed by `-h`; the console line of a warning with rc 0; `-werr resclash`: the error line, rc 10; `-w -resclash`: silent, rc 0; an unknown name: rc 20 |
| host test `test_ab3args` | the lists: `all`, `none`, `NAME`, `-NAME`, order, case, two options, the masks; broken input: unknown name, empty item, missing argument |
| `.xtra` identity | `prev_results`, `ted_ab` `P5`/`Pc` and the `xtra` host tests: a saved `.xtra` is unchanged |
| options requester | an emulator screenshot of TED with the second requester (layout); the state cycle by the test stub is not scripted (it answers requesters with their end gadget) |

### D9. Documentation

`docs/ab3/usage.md` (option list), the `-h` text, `docs/v2.10/blitz2.md`
(second options requester), `docs/formats.md` 1.1 (+24 has a meaning
now), `docs/architecture.md` 3.6 (warnings), `docs/libs/deviations.md`
(kind F, #36), `tests/README.md` (the job-line key).

## Risks / Trade-offs

- The modal requester for a warning at `.xtra` load time pops up when a
  source with clashing residents is opened; several such sources give one
  requester per load. Acceptable: the error did the same.
- Reusing word +24 gives the "meaning open" word a meaning: a tool that
  wrote something else than `$0000`/`$0080` there was not found in the CD
  files; the compiler reads bits 7 and 8 only.
- A warning that is an error at load time drops all residents (existing
  behaviour for an error): follow-up errors may confuse in the editor;
  the first message names the clash.
- Equal-value constants do not warn (D7).

## Decisions of 2026-11-08 (the questions Q1-Q6)

1. Q1: only differing values warn; same name and same value is silent.
2. Q2+Q5: named warnings with two sets (enabled, errors; a disabled
   warning is never an error; default all enabled, none an error);
   command line `-w LIST`, `-werr LIST`; the editor's one three-state
   setting off / on / errors in word +24 (D4, D5).
3. Q3: rc 0 with warnings; a warning that is an error gives rc 10.
4. Q4: the queued path only with the first compile-time warning.
5. Q6: CD sources that newly warn keep the warning; no list of known
   clashes.

## Findings of the implementation

- **Layout** (task 2.1): an emulator screenshot of TED (blitz2 with the
  real TED on the A1200 configuration, driven to the second requester by
  synthetic IntuiMessages, a throw-away helper) shows the row
  "WARNINGS:" with the 56-pixel button under OVERFLOW CHECKING, inside
  the frame and above OK/CANCEL; the three texts are drawn in place when
  the button is stepped.
- **Documentation place**: `docs/v2.10/blitz2.md` compares the binaries
  and has no description of the options requesters; the setting is
  described in `docs/architecture.md` 3.6 and `docs/formats.md` 1.1
  (D9), the command line in `docs/ab3/usage.md`.
- **Command line tests**: the residents of the `ab3_cli` cases are made
  in the build by `tools/blitzres` (`wclash1.res`, `wclash2.res`) and
  named in the embedded header of `wclash.ab3` (AB3 mode); the
  tedstub-based tests (`tests/bug-orig/compiler/compiler-loadres-clash-*`)
  compile their residents in the session.
- **The CD** (task 6.1): `corpus_cd` passes unchanged with the check -
  no executable changes and no source of the CD gets the warning (its
  logs have no "Warning: Clash in Residents"); `tests/data/blitz2-cd.tsv`
  needs no update.
- **Task 1.3** stays open: the queued path is decided to come with the
  first compile-time warning (Q4), outside this change.
