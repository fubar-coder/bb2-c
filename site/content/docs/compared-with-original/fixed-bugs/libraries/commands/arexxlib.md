---
title: arexxlib
weight: 3
description: The bugs of Blitz Basic 2 in the command library arexxlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `ReplyRexxMsg` gave the result string to the wrong messages {#replyrexxmsg-result}

A program that sends an ARexx message can ask for a result string: it
sets the flag RXFF_RESULT ($00020000) in the action it gives
`SendRexxCommand`, as an ARexx script does with `OPTIONS RESULTS`. The
program that gets the message answers with
`ReplyRexxMsg msg,Result1,Result2,ResultString`. If `Result1` is 0 and
the sender asked for a result, `ResultString` goes back as the result
string, in place of the number `Result2`, and the sender reads it with
`GetResultString`.

**What happened.** The original tested another bit of the action, the one
that marks a function call (RXFUNC, $02000000). A command that asked for
a result got none, only the number `Result2`, and a function call that
had not asked for one got the string in place of its number. In this
program `rp` is the program's own reply port and `rx` a port at which it
gets its own messages, so that it plays both sides:

```
SendRexxCommand r,"cmd",$01020000    ; a command (RXCOMM) with RXFF_RESULT
q.l=RexxEvent(rx)
ReplyRexxMsg q,0,7,"sent"
q=RexxEvent(rp)
NPrint "comm [",GetResultString(q),"]"
SendRexxCommand r,"func",$02000000   ; a function call without RXFF_RESULT
q=RexxEvent(rx)
ReplyRexxMsg q,0,7,"not sent"
q=RexxEvent(rp)
NPrint "func ",GetRexxResult(q,2)=7
```

**With the original**

```
comm []
func 0
```

**With the port**

```
comm [sent]
func -1
```

The command gets its result string, and the function call keeps the
number 7. Nothing changes for a function call that sets RXFF_RESULT,
which gets the string from the original and the port alike, nor for a
reply with a `Result1` other than 0, which carries the number in both.

**What to change.** A program that answers ARexx scripts needs no change:
a script that asks for a result gets `ResultString`. A program that sends
commands with RXFF_RESULT and read the number `Result2` of the reply with
`GetRexxResult` now finds the address of the result string there; read
the string with `GetResultString`. A program that sends function calls
without RXFF_RESULT and read their answer with `GetResultString` now gets
the empty string; set RXFF_RESULT, or read the number with
`GetRexxResult(msg,2)`.

Issue: [ReplyRexxMsg made the result string for the wrong messages (#48)](https://github.com/fubar-coder/bb2-c/issues/48).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### ARexx

These are the commands that send, answer and take apart ARexx messages.
One more fix of them changes what an existing program sees:
[`ReplyRexxMsg` with the result string](#replyrexxmsg-result).

* `ClearRexxMsg` and `FillRexxMsg` went on past the 16 argument slots of
  a message into the fields behind them, so `ClearRexxMsg` of a message
  with all slots used deleted its host and extension strings, and a 17th
  value of `FillRexxMsg` overwrote the field after the slots; they stop
  after the 16 slots
  ([#49](https://github.com/fubar-coder/bb2-c/issues/49)).
* `FillRexxMsg` ended the list at any value of 0, also at a number 0, so
  the arguments behind it were lost; only a string address of 0 ends it,
  and `ClearRexxMsg` clears all 16 slots
  ([#50](https://github.com/fubar-coder/bb2-c/issues/50)).
* `GetRexxResult` and `GetRexxCommand` read the fields next to the
  results or the argument slots for a number out of range; with runtime
  error checking, `GetRexxResult` with a number other than 1 or 2 is the
  error "Illegal Result Number", and `GetRexxCommand` with one outside 1
  to 16 the error "Illegal Argument Number"
  ([#51](https://github.com/fubar-coder/bb2-c/issues/51);
  [`GetRexxResult` and `GetRexxCommand` with a number out of range]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#rexx-numbers" >}})).
* `GetResultString` freed the result string of a message but left the
  message pointing to the freed memory, so a second call read and freed
  it again. The port clears the result, and a second call gives the
  empty string
  ([#53](https://github.com/fubar-coder/bb2-c/issues/53)).
* `SendRexxCommand` with no ARexx running (no port "REXX") showed its
  alert and went on with the interrupts disabled, so the Amiga stopped
  responding; it enables them again before the alert
  ([#55](https://github.com/fubar-coder/bb2-c/issues/55)).
* Without `rexxsyslib.library`, `DeleteRexxMsg`, `ClearRexxMsg`,
  `FillRexxMsg`, `SendRexxCommand`, `GetRexxResult`, `GetRexxCommand`
  and `GetResultString` showed their alert and then worked on a message
  at address 0, reading and writing low memory; after the alert they
  take the message as no ARexx message and do nothing
  ([#56](https://github.com/fubar-coder/bb2-c/issues/56)).
* An ArgString that `CreateArgString`, `CreateRexxMsg`, `FillRexxMsg` or
  `SendRexxCommand` made of a string of 65536 characters or more took
  memory for the whole string, but held only its length modulo 65536
  characters; it holds the first 65535 characters, the most an ArgString
  can describe
  ([#57](https://github.com/fubar-coder/bb2-c/issues/57)).

{{% fixed-bugs-library part="elsewhere" %}}
