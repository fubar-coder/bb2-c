# riarexxlib (133): ARexx ports and messages

"RIARexxLib 2" ((C) 1997 Red When Excited; Steven Matty, Stephen
McNamara). It gives a Blitz program one public message port and the
ARexx message handling around it:

* `RXCreatePort(name$)`, `RXDeletePort`;
* `RXReadPort`, `RXWaitPort`: the next received message becomes the
  current one;
* `RXNumArgs`, `RXGetArg(n)`, `RXNextArg`, `RXSendersName`: the current
  message;
* `RXReplyMsg errcode[,result$]`;
* `RXRunCommand [port$,]command$`: send, do not wait;
  `RXSendCommand([port$,]command$)`: send, wait, return the result
  string; `RXReturnCode`;
* `RXExecuteScript`/`RXRunScript scriptname$` (to "REXX");
* `RXDefaultPort port$`, `RXTokenise on|off`, `RXQuoteChar char$`,
  `RXMessageList`.

The commands have the error check `portchk` ("No message port opened!").

* **Source:** `rwelibsrc/rexxlib/NEWAREXXLIB.bb` (BB22 format, saved
  1997-09-21; readable: `build/amiga/rwelibsrc/rexxlib/NEWAREXXLIB.bb`).
  The source defines its number itself (`#myconst=133`, `!libheader
  {#myconst,...}`), so libnums.h of the variant has `LIB_riarexxlib 133`
  with that remark. BSS2 `RIAREXXLIB.OBJ` has 133 as well.
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`). The BSS2 binary is the older generation (newrexxlib7
  without RXTokenise, `docs/rwelibs/README.md` §3.2). The declarations,
  including the 11 nullsub slots of the dropped commands and the error
  check, are equal (ctest `rwe_declarations_riarexxlib`).
* **C file:** `libs/userlibs/rwe/riarexxlib.c`; test
  `libs/tests/rwe/riarexxlib.c` (snitch case "rwe riarexxlib").

## How it works

* **init** opens rexxsyslib.library V34. It sets the quote character to
  `"`, sets tokenising on and the default port name to "REXX", and clears
  the port pointer.
* **RXCreatePort** copies the name (maxslib AllocSt) and creates and adds
  the port. It allocates one block of 256 bytes: the list of received
  messages (32 longs) and the list of messages sent without waiting.
* **Sending.** A message is a 128-byte RexxMsg (AllocVec):
  * ln_Name = our port's name, ln_Pri 1, the reply port = ours;
  * rm_Action = RXCOMM|RXFF_STRING, plus RXFF_RESULT for RXSendCommand;
  * rm_Args = the command as one argument string, or with RXTokenise on
    (the default) split into words. Spaces separate words; text between
    quote characters is one word, and a doubled quote is a quote.
* **RXReadPort** takes everything from the port:
  * replies to our messages are freed;
  * messages are appended to the list;
  * other message types are replied at once.

  The first message of the list becomes the current one, and that is the
  result.
* **RXWaitPort** waits first, unless the list has a message.
* **RXSendCommand** puts its message and loops over WaitPort/readport
  until its reply arrives. Messages that arrive in the meantime stay in
  the list (`oktodelete`). The result string of the reply (copied with
  AllocSt) is written at a3, over the two arguments in the two-argument
  form (a3 back by both lengths + 2: "FUCKING SHITE STRING CLUDGE").

## C and assembler

C behind thunks. String results are written at a3 (`blitz_string`), and
a3 ends at their 0 as in the original. The original reads the string
lengths with `-(a2)`; the C build reads them without moving a2 (DIFF, as
in all ports: `docs/libs/cmdlibs-port.md` "General").

The variables from `rexxportname` on are one packed struct in the
original's order (`_Static_assert` on the offsets of the reference
build). RXDefaultPort copies without a limit into its 65 bytes and then
overwrites rexxbase, destport, … (BUG(orig), #529).

Small inline assembler for the exec and rexxsyslib calls whose leftovers
the original uses:

* the Z flag of AllocVec (`MOVE.l d0,a1:BEQ` is a MOVEA);
* d0 of AddPort (`TST.l d0` after it) and of PutMsg (RXRunCommand's
  result);
* a0 of FindPort (RXReplyMsg's `'shit` path).

maxslib AllocSt is called with `BLITZ_ALIBJSR(TOK_allocst)`.

## BUG(orig), kept

* **init** writes "REXX" and a 0 at +5: the byte at +4 of an earlier
  RXDefaultPort stays (after RXDefaultPort "RWERESP" and init: "REXXE")
  (#532).
* **RXExecuteScript/RXRunScript** set their own message type, which
  `_rxruncommand` overwrites. Both send RXCOMM|RXFF_STRING without
  RXFF_RESULT: the script name is run as a string program, tokenised
  (#544).
* **`_makemsg`** decides on AllocVec's flags (above): if they say 0, the
  previous message (or 0) is sent again (#533).
* **RXRunCommand** returns whatever d0 PutMsg leaves (#543).
* **Replies to messages sent without waiting.** `_finditem` falls from
  the found case into `'notfound` and always returns 0. So these replies
  are only FreeVec'd: their argument strings stay allocated, and the
  message stays in the sent list, which grows with every RXRunCommand
  and overflows the block after 32 (#531).
* **The reply of RXSendCommand:**
  * its Result2 is copied with AllocSt and never freed (#546);
  * the reply's own argument string is never deleted (#546);
  * Result2 is read as a string even when Result1 is not 0 (#545).
* **RXReturnCode** returns 0 while no message is current, e.g. right
  after an RXSendCommand that found the list empty (#542).
* **RXReplyMsg:**
  * it does not check whether a result was asked for (#539);
  * the replied message stays the current one (#541);
  * if no port of the message's ln_Name exists, the message is not
    replied, and its Result2 is deleted, as decided by the Result2 at the
    a0 that FindPort leaves (#540).
* **RXCreatePort** tests d0 of AddPort. On 0, or if the lists cannot be
  allocated, it deletes the port without RemPort, so the port stays in
  the public list (#528).
* **RXDeletePort** replies the queued messages with 5 but frees neither
  the lists nor the name (#530).
* **The tokenizer:**
  * the command (256 bytes) and every word (256 bytes) are copied without
    a limit; the original then overwrites its own code (C: the data
    behind the buffers) (#537);
  * a quote right after a closing quote is a quote, and any other
    character there is skipped (`"g"h` gives `g`) (#538, #535);
  * an empty word (`""`, `''`) ends the arguments (#536);
  * the 16 rm_Args slots are not checked (#527).
* **RXNextArg** does not advance after an empty argument (it returns ""
  again) (#534).
* **RXTokenise** stores the whole register of its word argument, so
  "off" with a set upper word stays on (#547).

Original behaviour that is not a defect: an empty result of
RXSendCommand gives 0 (AllocSt of length 0), the same as no result;
init clears the port pointer without deleting a port (it runs before
any RXCreatePort).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riarexxlib.txt` recorded from the original.

No RexxMast is needed (if one runs, as in the emulator's OS 3.2, the
scripts go to it). Messages go to:

* the library's own port (RWERX_P);
* the test's port RWETEST: messages the test sends, and their replies;
* a responder process (port RWERESP, and REXX if none exists). It logs
  every message and replies with a result code and string the test sets,
  which RXSendCommand needs because it waits.

Observed: d0, a3 and the string area of the string functions; the
messages the responder got (type, priority, length, action, arguments,
ln_Name and the reply port); the replies the test gets (Result1,
the Result2 string); the kept registers.

Covered:

* the functions without a port and without a message;
* portchk with a port; RXCreatePort twice;
* the tokenizer: spaces, quotes, doubled quotes, the skipped character,
  RXQuoteChar `'`, an empty command;
* RXTokenise off, and "off" with an upper word;
* RXGetArg 0..17, RXNextArg to the end, RXNumArgs, RXSendersName;
* RXReplyMsg in both forms: to our own port, to the test (Result2
  string), and to a sender that is gone (no reply);
* RXRunCommand in both forms, to a missing port and to the default port;
* RXWaitPort, both with a message in the list and waiting for a reply;
* RXSendCommand in both forms: with result code 10, an empty result, a
  missing port, and a message arriving during the wait;
* RXReturnCode with and without a current message;
* RXExecuteScript/RXRunScript (to RexxMast with the string program
  "exit", or to the responder: RXCOMM|RXFF_STRING);
* RXDeletePort with two queued messages (replied with 5), and without a
  port;
* finit with a port; init after RXDefaultPort ("REXXE").

Not covered:

* portchk without a port (TRAP #0);
* the exec leftovers in their failing cases (AllocVec's flags, AddPort's
  d0);
* the overflows (more than 16 arguments or 32 messages, long names);
* messages of other types.
