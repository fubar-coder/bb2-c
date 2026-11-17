# ridoslib (33): notification, device list, path splitting

Library 33 is reserved for `literallib` (#1018, `docs/token-table.md`):
RIDosLib keeps it in the RWE variants, which therefore have no
`literallib` and no literal characters; the two cannot be loaded
together (`docs/rwelibs/README.md` §3.1).

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
  `dol_Type` of the current entry (noted while the list is locked), -1
  if none; `FreeDeviceList` –
  `UnLockDosList`.
* `GetFileName(p$)` – the part behind the last `/` or `:`;
  `GetPathName(p$)` – the part up to and including it. Both give the
  argument's string space back and copy the result to its start (the
  original through a 256 byte buffer, #627).

init opens dos.library V36 and creates the port; finit ends the requests
still active, closes dos.library and deletes the port.

* **Source:** `rwelibsrc/DOSLib/ridoslib2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/DOSLib/ridoslib2.bb`).
* **Reference:** BSS2 `RIDOSLIB.OBJ`; declarations equal but for the error checks of #626 (ctest
  `rwe_declarations_ridoslib`).
* **C file:** `libs/userlibs/rwe/ridoslib.c`; test
  `libs/tests/rwe/ridoslib.c` (snitch case "rwe ridoslib").

## C and assembler

All routines are C behind `BLITZ_THUNK`, the system calls through the
NDK inlines; no other assembler. The tables `notifys`/`filelens` have the
original's zero long in front (an id of -1 reads it, as in the original);
AddNotify computes the table offset as a word (`ASL.w`/`ADD.w`), RemNotify
as a long, like the original. Routines that return with d0 unchanged in
the original (RemNotify, FreeDeviceList without dos.library) do so in C
too. The string lengths are read below a2 without changing it
(DIFF as in all C ports, `docs/libs/cmdlibs-port.md` "General"). The
thunks keep d1/a0/a1, the C code d2-d7/a2-a6; the original changes
d0-d2, d4/d5, a0/a1 and a6 in places.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* NextDevice and GetPathName leave a3 on the 0 of their result; the
  original copied the result with its 0 and left a3 behind it, so in a
  string expression the next part was written one byte too far
  (`GetPathName("DH0:dir/file")+"x"` held the 0 and lost the `x`)
  (#618).
* FreeDeviceList does nothing (d0 = 0) without a locked list. The
  original did not check it: a second call, or one without
  GetDeviceList, made UnLockDosList release a semaphore once too often
  (exec's AN_SemCorrupt alert, the semaphore re-initialised) and call
  Permit once too often, so later Forbid calls of the task had no effect
  (#623; test `ridoslib-freedevicelist-unchecked-unlock`).
* NextDevice without a locked list returns "" (d0 = 0). The original
  returned with d0 unchanged: the result string's length was whatever d0
  held, its characters whatever the string buffer held (#625; test
  `ridoslib-nextdevice-no-list-length`, `libs_emulator` cases
  `[fixed #625]` of `rwe_ridoslib`).
* DeviceEntryType returns the type that GetDeviceList and NextDevice
  noted while the list was locked; it reads no entry, so its result does
  not change after FreeDeviceList (no allocation). The original read
  `dol_Type` of the current entry also after the unlock, when the entry
  may have been removed and freed (#622; test
  `ridoslib-devicetype-after-free`).
* AddNotify without memory for the name copy or the request returns 0,
  frees the name copy if only the request failed and leaves the table
  alone. The original used both AllocMem results unchecked and wrote the
  name or the request to address 0 on (#619; tests
  `ridoslib-addnotify-allocmem-unchecked`,
  `ridoslib-addnotify-allocmem-request`).
* A request whose StartNotify fails is taken out of the table and freed
  with its name copy at once; finit looks at all 64 entries. The original
  left such a request in the table, uncounted, and finit stopped when
  the counter was 0, so it could stop before it: request and name copy
  stayed allocated after the end (#621; test
  `ridoslib-failed-startnotify-kept`).
* AddNotify with an id in use first ends and frees the request there, as
  RemNotify does, then enters the new one (once its memory is there).
  The original overwrote the entry: the old request stayed active, its
  messages kept coming, and it could no longer be ended (#620; test
  `ridoslib-addnotify-id-reused`). The manual's note "NO CHECKING is made
  to ensure slots are free" describes the original's behaviour.
* AddNotify and RemNotify use the id as the index of the two tables of
  64 entries (AddNotify's offset id*4 as a word, RemNotify's as a long);
  an id outside 0..63 writes or reads, ends and frees the library's data
  around them. The original has no error check area; the library now has
  one: with runtime error checking such an id is the runtime error
  "Notify number must be 0 to 63" (`notifyidchk`); the release code is
  unchanged, an invalid argument (#626; tests `ridoslib-notify-id-range`,
  `ridoslib-remnotify-id-range`; the declarations differ from the
  reference as `ridoslib.declarations-intended.txt` lists).
* finit unlocks a DOS list that GetDeviceList still holds (before it
  closes dos.library), and GetDeviceList unlocks a list it still holds
  before it locks again. The original's finit has the call commented out
  (`;  BSR _unlockdoslist`), and its GetDeviceList locked a second time
  and overwrote the first lock: after a program that ended without
  FreeDeviceList, or with two GetDeviceList and one FreeDeviceList, the
  list stayed locked for reading and every later writer (Assign, a disk
  change, mounting) waited for ever (#1081; tests
  `ridoslib-doslist-locked-at-end`, `ridoslib-getdevicelist-twice`,
  through inject's `doslist`).
* GetFileName and GetPathName copy their result straight into the
  argument's space (it is never longer than the argument). The original
  built it in a 256 byte buffer first, without a limit: a file or path
  part of 256 characters or more overwrote the code behind the buffer
  (`NotifyPort`, `CheckNotify` …), the C build other data (#627; test
  `ridoslib-path-buffer-overflow`).
* GetFileName and GetPathName read the length of their argument as a
  long. The original read its low word (`MOVE.w -(a2)`): for 65536
  characters or more it gave the wrong amount of string space back (the
  program then found the wrong characters as the result) and GetPathName
  searched only the first (length AND $FFFF) characters (#628; test
  `ridoslib-path-length-word`, `libs_emulator` cases `[fixed #628]` of
  `rwe_ridoslib`).

## BUG(orig)

* GetPathName never looks at the first character: `/abc` and `:x` give
  "", like a name without separator (#624).

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

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_ridoslib.txt` recorded from the original.
Every routine before init (0 or d0 unchanged); init and NotifyPort;
GetFileName and GetPathName with 19 paths (separators first, last,
doubled, none, empty) and an argument of $10006 characters – d0, a3 and
the result buffer; notifications on a file in RAM: (ids 5, 9, 40):
CheckNotify until empty after each write (at least one message),
WaitNotify, RemNotify twice and with id -1, a failing StartNotify on a
missing device (requesters off), finit with one request left;
GetDeviceList with devices+volumes+assigns, volumes, devices (stopped
after two), NextDevice to the end with DeviceEntryType after each entry,
then DeviceEntryType and NextDevice after FreeDeviceList. Kept registers
on every call. Not covered: ids outside 0..63 (they write outside the
tables) and parts longer than 255 characters (the original writes over
its code; `ridoslib-path-buffer-overflow`).
