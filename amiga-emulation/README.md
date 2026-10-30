# Emulation data (not in the repository)

For the emulator tests (`tools/lib/amiga_emulation`), a Kickstart ROM and
two hardfiles are stored here. They are not checked in (Kickstart and
AmigaOS must not be redistributed); everyone stores their own copies
here.

| File | Contents | Size | SHA-256 (as of 2026-10-27) |
|---|---|---|---|
| `A1200.47.115.rom` | Kickstart 3.2 (47.115), A1200 | 524288 | `27ddaf0a460e2ee85935139cbac1df234c283b3979c50526d118e9a55c61100a` |
| `Workbench_OS3.2-AB3-AGA.vhd` | AmigaOS 3.2, Workbench native (PAL HighRes Laced, 256 colors) – default | 67176960 | – |
| `Workbench_OS3.2-AB3.vhd` | AmigaOS 3.2, Workbench on RTG (uaegfx, 1680×1050) | 67176960 | – |

Everyone has set up the hardfiles themselves, so there is no checksum for
them; they only have to meet the conditions below.

## Configuration

`tools/lib/amiga_emulation/default.uae` is the default configuration
(A1200, AGA, 68020/68882, 8 MB chip, 128 MB Z3 fast, the AGA image).
Tests change it only by key=value differences. The RTG variant:

```
gfxcard_size=16
gfxcard_type=ZorroIII
hardfile2 / uaehf0: Workbench_OS3.2-AB3.vhd instead of Workbench_OS3.2-AB3-AGA.vhd
```

TED can only be tested with the AGA variant (on RTG its bitplanes would be
in graphics card memory, where the blitter cannot reach them).

## How many emulators at once

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

The limit is hard: every session holds a slot. A runner that runs several
sessions at once (`parallel=`) waits for a slot only while none of its
own sessions runs; if no slot is free for a further one, it ends one of
its own sessions first and starts the next then (it never waits for a
slot while holding one, so two
runners cannot block each other). A runner thus runs as many sessions at
once as it gets slots, at least one, and its sessions may run one after
the other.

## Snapshot mode

A session with a slot does not boot: it starts Amiberry from a savestate
of the booted system, taken when the system waits for its job (about
1.2 s to the script instead of 14 s; `tools/lib/amiga_emulation`,
`amiga_emulation.h`). `AMIGA_EMULATION_SNAPSHOT=0` in the environment
(or `amiga_run --cold`) turns it off: every session boots as before
(so does `AMIGA_EMULATION_MAX=0`: without a limit there are no slots).

* **Per-slot directories** `slot-K/` next to the lock files: Amiberry
  keeps the host paths of the mounted directories in the savestate and
  uses those on a restore, so a session mounts fixed paths of its slot.
  On a restore they are links to the session's own directories:
  `slot-K/test` to its `TEST:`, `slot-K/mnt-I` to its mount I (also the
  read-only ones). An extra volume `SNAP:` (`slot-K/ctl`) holds the waiter
  and the job. At the end of the session the links and `SNAP:` are
  removed; only the snapshots `slot-K/snap-KEY.uss` stay (about 0.6 MB
  on the A1200, 80 KB on the A500), the 24 most recently used per slot
  (the tests use about a dozen configurations; 5 slots × 24 × 0.6 MB is
  at most about 70 MB).
* **The first session** of a configuration in a slot boots cold, with
  real empty directories at those paths (Amiberry resolves a link when it
  mounts, the state would keep the link's target): `TEST:run.script` is a
  fixed prologue (AmigaDOS 1.3 syntax) that writes `SNAP:ready` and runs
  `SNAP:waitjob`, a 68000 program (`tools/lib/amiga_emulation/waitjob.s`,
  dos.library V0, polls `SNAP:job.script` every 40 ms). The library saves
  the state then, ends that Amiberry and restores the new state like
  every later session. So the first session per slot and configuration
  takes a cold boot plus about 2 s; with 5 slots up to 5 sessions per
  configuration boot.
* **The job** (`SNAP:job.script`) writes `SNAP:started`, sets the clock
  (`SetClock LOAD`; on the A500 machines a `DATE` line with the host's
  time, `SetClock LOAD` takes 1.8 s there) and executes the test's
  script. During the session `TEST:run.script` is the prologue; at the
  end it is the test's script again, as without snapshot.
* **The key** of a snapshot is a hash of the configuration (with the
  slot's paths, so it does not depend on the session's directories), the
  prologue, the waiter and the identity (device, inode, size,
  modification time) of the Kickstart ROM, the hardfile and the Amiberry
  program: a new hardfile or Amiberry gets new snapshots by itself.
* **A restore that fails** (no `SNAP:started` within 6 s, or Amiberry
  ends) is dropped, the state file too, and the session boots cold
  without snapshot. A test mount called `SNAP` also means a cold boot.
* `snapshot.log` in the work directory tells what the session did
  (restore, cold boot and save, a failed restore) with the seconds since
  its start.

Removing `slot-K/snap-*.uss` (while no tests run) is always safe: the
next session per slot boots and saves again.

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
* To change an image: start with a separate, writable configuration.

## A500 machine (Kickstart 1.3)

The machine `a500` (`params.machine`, `amiga_run --machine a500`, pass
file `machine=a500`, bug-orig header `; @machine a500`) is for tests of
bugs that only show on a 68000/68010 or under Kickstart 1.x. Its
configuration is `tools/lib/amiga_emulation/a500.uae` (taken from the
user's "A500 - Kick 1.3.uae", Amiberry 8.5.0): A500, ECS Agnus, 68000
cycle-exact, 24-bit addressing, `chipmem_size=2` (1 MB chip),
`bogomem_size=4` (1 MB slow memory at $C00000), no Zorro II/III fast
memory (Amiberry crashes with Zorro II fast memory in this setup), the
hardfile read-only. `a500-68010` is the same machine with a 68010
(`cpu_type`/`cpu_model`, e.g. for #42).

| File | Contents | Size | SHA-256 (as of 2026-10-30) |
|---|---|---|---|
| `amiga-os-130.rom` | Kickstart 1.3 (34.5), A500/A1000/A2000, Cloanto-encrypted (`AMIROMTYPE1`) | 262155 | `caa4910942b8d558c9ae2284405af81db3752535192cc25f7939765c3ea2d5e0` |
| `rom.key` | the Cloanto key for the encrypted ROM (Amiga Forever) | 1426 | – |
| `workbench-135.hdf` | Workbench 1.3.5 (34.34, Amiga Forever), RDB hardfile, partition DH0 (DOS\1, FFS in the RDB) | 3145728 | – |

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
  ROM must not be uploaded, every developer provides their own. The
  documented way is therefore the one Amiga Forever installs: copy its
  `amiga-os-130.rom` and `rom.key` (e.g. from `~/Amiberry/ROMs/` or the
  Amiga Forever `Shared/rom` directory) here unchanged.
* **The hardfile's startup:** `S:Startup-Sequence` ends with
  `IF EXISTS S:User-Startup` / `Execute S:User-Startup`, and
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
* **AmigaDOS 1.3 in scripts:** the redirection must follow the command
  name directly (`Echo >TEST:x "text"`; `Echo "text" >TEST:x` takes
  `>TEST:x` as an argument and asks for the volume `>TEST`, which blocks
  the run); there is no `>>` (append), no local variables (`$RC` stays
  text), only `If WARN`/`ERROR`/`FAIL` for the return code. The end mark
  of `amiga_emulation` is written in this form.
* **What runs there** (checked 2026-10-30): the CD's blitz2 2.10 with
  TED 1.1 starts, and so does the reference blitz2 1.95 (assembled with
  `-kick1hunks`: Kickstart 1.x's LoadSeg knows no HUNK_DREL32, vasm's
  default for relocations); programs compiled elsewhere (e.g. by a
  bug-orig run on the A1200) run. The port's `blitz2` does not start
  ("utility.library failed to load": its libnix startup opens
  utility.library, which Kickstart 1.x lacks; #936). Programs linked
  with libnix20 (the toolchain's default with `-noixemul`) crash at
  start; libnix13 works.
* **The bug-orig harness** runs there (machine= / `; @machine`, README,
  "Tests"): the session script in 1.3 syntax with `rcput` for the
  `rc=N` lines, tedstub and its edstub frame with Kickstart 1.2
  functions only, reqtrap linked with libnix13, patching only
  AutoRequest and calling the program itself (LoadSeg; only machine code
  programs - the BCPL commands of `C:`, e.g. `C:Version`, crash called
  that way). Not on 1.x: the gate of corpus_req (reqclosetest,
  reqclose: V36), inject and wbrun (`CreateMsgPort`, `CreateIORequest`,
  `CreateNewProcTags`, `RunCommand`, `CacheClearU`, libnix20 with
  utility.library). A command that cannot be loaded ("file is not an
  object module") ends the script on 1.3 despite `FailAt`: the session
  runs into its timeout.
