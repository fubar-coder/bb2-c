---
title: rimisclib
weight: 33
description: The bugs of Blitz Basic 2 in the RWE userlib rimisclib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rimisclib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### `NextArg$` did not end a quoted word after `""` {#nextarg-doubled-quote}

`InitArgParse` takes a line of text, such as the arguments a program got
from the CLI, and `NextArg$` returns its words one by one. Spaces
separate the words, quotes group a word that contains spaces, and `""`
inside a quoted word stands for one quote character. After such a `""`
the original took the closing quote for text: the word did not end
there but went on, spaces included, up to the next quote or the end of
the line. `"a""b" c` gave the one word `a"b c` instead of `a"b` and then
`c`, so a program lost an argument.

**What happened**

The program puts the line `"a""b" c` into memory and reads three words:

```
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
s$=Chr$(34)+"a"+Chr$(34)+Chr$(34)+"b"+Chr$(34)+" c"
For i=1 To Len(s$) : Poke.b b+i-1,Asc(Mid$(s$,i,1)) : Next
InitArgParse b
For k=1 To 3
  a$=NextArg$
  NPrint "<"+a$+"> "+Str$(Len(a$))
Next
FreeMem_ b,64
End
```

The original prints

```
<a"b c> 5
<> 0
<> 0
```

**With the port**

The closing quote ends the word, and the port prints

```
<a"b> 3
<c> 1
<> 0
```

Issue: [`NextArg$` took the closing quote after `""` inside quotes as text (#755)](https://github.com/fubar-coder/bb2-c/issues/755).

### Fixes in rimisclib {#rimisclib-fixes}

These fixes concern
[rimisclib]({{< relref "/docs/libraries/userlibs/rimisclib" >}}), the
miscellaneous commands of Reflective Images and Steve Matty; the fix
of [`NextArg$`](#nextarg-doubled-quote) is described above, and the fix
of [the text that follows a string function]({{< relref "/docs/compared-with-original/libraries#rwe-text-after-string-function" >}})
for several libraries together.

* `WorkbenchHere` works: it returns -1 if the title of an open screen
  contains "Workbench", and 0 if none does. The original had four
  faults:
  * It searched a list of screens at whatever address a register of the
    processor held
    ([#759](https://github.com/fubar-coder/bb2-c/issues/759)).
  * It did not stop at the end of the list, so it never returned 0 and
    could hang ([#761](https://github.com/fubar-coder/bb2-c/issues/761)).
  * It hung on a title that ends with the start of "Workbench", such as
    "My Work" ([#762](https://github.com/fubar-coder/bb2-c/issues/762)).
  * It missed "Workbench" behind another "W", as in "A Window on
    Workbench" ([#760](https://github.com/fubar-coder/bb2-c/issues/760)).
* Negative Quick and Float arguments of `EZRequest` and `EZRequestArgs`
  show their whole part, -3.5 as -3, as `Int` gives it; the original
  showed -3.5 as 65532
  ([#750](https://github.com/fubar-coder/bb2-c/issues/750)).
* Float arguments of `EZRequest` and `EZRequestArgs` are converted by
  the library itself; the original called the conversion of the maths
  library through whatever library base the program happened to hold, so
  the value shown depended on what the program had done before
  ([#749](https://github.com/fubar-coder/bb2-c/issues/749)).
* `EZRequestArgs` with a shorter text than the call before shows only
  its own; the original kept the end of the text before behind it
  ([#747](https://github.com/fubar-coder/bb2-c/issues/747)).
* `EZRequest`, `EZRequestArgs`, `ProgPath$`, `ProgDir$` and `ProgName$`
  work on Kickstart 1.x. The original called functions of Kickstart 2.0
  in them and crashed there. On Kickstart 1.x:
  * The requester is the older system requester. It has one or two
    gadgets and no title; the results are the same as from Kickstart 2.0
    on. With more than two gadgets no requester appears and the result
    is 0, and with runtime error checking the program stops with the
    error "More than two EZRequest gadgets need Kickstart 2.0".
  * `ProgDir$` is the current directory, and `ProgName$` is taken from
    the command the Shell started the program with, or is the name of
    its task without a Shell. A program started from another directory, with a path in
    front of its name, thus gets the current directory, not its own
    ([#1150](https://github.com/fubar-coder/bb2-c/issues/1150)).
* `Flip$` of an empty string or of one character returns it unchanged;
  the original exchanged 65536 pairs of bytes around the string and
  overwrote memory
  ([#751](https://github.com/fubar-coder/bb2-c/issues/751)).
* `InitArgParse` copies at most 255 characters and cuts off the rest;
  the original copied a line of any length into its 256 byte buffer and
  overwrote the code of the library behind it
  ([#752](https://github.com/fubar-coder/bb2-c/issues/752)).
* `NextArgChar$` before `InitArgParse`, or after the end of the text,
  returns "", as `NextArg$` does; the original read from memory address
  0 on ([#754](https://github.com/fubar-coder/bb2-c/issues/754)).
* `ProgPath$` of a program whose directory has no name that can be found
  is the program's name alone; the original put a "/" in front of it
  ([#758](https://github.com/fubar-coder/bb2-c/issues/758)).
* `OpenFontPrefs` returns 0 when there is not enough memory for the
  file; the original read the file to address 0, over the first bytes of
  memory that the processor and the system need
  ([#756](https://github.com/fubar-coder/bb2-c/issues/756)).
* A second `OpenFontPrefs` frees the memory of the first; the original
  lost 16 KB each time until the next reboot
  ([#757](https://github.com/fubar-coder/bb2-c/issues/757)).
* `DefaultFontHeight` and `DefaultFontName` look only at the font
  entries of the preferences file, and return 0 or "" for a font type the
  file does not set; the original also searched the memory behind the
  file and could return a value from leftover data there
  ([#745](https://github.com/fubar-coder/bb2-c/issues/745)).
* `ASCRaw` of a code outside 0 to 113, such as the code of a key being
  released, gives the character 0, and `RawASC` of a character that no
  key types, or of "", gives -1. The original read the bytes before or
  behind its table of keys and gave a meaningless character, and `RawASC` searched
  on through memory until it found the character, or for ever
  ([#744](https://github.com/fubar-coder/bb2-c/issues/744)).

#### `EZRequest` and `EZRequestArgs` with too many arguments {#ezrequest-too-many-arguments}

`EZRequest` of
[rimisclib]({{< relref "/docs/libraries/userlibs/rimisclib" >}}) shows a
system requester whose texts can hold placeholders, filled in by the
arguments after `gadget$`; `EZRequestArgs` shows the values of its
arguments. Both keep these arguments in a table of 16 longs: a string, a
long, a quick or a float takes one long, a word or a byte half of one.

**What happened.** The original did not limit the number of arguments,
also not with runtime error checking. More than fit into the table were
written over the code of the library behind it, the code of
`WorkbenchHere`. With the original, this call with 18 long arguments
showed its requester, and a `WorkbenchHere` afterwards ran into the
overwritten code and could crash the program:

```
EZRequest "BO","x","OK",q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q
```

**With the port.** With runtime error checking, arguments that do not
fit into the 16 longs are the error "Too Many EZRequest Arguments", and
no requester appears. Without runtime error checking, the commands are
as before.

**What to change.** Give `EZRequest` and `EZRequestArgs` at most 16
longs of arguments; show more values with a second requester.

Issue: [EZRequest/EZRequestArgs do not limit the number of arguments (16 longs) (#748)](https://github.com/fubar-coder/bb2-c/issues/748).

{{% fixed-bugs-library part="elsewhere" %}}
