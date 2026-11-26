---
title: rundebuglib
weight: 48
description: The bugs of Blitz Basic 2 in the command library rundebuglib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/rundebuglib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### Runtime error checking in supervisor mode outside `SetInt` {#supervisor-mode-traps}

A program compiled with runtime error checking, or one that uses
`SetErr`, handles its own runtime errors. Each program takes only its
own traps, through the trap handler of its task, so two such programs can
run at the same time without taking away each other's error handling.
The original put its handlers into the trap vectors of the whole
machine, and the program that ended first removed those of the other.

**What changes.** AmigaOS calls the trap handler of a task only while
the task runs in user mode. A `SetInt` routine runs in supervisor mode,
and its traps are handled as before. Other Blitz code compiled with
runtime error checking that runs in supervisor mode, such as a routine
called through exec's `Supervisor()` or an interrupt server installed by
hand, hangs the machine at its first trap; the original's handlers took
such a trap. AmiBlitz3 behaves like the port.

**What to change.** Compile the code that runs in supervisor mode
without runtime error checking, between `RunErrsOff` and `RunErrsOn`, or
make it a `SetInt` routine.

Issue: [Runtime error checking takes the global trap vectors: two programs break each other (#1069)](https://github.com/fubar-coder/bb2-c/issues/1069).

## Changes you may notice

### Ctrl-C did not stop a program with runtime error checking {#ctrl-c-runtime-errors}

On the Amiga, Ctrl-C in a Shell window (or the Shell command `Break`)
sends the program that runs there a break signal. A C program reacts to
it at its next input or output on the console: it prints `***Break` and
ends with the return code 20.

**What happened.** An executable compiled with runtime error checking
was meant to end at Ctrl-C, but its check never saw the signal: the
program ran on to its end. This program sends itself the break signal
with `SetSignal_`, as Ctrl-C does, and clears it again after the next
`NPrint`:

```
NPrint "start"
SetSignal_ $1000,$1000
NPrint "signalled"
SetSignal_ 0,$1000
NPrint "not stopped"
End
```

**With the original**

```
start
signalled
not stopped
```

**With the port**

```
start
signalled
***Break
```

The program ends with the return code 20, which the Shell reports as
`failed returncode 20`. It ends as at `End`, with the same clean-up of
the libraries.

The check comes once at the end of each `Print`, `NPrint`, `Edit$` and
`Edit`, so the text of the `Print` is still written, and `Edit$` ends the
program after it has read its line. It applies only where a C program
would react too:

* only in an executable compiled with runtime error checking, and not
  between `RunErrsOff` and `RunErrsOn`; an executable without runtime
  error checking is not changed;
* only on the program's standard input and output, the console window
  it was started from or what the Shell redirected them to; after
  `WindowOutput`, `BitMapOutput`, `FileOutput` or one of the matching
  input commands, the program goes on;
* a loop that does no input or output on the console still runs on, as
  it does in a C program.

**What to change.** A program that tests for Ctrl-C itself, for example
with `SetSignal_` to clean up in its own way, is ended at a `Print` on
the console that comes between the signal and its test. Test for the
signal before such output, or put the output between `RunErrsOff` and
`RunErrsOn`.

Issue: [Ctrl-C check in the TRAP #1 handler never effective (#43)](https://github.com/fubar-coder/bb2-c/issues/43).

## Runtime errors in executables {#runtime-error-texts}

An executable built with runtime error checking stops with a requester
when it meets an error. For three errors the original showed the
requester with no text at all: a command that needs a used object when
none is used (for example `RGB` without a screen), an offset out of range,
and the end of a program that has no `End`.

**What happened**

```
NPrint "hello"
```

Compiled with runtime error checking and saved with *Create Executable*,
the program prints `hello` and then stops with an empty requester.

**With the port**

The requester says `End of Program`. The other two errors say
`No currently used object!` and `Offset out of range`, the texts the
debugger shows. The program still stops at its end without `End`, as
before.

Issue: [Runtime errors with a numeric code show an empty requester in executables (#6)](https://github.com/fubar-coder/bb2-c/issues/6).
## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Debugger and runtime error checking {#debugger-fixes}

One more fix of this area changes what an existing program does:
[Runtime error checking in supervisor mode outside `SetInt`](#supervisor-mode-traps).

* A runtime error in an interrupt routine of an executable with runtime
  error checking lost its message on the way to the trap handler that
  was there before the program started, and closed `intuition.library`
  inside the interrupt. The message is kept, and `intuition.library` is
  closed only when the program ends. The program shows the message in
  its own requester, as the next point says; a trap handler that a
  program put into the vector table itself is not called
  ([#246](https://github.com/fubar-coder/bb2-c/issues/246)).
* A runtime error inside a `SetInt` routine of an executable compiled
  with runtime error checking is reported with the usual requester at
  the program's next statement, as the debugger does, and the routine
  is not called again. A program that waits, for example in `Wait_`,
  reports it when it goes on. The original hung the machine
  ([#1073](https://github.com/fubar-coder/bb2-c/issues/1073)).

{{% fixed-bugs-library part="elsewhere" %}}
