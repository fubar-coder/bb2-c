# amiga_run

Runs an AmigaDOS script in Amiberry, without a window and isolated (its
own work directory, read-only hardfile), so several can run at the same
time; the script sees its directory as `TEST:`. It is the by-hand and
CMake-script front end of `tools/lib/amiga_emulation`, whose README
([`tools/lib/amiga_emulation/README.md`](../lib/amiga_emulation/README.md))
describes the machines, the session states, the emulator slots and the
snapshot mode. Needs Amiberry in `PATH` and the Kickstart ROM and
hardfiles in `amiga-emulation/` (`AMIGA-EMULATION-SETUP.md`).

## Usage

```
usage: amiga_run [--machine NAME] [--rtg] [--visible] [--log] [--timeout SEC]
                 [--set KEY=VALUE]... [--unset KEY]... [--mount VOL=DIR]...
                 [--mount-ro VOL=DIR]... [--dir WORKDIR] [--cold] SCRIPT|-
       amiga_run --purge-snapshots [OWNER]
```

Runs the script `SCRIPT` (a file, or `-` for stdin):

* `--machine NAME`: `a1200` (the default), `a500` (68000, Kickstart 1.3),
  `a500-68010`, `a600` (68000, Kickstart 3.1);
* `--rtg`: the Workbench hardfile with an RTG card;
* `--visible`: the Amiberry window (debugging);
* `--log`: Amiberry's own log into `WORKDIR/amiberry.log` (with `--dir`);
* `--timeout SEC`: the time limit of the session, boot or restore
  included (default 60);
* `--set KEY=VALUE`, `--unset KEY`: changes of the machine's
  configuration (`tools/lib/amiga_emulation/default.uae` and the other
  templates);
* `--mount VOL=DIR`, `--mount-ro VOL=DIR`: a host directory as the volume
  `VOL:`, writable or read-only;
* `--dir WORKDIR`: the work directory (`TEST:` is `WORKDIR/test`); it
  stays. Without it the work directory is temporary: the files the
  script wrote to `TEST:` are printed and the directory is removed;
* `--cold`: boot, do not start from a snapshot.

`--purge-snapshots [OWNER]` removes the snapshots of `OWNER` (a build
directory; without it `$AMIGA_EMULATION_OWNER`, unset: the snapshots
without owner, those of `amiga_run` by hand) from all slots and nothing
else. The tests keep their snapshots across runs, and a session discards
one whose inputs have changed by itself; this is for manual cleanup.

## Inputs and outputs

The script is AmigaDOS script text. With `--dir` the work directory has
the session's files (`test/` = `TEST:`, Amiberry's configuration and,
after a time limit or a halted CPU, `timeout.png` and `timeout.txt`); the
program prints `work directory: DIR`. Without `--dir` it prints the
files of `TEST:`.

## Exit codes

* 0 the script ran to its end (`--purge-snapshots`: always, also without a
  slot directory);
* 1 the session did not end with the script (time limit, halt, early
  exit), said on stderr;
* 2 usage error, the script cannot be read;
* 77 the emulator cannot start (Amiberry or its data missing: skipped).

Messages go to stderr as `amiga_run: message` (a script that cannot be
opened: `SCRIPT: reason`).

## Callers

The emulator tests that run a script themselves: `libs/tests`
(`CMakeLists.txt`, `emulator.cmake`: the library tests), the TED oracle
(`programs/ted/tests/oracle`), `tests/ab3` (`cli.cmake`, `tokenise.cmake`),
`tests/bug-orig` (the inject tests, `inject-fill.cmake`,
`inject-datestamp.cmake`), `tests/bug-port` (`run.cmake`), the `-h`
tests of the Amiga programs (`cmake/check-help-amiga.cmake`). The
runners (`corpusrun`, `tedab`) use the library directly. Tests:
`help_amiga_run`, `readme_amiga_run`.

## Example

```
$ echo 'Version >TEST:v.txt' | build/amiga/host/tools/amiga_run/amiga_run -
```
