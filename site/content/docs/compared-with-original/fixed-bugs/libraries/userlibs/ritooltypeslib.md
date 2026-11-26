---
title: ritooltypeslib
weight: 46
description: The bugs of Blitz Basic 2 in the RWE userlib ritooltypeslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `PutIconObject` reported a failed write as success {#puticonobject-result}

[ritooltypeslib]({{< relref "/docs/libraries/userlibs/ritooltypeslib" >}})
reads and changes the icons of files. It works on one icon at a time:
`GetIconObject` loads it, the other commands change it, and
`PutIconObject` writes it to the icon of a file and drops it.
`FreeIconObject` drops it without writing it. While an icon is loaded,
`GetIconObject` returns 0.

**What happened.** `PutIconObject` returned -1, success, also when the
icon could not be written, for example into a directory that does not
exist, and dropped the icon all the same. The program could not tell
that the icon was lost. This program loads the icon of `SYS:Prefs`,
writes it where no directory is and then drops it:

```
If GetIconObject("SYS:Prefs")
  NPrint "icon"
  NPrint PutIconObject("RAM:bug-orig-no-such-dir/test")
  NPrint FreeIconObject
Else
  NPrint "no icon"
EndIf
End
```

**With the original**

```
icon
-1
0
```

**With the port**

```
icon
0
-1
```

With the original, `PutIconObject` reported success and `FreeIconObject`
found no icon. With the port, `PutIconObject` returns 0 and the icon
stays loaded, so `FreeIconObject` drops it and returns -1. A successful
write is as before.

**What to change.** Test the result of `PutIconObject`. After a failed
write, write the icon again, for example to another file, or drop it
with `FreeIconObject`: until then, `GetIconObject` returns 0 because an
icon is still loaded.

Issue: [PutIconObject returns -1 even when PutDiskObject fails (#851)](https://github.com/fubar-coder/bb2-c/issues/851).

### `SetToolValue` replaced a tool type whose name only began with the name given {#settoolvalue-prefix}

Tool types are the lines `NAME=value` or `NAME` in the information
window of an icon. `SetToolValue tooltype$,value$` of
[ritooltypeslib]({{< relref "/docs/libraries/userlibs/ritooltypeslib" >}})
replaces the tool type `tooltype$` of the loaded icon.

**What happened.** The original replaced the first tool type that only
began with `tooltype$`: `SetToolValue "FL","x"` replaced `FLAG` by
`FL=x` when `FLAG` came before `FL`. This program makes the two tool
types `FLAG` and `FL=1` and sets `FL`:

```
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  r=NewToolType("FLAG","")
  r=NewToolType("FL","1")
  NPrint SetToolValue("FL","x")
  NPrint FindToolNumber(0)
  NPrint FindToolNumber(1)
Else
  NPrint "no icon"
EndIf
End
```

**With the original**

```
-1
FL=x
FL=1
```

**With the port**

```
-1
FLAG
FL=x
```

The port replaces only the tool type of that name, switched off in
brackets or not, and `FLAG` stays.

**What to change.** A program that gave `SetToolValue` an abbreviation
of a name on purpose has to give the whole name; with an abbreviation,
`SetToolValue` returns 0 now.

Issue: [SetToolValue replaces the first entry that only begins with the name ("FL" replaces "FLAG") (#853)](https://github.com/fubar-coder/bb2-c/issues/853).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ritooltypeslib {#ritooltypeslib-fixes}

These fixes concern
[ritooltypeslib]({{< relref "/docs/libraries/userlibs/ritooltypeslib" >}}),
the icon commands of Reflective Images; the fixes of
[`PutIconObject`](#puticonobject-result) and
[`SetToolValue`](#settoolvalue-prefix) are described above.

* `GetIconObject` works on Kickstart 1.x. There a file without an icon
  gives 0, since icon.library of Kickstart 1.x has no default icons. The
  original called a function of Kickstart 2.0 there and crashed, hung,
  or loaded a garbage icon that crashed later
  ([#1149](https://github.com/fubar-coder/bb2-c/issues/1149)).
* An icon can have any number of tool types: `GetIconObject` and
  `NewToolType` make room for them, and return 0 when there is no memory
  for it. The original had room for 100 entries, and an icon with more
  tool types, or a 100th `NewToolType`, wrote behind the list
  ([#855](https://github.com/fubar-coder/bb2-c/issues/855)).
* `PutIconObject` frees the memory of the icon it has written, as
  `FreeIconObject` does; the original kept it until the next reboot
  ([#850](https://github.com/fubar-coder/bb2-c/issues/850)).
* `FindToolType tooltype$` with one argument never finds a switched-off
  `(tooltype$)`; the original also found it, and returned 1, after an
  earlier `FindToolType` with `bracket_check` had ended before its
  search for the brackets
  ([#843](https://github.com/fubar-coder/bb2-c/issues/843)).
* `FindToolType` with `bracket_check` takes a name of any length; the
  original wrote a name of more than 253 characters behind its buffer
  ([#842](https://github.com/fubar-coder/bb2-c/issues/842)).
* `FindToolValue` returns the whole value, also when it holds a `)`; the
  original cut it at the first `)`, so `PAREN=a)b` gave `a`
  ([#844](https://github.com/fubar-coder/bb2-c/issues/844)).
* `SetToolValue` and `NewToolType` with an empty name make the tool type
  `=value`; the original copied the bytes behind the name into it, and
  the tool type read back as empty
  ([#841](https://github.com/fubar-coder/bb2-c/issues/841)).
* `SetToolValue`, `NewToolType` and `IconDefTool` free their new string
  and return 0 when the icon's list of memory to free cannot take it; the
  original freed memory at address 0 instead and lost the string
  ([#840](https://github.com/fubar-coder/bb2-c/issues/840)).
* `SetIconType` without a loaded icon returns 0; the original returned
  its argument, which a program took for success
  ([#852](https://github.com/fubar-coder/bb2-c/issues/852)).
* `IconToShape` copies only the data the shape has room for; with an
  image whose width is a multiple of 16, the original copied one word
  per line too many and wrote behind the shape's data
  ([#847](https://github.com/fubar-coder/bb2-c/issues/847)).
* `ShapeToIcon` with two shapes makes the selected image show all
  bitplanes of the shape; the original set it up to show none of them
  ([#854](https://github.com/fubar-coder/bb2-c/issues/854)).
* `ShapeToIcon` makes the icon's image show the bitplanes of the shape;
  the original kept those of the icon's old image, so a shape of another
  depth was drawn with planes it does not have, read from the memory
  behind it, or without some of its planes
  ([#1083](https://github.com/fubar-coder/bb2-c/issues/1083)).

{{% fixed-bugs-library part="elsewhere" %}}
