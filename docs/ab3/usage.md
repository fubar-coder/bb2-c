# blitz2-ab3: AmiBlitz3 sources

`blitz2-ab3` (in `build/<preset>/blitz2/`, with `ab3cli` next to it) is
the compiler extended by AmiBlitz3's mode (docs/ab3/variant.md): it
compiles a Blitz Basic 2 source exactly as `blitz2` and an AmiBlitz3
source with AmiBlitz3 v3.14.0's token list and its binary libraries (AB3 mode). Started
without options it is the editor session of `blitz2` (always BB2 mode);
from the shell it compiles one source without the editor, with the
option names of AmiBlitz3's CLI mode:

```
blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2] [-w LIST] [-werr LIST]
```

| Option | |
|---|---|
| `-s`, `-source FILE` | the source: text (tokenised while loading, as TED does) or tokenised |
| `-e`, `-exe FILE` | the executable to write |
| `-r`, `-release` | no runtime error checking, no debugger, no debug information |
| `-mode ab3\|bb2` | force the mode (an extension of this repository) |
| `-w LIST` | the warnings that are shown (an extension of this repository) |
| `-werr LIST` | the warnings that are errors (an extension of this repository) |
| `-h`, `-help` | the usage |

**Warnings.** The compiler's warnings have names (so far one,
`resclash`: a constant defined with different values by two residents).
It keeps two sets: the warnings that are shown and the warnings that are
errors; a warning that is not shown is never an error. The source's
setting (the editor's "WARNINGS: OFF / ON / ERRORS" in the second
options requester, `.xtra` word +24; the default ON: all shown, none an
error) is the start; `-w` and `-werr` change it. A LIST is
comma-separated items, applied left to right: `all`, `none`, `NAME`
(add), `-NAME` (remove); several `-w` (`-werr`) are applied in order.
Examples: `-w -resclash` or `-w all,-resclash` (the clash is not
reported), `-werr resclash` (it is an error). A warning is printed as
`SOURCE: warning: MESSAGE [NAME]` (with `:LINE` when it has a line); a
compile with warnings and no error ends with return code 5 (AmigaDOS
`RETURN_WARN`: with the Shell's default `FailAt 10` a script goes on
and can test it with `If WARN`; `FailAt 5` makes warnings stop it). A
warning that is an error is printed and counted as an error (return
code 10). An unknown name or an empty item is an argument error (20).

The mode is AB3 for a text whose first line is AmiBlitz3's embedded
header (`; XTRA`), for a file named `*.ab3`, and for a tokenised source
whose companion `.xtra` has AmiBlitz3's format; anything else is BB2.
The options of the compile come from the embedded header, else from the
companion `SOURCE.xtra`, else they are the compiler's own. Includes of
text are tokenised the same way with the program's token list; a
tokenised include of the other mode is refused. Relative names
(source, executable, includes) are relative to the shell's current
directory. Errors go to the console – a compile error as
`FILE:LINE: MESSAGE`, inside an include with the include's name and
line – and the return code is 0 (compiled without warnings), 5
(compiled with warnings), 10 (compile error, or a warning that is an
error; also when warnings came before it) or 20 (argument or set-up
error). AmiBlitz3's other options
(`-d`, `-run`, `-dump`, `-PED`, `-save`, …) are refused with 20.

Structure (docs/ab3/variant.md, "Structure"): both programs link one
core, the chunk files of `programs/blitz2/src` compiled once (CMake
object library `blitz2_core`). Where AB3 mode differs, the core calls an
extension point declared in `src/ext.h`: `blitz2` links `src/ext_bb2.c`
(BB2 only, nothing handled), `blitz2-ab3` the AB3 modules `src/ab3*.c`
(`ab3main.c` command line and editor, `ab3libs.c` libraries, `ab3res.c`
residents, `ab3inc.c` includes, `ab3tok.c` mode and token list; their
shared state in `ab3.h`) with `shared/xtra` and `shared/ab3job`. New AB3
behaviour (G2 on) goes behind such a point, not into the chunk files;
the link of both programs refuses `BLITZ2_AB3` in `src/`
(`programs/blitz2/checkcore.cmake`).

AB3 mode needs AmiBlitz3's files under `Blitz3:` (`BlitzLibs/baseLibs`,
`utilLibs`, `OsLibs`, `csLibs`, `BlitzLibs/UserLibs/`,
`BlitzLibs/Residents/`); BB2 mode the `blitz2:` directory as `blitz2`.
Example, with `blitz2:` the directory of `build/<preset>/blitz2/` and
`Blitz3:` an AmiBlitz3 v3.14.0 directory (`archive/amiblitz3/github-58a11658/`
after `fetch-originals` has its `BlitzLibs/` and the test program used
here):

```
cd Blitz3:
blitz2:blitz2-ab3 -s Sourcecodes/Amiblitz3/Tests/atoi.ab3 -e RAM:atoi
RAM:atoi
```

`ab3cli` is the headless editor of such a run (`programs/ab3cli`: the
edstub frame, TED's core for loading and tokenising, `shared/xtra` for
the header); `blitz2-ab3` loads it as `PROGDIR:ab3cli` instead of
`blitz2:ted`. It needs Kickstart 2.0 (`PROGDIR:`), the editor session
does not.

The emulator test `ab3_cli` (`tests/ab3/cli.cmake`) compiles AmiBlitz3's
test programs and the cases of `tests/ab3/cli/` this way, runs them and
compares the console output, the programs' output and the return codes
with expect files (`results match`, `tools/results`; docs/ab3/variant.md, "The command
line"); `ab3_cli_ref` compiles the AmiBlitz3 cases with AmiBlitz3
v3.14.0's own compiler in CLI mode and compares the programs' output
with the same files (its differences: docs/ab3/variant.md).
`ab3_tokenise` (`tests/ab3/tokenise.cmake`) compares the tokens of AB3
mode with AmiBlitz3's: AmiBlitz3 writes the lines it loaded (`-save
NAME.bb2`), `tools/ab3tok` tokenises the same sources with
`shared/tokenizer` (docs/ab3/variant.md, "AB3 mode's tokeniser").
