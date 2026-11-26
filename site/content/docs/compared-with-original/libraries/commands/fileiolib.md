---
title: fileiolib
weight: 22
description: The bugs of Blitz Basic 2 in the command library fileiolib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/fileiolib/]
---

{{< fixed-bugs-library >}}

## Files

### `Loc` and `Lof` were one too high after `Eof` {#loc-lof-after-eof}

`Eof` tells whether the position in a file is at its end. To find out,
the original tried to move one byte further, and when that failed at the
end of the file it kept the new position all the same. Once `Eof` had
been true, `Loc` and `Lof` were one too high: a file of 3 bytes had the
length 4. A program that adds to a file with `FileSeek f,Lof(f)`, as the
manual shows, then asked for a position behind the end of the file.

**What happened**

```
f$="RAM:abc.dat"
KillFile f$
If WriteFile(0,f$)
  e=Eof(0)
  NPrint "new eof ",e
  FileOutput 0 : Print "abc" : DefaultOutput
  l1.l=Loc(0) : l2.l=Lof(0)
  NPrint "written loc ",l1," lof ",l2
  CloseFile 0
EndIf
NPrint "exists ",Exists(f$)
If ReadFile(1,f$)
  FileInput 1 : a$=Inkey$(3) : DefaultInput
  e=Eof(1)
  l1.l=Loc(1) : l2.l=Lof(1)
  NPrint "read [",a$,"] eof ",e," loc ",l1," lof ",l2
  CloseFile 1
EndIf
KillFile f$
End
```

The program asks `Eof` of a new, empty file and writes the 3 bytes `abc`
into it, then reads them back and asks `Eof` again. The original prints

```
new eof -1
written loc 4 lof 4
exists 3
read [abc] eof -1 loc 4 lof 4
```

The file has 3 bytes, as `Exists` shows, but `Loc` and `Lof` say 4.

**With the port**

```
new eof -1
written loc 3 lof 3
exists 3
read [abc] eof -1 loc 3 lof 3
```

Positions count from 0, and at the end of the file `Loc` and `Lof` are
both its length, so `FileSeek f,Lof(f)` goes to the end.

Issue: [Loc and Lof were one too high after Eof at the end of a file (#144)](https://github.com/fubar-coder/bb2-c/issues/144).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Files and banks

* `OpenFile`, `ReadFile` or `WriteFile` on a file number whose file was still
  open with `Fields` cut the new file name short, so a long name was not
  found; the name stays whole
  ([#138](https://github.com/fubar-coder/bb2-c/issues/138)).
* `CloseFile` freed the record fields but kept the record length and a
  pointer to the freed fields, so a `Fields` before the next open added
  to the old record; closing clears both
  ([#140](https://github.com/fubar-coder/bb2-c/issues/140)).
* `Exists` returned -1 only for a drawer: the root of a volume, on file
  systems that report it as a root, gave its size, normally 0, and a
  file that could not be examined gave the size of an earlier file; every
  kind of directory gives -1, and a file that cannot be examined gives 0
  ([#1026](https://github.com/fubar-coder/bb2-c/issues/1026)).

{{% fixed-bugs-library part="elsewhere" %}}
