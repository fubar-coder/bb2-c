---
title: Libraries
weight: 3
description: Fixed bugs of Blitz Basic 2 in the command libraries, in their error handling without the debugger, in AmigaOS calls and in the RWE userlibs, with examples of the ones ordinary programs meet.
---

These bugs are in the libraries that provide the commands of Blitz Basic
2 ([Libraries]({{< relref "/docs/libraries" >}})). A fix in a library
changes only that library, and a program behaves differently only where
it ran into the bug. The first sections show the ones an ordinary program
can run into, each with a short program; the other fixes follow as a
list, one sentence each with a link to its issue.

The outputs shown were recorded in an emulator: the original is the
program compiled with the libraries of the Blitz Basic 2 CD, the port the
same program compiled with the port's libraries. The programs run without
runtime error checking unless the text says otherwise.

## Dates

### `Date$` showed 1 January after a leap year as 32 December {#date-after-leap-year}

`Date$` turns a day number, the days since 1 January 1978 that
`SystemDate` returns, into a date string. For one day in every four years
it gave a date that does not exist: the 1 January after each leap year,
such as 1 January 1981 or 1 January 2025, came out as the 32nd of
December of the leap year. A program that shows today's date showed it
wrong on that day.

**What happened**

```
NPrint Date$(1095)
NPrint Date$(1096)
NPrint Date$(1097)
End
```

Day 1096 is 1 January 1981. The original prints

```
31/12/1980
32/12/1980
02/01/1981
```

`Days`, `Months` and `Years` after that `Date$` return 32, 12 and 1980
as well. Day 17167, 1 January 2025, comes out as `32/12/2024`.

**With the port**

```
31/12/1980
01/01/1981
02/01/1981
```

`Days`, `Months` and `Years` return 1, 1 and 1981, and day 17167 is
`01/01/2025`.

Issue: [Date$ showed the 1st of January after a leap year as "32/12" (#120)](https://github.com/fubar-coder/bb2-c/issues/120).

## Text

### Umlauts were not drawn {#umlauts}

The text commands draw a character only when it lies in the range of the
font. The original compared the character with the font's first
character as a signed byte, so every character 128 or more above it was
taken as lying below the font. With the standard font, which starts at
the space, that is every character from code 160 on: the umlauts and
the other accented letters were not drawn, and the cursor did not move.

**What happened**

```
BitMap 0,320,16,1
BitMapOutput 0
Print "A"
a.l=CursX
Print Chr$(196)
b.l=CursX
DefaultOutput
NPrint "after A: ",a
NPrint "after umlaut: ",b
End
```

The program prints "A" and the umlaut Ä (code 196) on a bitmap, and after
each the column of the cursor. The original prints

```
after A: 1
after umlaut: 1
```

The Ä is not drawn, and the cursor stays where it was.

**With the port**

```
after A: 1
after umlaut: 2
```

Issue: [Characters far above the font's first character were not drawn (#148)](https://github.com/fubar-coder/bb2-c/issues/148).

## Strings

### Text after a string function lost its last character {#text-after-string-function}

A string expression such as `FileRequest$(...)+".bak"` or
`ProgDir$+"/data"` is built in a work area, one part behind the other. A
function that returns a string writes its result there and leaves the
place where the next part goes. Several library functions left that place
one byte too far, behind the 0 byte that ends their result. The joined
string had the right length, but it held a 0 byte between the result and
the next part, and the last character of the next part was missing. A
program that adds an extension to a chosen file name, or a directory to
the program's own one, got a name that no file has.

**What happened**

```
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="x"
r$=FileRequest$("FQ",p$,f$)+".bak"
NPrint Len(r$)," ",Asc(Mid$(r$,6))," ",Asc(Mid$(r$,9))
End
```

The file requester starts with the path `RAM:` and the file `x`, and
the user clicks OK. The program prints the length of the result and the
codes of its 6th and 9th characters. The original prints

```
9 0 97
```

The 6th character is a 0 byte instead of the dot, and the 9th is an `a`:
the result is `RAM:x`, a 0 byte and `.ba`.

**With the port**

```
9 46 107
```

The result is `RAM:x.bak`.

Issue: [FileRequest$ followed by more text lost its last character (#161)](https://github.com/fubar-coder/bb2-c/issues/161).

The same fault was in `GetRexxCommand` and `GetResultString`
([#52](https://github.com/fubar-coder/bb2-c/issues/52)), and in several
functions of the [RWE userlibs](#rwe-text-after-string-function). Two
more functions left the place of the next part wrong in other ways:

* `Replace$` with an empty search string left it behind its arguments,
  so a byte of the arguments took the place of the text that followed:
  `Replace$("ab","","c")+"x"` gave `ab` and a 0 byte instead of `abx`
  ([#424](https://github.com/fubar-coder/bb2-c/issues/424)).
* `GTGetAttrs` and `GTSetAttrs`, used as functions in a string
  expression, left it on a tag list inside the library: the joined string
  came out wrong, and the rest of the expression was written over that
  list and the memory behind it
  ([#328](https://github.com/fubar-coder/bb2-c/issues/328)).

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

## GadTools gadgets

### A gadget took over the values of the one before {#gadtools-values}

Several GadTools gadget commands take optional values: the active entry
of `GTCycle` and `GTMX`, the start text of `GTString`, the position of
`GTScroller` and `GTSlider`, the selected entry of `GTListView`. The
original kept these values from one gadget to the next, so a gadget
created without the value got the one of the gadget created before it.

**What happened**

```
DEFTYPE .l
WbToScreen 0
GTCycle 0,1,10,10,100,14,"A",0,"x|y|z",2
GTCycle 0,2,10,30,100,14,"B",0,"p|q|r"
a1=GTGetAttrs(0,1,$8008000F)
a2=GTGetAttrs(0,2,$8008000F)
NPrint "cycle ",a1," ",a2
Free GTList 0
End
```

The first cycle gadget starts at its entry 2, the second has no start
value. `GTGetAttrs` with the tag `$8008000F` reads the active entry of
each. The original prints

```
cycle 2 2
```

so the second gadget starts at "r" instead of "p".

**With the port**

A gadget without the value starts with the default:

```
cycle 2 0
```

Issue: [GadTools gadgets took over optional values of an earlier gadget (#169)](https://github.com/fubar-coder/bb2-c/issues/169).

## Windows

### Closing a window lost the events of the others {#closewindow-events}

All windows of a program share one message port for their events. When
the original closed a window, it threw away every event waiting at that
port, also those of the other windows. A program with two windows that
closed one of them lost, for example, a click in the other one that it
had not read yet.

**What happened**

An event cannot be clicked in a recorded test, so the test program puts
an event for window 1 into the shared port itself, the way Intuition
delivers one. Then it closes window 0 and reads the next event of
window 1:

{{% details title="The test program" closed="true" %}}

```
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
Window 0,20,30,200,100,$000E,"Close 0",1,2
Window 1,240,30,200,100,$000E,"Close 1",1,2
DefaultOutput
w1=Peek.l(Addr Window(1))
port=Peek.l(w1+86)
rport=CreateMsgPort("")
Dim mb.l(16)
pm=&mb(0)
Poke.l pm+14,rport : Poke.w pm+18,52
Poke.l pm+20,4 : Poke.w pm+24,7 : Poke.l pm+44,w1
GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
MOVE.l 4,a6 : JSR -366(a6)
CloseWindow 0
Use Window 1 : DefaultOutput
e=Event : c=EventCode : NPrint "event ",Hex$(e)," code ",c
CloseWindow 1
End
```

{{% /details %}}

The original prints

```
event 00000000 code 0
```

The event is gone.

**With the port**

Only the closed window's events are thrown away:

```
event 00000004 code 7
```

Issue: [Closing one window discarded the events of all other windows (#322)](https://github.com/fubar-coder/bb2-c/issues/322).

### `WSize` and `WMove` hung at a limit {#wsize-wmove}

`WSize` and `WMove` ask Intuition to change a window and then wait until
the window has the new size or position. Intuition does not go beyond
the size limits of a window or the edge of the screen, and the original
then waited forever.

**What happened**

```
DEFTYPE .l
WbToScreen 0
SizeLimits 50,50,250,150
Window 0,0,20,200,100,$000F,"Limits",1,2
DefaultOutput
WSize 300,100
NPrint "size ",WindowWidth," ",WindowHeight
WMove 2000,20
NPrint "moved ",WindowX>0 AND WindowX<2000," ",WindowY
CloseWindow 0
End
```

The window may be at most 250 pixels wide, and `WSize` asks for 300.
The original hangs in `WSize` and prints nothing.

**With the port**

Both commands return, and the window has the size and position Intuition
gave it:

```
size 250 100
moved -1 20
```

Issue: [WMove and WSize hung when Intuition limited the new position or size (#320)](https://github.com/fubar-coder/bb2-c/issues/320).

### Window titles were never freed {#window-titles}

`Window`, `WindowTags` and `WTitle` copy the titles they get into new
memory. The original never freed these copies, so every window opened
and every new title cost memory until the program ended. A program that
shows changing information in a title bar lost memory all the time.

**What happened**

```
DEFTYPE .l
WbToScreen 0
Window 0,20,30,200,100,$000E,"Leak",1,2
DefaultOutput
t$=String$("t",1000)
m1=AvailMem_(0)
For i=1 To 100
  WTitle t$,t$
Next
m2=AvailMem_(0)
NPrint "lost over 100000: ",m1-m2>100000
CloseWindow 0
End
```

The program sets a window title and a screen title of 1000 characters a
hundred times. The original prints

```
lost over 100000: -1
```

**With the port**

The copies are freed with the window or the next title:

```
lost over 100000: 0
```

Issue: [Window titles were copied and never freed (#318)](https://github.com/fubar-coder/bb2-c/issues/318).

## Screens and palettes

### Decoded palettes were black on Kickstart before 3.0 {#palette-kickstart-13}

Before Kickstart 3.0 the palette library sets colours with 4 bits per
component, and it took these from the wrong byte of the 32-bit colour
values that `DecodePalette` and the fades produce. Such colours came out
black on an Amiga with Kickstart 1.3 or 2.0.

**What happened**

On Kickstart 1.3:

```
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"pal",1,2
Dim mm.l(20) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,24 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,12
For i=0 To 11 : Poke.b a+20+i,i*21+5 : Next
DecodePalette 0,a : ShowPalette 0
DefaultOutput
For i=1 To 3 : NPrint Red(i)," ",Green(i)," ",Blue(i) : Next
CloseScreen 0
End
```

The program builds a small IFF colour map in memory, decodes it into a
palette, shows it, and prints the red, green and blue of colours 1 to 3.
The original prints

```
0 0 0
0 0 0
0 0 0
```

**With the port**

```
4 5 6
8 9 10
12 13 14
```

Issue: [Faded and decoded palettes were wrong or black on Kickstart before 3.0 (#224)](https://github.com/fubar-coder/bb2-c/issues/224).

## Other fixes in the command libraries

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
* `LoadBank` of a drawer could turn the bank into one of size -1 at
  another address; a drawer leaves the bank as it is, like a missing file
  ([#72](https://github.com/fubar-coder/bb2-c/issues/72)).

### Lists

* `Dim List` with size 0 wrote behind its memory block; nothing is
  written there ([#59](https://github.com/fubar-coder/bb2-c/issues/59)).

### GadTools

* All `GTShape` gadgets showed the images of the last one, and GadTools'
  own image was overwritten; each gadget keeps its own images
  ([#173](https://github.com/fubar-coder/bb2-c/issues/173)).
* `GTShape` wrote to low memory when the gadget could not be created; a
  failed gadget is left alone
  ([#327](https://github.com/fubar-coder/bb2-c/issues/327)).
* `GTPalette` with a colour ignored the colour and broke a following tag
  list; the colour is set
  ([#168](https://github.com/fubar-coder/bb2-c/issues/168)).
* Cycle and MX gadgets had room for a limited number of labels without a
  check, and their memory was never freed; the labels are counted and the
  memory is freed with the gadget
  ([#174](https://github.com/fubar-coder/bb2-c/issues/174)).

### Windows and screens

* `WindowTags` with a long tag list overwrote code of the library; a long
  list is copied only in part and the rest is linked
  ([#317](https://github.com/fubar-coder/bb2-c/issues/317)).
* A screen mode with a monitor part, such as PAL or NTSC, opened on the
  default monitor; the whole mode is used
  ([#250](https://github.com/fubar-coder/bb2-c/issues/250)).
* `FileRequest$` on a screen of 645 lines or more, such as a 1280×1024
  RTG Workbench, wrote far behind its data and could crash the program
  after the requester closed; the list shows at most 64 lines and the
  window is at most 597 lines high. The editor's file requester is the
  same code and is fixed with it
  ([#160](https://github.com/fubar-coder/bb2-c/issues/160)).

### Dates and numbers

* `Date$` and `NumDays` made every hundredth year a leap year, so dates
  before 1 March 1900 and from 1 March 2100 on were a day off per
  century; they follow the Gregorian calendar, in which 1900, 2100, 2200
  and 2300 are not leap years
  ([#1001](https://github.com/fubar-coder/bb2-c/issues/1001)).
* `WeekDay` was two days off for a few dates, such as 15 and 16
  September 1973; it is right for every date
  ([#121](https://github.com/fubar-coder/bb2-c/issues/121)).
* `NumDays("31/12/1977")` gave 65535 instead of -1, and the day 65535, in
  the year 2157, came back from `NumDays(Date$(65535))` as 131071; both
  are right
  ([#122](https://github.com/fubar-coder/bb2-c/issues/122)).
* `NumDays` of a date with an empty field, such as `"/5/1990"`, took the
  empty field as 47; it returns 0, as for other dates it cannot read
  ([#123](https://github.com/fubar-coder/bb2-c/issues/123)).
* `NumDays` of a month outside 1 to 12 returned a number taken from
  unrelated memory; it returns 0
  ([#124](https://github.com/fubar-coder/bb2-c/issues/124)).
* `NumDays` of a year with fewer than four digits at the end of the
  string, such as `"1/1/78"`, read past the end of the string, and the
  result depended on the memory behind it; the year ends with the
  string, so `"1/1/78"` is the year 78, and an empty year gives 0
  ([#125](https://github.com/fubar-coder/bb2-c/issues/125)).
* `QAngle` gave a wrong angle for points 32768 or more apart in x or y;
  it gives the right one
  ([#1003](https://github.com/fubar-coder/bb2-c/issues/1003)).

### Graphics

* `LoadShapes`, `DecodeShapes` and `LoadSprites` with a raised object
  maximum (1025 shapes or 2049 sprites and more) loaded only the first
  object; they load the whole range
  ([#999](https://github.com/fubar-coder/bb2-c/issues/999)).
* `Polyf` drew polygons whose corners lie far outside the bitmap (from
  about 8000 pixels away) wrongly or not at all; they are drawn
  ([#1000](https://github.com/fubar-coder/bb2-c/issues/1000)).
* `Rotate` made a rotated shape one pixel too large and lost its edge
  pixels, so a quarter turn of a 21×9 shape gave 10×21; it rounds to the
  nearest pixel and gives 9×21
  ([#1004](https://github.com/fubar-coder/bb2-c/issues/1004)).
* `DecodePalette` and `LoadPalette` cycled a colour range that the
  picture marks as disabled, and ran ranges backwards without their
  reverse flag; a range cycles only when enabled, backwards only when
  marked so
  ([#1002](https://github.com/fubar-coder/bb2-c/issues/1002)).
* `DecodeShapes` uses shape data that lies in chip memory where it is,
  and marks the shape so that this memory, which the library did not
  allocate, is not freed. The mark stayed when the shape was freed or
  replaced, so the next shape in that object was never freed before the
  program ended; the mark goes with the shape
  ([#269](https://github.com/fubar-coder/bb2-c/issues/269)).
* `CopyShape` of such a shape copied the mark, so the memory of the copy
  was never freed before the program ended; the copy has no mark
  ([#268](https://github.com/fubar-coder/bb2-c/issues/268)).
* A file that `SaveShapes` wrote from such a shape held the mark, and
  `LoadShapes` of the file, or `DecodeShapes` of it outside chip memory,
  took it over, so the loaded shapes were never freed before the program
  ended; the mark is not taken from the file
  ([#267](https://github.com/fubar-coder/bb2-c/issues/267)).
* `SaveBitmap` of a bitmap whose flags are set (not one made with
  `BitMap`, which sets none) read the depth together with the flags and
  wrote hundreds of planes per line, from memory behind the bitmap, into
  an invalid file; it writes the planes the bitmap has
  ([#180](https://github.com/fubar-coder/bb2-c/issues/180)).
* `LoadBitMap` and `LoadScreen` of a picture with more planes than a
  bitmap whose flags are set wrote the extra planes through the memory
  behind the bitmap; they load only the planes the bitmap has
  ([#188](https://github.com/fubar-coder/bb2-c/issues/188)).

### Sounds

* `DecodeSound` uses sound data that lies in chip memory where it is, and
  marks the sound so that this memory is not freed. The mark stayed when
  the next sample was put into the same sound object, by `DecodeSound` of
  other memory, `LoadSound` or `InitSound`, so that sample was never freed
  before the program ended; it is freed when the object is reused or
  freed ([#68](https://github.com/fubar-coder/bb2-c/issues/68)).

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

## Runtime errors without the debugger

Every library has an area of error checks that is left out when a
program is compiled without runtime error checking. Several libraries had
error handling that jumped into that area, or kept the text of an error
message there. Without runtime error checking, such an error jumped to
memory behind the library, or reported a message that pointed into
nowhere. The port puts this code and these messages where they belong, so
the error is reported with its proper text in both cases. With runtime
error checking on, nothing changes.

The fixes concern:

* [memory errors, "Out of Memory!" (#956)](https://github.com/fubar-coder/bb2-c/issues/956)
* [fonts, "Can't Open Font" and others (#955)](https://github.com/fubar-coder/bb2-c/issues/955)
* [screens, "Can't open Screen" and others (#251)](https://github.com/fubar-coder/bb2-c/issues/251)
* [displays and copper lists (#412)](https://github.com/fubar-coder/bb2-c/issues/412)
* [shapes, `DecodeShapes` of address 0 (#270)](https://github.com/fubar-coder/bb2-c/issues/270) and [the shape error messages (#953)](https://github.com/fubar-coder/bb2-c/issues/953)
* [sprite files (#954)](https://github.com/fubar-coder/bb2-c/issues/954)
* [`SetPeriod` without a sound, and the sound error messages (#70)](https://github.com/fubar-coder/bb2-c/issues/70)
* [animations, "Can't Open Anim File" (#398)](https://github.com/fubar-coder/bb2-c/issues/398)
* [music modules, "Error Accessing Module File" (#957)](https://github.com/fubar-coder/bb2-c/issues/957)
* [IFF pictures (#192)](https://github.com/fubar-coder/bb2-c/issues/192) and [saving IFF files, "Can't open File" (#185)](https://github.com/fubar-coder/bb2-c/issues/185)
* [Workbench arguments without memory at the start of a program (#305)](https://github.com/fubar-coder/bb2-c/issues/305)

## AmigaOS calls

These fixes concern the calls of AmigaOS functions
([AmigaOS libraries]({{< relref "/docs/libraries/amigaos" >}})).

### A missing AmigaOS library hung the program at its start {#missing-os-library}

A program opens every AmigaOS library it calls when it starts. When a
library could not be opened, the original tried again, without end. A
program that calls a function of a library the system does not have, such
as `gadtools.library` on Kickstart 1.3, hung before its first line, even
if it checks the system version and never makes that call.

**What happened**

On Kickstart 1.3, which has no `gadtools.library`:

```
DEFTYPE .l
NPrint "start"
Goto skip
x=CreateContext_(&x)
skip:
NPrint "end"
End
```

The original hangs at its start and prints nothing.

**With the port**

The program starts. The call of the missing library is never made, and
the program prints

```
start
end
```

A call of a function of the missing library would still crash.

Issue: [A program hung at its start if an AmigaOS library it calls was missing (#44)](https://github.com/fubar-coder/bb2-c/issues/44).

### Other fixes

* A program that calls a resource, such as the battery clock or the CIA
  resources, hung at its start; the resource is opened
  ([#45](https://github.com/fubar-coder/bb2-c/issues/45)).
* `CachePreDMA_` and `CachePostDMA_` passed their flags in the wrong
  register; the flags reach the system
  ([#959](https://github.com/fubar-coder/bb2-c/issues/959)).

## RWE userlibs

These fixes concern the
[RWE userlibs]({{< relref "/docs/libraries/userlibs" >}}).

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

### Text after a string function {#rwe-text-after-string-function}

These functions had the fault described under
[Text after a string function lost its last character](#text-after-string-function):
followed by more text in a string expression, such as
`ProgDir$+"/data"`, the result held a 0 byte and the last character of
the text was missing. In the port the text follows the result.

* `ProgPath$`, `ProgDir$`, `ProgName$` and `DefaultFontName`
  ([#743](https://github.com/fubar-coder/bb2-c/issues/743))
* `GetPathName` and `NextDevice`
  ([#618](https://github.com/fubar-coder/bb2-c/issues/618))
* `NextAppFile` and `AppFile`
  ([#519](https://github.com/fubar-coder/bb2-c/issues/519))
* `RTEZLoadFile` and `RTFileRequest`
  ([#461](https://github.com/fubar-coder/bb2-c/issues/461))
* `GetLibraryValue$`, `GetRegistryStringVariable`, `ParseItemName$` and
  `ParseItemValue$` ([#783](https://github.com/fubar-coder/bb2-c/issues/783))
* `ReqFileRequest`
  ([#784](https://github.com/fubar-coder/bb2-c/issues/784))
* `SerialReadString`
  ([#795](https://github.com/fubar-coder/bb2-c/issues/795))

More functions left the place of the next part wrong in other ways:

* Text after `RTEZSaveFile` was written over the end of the chosen path,
  or behind bytes of its arguments; it follows the path
  ([#465](https://github.com/fubar-coder/bb2-c/issues/465)).
* `EZRequest` and `EZRequestArgs`, used as functions in a string
  expression, left the place at address 0 or on their argument list
  inside the library, so the rest of the expression was written over the
  start of memory or over that list: a line such as
  `a$="Answer "+Str$(EZRequest("T","Body","Yes|No"))` came out wrong or
  crashed the program. The rest of the expression follows the result
  ([#1027](https://github.com/fubar-coder/bb2-c/issues/1027)).
* `ChangeWBSprite`, used as a function in a string expression, left the
  place on a tag list inside the library, so the rest of the expression
  was written over that list; the place stays where it was
  ([#1028](https://github.com/fubar-coder/bb2-c/issues/1028)).

### Other fixes

* `AddAppIcon` set the base of the local variables to 0: inside a
  Statement or Function the local variables used after the command were
  read and written in the wrong place, and the procedure crashed when it
  returned. Used as a function in a string expression, it also let the
  rest of the expression be written into the data of the icon. Local
  variables and string expressions work after `AddAppIcon`
  ([#516](https://github.com/fubar-coder/bb2-c/issues/516)).
* `InitArgParse` copied a line of any length into its 256 byte buffer and
  overwrote the code of the library behind it; it copies at most 255
  characters and cuts off the rest
  ([#752](https://github.com/fubar-coder/bb2-c/issues/752)).
* `RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on
  some data that is not a `FORM ANIM`, such as an IFF `LIST` of type
  `ANIM`; they return normally, and for such a `LIST` `RIAnimDepth` returns
  0 ([#513](https://github.com/fubar-coder/bb2-c/issues/513)).
* `LELoadShapes` and `LECludgeShapes` into shapes that an earlier
  `LECludgeShapes` had linked into memory read the shape from a wrong
  place and ended early without closing the file, and a second such shape
  crashed the program; such a shape is replaced like any other
  ([#807](https://github.com/fubar-coder/bb2-c/issues/807)).
* `CludgeSound`, in both sound libraries, on a sound that `DecodeSound`
  made from data in chip memory stopped the program with "Unable to Free
  Memory", because it freed memory that no library had allocated; that
  memory is left alone
  ([#945](https://github.com/fubar-coder/bb2-c/issues/945)).
* `CludgeSound` of the second sound library kept the mark by which
  `DecodeSound` keeps such data from being freed, so the copy of the sample it makes was never freed before the
  program ended; the mark is cleared
  ([#925](https://github.com/fubar-coder/bb2-c/issues/925)).
* Removing the CIA interrupt of the tracker library, by
  `ChangeTrackerVBlank`, by `StartTracker` in CIA mode or at the end of
  the program, switched the audio filter, and with it the power LED, over;
  both stay as they are
  ([#449](https://github.com/fubar-coder/bb2-c/issues/449)).
* `PopUpWindow` switched the audio filter and the power LED over for
  every menu with sub menus it showed; both stay as they are
  ([#468](https://github.com/fubar-coder/bb2-c/issues/468)).
* Freeing a chunky bitmap, when `ChunkyBitmap` makes it again or at the
  end of the program, switched the audio filter and the power LED over;
  both stay as they are
  ([#886](https://github.com/fubar-coder/bb2-c/issues/886)).
* `ChunkyToPlanar` read the depth of the bitmap together with its flags:
  depending on the flags it wrote 8 planes into a bitmap with fewer, or
  refused a bitmap of depth 8; it checks the depth alone
  ([#883](https://github.com/fubar-coder/bb2-c/issues/883)).
* `ClearBitmap`, `FadeInBitmap` and `ZoomyZoom` read the depth of the
  bitmap together with its flags, so a bitmap whose flags are set counted
  as one with hundreds of planes and the program crashed; they read the
  depth alone ([#670](https://github.com/fubar-coder/bb2-c/issues/670)).
