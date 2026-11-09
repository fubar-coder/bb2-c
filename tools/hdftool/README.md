# hdftool

Reads and adapts AmigaDOS hardfiles: an RDB hardfile with one partition or
a raw partition image, OFS or FFS (`DOS\0` to `DOS\3`). It sets up the
Workbench hardfiles of the emulator tests from Amiga Forever
(`cmake/setup-amiga-forever.cmake`, `AMIGA-EMULATION-SETUP.md`). The logic
is `tools/lib/hdf` (`hdf.h`), tested by `tools/tests/test_hdf.c`.

## Usage

```
Usage: hdftool [OPTIONS] SUBCOMMAND

AmigaDOS hardfiles: an RDB hardfile with one partition or a raw partition, DOS\0
to DOS\3. A PATH or DIR goes from the root, separated by '/'.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  info                        one line about the image: RDB or raw, dostype, blocks, volume
  list                        a directory of the image, one entry per line
  cat                         a file of the image to stdout
  check                       the file system of the image
  edit                        the operations applied to the image IN, the result checked and
                              written to OUT
```

A `PATH` or `DIR` goes from the root of the volume, its parts separated by
`/` (names compared without regard to case, as AmigaDOS does). DOS\4 and
DOS\5 (directory caches) are refused.

### info

```
Usage: hdftool info [OPTIONS] IMAGE

one line about the image: RDB or raw, dostype, blocks, volume

POSITIONALS:
  IMAGE TEXT REQUIRED         the hardfile

OPTIONS:
  -h,     --help              Print this help message and exit

rdb NAME or raw, the dostype, the number of blocks, the reserved blocks, the
byte offset of the partition, the root block, the free blocks and the volume
name. Exit status 0, 1 the image cannot be read or is no hardfile it knows, 2
usage error.
```

`info` prints one line: `rdb NAME` or `raw`, the dostype, the number of
blocks, the reserved blocks, the byte offset of the partition, the root
block, the free blocks and the volume name.

### list

```
Usage: hdftool list [OPTIONS] IMAGE [DIR]

a directory of the image, one entry per line

POSITIONALS:
  IMAGE TEXT REQUIRED         the hardfile
  DIR TEXT                    the directory, from the root (default: the root)

OPTIONS:
  -h,     --help              Print this help message and exit

In hash table order, NAME SIZE for a file, NAME/ for a directory. Exit status 0,
1 the image cannot be read or is no hardfile it knows, or DIR is no directory, 2
usage error.
```

`list` lists a directory (the root without `DIR`): one line per entry
in hash table order, `NAME SIZE` for a file, `NAME/` for a directory.

### cat

```
Usage: hdftool cat [OPTIONS] IMAGE PATH

a file of the image to stdout

POSITIONALS:
  IMAGE TEXT REQUIRED         the hardfile
  PATH TEXT REQUIRED          the file, from the root

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0, 1 the image cannot be read or is no hardfile it knows, or PATH is
no file, 2 usage error.
```

`cat` writes a file's contents to stdout.

### check

```
Usage: hdftool check [OPTIONS] IMAGE

the file system of the image

POSITIONALS:
  IMAGE TEXT REQUIRED         the hardfile

OPTIONS:
  -h,     --help              Print this help message and exit

Checksums and types of all blocks reachable from the root, no block used twice,
every used block allocated in the bitmap and every allocated block used. Exit
status 0, 1 the image cannot be read or the check fails, 2 usage error.
```

`check` checks the file system: checksums and types of all blocks
reachable from the root, no block used twice, every used block
allocated in the bitmap and every allocated block used.

### edit

```
Usage: hdftool edit [OPTIONS] IN OUT

the operations applied to the image IN, the result checked and written to OUT

POSITIONALS:
  IN TEXT REQUIRED            the hardfile
  OUT TEXT REQUIRED           the image written; it must not exist

OPTIONS:
  -h,     --help              Print this help message and exit
          --partition         write only the partition, a raw image without the RDB
          --write PATH TEXT   write a file (create or replace)
          --write-hex PATH HEX
                              the same, the contents as hexadecimal digits
          --replace PATH OLD NEW
                              replace text that occurs exactly once
          --copy FROM TO      copy a file inside the image

The operations are applied in memory in the order given; then the file system is
checked and OUT written, which must not exist (it is never overwritten). Exit
status 0, 1 IN cannot be read, an operation or the check fails, OUT exists or
cannot be written, 2 usage error.
```

`edit` applies the operations `OP` in memory, in order, checks the file
system and only then writes `OUT`, which must not exist (it is never
overwritten; if it cannot be written whole it is removed). With
`--partition` it writes only the partition, a raw image without the RDB.

* `--write PATH TEXT` writes a file (created or replaced),
* `--write-hex PATH HEX` the same with the contents as hexadecimal
  digits,
* `--replace PATH OLD NEW` replaces text that occurs exactly once in the
  file,
* `--copy FROM TO` copies a file inside the image.

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
