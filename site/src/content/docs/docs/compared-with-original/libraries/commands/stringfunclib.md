---
title: stringfunclib
description: The bugs of Blitz Basic 2 in the command library stringfunclib that
  the BB2 C port fixes.
sidebar:
  order: 58
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/stringfunclib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Strings {#other-strings}

* `Right$` and `UnRight$` took their characters from before the string
  when the part they leave out was 32768 characters or longer, so
  `Right$(a$,3)` of a string of 40003 characters did not give its last
  three; they take the right end of any string
  ([#285](https://github.com/fubar-coder/bb2-c/issues/285)).
* `Mid$` with a length so large that the start plus the length passed
  $7FFFFFFF returned a string of that length, far beyond the string and
  the string buffer: `Len(Mid$("Hello",2,$7FFFFFFF))` gave 2147483647;
  the result stops at the end of the string, and that length is 4
  ([#286](https://github.com/fubar-coder/bb2-c/issues/286)).
* `Instr` with a start position near $7FFFFFFF went on searching in
  memory far from the string, until it found the text there or hung; it
  returns 0
  ([#1030](https://github.com/fubar-coder/bb2-c/issues/1030)).

#### `Instr` with a start position below 1 {#instr-startpos}

**What happened.** `Instr(String$,Findstring$,Startpos)` did not check
the start position: with 0 or less it compared the bytes before the
string, so an empty search string was found at position 0 or at the
negative start: `Instr(a$,"",0)` gave 0, `Instr(a$,"",-5)` -5.

**With the port.** A start position below 1 searches from the first
character, as `Instr` without a start position does. With runtime error
checking, it is the error "Instr Startpos MUST be greater than zero".

**What to change.** Start at position 1 or later; `Instr(a$,f$)`
searches from the first character.

Issue: [Instr compared the bytes before the string for a start position of 0 or less (#287)](https://github.com/fubar-coder/bb2-c/issues/287).

#### `Cvi`, `Cvl` and `Cvq` of a short string {#cvi-short-string}

**What happened.** `Cvi` always read 2 bytes of its string, `Cvl` and
`Cvq` 4, whatever the length of the string: a shorter string gave a
number made partly of the bytes behind it, whatever an earlier string
expression had left there.

**With the port.** The missing bytes count as 0, so `Cvl("ab")` is
$61620000 and `Cvi("")` is 0. With runtime error checking, a string that
is too short is the error "Cvi needs a String of at least 2 Characters"
("Cvl needs ...", "Cvq needs a String of at least 4 Characters").

**What to change.** Give `Cvi` a string of 2 characters, `Cvl` and `Cvq`
one of 4, as `Mki$`, `Mkl$` and `Mkq$` make them.

Issue: [Cvi, Cvl and Cvq read past the end of a short string (#288)](https://github.com/fubar-coder/bb2-c/issues/288).

::fixed-bugs-library{part="elsewhere"}
