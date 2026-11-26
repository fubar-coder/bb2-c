---
title: riarexxlib
weight: 8
description: The bugs of Blitz Basic 2 in the RWE userlib riarexxlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `RXRunCommand` returned a value the system left {#rxruncommand-result}

[riarexxlib]({{< relref "/docs/libraries/userlibs/riarexxlib" >}})
gives a program an ARexx port. `RXRunCommand` sends a command to the
port of another program, or to ARexx, and goes on without waiting for
the reply; its result tells whether the command was sent.
`RXRunScript` and `RXExecuteScript`, which send a script to ARexx, give
the same result.

**What happened.** When the command was sent, the original returned
whatever the system's call that sends a message had left in a register,
not a defined value. Two runs of the same program gave 0 and 256, so a
program that tested the result could take a command that was sent for
one that was not. This program sends a command to its own port and
prints whether the result is -1:

```
DEFTYPE .l
p=RXCreatePort("BUGORIG")
r=RXRunCommand("BUGORIG","hello")
NPrint "result -1: ",r=-1
m=RXReadPort
If m Then RXReplyMsg 0
End
```

**With the original**

```
result -1: 0
```

**With the port**

```
result -1: -1
```

With the port, `RXRunCommand` returns -1 when the command was sent and
0 when it was not: when the port is not found, when rexxsyslib.library
is missing, when there is no memory for the message, or when 31
commands sent this way still wait for their replies.

**What to change.** A program that ignored the result works as before.
One that tested it now finds it true whenever the command was sent;
test it as a truth value, `If RXRunCommand(...)`, or compare it with
0.

Issue: [RXRunCommand returns whatever d0 PutMsg leaves (#543)](https://github.com/fubar-coder/bb2-c/issues/543).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riarexxlib {#riarexxlib-fixes}

These fixes concern
[riarexxlib]({{< relref "/docs/libraries/userlibs/riarexxlib" >}}), the
ARexx port by Red When Excited. `RXCreatePort` gives the program a
public message port; `RXReadPort` and `RXWaitPort` make a received
message the current one, and `RXReplyMsg` answers it. `RXSendCommand`
sends a command and waits for the reply, `RXRunCommand` sends one
without waiting, and both split the command into words, one argument
each, unless `RXTokenise Off` is given. The fix of
[the result of `RXRunCommand`](#rxruncommand-result) is described above.

The port and the library:

* `RXCreatePort` works on Kickstart 1.x: the library builds its port,
  its lists and its messages with functions that every Kickstart has.
  The original used functions of Kickstart 2.0 for them, and a program
  crashed at its first `RXCreatePort` on Kickstart 1.x
  ([#1144](https://github.com/fubar-coder/bb2-c/issues/1144)).
* `RXCreatePort` returns 0 when a public port of that name exists, as
  the guide says. The original added a second port of the same name, so
  messages for it reached whichever of the two ports the system found
  first, and the program could not tell that its name was taken
  ([#1096](https://github.com/fubar-coder/bb2-c/issues/1096)).
* `RXCreatePort` that cannot set up its port completely frees it and
  returns 0. The original freed the port but left it in the system's
  list of public ports
  ([#528](https://github.com/fubar-coder/bb2-c/issues/528)).
* `RXDeletePort`, and the end of the program, free the lists of messages
  and the copy of the port's name; the original lost that memory at
  every `RXCreatePort` and `RXDeletePort`
  ([#530](https://github.com/fubar-coder/bb2-c/issues/530)).
* `RXDeletePort`, and the end of the program, also reply the message
  that is current, with the return code 5 as the others. The original
  replied only the messages still waiting, so the sender of the current
  one waited for ever
  ([#1101](https://github.com/fubar-coder/bb2-c/issues/1101)).
* `RXDeletePort`, and the end of the program, wait up to 5 seconds for
  the replies to commands sent with `RXRunCommand` or `RXRunScript`
  that are still with their receivers, and reply the messages that
  arrive meanwhile with the return code 5. A reply that is still out
  after 5 seconds finds the port taken off the public list but kept in
  memory for it. The original deleted the port at once, so a late reply
  was written into freed memory. A program whose receiver is slow, or
  never replies, takes up to 5 seconds longer to end
  ([#1095](https://github.com/fubar-coder/bb2-c/issues/1095)).
* A program that uses the library closes rexxsyslib.library at its end;
  the original left it open, once more with every run, so the system
  could never remove it from memory
  ([#1093](https://github.com/fubar-coder/bb2-c/issues/1093)).
* Without rexxsyslib.library the library sends nothing: `RXRunCommand`,
  `RXRunScript` and `RXExecuteScript` return 0 and `RXSendCommand` an
  empty string, and `RXReplyMsg` replies without a result string. The
  original crashed at the first message
  ([#1094](https://github.com/fubar-coder/bb2-c/issues/1094)).
* `RXDefaultPort` takes a name of any length. The original copied it
  into 65 bytes without a limit, so a longer name overwrote the library's
  other data and the next command crashed
  ([#529](https://github.com/fubar-coder/bb2-c/issues/529)).
* A program run a second time from the editor sends to the default port
  `REXX` again. After an `RXDefaultPort "RWERESP"` in the run before,
  the original sent to `REXXE`, a port that does not exist
  ([#532](https://github.com/fubar-coder/bb2-c/issues/532)).

Sending commands:

* `RXExecuteScript` and `RXRunScript` run the ARexx script file they
  name. `RXExecuteScript` waits until the script has ended, and its
  return code is `RXReturnCode`; `RXRunScript` does not wait. The
  original sent the name as a line of ARexx to execute, so no script
  file was ever loaded, and neither command waited
  ([#544](https://github.com/fubar-coder/bb2-c/issues/544)).
* Replies to `RXRunCommand` and `RXRunScript` are freed with their
  arguments. The original lost memory with every such command and kept
  each in its list of sent messages, which ran over other memory after
  32 commands. A program can have 31 such commands waiting for their
  replies; with more, nothing is sent and the result is 0
  ([#531](https://github.com/fubar-coder/bb2-c/issues/531)).
* `RXSendCommand` returns an empty string for a command that failed.
  The original read the error number of the reply as the address of a
  string: after a failing ARexx program it returned the bytes from
  address 6 on ([#545](https://github.com/fubar-coder/bb2-c/issues/545)).
* `RXSendCommand` frees the result string of each reply; the original
  lost two strings with every call
  ([#546](https://github.com/fubar-coder/bb2-c/issues/546)).
* `RXReturnCode` gives the return code of the last `RXSendCommand` also
  while no received message is current. The original returned 0 then,
  so the code of a failed command was often lost
  ([#542](https://github.com/fubar-coder/bb2-c/issues/542)).
* Without memory for a new message nothing is sent, and the result is 0
  or an empty string. The original could go on with the message made
  before, write the new arguments into it and send it again
  ([#533](https://github.com/fubar-coder/bb2-c/issues/533)).

Splitting commands into words:

* The character right after a closing quote starts a new word:
  `"g"h` gives the two words `g` and `h`. The original dropped that
  character, and when a command ended with a quoted word, it sent the
  rest of an earlier, longer command as further words: `say "hi"` after
  `one two three four five six` gave `say`, `hi`, `hree`, `four`,
  `five` and `six` ([#538](https://github.com/fubar-coder/bb2-c/issues/538)).
* Inside a quoted word, a doubled quote stands for one quote and the
  next single quote ends the word: `"e""f" c d` gives `e"f`, `c` and
  `d`. The original gave the one word `e"f c d`
  ([#535](https://github.com/fubar-coder/bb2-c/issues/535)).
* An empty quoted word, `""`, is sent as an empty argument: `a "" c`
  gives `a`, an empty word and `c`. The original ended the command there
  and dropped the words behind it
  ([#536](https://github.com/fubar-coder/bb2-c/issues/536)).
* Commands and words of any length are split whole. The original copied
  them into buffers of 256 bytes without a limit, so a command of more
  than 255 characters overwrote the library's own code
  ([#537](https://github.com/fubar-coder/bb2-c/issues/537)).
* A command of more than 16 words is sent with 16 arguments, the most a
  message has; the 16th holds the rest of the command, as it is, for the
  receiver to split. The original wrote the 17th and further words over
  other fields of the message
  ([#527](https://github.com/fubar-coder/bb2-c/issues/527)).
* `RXTokenise Off` always switches the splitting off; the original could
  leave it on, depending on a part of a register that the program had
  left as it happened to be
  ([#547](https://github.com/fubar-coder/bb2-c/issues/547)).

Received messages:

* After `RXReplyMsg` no message is current: `RXGetArg`, `RXNextArg`,
  `RXNumArgs` and `RXSendersName` give 0 or an empty string until the
  next `RXReadPort` or `RXWaitPort`. The original went on reading the
  replied message, which its sender may already have freed. A program
  that reads a message before it replies, as the guide asks, sees no
  change ([#541](https://github.com/fubar-coder/bb2-c/issues/541)).
* `RXReplyMsg` gives a result string only to a message that asked for
  one, and only with the return code 0. The original always gave one, so
  a sender that had not asked never freed it, and one that read the
  field as a second error code got an address
  ([#539](https://github.com/fubar-coder/bb2-c/issues/539)).
* `RXReplyMsg` to a message whose sender's port is gone makes no result
  string. The original made the string first and then lost it, or freed
  a string at address 0
  ([#540](https://github.com/fubar-coder/bb2-c/issues/540)).
* `RXNextArg` moves on after an empty argument; the original returned
  the empty argument again and again
  ([#534](https://github.com/fubar-coder/bb2-c/issues/534)).
* The list of received messages has room for 31. While it is full,
  `RXReadPort` and `RXWaitPort` leave further messages in the port, in
  the order they came, and take them into the list as it gets room;
  replies to the program's own commands are still taken at once. The
  original wrote the 32nd and later messages past the end of the list,
  over the list of sent messages and the memory behind it
  ([#1100](https://github.com/fubar-coder/bb2-c/issues/1100)).

{{% fixed-bugs-library part="elsewhere" %}}
