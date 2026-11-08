# hdftool

Reads and adapts AmigaDOS hardfiles: an RDB hardfile with one partition or
a raw partition image, OFS or FFS (`DOS\0` to `DOS\3`). It sets up the
Workbench hardfiles of the emulator tests from Amiga Forever
(`cmake/setup-amiga-forever.cmake`, `AMIGA-EMULATION-SETUP.md`). The logic
is `tools/lib/hdf` (`hdf.h`), tested by `tools/tests/test_hdf.c`.

## Usage

```
usage: hdftool info IMAGE
       hdftool list IMAGE [DIR]
       hdftool cat IMAGE PATH
       hdftool check IMAGE
       hdftool edit IN OUT [--partition] OP...
```

A `PATH` or `DIR` goes from the root of the volume, its parts separated by
`/` (names compared without regard to case, as AmigaDOS does). DOS\4 and
DOS\5 (directory caches) are refused.

* `info` prints one line: `rdb NAME` or `raw`, the dostype, the number of
  blocks, the reserved blocks, the byte offset of the partition, the root
  block, the free blocks and the volume name.
* `list` lists a directory (the root without `DIR`): one line per entry
  in hash table order, `NAME SIZE` for a file, `NAME/` for a directory.
* `cat` writes a file's contents to stdout.
* `check` checks the file system: checksums and types of all blocks
  reachable from the root, no block used twice, every used block
  allocated in the bitmap and every allocated block used.
* `edit` applies the operations `OP` in memory, in order, checks the file
  system and only then writes `OUT`, which must not exist (it is never
  overwritten; if it cannot be written whole it is removed). With
  `--partition` it writes only the partition, a raw image without the RDB.
  * `--write PATH TEXT` writes a file (created or replaced),
  * `--write-hex PATH HEX` the same with the contents as hexadecimal
    digits,
  * `--replace PATH OLD NEW` replaces text that occurs exactly once in the
    file,
  * `--copy FROM TO` copies a file inside the image.

`-h` or `--help` prints the usage on stdout. The subcommands are
dispatched by the program itself, not by `tools/lib/subcmd`;
`hdftool SUB -h` is not a help of its own (planned with the move of the
host programs to CLI11).

## Inputs and outputs

Input is a hardfile (an image as Amiberry mounts it). `info`, `list` and
`cat` write to stdout; `edit` writes the new image `OUT`. The tool has no
clock: a new file takes the date of its directory, a copy that of its
source, blocks are taken from the bitmap in a fixed order; the same inputs
give the same image (the setup script checks the SHA-256 of every result).

## Exit codes

* 0 success;
* 1 the image cannot be read or is no hardfile it knows, the file system
  check fails (also after `edit`'s operations), an operation fails, or
  `OUT` exists or cannot be written;
* 2 usage error (no or an unknown subcommand, the wrong number of
  arguments, an unknown operation).

Messages go to stderr as `hdftool: [FILE: ]message`.

## Callers

* `cmake/setup-amiga-forever.cmake` (by hand, `AMIGA-EMULATION-SETUP.md`):
  `edit` with `--write`, `--write-hex`, `--replace`, `--copy` and
  `--partition`.
* Tests: `help_hdftool`, `readme_hdftool` (`tools/hdftool/CMakeLists.txt`);
  the library: `hdf` (`tools/tests/test_hdf.c`).

## Example

```
$ hdftool info amiga-emulation/workbench-135.hdf
rdb DH0 DOS\1 blocks 4040 reserved 2 offset 1034240 root 2020 free 614 volume Workbench1.3
$ hdftool cat amiga-emulation/workbench-135.hdf S/Startup-Sequence | head -1
; $VER: Startup-Sequence_HardDrive 34.35 (12.2.04)
```
