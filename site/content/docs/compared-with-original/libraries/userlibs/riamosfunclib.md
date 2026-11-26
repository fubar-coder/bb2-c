---
title: riamosfunclib
weight: 5
description: The bugs of Blitz Basic 2 in the RWE userlib riamosfunclib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/riamosfunclib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `BankLimit n` returned an address {#banklimit-result}

[riamosfunclib]({{< relref "/docs/libraries/userlibs/riamosfunclib" >}})
holds AMOS-style helpers by Red When Excited, among them memory banks:
blocks of memory with a number, from 0 to 49 at the start. `BankLimit`
without an argument gives the number of banks, and `BankLimit n` sets a
new number and frees every bank in use. The manual gives its syntax as
`[numbanks=]BankLimit[(maxbanks)]`: used as a function, the form with an
argument returns the number of banks as well.

**What happened.** The form with an argument returned the address of the
library's new table of banks instead of the number. This program sets 60
banks and tests the result:

```
DEFTYPE .l
s=BankLimit(60)
If s=60 Then NPrint "result 60" Else NPrint "result not the limit"
NPrint BankLimit
End
```

**With the original**

```
result not the limit
60
```

**With the port**

```
result 60
60
```

**What to change.** Nothing, unless a program used the result of
`BankLimit n` as an address: it gets the number of banks now. The banks
themselves are reached with `Start` and `Length`, as before.

Issue: [BankLimit n returns the address of the new start table (#483)](https://github.com/fubar-coder/bb2-c/issues/483).

### `FindVolume` returned an address instead of -1 {#findvolume-result}

`FindVolume(name)` of
[riamosfunclib]({{< relref "/docs/libraries/userlibs/riamosfunclib" >}})
tells whether a device, volume or assign of that name is there; the
argument is the address of the name without the colon, for example
`&v$`. The manual says that it returns -1 if the name is there and 0 if
it is not.

**What happened.** For a name that is there, the original returned the
address of its entry in the system's list of devices, volumes and
assigns instead of -1. As a truth value, in `If FindVolume(&v$)`, that
worked; a program that compared the result with -1 never found the
volume. This program asks for `RAM`, which is always there, and for a
name that is not:

```
DEFTYPE .l
v$="RAM"
r=FindVolume(&v$)
If r=-1 Then NPrint "RAM: -1" Else If r=0 Then NPrint "RAM: 0" Else NPrint "RAM: other"
v$="RWENOSUCHVOLUME"
NPrint "missing: ",FindVolume(&v$)
End
```

**With the original**

```
RAM: other
missing: 0
```

**With the port**

```
RAM: -1
missing: 0
```

**What to change.** A program that tests the result as a truth value or
compares it with -1 or 0 works as it is. One that used the address of
the entry gets -1 now; the original gave that address out after it had
released the list, so the entry could change or go away at any time.

Issue: [FindVolume returns the DosList entry instead of -1 (#1082)](https://github.com/fubar-coder/bb2-c/issues/1082).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riamosfunclib {#riamosfunclib-fixes}

These fixes concern
[riamosfunclib]({{< relref "/docs/libraries/userlibs/riamosfunclib" >}}),
the AMOS-style helpers of Red When Excited; the fixes of
[`BankLimit n`](#banklimit-result) and [`FindVolume`](#findvolume-result)
are described above.

The memory banks:

* A bank number is a number from 0 to one below the bank limit. `Start`,
  `Length`, `Erase` and `SwapBank` treat any other number as a bank not
  in use, `Reserve(bank,length)` returns 0 for it, `NextBank` returns -1
  for a start outside, and `FillMem`, `BLoad` and `BSave` take a negative
  number as an address. The original also took negative numbers and
  numbers above the limit as banks and read and wrote before or behind
  its tables of banks: `Start(-1)` gave the length of the last bank, and
  `Reserve` of a bank above the limit wrote over the address of another
  bank ([#484](https://github.com/fubar-coder/bb2-c/issues/484)).
* `Reserve(length)` returns -1 when every bank is in use; the original
  reserved a bank before the first one and damaged the library's tables
  ([#498](https://github.com/fubar-coder/bb2-c/issues/498)).
* `Reserve(length)` with a negative length returns -1, as for its other
  failures; the original returned 0, the number of the first bank
  ([#500](https://github.com/fubar-coder/bb2-c/issues/500)).
* `NextBank` without an argument searches from bank 0, as the manual
  says ("the first available bank"); the original started at whatever
  number a register of the processor held, and could return a later bank
  or -1 ([#497](https://github.com/fubar-coder/bb2-c/issues/497)).
* `EraseAll`, `BankLimit` and the end of the program free every bank,
  also with more than 65536 banks; the original freed only those from
  the highest multiple of 65536 on, and after `BankLimit 0` it went through
  65536 bank numbers below 0, outside its tables
  ([#495](https://github.com/fubar-coder/bb2-c/issues/495)).
* `CopyByte`, `CopyWord`, `CopyLong` and `FillMem` with a count of 0 copy
  or fill nothing, and `FillMem` with a negative length does nothing, as
  the copies do for a negative count; the original copied or filled 2^32
  bytes, words or longs over all memory
  ([#492](https://github.com/fubar-coder/bb2-c/issues/492)).

Loading and saving:

* `BLoad` into a bank opens the file before it touches the bank, so a
  file that cannot be opened leaves the bank as it was; the original
  erased the bank and reserved it anew first: a bank in use lost its
  contents, a free one stayed reserved
  ([#487](https://github.com/fubar-coder/bb2-c/issues/487)).
* `BLoad` returns 0 when reading the file fails; the original returned
  -1, success. A file shorter than the length asked for is still loaded
  with success
  ([#488](https://github.com/fubar-coder/bb2-c/issues/488)).
* `BLoad(name$)` returns -1 for every failure, as the manual says: an
  empty or missing file, no free bank, not enough memory, or a failed
  read, after which the bank is freed again. The original returned 0, the
  number of the first bank, when no bank could be reserved, and the
  number of a bank with undefined contents when the read failed
  ([#486](https://github.com/fubar-coder/bb2-c/issues/486)).
* `BSave` with a bank writes at most the bank's length, as the manual
  says; the original wrote `length` bytes also when the bank was shorter,
  and saved the memory behind it
  ([#489](https://github.com/fubar-coder/bb2-c/issues/489)).
* `SaveExec` returns 0 when the file cannot be created, and for a list
  without code hunks it writes no file and returns 0; the original
  returned an address in both cases, and for an empty list wrote the end
  of the list as a hunk or left the file open
  ([#501](https://github.com/fubar-coder/bb2-c/issues/501)).
* `SaveExec` numbers only the code hunks it writes; the original also
  counted the other items of the list that it leaves out, so with such an
  item before a code hunk the file could not be loaded
  ([#502](https://github.com/fubar-coder/bb2-c/issues/502)).
* `SaveExec` writes a loadable executable for any number of code hunks;
  the original's file with two or more code hunks could not be loaded. A
  list with one code hunk gives the same file as before
  ([#503](https://github.com/fubar-coder/bb2-c/issues/503)).

Directories and devices:

* `MakeDir` gives back the lock of the directory it created; the
  original gave back whatever dos.library left in a register, so with
  some versions of dos.library the new directory stayed locked and could
  not be deleted or renamed, or something else was released
  ([#496](https://github.com/fubar-coder/bb2-c/issues/496)).
* `ChangeDir` returns -1 and remembers the new directory also when the
  current directory before was the root of the boot volume; the original
  returned 0 then, although it had changed the directory, and never
  released the new one
  ([#490](https://github.com/fubar-coder/bb2-c/issues/490)).
* A program that uses `ChangeDir` ends in the directory it was started
  in, unless it changed the directory itself since, and `BankLimit` no
  longer touches the current directory. The original released the
  directory chosen last while it was still the current one, also at
  `BankLimit`, and left the Shell that started the program with a
  directory that was released
  ([#491](https://github.com/fubar-coder/bb2-c/issues/491)).
* `DeviceName$("")` leaves the string expression in front of it alone;
  the original cleared a ":" at the end of that part, so
  `"X:"+DeviceName$("")` lost its colon
  ([#493](https://github.com/fubar-coder/bb2-c/issues/493)).
* `DeviceName$` returns "" for a device or volume without a running
  handler; the original read from near address 0 and returned nonsense
  ([#494](https://github.com/fubar-coder/bb2-c/issues/494)).

Kickstart 1.x:

* `FindVolume`, `DeviceName$`, `CachesOn`, `CachesOff` and `Reboot`
  work on Kickstart 1.x. `FindVolume` and `DeviceName$` read the list of
  devices, volumes and assigns in the way Kickstart 1.x knows,
  `CachesOn` and `CachesOff` switch nothing there and return 0, and
  `Reboot` uses the reset routine that Commodore published for the
  Kickstarts before 2.0. The original called functions of Kickstart 2.0
  in all five commands and crashed there
  ([#1145](https://github.com/fubar-coder/bb2-c/issues/1145)).

{{% fixed-bugs-library part="elsewhere" %}}
