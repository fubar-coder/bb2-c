---
title: ritrackdisklib
weight: 47
description: The bugs of Blitz Basic 2 in the RWE userlib ritrackdisklib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `OpenDisk` without a name opened the device of an earlier `OpenDisk` {#opendisk-device-name}

`OpenDisk unit#` of
[ritrackdisklib]({{< relref "/docs/libraries/userlibs/ritrackdisklib" >}})
opens a floppy drive, unit 0 to 3 (DF0: to DF3:), through
trackdisk.device for the sector commands. `OpenDisk unit#,devicename$`
opens the unit of another device of the same kind.

**What happened.** The original copied the device name over its own
"trackdisk.device" and kept it there. Every later `OpenDisk` without a
name opened that device again, even when the `OpenDisk` with the name
had failed. This program tries a device that does not exist and then
opens DF0: as usual:

```
NPrint OpenDisk(0,"nosuch.device")
NPrint OpenDisk(0)
NPrint CloseDisk(0)
End
```

**With the original**

```
0
0
0
```

**With the port**

```
0
-1
-1
```

With the port, `OpenDisk unit#` always opens trackdisk.device, as the
library's guide documents, and a device name counts only for the
`OpenDisk` it is given to.

**What to change.** A program that opened another device once and then
used `OpenDisk unit#` to open it again has to give the device name each
time.

Issue: [OpenDisk unit# opens the device of the last OpenDisk unit#,devicename$ instead of trackdisk.device (#859)](https://github.com/fubar-coder/bb2-c/issues/859).

## Changes you may notice

### `DiskMotor` reported the motor as always on {#diskmotor-state}

`DiskMotor` of
[ritrackdisklib]({{< relref "/docs/libraries/userlibs/ritrackdisklib" >}})
tells whether the motor of a floppy drive is on; its help text says that
it returns -1 when the motor is on.

**What happened.** The original read the motor line of the drives from
the hardware. trackdisk.device leaves that line in the same state after
every request, whether the motor is on or off, so `DiskMotor` returned -1
at all times. This program follows the motor of DF1: through the
library's commands:

```
DEFTYPE .l
rb=AllocMem_(512,$10002)
r=OpenDisk(1)
NPrint "opened: ",DiskMotor
r=MotorOn(1)
NPrint "after MotorOn: ",DiskMotor
r=MotorOff(1)
NPrint "after MotorOff: ",DiskMotor
r=ReadSector(1,0,rb)
NPrint "after ReadSector: ",DiskMotor
CloseDisk 1
NPrint "after CloseDisk: ",DiskMotor
FreeMem_ rb,512
End
```

The original printed -1 in every line. The port prints 0, -1, 0, -1
and 0.

**With the port.** `DiskMotor` returns -1 when the motor of a unit that
the library has open is on, and 0 otherwise. The library knows the state
from its own commands: the motor is on after `MotorOn` and after a
`ReadSector`, `WriteSector`, `FormatTrack`, `WriteBoot` or `BootProg`
that worked, since trackdisk.device leaves it running after them, and
off after `MotorOff` and `CloseDisk`. What other programs do with the
drives, it does not see.

**What to change.** Nothing, unless a program relied on the -1: it now
gets the state the help text describes.

Issue: [DiskMotor returns -1 when the motor line is off, 0 when it is on (#857)](https://github.com/fubar-coder/bb2-c/issues/857).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ritrackdisklib {#ritrackdisklib-fixes}

These fixes concern
[ritrackdisklib]({{< relref "/docs/libraries/userlibs/ritrackdisklib" >}}),
the sector access to floppy disks of Red When Excited. `OpenDisk`
without a device name is described in
[Changes for existing programs](#opendisk-device-name), `DiskMotor` in
[Changes you may notice](#diskmotor-state).

* Programs that use the library start on Kickstart 1.x. The library
  builds its message port and its requests itself, with functions that
  every Kickstart has; the original called functions of Kickstart 2.0
  there, and every program with the library crashed at its start
  ([#941](https://github.com/fubar-coder/bb2-c/issues/941)).
* On Kickstart 1.x, `ReadSector`, `WriteSector`, `FormatTrack`,
  `BootProg` and `WriteBoot unit,bootdata` move a buffer that is not in
  chip memory through a chip memory buffer of the library, one track at
  a time; without chip memory for it they send nothing and return 31.
  trackdisk.device of Kickstart 1.x reaches only chip memory: given
  another buffer, it wrote unrelated chip memory to the disk, or read
  the sectors into it, and reported success. The original never got so
  far, since it crashed at the start on Kickstart 1.x
  ([#1163](https://github.com/fubar-coder/bb2-c/issues/1163)).
* `BootProg` stores the checksum that Kickstart checks, so a disk with
  the boot block boots. The original calculated it wrongly
  ([#856](https://github.com/fubar-coder/bb2-c/issues/856)).
* `FormatTrack` without a count formats one whole track, and both forms
  of `WriteBoot` write the boot block. The original gave
  trackdisk.device a length it refuses, so these wrote nothing. The count
  of `FormatTrack` stays a number of sectors: only multiples of 11 work,
  11 for each track, and the buffer holds 5632 bytes per track
  ([#858](https://github.com/fubar-coder/bb2-c/issues/858)).
* `WriteBoot unit,bootdata` writes the 1024 bytes at `bootdata`. The
  original wrote from an address that happened to be in a register
  ([#864](https://github.com/fubar-coder/bb2-c/issues/864)).
* `WriteBoot unit` writes a bootable standard boot block and keeps the
  file system type of the boot block on the disk. The original wrote 8
  bytes of a boot block and 1016 bytes of whatever memory followed them
  ([#865](https://github.com/fubar-coder/bb2-c/issues/865)).
* `MotorOff` first writes the sectors that trackdisk.device still holds
  in its track buffer to the disk, and `CloseDisk` and the end of the
  program switch the motor off the same way. The device writes that
  buffer only when it moves to another track, so with the original the
  last track written, after `BootProg` and `WriteBoot` the boot block
  itself, was lost when the disk was taken out. A program that keeps the
  motor running until the disk is changed still loses it
  ([#1154](https://github.com/fubar-coder/bb2-c/issues/1154)).
* `OpenDisk unit#,devicename$` gives the name to the system as it is. The
  original copied it into a buffer of the library, and a name of 57
  characters or more overwrote the library's data
  ([#860](https://github.com/fubar-coder/bb2-c/issues/860)).
* `OpenDisk` with a negative unit returns 0. The original opened the
  device and then lost the request, which stayed open
  ([#861](https://github.com/fubar-coder/bb2-c/issues/861)).
* `OpenDisk` of a unit that is already open returns 0 and leaves the
  unit as it is. The original opened it a second time and lost the first
  request, which stayed open
  ([#862](https://github.com/fubar-coder/bb2-c/issues/862)).
* The end of a program closes unit 3 too, and a program run again from
  the editor starts with all four units closed. The original left unit 3
  open ([#863](https://github.com/fubar-coder/bb2-c/issues/863)).

{{% fixed-bugs-library part="elsewhere" %}}
