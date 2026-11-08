# amiga_emulation: running scripts in Amiberry

The library `tools/lib/amiga_emulation` (header `amiga_emulation.h`, the
authoritative description of the interface) and the tool
`tools/amiga_run` start AmigaDOS scripts in Amiberry for the emulator
tests. This file is for developers of the library and of the test
runners. What a user sets up locally (Amiberry, ROMs, hardfiles, the slot
limit and how to change it): `AMIGA-EMULATION-SETUP.md` in the repository
root. The test suites that use it: `tests/README.md`.

## Usage

`tools/lib/amiga_emulation` runs AmigaDOS scripts in Amiberry – without a
window, isolated (own working directory, own XDG directories, read-only
hardfile), hence also in parallel: `amiga_emulation_start()`
(configuration = the machine's template plus changes, script, time
limit; machines: `a1200` = `default.uae`, `a500` = `a500.uae` with
Kickstart 1.3, `a500-68010`),
`amiga_emulation_command()` (control socket of Amiberry, e.g.
`SCREENSHOT`, `READ_MEM`), `amiga_emulation_join()`. The script sees its
directory as `TEST:`. By hand with `amiga_run`
([`tools/amiga_run/README.md`](../../amiga_run/README.md)):

```
echo 'Version >TEST:v.txt' | build/amiga/host/tools/amiga_run/amiga_run -
```

`--log` writes Amiberry's own log into `amiberry.log` of the working
directory (e.g. with `--set log_illegal_mem=true`: every access to
unmapped memory as `Illegal bget at ADDRESS PC=...`).

Requirement: Amiberry ≥ 8.4 in `PATH`, Kickstart ROM and hardfiles in
`amiga-emulation/` (`AMIGA-EMULATION-SETUP.md`).

## Session states

`amiga_emulation_join()` returns the state of a session
(`enum amiga_emulation_status` in `amiga_emulation.h`):

* `done`: the script ran to its end (`TEST:run.done` was written);
* `timeout`: killed at the time limit;
* `halt`: killed after Amiberry had halted the emulated CPU (a double
  fault, a jump to no memory: it never runs again), before the time
  limit; the supervisor asks for the CPU's registers once a second and
  takes three equal answers with interrupt mask 7 for a halt;
* `exited` (`exited early`): Amiberry ended before the script did;
* `failed`: the run could not be handled.

With `AMIGA_EMULATION_TIMEOUT_STATE=1`, a session that ends by `timeout`
or `halt` leaves `timeout.txt` in its working directory (how it ended,
CPU registers, whether the CPU is halted, its threads) and the machine's
state (`timeout.uss`, `timeout.uae`); `AMIGA_EMULATION_LOG=1` (as
`--log`) writes Amiberry's log into `amiberry.log` there. The test
presets `emulator`, `all` and `bug-orig` set both.

## Slots: how runners wait

The limit of simultaneous sessions, its file and variable are described
in `AMIGA-EMULATION-SETUP.md`. How the library applies it:


The limit is hard: every session holds a slot, from its start until its
end. The session's supervisor process holds it, not the runner: it is
free again as soon as the session has ended, also while the runner
still waits for an earlier session before it joins this one. A runner
that runs several sessions at once (`parallel=`) waits for a slot only
while it has no session of its own that it has not freed yet (one that
has ended but is not joined counts); if no slot is free for a further
one, it tries again every half second and collects meanwhile those of
its sessions that have ended, so it takes the next slot that any runner
frees. Two runners cannot block each other: every session ends at its
time limit on its own and frees its slot. A runner thus runs as many
sessions at once as it gets slots, at least one, and its sessions may run
one after the other.

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
* **The job** (`SNAP:job.script`) writes a file to `TEST:` and deletes
  it again, then (only if that worked) writes `SNAP:started`, sets the clock
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
    the hardfiles have several MB each, which would cost every session more
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
* **A restore that fails** (no `SNAP:started` within 15 s, also when
  `TEST:` takes no file, or Amiberry ends) is dropped and the session
  boots cold without snapshot. The state file is removed only when
  Amiberry ended or booted instead (it cannot load it); without a start
  in time it stays (a restore on a busy host can be that slow) and the
  next session tries it again. A state file that does not begin as a
  savestate (`ASF `) is removed before the restore; the session saves
  anew. A test mount called `SNAP` also means a cold boot.
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

## Machine a500: what runs there

The files and the configuration of the machines are in
`AMIGA-EMULATION-SETUP.md`; this is what the library and the tests do
under Kickstart 1.x.

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
* **The bug-orig harness** runs there (machine= / `; @machine`, `tests/README.md`,
  "bug-orig"): the session script in 1.3 syntax with `rcput` for the
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
