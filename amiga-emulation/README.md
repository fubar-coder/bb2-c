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
  TED 1.1 starts; programs compiled elsewhere (e.g. by a bug-orig run on
  the A1200) run. The port's `blitz2` does not start ("utility.library
  failed to load": its libnix startup opens utility.library, which
  Kickstart 1.x lacks). The bug-orig/corpus harness does not run on 1.x
  yet: its session script uses `$RC`, `>>` and redirection behind the
  arguments; tedstub (with the edstub frame), reqtrap, inject and wbrun
  call V36+ functions (`CreateMsgPort`, `CreateIORequest`,
  `AllocDosObject`, `EasyRequestArgs`, `SystemTags`, `CreateNewProcTags`,
  `RunCommand`, `CacheClearU`).
