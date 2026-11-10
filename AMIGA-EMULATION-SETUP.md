# Setting up the emulator tests

The emulator tests (`ctest --preset emulator`, `all`, `bug-orig`) run the
Amiga programs in Amiberry. Kickstart and AmigaOS must not be
redistributed, so everyone sets up their own copies in `amiga-emulation/`
(not checked in). Without Amiberry or without these files the emulator
tests are skipped (return value 77). How the test runs use the emulator:
`tools/lib/amiga_emulation/README.md`; the test suites: `tests/README.md`.

## Amiberry

Amiberry ≥ 8.4 in `PATH`. The configurations of the machines below were
taken from Amiberry 8.5.0.

## Machine a1200 (default): Kickstart 3.1, AGA

The a1200 (and the a600, below) run on Amiga Forever 11's Kickstart 3.1
and Workbench 3.1, the reference set of the emulator tests. The setup
from Amiga Forever (next section) stores them in `amiga-emulation/`:
`amiga-os-310-a1200.rom` (Kickstart 40.68, encrypted, with `rom.key`),
`workbench-311.hdf` (the Workbench on the native screen, the default) and
`workbench-311-rtg.hdf` (the same with the uaegfx monitor).

### Configuration

`tools/lib/amiga_emulation/default.uae` is the default configuration
(A1200, AGA, 68020/68882, 8 MB chip, 128 MB Z3 fast, `workbench-311.hdf`,
the JIT with `comp_flushmode=hard`).
Tests change it only by key=value differences. The JIT's flush mode is
`hard` because Amiberry's `soft` mode keeps a translated block after
`CacheClearU` when a checksum of its words (their sum and xor) is
unchanged: it misses code whose words were only rearranged, so a program
loaded at the address of a different one could run the old program's
translated code from a block's 6th execution on. Any session that starts
different programs in turn can meet it; the `ted_ab` combinations
`P5`/`Pc` do (the current and the previous blitz2 at the same address:
with `soft` the previous one ends at once with return code 260 or the
machine reboots). `hard` drops every translated block at `CacheClearU`
and costs `ctest --preset all` about a tenth more time. The RTG variant:

```
gfxcard_size=16
gfxcard_type=ZorroIII
hardfile2 / uaehf0: workbench-311-rtg.hdf instead of workbench-311.hdf
```

A test uses it with `amiga_run --rtg` or, in a corpus pass (a bug-orig
test), with `rtg=on` (`; @pass rtg=on`, e.g. `freqlib-64-lines`: a
uaegfx screen taller than the native modes).

TED can only be tested with the AGA variant (on RTG its bitplanes would be
in graphics card memory, where the blitter cannot reach them).

### What the tests bring along

Nothing is installed on the Workbench besides `S:User-Startup` and
`ENVARC:Sys/font.prefs` (see "What the script changes"); what a
test needs beyond Workbench 3.1, it brings itself:

* AmiBlitz3 v3.14.0 (`ab3_cli_ref`, `ab3_tokenise`) opens its
  `ab3pft.library`: fetched with the other AmiBlitz3 files
  (`cmake/fetch-originals.cmake`, the AmiBlitz3 repository, Apache-2.0)
  and copied to the test's `Blitz3:Libs`, which AmiBlitz3 adds to `LIBS:`.
* The TED A/B scripts type umlauts with the German keymap: `tedprobe`
  (gate 0) makes `DEVS:Keymaps/d` the default keymap of the session.

### Another Kickstart or Workbench

A Kickstart 3.2 ROM and a Workbench 3.2 hardfile (also `DOS\5`) work as
well, in an emulation directory of their own (`AMIGA_EMULATION_DIR`,
below) under the names above and with the conditions below; they are not
needed. Some results depend on the system and expect 3.1 there: the
Kickstart version the `amiga_emulation_*` tests check (40), the
executables of `corpus_cd`'s table that use float constants (the compiler
evaluates them with the ROM's `mathffp.library`, whose last bit differs
between Kickstart 3.1 and 3.2: `LINE_TORUS*`), the volume name `Ram Disk`
and the float results of `mathtrans.library` in AmiBlitz3's test programs
(their expect files allow both).

## Setup from Amiga Forever 11

Amiga Forever 11 (Cloanto), which every developer buys for themselves,
has Kickstart ROMs and Workbench hardfiles. The script
`cmake/setup-amiga-forever.cmake` copies them from the Amiga Forever
medium (or installation) and adapts the hardfiles for the tests, with
CMake and `hdftool` (`tools/hdftool`, a host tool of this build) only:

```
cmake --preset amiga && cmake --build --preset amiga      # builds hdftool
cmake -DAMIGA_FOREVER=/run/media/$USER/AmigaForever -P cmake/setup-amiga-forever.cmake
```

It sets up all machines: the a1200 and a600 (Kickstart and Workbench 3.1)
and the a500 (Kickstart 1.3, Workbench 1.3.5).

`AMIGA_FOREVER` is the medium or installation (a directory with
`Amiga Files/Shared`, `Shared`, or directly `rom/` and `hdf/`); it is only
read. Further options: `-DDEST=DIR` (default `amiga-emulation/`),
`-DHDFTOOL=PROGRAM` (default `build/amiga/host/tools/hdftool/hdftool`),
`-DFORCE=ON`.

* **Inputs:** every file used must have the size and SHA-256 of Amiga
  Forever 11 (`rom/amiga-os-310-a1200.rom`, `rom/amiga-os-310-a600.rom`,
  `rom/amiga-os-130.rom`, `rom/rom.key`, `hdf/workbench-311.hdf`,
  `hdf/workbench-135.hdf`); another edition is refused.
* **Results:** `hdftool` works reproducibly (no host clock, a fixed
  order of the blocks), so every result has a known SHA-256 too, which the
  script checks. A result is written as `NAME.part` and renamed when it is
  right.
* **Existing files** in `DEST` with the expected contents stay untouched;
  a file with other contents stops the script (nothing is overwritten)
  unless `-DFORCE=ON` is given.
* **Another directory:** the tests take the emulation data from
  `$AMIGA_EMULATION_DIR` if it is set (else from `amiga-emulation/`), e.g.
  `AMIGA_EMULATION_DIR=$PWD/build/af-emulation ctest --preset emulator`
  after a setup with `-DDEST=build/af-emulation`.

| File | Contents | Size | SHA-256 |
|---|---|---|---|
| `amiga-os-130.rom` | Kickstart 1.3 (34.5), A500/A1000/A2000, Cloanto-encrypted (`AMIROMTYPE1`) | 262155 | `caa4910942b8d558c9ae2284405af81db3752535192cc25f7939765c3ea2d5e0` |
| `rom.key` | the key of the encrypted ROMs (a500 machine, below) | 1426 | `f3b3b35593fef9a677225f559f8155e2b5d97b5bbfb2ccf24ee93738124d9a71` |
| `workbench-135.hdf` | Workbench 1.3.5 (34.34), RDB hardfile, partition DH0 (DOS\1, FFS in the RDB) | 3145728 | `a7b4fd7e1509a6086baf04f14c44894af41107c3ee9093e585261154665d5c28` |
| `amiga-os-310-a1200.rom` | Kickstart 3.1 (40.68), A1200, Cloanto-encrypted | 524299 | `541f7467bce4f4b07a6eb5250d4586a4679c2da51ad30b48ba93421a901b1bb0` |
| `amiga-os-310-a600.rom` | Kickstart 3.1 (40.63), A500/A600/A2000, Cloanto-encrypted | 524299 | `03ec7583249c686a3f63b5b92634b12e14ce72fc27b0066a8c3cca57f869443e` |
| `workbench-311.hdf` | Workbench 3.1 (40.42), raw partition image, DOS\3 | 6258688 | `b3963c5dafb75027ffd6a5f5a1a6572246337bfb7d0f1b31cfdb50b3ae843c01` |
| `workbench-311-rtg.hdf` | the same with the uaegfx monitor | 6258688 | `f092de1ec92bff7854701c32bc5ad9f54f16de9264ef09dffba010816a77fba4` |

What the script changes in Amiga Forever's hardfiles (both are RDB
hardfiles with one partition `DH0`; `S:User-Startup` as in "Conditions
for the hardfiles"):

* `workbench-135.hdf`: stays an RDB hardfile (Kickstart 1.3 has no FFS in
  the ROM; the FFS comes from the RDB). Workbench 1.3's
  `S/Startup-Sequence` knows no User-Startup: before `LoadWB DELAY` it
  gets `IF EXISTS S:User-Startup` / `Execute S:User-Startup` / `ENDIF`,
  and `S/User-Startup` is new (1.3 syntax).
* `workbench-311.hdf`: the partition alone, without the RDB (cylinders 2
  to 383 of 32 blocks, 1 surface, 2 reserved blocks: the geometry of the
  `hardfile2` line of the a1200 and a600, mounted by Kickstart 3.1's FFS
  in the ROM), and a new `S/User-Startup`, which its `S/Startup-Sequence`
  already executes (before `S:AFShared-Startup` and `LoadWB`), and a new
  `Prefs/Env-Archive/Sys/font.prefs` (`ENVARC:Sys/font.prefs`, copied to
  `ENV:` at boot): Workbench 3.1 ships none (the 3.2 image had one), but
  tests such as `bug_orig_rimisclib_openfontprefs_alloc_unchecked` read
  `ENV:SYS/Font.Prefs`. It is the file Font preferences saves with the
  default fonts (neither Amiga Forever's disks nor hardfiles have one, so
  the script writes it with `hdftool edit --write-hex`): an IFF FORM PREF
  with PRHD and three FONT chunks (Workbench, system, screen; `struct
  FontPrefs` of `prefs/font.h`), topaz 8 each, 518 bytes. IPrefs takes
  it at boot without a requester and the fonts stay as without it.
* `workbench-311-rtg.hdf`: as `workbench-311.hdf`, plus
  `Storage/Monitors/UAE` and `UAE.info` (Picasso96 with
  `BOARDTYPE=uaegfx`) copied to `Devs/Monitors`, so the Startup-Sequence
  starts the monitor and the RTG modes exist; the Workbench itself stays
  on the native screen (no `screenmode.prefs`). It has the font prefs as
  well.

Checked 2026-11-06 (Amiberry, `AMIGA_EMULATION_DIR` on a separate
directory): all three Kickstarts boot their hardfile and run
`TEST:run.script` (`Version`: 34.5/34.34, 40.68/40.42 on the a1200 and
with RTG, 40.63/40.42 on the a600).

## How many emulators at once (slots)

At most 4 Amiberry instances run at the same time on the machine, over
all runners, build directories and worktrees (the slots of
`tools/lib/amiga_emulation`, lock files in
`$XDG_RUNTIME_DIR/amiga-emulation-slots/`, without `XDG_RUNTIME_DIR` in
`/tmp/amiga-emulation-slots-UID/`). The limit can be changed while tests
run, by the file `max` in that directory:

```
echo 6 > $XDG_RUNTIME_DIR/amiga-emulation-slots/max
```

A session that waits for a slot reads it again on every try, so the new
limit also counts for the sessions that already wait (a lower limit does
not stop running sessions; the next ones wait). Only a number from 1 to
64 counts; a missing file or anything else (text, 0, a larger number)
means 4. `AMIGA_EMULATION_MAX` in the environment of a runner overrides
the file for that runner (0 = no limit).

With 16 cores, 8 to 10 slots are sensible, depending on the temperature
of the machine. If something is odd (a session that fails to restore, a
changed ROM or hardfile that is not noticed), `amiga_run
--purge-snapshots [BUILDDIR]` clears the snapshots of a build directory;
`tools/lib/amiga_emulation/README.md` explains the snapshot mode.

## Conditions for the hardfiles

* **`user-startup`** executes `TEST:run.script` if `TEST:` is mounted,
  and queries the drive safely (without `TEST:` there would otherwise be
  the requester „Datenträger einlegen“ (insert volume), which blocks the
  startup):

  ```
  Assign >NIL: TEST: EXISTS
  If NOT WARN
    If EXISTS TEST:run.script
      Execute TEST:run.script
    EndIf
  EndIf
  ```
  Do not assume any further drives that are not always mounted.
* **Read-only:** the hardfiles are mounted `ro`; several instances share
  the same image, nothing changes it. `T:` and `ENV:` are in RAM. Every
  write attempt to `SYS:` opens the requester „Datenträger Workbench ist
  schreibgeschützt“ (volume Workbench is write-protected) and blocks the
  run until the time limit – also by system programs: `ShowConfig` writes
  to `SYS:` and therefore hangs (checked 2026-10-27). Tests write only to
  `TEST:` or `RAM:`.
* **Format:** raw images of one partition (`DOS\3` for Workbench 3.1,
  which Kickstart 3.1's FFS in the ROM mounts; no RDB, no container like
  VHD), with the geometry of the `hardfile2` line (32 sectors per
  track, 1 surface, 2 reserved blocks, 512-byte blocks).
* To change an image: start with a separate, writable configuration, or
  change it on the host with a tool that reads Amiga hardfiles (`hdftool
  edit` of this build for OFS/FFS without dircache, `xdftool` of amitools)
  – not while tests run. The changed file gets a
  new modification time, so the snapshots made from it are not used
  again (`tools/lib/amiga_emulation/README.md`, "Snapshot mode").

## A500 machine (Kickstart 1.3)

The machine `a500` (`params.machine`, `amiga_run --machine a500`, pass
file `machine=a500`, bug-orig header `; @machine a500`) is for tests of
bugs that only show on a 68000/68010 or under Kickstart 1.x. Its
configuration is `tools/lib/amiga_emulation/a500.uae` (taken from the
user's "A500 - Kick 1.3.uae", Amiberry 8.5.0): A500, ECS Agnus, 68000
not cycle-exact, 24-bit addressing, `chipmem_size=2` (1 MB chip),
`bogomem_size=4` (1 MB slow memory at $C00000), no Zorro II/III fast
memory (Amiberry 8.5.0 crashes with 8 MB of Zorro II fast memory and the
Zorro II RTG card of the user's configuration, `BlitterStudio/amiberry#2585`), the
hardfile read-only. `a500-68010` is the same machine with a 68010
(`cpu_type`/`cpu_model`, e.g. for #42).

It uses `amiga-os-130.rom`, `rom.key` and `workbench-135.hdf` as the
setup from Amiga Forever (above) makes them.

* **`rom.key`:** Amiberry decrypts the ROM only with `rom.key` in the
  directory of the ROM's real path (it resolves symbolic links; a key
  elsewhere, e.g. in `~/Amiberry/ROMs`, or `kickstart_key_file=`, which
  Amiberry ignores as obsolete, does not help: "Unknown ROM", the CPU
  halts, the screen stays black). `amiga_emulation` checks this before
  starting and refuses the run (the test is skipped). Instead of the
  key, a decrypted ROM (262144 bytes) under the same name works too.
  Why at all: Cloanto ships the ROMs of Amiga Forever encrypted (an
  11-byte header `AMIROMTYPE1`, then the ROM XOR'ed with the bytes of
  `rom.key`, repeated) so that a ROM file alone is of no use without the
  product's key - a licence measure, not a protection: the key is the
  same for every copy of the product, not personal. Both files belong to
  the user's own licence and are, like everything here, not in git: the
  ROM must not be uploaded, every developer provides their own, with
  the setup above (which copies both unchanged).
* **The hardfile's startup** (made by the setup): `S:Startup-Sequence`
  executes `S:User-Startup` before `LoadWB`, and
  `S:User-Startup` executes `TEST:run.script` (same conditions as above,
  in 1.3 syntax):

  ```
  FAILAT 21
  ASSIGN >NIL: TEST: EXISTS
  IF NOT WARN
  IF EXISTS TEST:run.script
  EXECUTE TEST:run.script
  ENDIF
  ENDIF
  ```
  The startup also runs Amiga Forever's `S:AFShared-Startup` (harmless
  without the Amiga Forever host). Checked 2026-10-30: boot to the end of
  a short script in about 14 s; `TEST:` (uae filesystem, directory mount)
  works under Kickstart 1.3, also writing, `MakeDir`, `Copy`.

## A600 machine (68000, Kickstart 3.1)

The machine `a600` (`--machine a600`, pass file `machine=a600`, bug-orig
header `; @machine a600`) is for bugs that only show on a 68000 (address
errors on odd addresses, 68020 instructions) in libraries that need
Kickstart 2.0 or newer, which the `a500` (Kickstart 1.3) cannot start.
Its configuration is `tools/lib/amiga_emulation/a600.uae` (taken from the
user's "A600.uae", Amiberry 8.5.0): A600 (Gayle, IDE, PCMCIA), ECS, 68000
with 24-bit addressing, not cycle-exact, `chipmem_size=4` (2 MB chip),
`bogomem_size=4` (1 MB slow memory), no fast memory and no RTG card.
Amiberry 8.5.0 crashes at the start with 8 MB of Zorro II fast memory
together with a Zorro II RTG card, in `clear_bank()`
(`src/expansion.cpp`; reported as `BlitterStudio/amiberry#2585`). It
boots the a1200's `workbench-311.hdf` read-only, with
`amiga-os-310-a600.rom` (Kickstart 40.63, A500/A600/A2000) of the setup
from Amiga Forever.

Without the ROM the a600 part of `amiga_emulation_a500` is skipped.
