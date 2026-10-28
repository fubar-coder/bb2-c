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
