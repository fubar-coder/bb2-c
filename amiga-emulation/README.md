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

The limit is hard: every session holds a slot, from its start until its
end. The session's supervisor process holds it, not the runner: it is
free again as soon as the session has ended, also while the runner
still waits for an earlier session before it joins this one. A runner
that runs several sessions at once (`parallel=`) waits for a slot only
while it has no session of its own that it has not freed yet (one that
has ended but is not joined counts); if no slot is free for a further
one, it ends one of its own sessions first and starts the next then (it
never waits for a slot while holding one, so two runners cannot block
each other). A runner thus runs as many sessions at
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
  removed; only the snapshots `slot-K/snap-OWNER-ID-KEY.uss` stay (about
  0.6 MB on the A1200, 80 KB on the A500), the 24 most recently used per
  slot over all owners (the tests use about a dozen configurations; 5
  slots × 24 × 0.6 MB is at most about 70 MB). The pruning also removes
  the snapshots of build directories that are gone or no longer tested.
* **The first session** of a configuration in a slot boots cold, with
  real empty directories at those paths (Amiberry resolves a link when it
  mounts, the state would keep the link's target): `TEST:run.script` is a
  fixed prologue (AmigaDOS 1.3 syntax) that writes `SNAP:ready` and runs
  `SNAP:waitjob`, a 68000 program (`tools/lib/amiga_emulation/waitjob.s`,
  dos.library V0, polls `SNAP:job.script` every 40 ms). The session's
  supervisor saves the state then, ends that Amiberry and restores the
  new state like every later session. So the first session per slot and
  configuration takes a cold boot plus about 2 s; with 5 slots up to 5
  sessions per configuration boot – once per build directory, not per
  ctest run (the life cycle below).
* **The job** (`SNAP:job.script`) writes `SNAP:started`, sets the clock
  (`SetClock LOAD`; on the A500 machines a `DATE` line with the host's
  time, `SetClock LOAD` takes 1.8 s there) and executes the test's
  script. During the session `TEST:run.script` is the prologue; at the
  end it is the test's script again, as without snapshot.
* **The key** of a snapshot (`amiga_emulation_snapshot_key`) is a hash
  of everything the booted state is made from:
  * the effective configuration: the machine's template with the
    machine's and the test's changes, without comments, the media by
    their real paths, and the slot's own paths (`TEST:`, the mounts,
    `SNAP:`) replaced by a placeholder, so neither the slot nor the
    session's directories change the key, but every other line does
    (CPU, memory, chipset, ROM, a mount's volume or access);
  * the machine's name;
  * the identity of every medium of the configuration – the Kickstart
    ROM, `kickstart_ext_rom_file`, `cart_file`, `flash_file`,
    `floppyN`, `cdimageN`, the image of every `hardfile2` line – and of
    `rom.key` beside the ROM and of the Amiberry program: real path,
    device, inode, size, modification time. Not a hash of the content:
    the hardfiles have 64 MB each, which would cost every session more
    time than the restore; a change through the file system changes the
    time, a copy the inode;
  * the prologue, the waiter and a version of the library's way of
    making a state (`SNAP_FORMAT` in `amiga_emulation.c`, counted up when
    a change there makes older states unfit and the rest of the key does
    not show it).

  A new hardfile, ROM, template or Amiberry thus gets new snapshots by
  itself.
* **The configuration id** (`amiga_emulation_snapshot_config_id`) is
  what the session asks for: the machine, the test's changes and the
  mounts' volumes and access (not their directories). The file name has
  both, `snap-OWNERHASH-ID-KEY.uss`; when a session saves a new state, the
  owner's snapshot of the same id with another key in that slot is
  removed (`amiga_emulation_snapshot_drop_stale`, "removed 1 stale
  snapshot" in `snapshot.log`): it was made from inputs that have changed
  since and would never be restored again. So is a snapshot of the
  owner with the older name without id (`snap-OWNERHASH-KEY.uss`).
* **The owner** of a snapshot is the build directory whose tests saved
  it: `AMIGA_EMULATION_OWNER` in the environment, which CMake sets for
  every emulator test (the top-level build directory, also for the host
  side's emulator tests; `cmake/emulation-queue.cmake`). The file name
  has a hash of it, the id and the key, `snap-OWNERHASH-ID-KEY.uss` (16
  hex digits each); a session restores only snapshots of its own owner.
  Without the variable (e.g. `amiga_run` by hand) the owner is "none"
  (hash `0000000000000000`). Snapshots are not shared between build
  directories or worktrees (each builds its own waiter and may have its
  own state of this code).
* **The life cycle:** a snapshot is made by the first session of its
  configuration, owner and key in a slot – right after the cold boot,
  before the job exists and with empty directories at the slot's paths,
  so it holds no state of any test; it is never saved again after a job.
  It is kept across ctest runs and restored by every later session with
  the same owner, configuration and key, in this and later runs (a
  restore touches it). It is discarded when
  * an input of the key changes: the next session of that configuration
    in the slot finds no snapshot with its key, boots, saves and removes
    the stale one (above);
  * a restore of it fails (above);
  * the slot's pruning drops it, the 24 most recently used per slot
    staying;
  * by hand: `amiga_run --purge-snapshots [BUILDDIR]` removes the
    `snap-*`/`save-*` files of one owner in all slots and nothing else
    (`amiga_emulation_snapshot_purge`; without `BUILDDIR`
    `$AMIGA_EMULATION_OWNER`, unset the snapshots without owner; no slot
    directory yet is not an error), e.g. after a change of this code that
    the key does not see and before `SNAP_FORMAT` has been counted up.
    A session of that owner that runs meanwhile and whose snapshot goes
    away falls back to a cold boot.

  A restored session sees its own `TEST:` and mounts through the slot's
  links, made anew for every session, and `SNAP:` is emptied before
  every start, so nothing of an earlier session reaches it.
* **A restore that fails** (no `SNAP:started` within 6 s, or Amiberry
  ends) is dropped, the state file too, and the session boots cold
  without snapshot. A test mount called `SNAP` also means a cold boot.
* **The supervisor:** every session (also without snapshot) is driven by
  a process of its own that the library forks at the start: it starts
  Amiberry, saves and restores, falls back, ends the session at
  `TEST:run.done` or at the time limit. The progress of a session thus
  does not depend on when its runner joins it (a runner joins its
  sessions in start order; before, a first session sat idle at
  `SNAP:ready` while the runner waited for an earlier one and could run
  out of time without its job). The time limit counts from the start of
  the Amiberry that gets the job (boot or restore included); a restart
  (the restore after the save, a fallback) starts it again.
* `snapshot.log` in the work directory tells what the session did
  (restore, cold boot and save, a failed restore) with the seconds since
  its start.

Removing `slot-K/snap-*.uss` (while no tests run) is always safe: the
next session per slot boots and saves again. Snapshots of older names
(`snap-KEY.uss` from before the owner, `snap-OWNERHASH-KEY.uss` from
before the configuration id) are never restored; the pruning removes them
over time, the owner's own ones its next save in the slot.

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
memory (Amiberry 8.5.0 crashes with 8 MB of Zorro II fast memory and the
Zorro II RTG card of the user's configuration, `BlitterStudio/amiberry#2585`), the
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
  bug-orig run on the A1200) run. The port's `blitz2` starts too since
  #936 (before, it stopped with "utility.library failed to load":
  libnix20's division helpers made its startup open utility.library,
  which Kickstart 1.x lacks; now it is linked with libnix13,
  `programs/blitz2/checklink.cmake`); checked 2026-10-30: compiling an
  executable and COMPILE/RUN give the same log, output and executable as
  the CD's blitz2 2.10. Programs linked with libnix20 (the toolchain's
  default with `-noixemul`) crash at start; libnix13 works.
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

## A600 machine (68000, Kickstart 3.2)

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
(`src/expansion.cpp`; reported as `BlitterStudio/amiberry#2585`). It boots the a1200's Workbench 3.2 hardfile
read-only.

| File | Contents | Size | SHA-256 (as of 2026-10-31) |
|---|---|---|---|
| `CDTVA500A600A2000.47.115.bin` | Kickstart 3.2 (47.115), A500/A600/A2000/CDTV | 524288 | `2f48c3991748f4d812f1c0d13a99581cc17a058261d73856aee9bafda2d07fee` |

Without the ROM the a600 part of `amiga_emulation_a500` is skipped.
