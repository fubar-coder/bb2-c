# ricommoditieslib (89)

**What it does:** turns a Blitz program into a commodity
(commodities.library V37). `MakeCommodity name$,title$,description$`
creates a message port (once) and a broker (`NBU_UNIQUE|NBU_NOTIFY`,
`COF_SHOW_HIDE`) and activates it; `SetHotKey n,hotkey$` attaches a
filter with a sender (id `n`, 16 slots); `CommodityEvent`/`WaitCommodity`
fetch a message, `HotKeyHit` (id of a `CXM_IEVENT`, else -1) and
`ExchangeMessage` (command of a `CXM_COMMAND`, else 0) evaluate it;
`ExchangeDisable` … `ExchangeUnique` test the last id against a
`CXCMD_` code, `CxDisable` … `CxUnique` are inline constants of those
codes; `SetCxStatus on|off`, `CommodityMsgPort`.

**Source / reference:** `rwelibsrc/Commoditieslib/CommoditiesLib2.bb`,
BSS2 `RICOMMODITIESLIB.OBJ` (the source builds to it byte for byte,
[README](../README.md) 3.1). C file: `libs/userlibs/rwe/ricommoditieslib.c`.

**C:** every routine; the commodities and exec calls through the NDK
inlines (base `st_cbase`, exec from address 4). Each entry is a thunk that
keeps d1/a0/a1 (the original keeps them in some paths; GCC keeps
d2-d7/a2-a6), so every register the original keeps is kept. The variables
are one packed struct in the order of the original's data block, so that
`SetHotKey`'s unchecked slot index hits the same neighbours.

**Assembler, and why:**

* the inline constants `CxDisable` … `CxUnique` (`BLITZ_INLINE`): the
  compiler copies their bytes into the program.

**Fixed** (intended deviations from the original,
`docs/libs/deviations.md` 3a, kind F):

* without commodities.library V37 (init's `OpenLibrary` failed)
  `MakeCommodity` returns 0 before it makes the port; the original did
  not check the base and called `CxBroker` through a base of 0, a crash
  (#559). The other commands need the broker or the port of a
  successful `MakeCommodity`.
* `SetCxStatus` tests the broker; the original tested the message port,
  which a `MakeCommodity` that fails at `CxBroker` (the name taken)
  leaves without a broker, and called `ActivateCxObj(0,…)` and returned
  -1 (#562). Values other than -1/0 do nothing and return 0, as in the
  original.
* the `Exchange…` functions return -1 only for a `CXM_COMMAND` message
  with that code; the original compared the id of the last message
  whatever its type, so a hotkey with id 15 counted as `ExchangeDisable`
  (#555).
* `WaitCommodity` takes the first message (`GetMsg`) and on an empty port
  waits for one (`WaitPort`, `GetMsg`). The original branched on the
  condition codes `GetMsg` leaves, inverted: exec (Kickstart 3.1, as
  measured by the scenario) leaves "ne", so it went to `WaitPort` and
  took its result, the head, which stays queued; with messages queued
  the one `GetMsg` removed was lost and a following `CommodityEvent`
  returned the same message again (#566).
* messages are replied: `CommodityEvent` and `WaitCommodity` reply the
  message fetched before (`HotKeyHit` and the others read the current
  one until the next fetch), finit replies the last one and, after the
  broker is deleted, those still queued, before it deletes the port. The
  original never replied a message (`replymessage` starts with `RTS` and
  is never called; not translated) and deleted the port with what was
  queued, so commodities could never free them: about 100 bytes per
  message (#561).
* finit closes commodities.library last, after the broker and the port;
  the original closed it first and then called `DeleteCxObj` through the
  closed base, which works only while the library stays in memory (it is
  expunged only if the program was its last user) (#556).
* finit deletes the broker with `DeleteCxObjAll`, the hotkey filters and
  senders attached to it with it, and clears the 16 slots; the
  original's `DeleteCxObj` deleted only the broker and left them
  allocated (and in the slots, for a second run from the editor) (#557).
* the library has an error check area (the original has none): with
  runtime error checking `SetHotKey` with a number outside 0–15 is the
  runtime error "Hotkey number must be 0 to 15" (`hotkeynumchk`). The
  number indexes the tables of 16 filters and senders, which lie in the
  variables' original layout; outside 0–15 it removes and overwrites
  the variables around them (with -1 the broker). Release code
  unchanged: an invalid argument (#564). The library header's error
  field and `SetHotKey`'s check differ from the reference's declarations
  (`ricommoditieslib.declarations-intended.txt`).
* `SetHotKey` deletes the filter of a replaced hotkey with its sender
  (`DeleteCxObjAll`, which detaches it too), and when a step fails it
  deletes the filter it made (with a sender attached to it) and clears
  the slots. The original only removed the replaced filter and sender
  (`RemoveCxObj`, the sender only if the filter existed), so every
  replacement lost two objects, and after a failure the half-made
  hotkey stayed in its slot and in the broker (#563).
* `SetHotKey` refuses a description commodities cannot parse: when
  `CxObjError` of the new filter is not 0 (`COERR_BADFILTER`), the filter
  is deleted and the result is 0. It no longer tests the d0 that the
  void `AttachCxObj` leaves. The original never called `CxObjError` (its
  `'oops2` tail is unreachable, not translated), so a misspelt hotkey
  returned -1 and never triggered, and it tested `AttachCxObj`'s d0
  (not 0 with Workbench 3.1's commodities.library) (#565).

**BUG(orig)** (kept, commented in the code):

* the help text of `MakeCommodity` lacks the closing parenthesis (#558);
* `MakeCommodity` makes a new broker on every call without deleting the
  previous one (which stays active, with the same port: finit deletes
  only the last one, so the first keeps the deleted port – a crash as
  soon as commodities sends it something, e.g. `CXCMD_LIST_CHG`) (#560);

No `DIFF`.

**Test** (`libs/tests/rwe/ricommoditieslib.c`, snitch case
"rwe ricommoditieslib"): each run loads one library set (the C build or
the original) and runs the scenario once (init … finit); every result is
observed as a case, and every call writes the kept registers of its
routine (stated per routine, `KEEPS`), compared with the expected data
`libs/tests/expected/rwe_ricommoditieslib.txt` recorded from the
original (450 cases). Only values that do not depend on addresses are
observed (results, "port != 0", message counts):

* every command before `MakeCommodity` (no broker, no port);
* the inline constants, copied with an RTS as the compiler does;
* `MakeCommodity` while a broker of the test has the name (fails at
  `CxBroker`, the port is made) and `SetCxStatus -1`/`0` with that port
  and no broker;
* `MakeCommodity`; a second one with the same name (fails,
  `CXCMD_UNIQUE` to the first broker through `NBU_NOTIFY`);
* `CXCMD_ENABLE`, `DISABLE`, `APPEAR`, `DISAPPEAR`, `KILL`, `LIST_CHG`
  through commodities' private `BrokerCommand` (LVO -198, as Exchange),
  each fetched with `CommodityEvent` and evaluated with `HotKeyHit`,
  `ExchangeMessage` and all seven `Exchange…`; `CommodityEvent` on an
  empty port;
* hotkeys on the input event class `timer` (input.device sends timer
  events continuously, so messages come without input): slots 0 and 3
  (ids in the order of the broker's list), slot 0 replaced (now behind
  3), slot 5 with a description ParseIX does not understand, slot 6 whose
  sender cannot be made (`CreateCxObj(CX_SEND)` made to fail);
* `WaitCommodity` with two messages queued (takes and returns the first;
  the original took the first and returned the second, #566);
* `SetCxStatus` 0 (no messages any more), 5 (nothing), -1 (messages
  again);
* finit with a message of the test queued (does it come back to the
  test's reply port).

While the scenario runs, commodities' `CreateCxObj`, `DeleteCxObj`,
`DeleteCxObjAll`, `RemoveCxObj` and exec's `ReplyMsg` and `CloseLibrary`
are patched (SetFunction): a stub notes the calls the library makes (per
`SetHotKey` and in finit: the commodities calls, whether commodities was
closed before the delete; after every fetch: whether the library replied
the message it took before) and then calls the real function with the
registers it got. The test does not reply the messages the library takes
from its port; it drains (and replies) what stays queued. The broker
name must be shorter than 24
characters (commodities keeps 23 and compares them with the full name).

**Not covered:** `WaitCommodity` on an empty port (waits, without a
hotkey forever), real key hotkeys (the keys would reach the console),
`SetHotKey` with a number outside 0–15 (bug-orig test
`ricommoditieslib-sethotkey-range`, with runtime error checking), two
successful `MakeCommodity`
(leaves a broker with a deleted port, see above), a missing
commodities.library (bug-orig test `ricommoditieslib-init-unchecked`,
inject `openlibfail=`).
