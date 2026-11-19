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

* **init** opens rexxsyslib.library V34 (finit closes it). It sets the
  quote character to `"`, sets tokenising on and the default port name
  to "REXX", and clears the port pointer.
* **RXCreatePort** allocates one block of 256 bytes, the list of
  received messages (32 longs) and the list of messages sent without
  waiting (31 and the end), copies the name (maxslib AllocSt), creates
  the port and adds it, unless a public port of the name exists.
* **Sending.** A message is a 128-byte RexxMsg (AllocVec):
  * ln_Name = our port's name, ln_Pri 1, the reply port = ours;
  * rm_Action = RXCOMM|RXFF_STRING, plus RXFF_RESULT for RXSendCommand;
    RXCOMM|RXFF_RESULT for RXExecuteScript, RXCOMM for RXRunScript;
  * rm_Args = the command as one argument string (also the script name
    of RXExecuteScript/RXRunScript), or with RXTokenise on (the default)
    split into at most 16 words. Spaces separate words; text between
    quote characters is part of a word, a doubled quote in it is a
    quote, and the character after a closing quote starts a new word.
* **RXReadPort** takes everything from the port:
  * replies to our messages are freed;
  * messages are appended to the list;
  * other message types are replied at once.

  The first message of the list becomes the current one, and that is the
  result.
* **RXWaitPort** waits first, unless the list has a message.
* **RXSendCommand** puts its message and loops over WaitPort/readport
  until its reply arrives (RXExecuteScript too). Messages that arrive in
  the meantime stay in the list (`oktodelete`). The result string of the
  reply is written at a3, over the two arguments in the two-argument
  form (a3 back by both lengths + 2: "FUCKING SHITE STRING CLUDGE"), and
  the reply's string deleted.

## C and assembler

C behind thunks. String results are written at a3 (`blitz_string`), and
a3 ends at their 0 as in the original. The original reads the string
lengths with `-(a2)`; the C build reads them without moving a2 (DIFF, as
in all ports: `docs/libs/cmdlibs-port.md` "General").

The variables from `rexxbase` on are one packed struct in the
original's order (`_Static_assert` on the offset of the reference
build).

Small inline assembler for the rexxsyslib calls (CreateArgString,
DeleteArgString).

maxslib AllocSt is called with `BLITZ_ALIBJSR(TOK_allocst)`.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* **init** makes the default port "REXX" again. The original wrote
  "REXX" and a 0 at +5 over the name, so the byte at +4 of an earlier
  RXDefaultPort stayed: after RXDefaultPort "RWERESP" and a second run
  of the program with the library still loaded (from the editor) the
  default port was "REXXE" (#532).
* **RXTokenise** stores and tests the word of its argument. The original
  stored the whole register (`MOVE.l d0`), so "off" with a set upper
  word stayed on (#547).
* **RXRunCommand** (and RXExecuteScript, RXRunScript, which share its
  code) returns -1 when the message was sent, 0 when not, as the manual
  says of the older generation's RXSendMsg. The original returned
  whatever d0 PutMsg leaves (#543).
* **RXNextArg** advances past every argument it returns, an empty one
  too (the argument slot is not 0). The original tested the length, so
  at an empty argument it returned "" for ever (#534).
* **The tokenizer:** the character after a closing quote starts the next
  word (user decision W4 Q2 (a)): `"g"h` gives `g` and `h`. The original
  skipped it, meant for the space behind the word: `"g"h` gave `g` alone,
  and after a quoted last word it skipped the terminating 0 and read on
  in its buffer, so the rest of an earlier, longer command became further
  arguments (`say "hi"` after `one two three four five six`: `say`, `hi`,
  `hree`, `four`, `five`, `six`) (#538).
* **The tokenizer:** in a quoted word two quotes are one quote character
  and any other quote closes the word: `"e""f" c d` gives `e"f`, `c`,
  `d`. The original counted the quotes, so after a doubled quote the
  next quote did not close the word and the character behind it was
  taken in: one word `e"f c d` (#535).
* **The tokenizer:** an empty quoted word (`""`, `''`) is an empty
  argument string: `a "" c` gives `a`, ``, `c`. The original took the
  word's length 0 for the end of the command and dropped it and every
  word behind it (#536).
* **The tokenizer** reads the command where it is and builds each word
  in a buffer of the command's length, allocated for the message (out of
  memory: nothing is sent, the result is 0, D-018). A command or a word
  of any length is split whole (a long string is a value). The original
  copied the command into `tempstorage` and every word into
  `stringbuffer`, 256 bytes each, without a limit: a longer command
  overwrote what follows, in the original its own code (#537).
* **The tokenizer** makes at most 16 arguments, the slots of a RexxMsg.
  With more words the 16th argument holds the rest of the command from
  its 16th word on, as it is, for the receiver to split (user decision W4
  Q3 (b)). The original had no limit: the 17th and further words went
  over rm_PassPort, rm_CommAddr and the fields behind them (#527).
* **`_makemsg`** tests the result of AllocVec: without memory for the
  message nothing is sent, RXRunCommand returns 0 and RXSendCommand ""
  (D-018). The original tested the flags AllocVec leaves (`MOVE.l
  d0,a1:BEQ` is a MOVEA, which sets none): when they said 0 it went on
  with the message made or received before, wrote the new arguments into
  it and put it to the port again (#533).
* **RXDeletePort** frees the 256-byte block of the message lists
  (FreeVec) and the copy of the port name (maxslib FreeSt) after it has
  deleted the port; finit frees the block and leaves the name to maxslib,
  which frees its strings at the end of the program. The original freed
  neither: the block (exec memory, not Blitz's) was lost for good at
  every RXCreatePort/RXDeletePort pair and at the end of the program,
  the name until the end of the program (#530).
* **Replies to messages sent without waiting** (RXRunCommand,
  RXExecuteScript, RXRunScript) are looked up in the sent list and freed
  with their argument strings and their entry. The list holds 31
  messages; when it is full nothing is sent and the result is 0. The
  original's `_finditem` fell from the found case into `'notfound` and
  always returned 0, so these replies were only FreeVec'd: their argument
  strings stayed allocated, and the message stayed in the sent list,
  which grew with every message and overflowed the block after 32 (#531).
* **RXCreatePort** allocates the block of the lists first and makes the
  port public last; AddPort, which returns nothing, is not tested. If the
  block or the port cannot be made, it frees what it has and returns 0
  (D-018). The original added the port first and tested the d0 AddPort
  leaves; on 0, or if the lists could not be allocated, it deleted the
  port without RemPort, so the freed port stayed in exec's list of
  public ports (#528).
* **RXDefaultPort** keeps the name as a copy of its own length (maxslib
  AllocSt) and frees the copy before; a long name is a value, not cut.
  The original copied it without a limit into the 65 bytes of
  `rexxportname`, over rexxbase, destport and the other variables behind
  it: with a name of more than 65 characters the next message jumped
  through the overwritten rexxsyslib base (#529).
* **RXReplyMsg errcode,result$** gives a result string only to a message
  that asked for one (RXFF_RESULT in rm_Action) and only with the code 0;
  otherwise rm_Result2 stays 0 (with a code it is a secondary error
  code). The original always put an argument string of result$ into
  rm_Result2: a sender that asked for no result never deletes it, and one
  that reads rm_Result2 as a code gets an address (#539).
* **RXReplyMsg** looks for the sender's port (the message's ln_Name)
  first; when it is gone the message is not replied and no result string
  is made. The original made the result string first and, with the port
  gone, decided whether to delete it again by the long 36 bytes behind
  the a0 that FindPort leaves: the string was lost, or the form without a
  result string called DeleteArgString with 0 (#540).
* **RXReplyMsg** leaves no current message, also when the sender's port
  is gone: RXGetArg, RXNextArg, RXNumArgs, RXSendersName give 0 or ""
  until the next RXReadPort/RXWaitPort. The original kept the replied
  message current, so these read a message that belonged to its sender
  again and could already be freed (#541). The manual asks for the
  information to be taken before the reply, so a program that follows it
  sees no change.
* **RXSendCommand** takes the reply's Result2 as its result string only
  when Result1 is 0; with an error (Result1 not 0) Result2 is a secondary
  error code and the result is "". The original read it as the address
  of a string also then: after a failing ARexx program (error 6,
  "unterminated comment") it returned the bytes from address 6 on (#545).
* **RXSendCommand** copies the reply's result string to a3 and deletes
  it (rexxsyslib DeleteArgString, the sender's duty). The original copied
  it with AllocSt, never freed the copy and never deleted the reply's
  string: two strings lost per call (#546).
* **RXReturnCode** returns the Result1 of the last reply to
  RXSendCommand also while no message is current. The original returned
  0 then, e.g. right after an RXSendCommand that found no message of its
  own waiting, so the code of a failed command was lost (#542).
* **RXExecuteScript** sends the script name as one argument with
  RXCOMM|RXFF_RESULT to the port REXX and waits for the end of the
  script, as RXSendCommand waits (messages that arrive meanwhile go to
  the message list); the script's code is RXReturnCode, a result string
  is deleted (no command reads it), the result is -1 when sent, 0 when
  there is no REXX port. **RXRunScript** sends the name as one argument
  with RXCOMM and does not wait, as RXRunCommand (user decision W4 Q1
  (b); the declarations are unchanged). The original set these types
  but `_rxruncommand` overwrote them: both sent RXCOMM|RXFF_STRING, the
  script name as a tokenised string program, so no script was ever
  loaded, and neither waited (#544). A script that sends a command to
  the program's own port and waits for its reply cannot be run with
  RXExecuteScript, which waits too: RXRunScript is for it.
* **finit** closes rexxsyslib.library after it has deleted the port. The
  original never closed it: one open count of the library was lost per
  program run, so it could never be expunged (#1093).
* **Without rexxsyslib.library** (init could not open it) nothing is
  sent: RXRunCommand, RXExecuteScript and RXRunScript return 0,
  RXSendCommand ""; RXReplyMsg replies without a result string. The
  original never checked the base and called CreateArgString through a
  base of 0 at the first message (a crash) (#1094).
* **RXCreatePort** returns 0 when a public port of the name exists, as
  the manual says ("Failure is usually due to a public message port
  existing with the same name"). The original added a second port of the
  name, so messages for it reached whichever port FindPort found first
  and the program could not tell that its name was taken (#1096).
* **RXDeletePort** (and finit) reply the current message first, with
  code 5 as the other waiting messages. The original's loop called
  RXReadPort first, which made the next message of the list current
  without replying the one that was current: a message taken with
  RXReadPort/RXWaitPort and not yet replied when the port was deleted
  never got its reply, and its sender waited for ever (#1101). The
  library test's case `RXDeletePort|replies at the test's port|33
  messages` has a current message: the original replies 32 of the 33.

## BUG(orig), kept

* **RXDeletePort** and finit do not wait for the replies to the messages
  sent without waiting (RXRunCommand, RXRunScript) that are still with
  their receivers: a reply that arrives later goes to the deleted port
  (`RXRunCommand "REXX",... : End` is enough). The manual says RXDeletePort
  waits for them (#1095). The case `RXDeletePort|replied before it
  returned|reply outstanding` of the library test holds a message at the
  responder while RXDeletePort runs: the original returns with the reply
  outstanding (0).
* **RXReadPort**, **RXWaitPort** (and finit, RXDeletePort through the
  same routine) append every message they take from the port to the
  message list with `addtolist`, which has no limit. The list has 32
  longs (the first half of the 256-byte block, the sent list is the
  second): the 32nd message overwrites the list's end mark, the 33rd to
  the 64th run into the sent list and later ones past the block. The
  library test puts 33 messages to the port and observes the list's
  32nd long, the one behind it and the number of longs of the block
  that are not 0 (`RXMessageList|long 31 of the list|33 messages` and
  its two neighbours; the 33 stay inside the block, the test does not
  corrupt memory) (#1100). There is no bug-orig test (D-019): a Blitz
  program that shows the overflow needs more than 64 messages, writes
  behind the block and then hangs or not depending on what lies there
  (RXDeletePort replies up to the first 0 long), so the outcome differs
  between the configurations.

Original behaviour that is not a defect: an empty result of
RXSendCommand gives 0 (AllocSt of length 0), the same as no result;
init clears the port pointer without deleting a port (it runs before
any RXCreatePort).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riarexxlib.txt` recorded from the original.

No RexxMast is needed; the sessions (Kickstart/Workbench 3.1) do not
start it (if one runs, the scripts go to it; the bug-orig tests that
need it start it themselves). Messages go to:

* the library's own port (RWERX_P);
* the test's port RWETEST: messages the test sends, and their replies;
* a responder process (port RWERESP, and REXX if none exists). It logs
  every message and replies with a result code and string the test sets,
  which RXSendCommand needs because it waits.

Observed: d0, a3 and the string area of the string functions; the
messages the responder got (type, priority, length, action, arguments,
ln_Name and the reply port); the replies the test gets (Result1,
whether there is a Result2 and its string); the kept registers.

Covered:

* the functions without a port and without a message;
* portchk with a port; RXCreatePort twice;
* the tokenizer, one command per case: spaces, quotes, empty quotes in
  a word, a doubled quote, the character after a closing quote, an empty
  word, a closing quote at the end of a command after a longer one,
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
* RXExecuteScript/RXRunScript (to the responder, or to RexxMast if one
  runs): the message's action;
* RXDeletePort with two queued messages (replied with 5), with 33
  messages and one of them current (#1100, #1101), with a reply the
  responder holds for 2 s and one it holds for 8 s (#1095), and without
  a port;
* finit with a port; init after RXDefaultPort (the original: "REXXE",
  nothing sent; fixed: "REXX", #532).

Not covered:

* portchk without a port (TRAP #0);
* more than 16 arguments, more than 31 messages sent, long names, out
  of memory, no rexxsyslib.library (the bug-orig tests of #527, #531,
  #529, #537, #528, #533, #1094);
* messages of other types.
