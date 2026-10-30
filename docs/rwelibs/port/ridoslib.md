# ridoslib (33): notification, device list, path splitting

No author in the source (BSS2 of Red When Excited). File notification
through one message port, the DOS device list and two string functions:

* `AddNotify id,file$` – a NotifyRequest (48 bytes, `MEMF_CLEAR`) with a
  copy of the name, UserData = id, `NRF_SEND_MESSAGE` to the library's
  port, kept in a table of 64 entries; -1 if StartNotify worked, else 0.
* `RemNotify id` – EndNotify, free request and name copy.
* `CheckNotify` / `WaitNotify` – the id of the next message (replied), 0
  if there is none; WaitNotify waits for one first.
* `NotifyPort` – the port.
* `GetDeviceList flags` – `LockDosList(flags | LDF_READ)` through init's
  `_dosbase` (the C build through a6, the program's dos base of `!libs
  #doslib,#la6`: the same pointer); `NextDevice` – the name of
  the next entry (`NextDosEntry`), "" at the end; `DeviceEntryType` –
  `dol_Type` of the current entry, -1 if none; `FreeDeviceList` –
  `UnLockDosList`.
* `GetFileName(p$)` – the part behind the last `/` or `:`;
  `GetPathName(p$)` – the part up to and including it. Both give the
  argument's string space back and build the result in a 256 byte buffer.

init opens dos.library V36 and creates the port; finit ends the requests
still active, closes dos.library and deletes the port.

* **Source:** `rwelibsrc/DOSLib/ridoslib2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/DOSLib/ridoslib2.bb`).
* **Reference:** BSS2 `RIDOSLIB.OBJ`; declarations equal (ctest
  `rwe_declarations_ridoslib`).
* **C file:** `libs/userlibs/rwe/ridoslib.c`; test
  `libs/tests/rwe/ridoslib.c` (snitch case "rwe ridoslib").

## C and assembler

All routines are C behind `BLITZ_THUNK`, the system calls through the
NDK inlines; no other assembler. The tables `notifys`/`filelens` have the
original's zero long in front (an id of -1 reads it, as in the original);
AddNotify computes the table offset as a word (`ASL.w`/`ADD.w`), RemNotify
as a long, like the original. Routines that return with d0 unchanged in
the original (RemNotify, NextDevice, FreeDeviceList on their early exits)
do so in C too. The string lengths are read below a2 without changing it
(DIFF as in all C ports, `docs/libs/cmdlibs-port.md` "General"). The
thunks keep d1/a0/a1, the C code d2-d7/a2-a6; the original changes
d0-d2, d4/d5, a0/a1 and a6 in places.

## BUG(orig)

* AddNotify: no range check of the id (64 entries; the offset is id*4 as
  a word), AllocMem's results are not checked, an id used twice loses the
  first request. A request whose StartNotify failed stays in the table
  (RemNotify or finit would EndNotify it) and is not counted, so finit,
  which stops when the counter is 0, may never reach it: it stays
  allocated (#626, #619, #620, #621).
* RemNotify: no range check either (#626).
* finit removes the requests from id 63 down only while the counter is
  not 0 (#621).
* FreeDeviceList does not check that the list is locked (#623), and it
  leaves the current entry: DeviceEntryType then reads an entry of the
  unlocked list (#622).
* NextDevice without GetDeviceList returns with d0 unchanged (the result
  string's length is whatever d0 was) (#625). It copies the name up to
  its 0 (the BSTRs of the DOS list have one) and leaves a3 behind the 0
  (#618).
* GetPathName never looks at the first character: `/abc` and `:x` give
  "", like a name without separator (#624); a3 ends behind the result's
  0 (#618). GetFileName and GetPathName take the length as a word
  (`MOVE.w -(a2)`: the low word of the long) (#628), and nothing limits
  the 256 byte buffer (the original overwrites the code behind it, the C
  build other data) (#627).

Like the original, and no defects: RemNotify leaves `filelens[id]` set
(the next AddNotify of the id sets it again) and sets d4 to `$12345678`
(a debugging leftover; d4 is no result); finit deletes the port only if
dos.library was open (V36 is always there where init's CreateMsgPort,
exec V36, is); NextDevice's d0 is the BSTR's length byte (the length
of the string it copied); GetFileName writes no 0, a3 on the end of the
characters (string results need none, the compiler adds it, see #257).
FreeDeviceList, like GetDeviceList and
NextDevice, goes through init's `_dosbase` (it also checks it); the C
build uses a6, the program's dos base: the same pointer, nothing
observable.

## Test

Every routine before init (0 or d0 unchanged); init and NotifyPort;
GetFileName and GetPathName with 19 paths (separators first, last,
doubled, none, empty) and a length with the high word set – d0, a3 and
the result buffer; notifications on a file in RAM: (ids 5, 9, 40 – each
set has its own port, so one write gives both their messages):
CheckNotify until empty after each write (at least one message),
WaitNotify, RemNotify twice and with id -1, a failing StartNotify on a
missing device (requesters off), finit with one request left;
GetDeviceList with devices+volumes+assigns, volumes, devices (stopped
after two), NextDevice to the end with DeviceEntryType after each entry,
then DeviceEntryType and NextDevice after FreeDeviceList. Kept registers
on every call. Not covered: ids outside 0..63 and names longer than the
buffer (they write outside the data).
