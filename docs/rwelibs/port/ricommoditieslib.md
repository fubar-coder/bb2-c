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
`AttachCxObj` (void) is called through the jump table
(`BLITZ_BASEJSR`), because the original tests the d0 it leaves.

**Assembler, and why:**

* the inline constants `CxDisable` … `CxUnique` (`BLITZ_INLINE`): the
  compiler copies their bytes into the program;
* `getmsg_cc`: the `GetMsg` of `WaitCommodity` with an `SNE` right after
  it – the original branches on the condition codes exec leaves, which C
  cannot see.

**BUG(orig)** (kept, commented in the code):

* the help text of `MakeCommodity` lacks the closing parenthesis;
* init does not check `OpenLibrary`; without commodities.library V37
  every command calls through a base of 0;
* finit closes commodities.library first and then calls `DeleteCxObj`
  through the closed base; `DeleteCxObj` (not `DeleteCxObjAll`) leaves
  the filters and senders allocated; the port is deleted with whatever is
  still queued;
* `WaitCommodity` tests the CCR after `GetMsg` and is inverted: "ne" →
  `WaitPort` (waits for the *next* message and leaves it queued; the one
  `GetMsg` removed is lost), "eq" → `msg` = the result of `GetMsg`. Exec
  (3.2) leaves "ne", so it waits when the port is empty and skips one
  message when it is not;
* messages are never replied (`replymessage` starts with `RTS` and is
  never called; not translated);
* `SetHotKey` does not check the number; a replaced filter/sender is
  removed but not deleted; the sender is only removed if the filter
  existed; objects made before a failure stay; `CxObjError` is not looked
  at (the `'oops2` tail is unreachable, not translated);
* `MakeCommodity` makes a new broker on every call without deleting the
  previous one (which stays active, with the same port: finit deletes
  only the last one, so the first keeps the deleted port – a crash as
  soon as commodities sends it something, e.g. `CXCMD_LIST_CHG`);
* `SetCxStatus` tests the message port, not the broker; values other than
  -1/0 do nothing and return 0;
* the `Exchange…` functions look at the id of the last message whatever
  its type (a hotkey with id 15 counts as `ExchangeDisable`).

No `DIFF`.

**Test** (`libs/tests/rwe/ricommoditieslib.c`, snitch case
"rwe ricommoditieslib"): the same scenario on the original and then on
the C build (init … finit each); all results and the register
preservation of every call are compared (about 400 checks):

* every command before `MakeCommodity` (no broker, no port);
* the inline constants, copied with an RTS as the compiler does;
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
  3), slot 5 with a description ParseIX does not understand;
* `WaitCommodity` with two messages queued (takes the first, returns the
  second);
* `SetCxStatus` 0 (no messages any more), 5 (nothing), -1 (messages
  again).

The test replies the messages the library takes from its port and drains
the port before finit. The broker name must be shorter than 24
characters (commodities keeps 23 and compares them with the full name).

**Not covered:** `WaitCommodity` on an empty port (waits, without a
hotkey forever), real key hotkeys (the keys would reach the console),
`SetHotKey` with a number outside 0–15, two successful `MakeCommodity`
(leaves a broker with a deleted port, see above), a missing
commodities.library.
