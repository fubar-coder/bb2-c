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

## Changes for existing programs

Four of the fixes change what an existing program shows. Two of them
concern programs that were written to make up for the bug, drawing on
bitmaps; no known example program, demo or game uses these commands this
way, but a program of your own might. The other two concern the custom
copper instructions of the display library and change every program that
uses the commands with the values named.

### `RenderToBitmap` with a dither bitmap drew on the wrong bitmap {#rendertobitmap-dither}

`RenderToBitmap BitMap#,DitherBitMap#` makes `RenderPoly` draw on
`BitMap#`, filled with the pattern of `DitherBitMap#`, as `BitMapPoly`
does with a dither bitmap.

**What happened.** The original took the two bitmaps the other way
round: after

```
RenderToBitmap 0,1
RenderPoly 3,&pts(0),2
```

it drew the polygon on bitmap 1, with bitmap 0 as the pattern, and left
bitmap 0 unchanged. A program that wanted to draw on bitmap 0 had to write
`RenderToBitmap 1,0`.

**With the port.** `RenderPoly` draws on the first bitmap with the second
as the pattern, as the command is documented: the lines above draw on
bitmap 0. `RenderToBitmap` with one bitmap is not affected.

**What to change.** A program that swapped the two arguments to get the
polygon on the right bitmap now draws on the dither bitmap. Write the
bitmap to draw on first and the dither bitmap second.

Issue: [RenderToBitmap b,d and RenderPoly drew on the dither bitmap (#421)](https://github.com/fubar-coder/bb2-c/issues/421).

### `Box` added the bitmap origin twice {#box-origin}

`BitMapOrigin` moves the point 0,0 of a bitmap for the drawing commands.
`Line`, `Boxf` and `Plot` add the origin to their coordinates once;
`Box` added it twice, so after `BitMapOrigin 0,50,50` the line
`Box 0,0,10,10,1` drew its corner at 100,100.

**With the port.** `Box` adds the origin once, like the others: the corner
is at 50,50. Without `BitMapOrigin`, or with the origin 0,0, nothing
changes.

**What to change.** A program that gave `Box` coordinates reduced by the
origin to make up for the doubling now draws the box displaced. Give
`Box` the same coordinates as `Line` and `Boxf`.

Issue: [Box added the bitmap origin twice (#391)](https://github.com/fubar-coder/bb2-c/issues/391).

### `CustomSprites` took its offset 16 times too large {#customsprites-ccoffset}

A copper list of the display library can hold custom copper
instructions, reserved with the `numcustoms` of `InitCopList`.
`CustomColors`, `CustomString`, `CustomSprites` and `CustomChunky` fill
them at a line of the display, `CCOffset` instructions into that space;
an instruction is four bytes. `CustomSprites CopList#,CCOffset,YPos,NumSprites`
sets up the further sprites, the channels 8 to 15 of a list with 8.

**What happened.** The original took the `CCOffset` of `CustomSprites`
as a number of 16 instructions: `CustomSprites 0,2,14,2` put its
instructions 32 instructions into the custom space instead of 2, over
those of other custom commands or behind the space. With runtime error
checking, an offset above the sprite maximum of the compiler options
was the error "Value out of Maximum Range".

**With the port.** `CCOffset` counts instructions, as for the other
custom commands and as the command is documented: the line above starts
the sprite instructions 2 instructions into the custom space. An offset
of 0 is not affected, and the offset is not checked against the sprite
maximum.

**What to change.** A program that gives `CustomSprites` a `CCOffset`
other than 0 now puts the sprite instructions at another place. Give
the offset in instructions, counted like those of `CustomColors` and
`CustomString`. To keep the place the original used, multiply the old
offset by 16.

Issue: [CustomSprites misused CCOffset as a sprite object number (#324)](https://github.com/fubar-coder/bb2-c/issues/324).

### `CustomColors` with a `thru256flag` of 0 acted as 1 {#customcolors-thru256flag}

A copper wait holds only the lower 8 bits of a line, so a custom
command at line 256 of the display or below first adds a wait for the
end of line 255. The last argument of
`CustomColors CopList#,CCOffset,YPos,Palette,startcol,numcols,thru256flag`
tells the display library that the colour change itself runs on past
line 255, as a large AGA palette change can: the custom commands after it
then add no second wait for line 255.

**What happened.** The original did not read the value of the flag: a
`thru256flag` of 0 acted as 1. After

```
InitCopList 0,44,256,3,8,8,20
CustomColors 0,0,20,0,0,1,0
CustomColors 0,4,220,0,0,1
```

the second colour change, at line 264 of the display, had no wait for
line 255, and the copper made it about 256 lines too early.

**With the port.** A `thru256flag` of 0 is the same as leaving the flag
out; any other value sets it. The second colour change above gets its
wait for line 255 and comes at its line.

**What to change.** A program that gives a `thru256flag` of 0 and needs
the flag set has to give 1.

Issue: [CustomColors ignored the value of thru256flag (#951)](https://github.com/fubar-coder/bb2-c/issues/951).

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

## Printing numbers

### Quicks were printed with a stray point or one too low {#quick-print}

A Quick has 16 bits after the point, so its fraction is a multiple of
1/65536. `Print`, `NPrint` and `Str$` show it rounded to four decimals
and leave out the trailing zeros. When the fraction rounded to `.0000`,
the original removed the zeros but kept the point, and when it rounded up
to a whole number, it lost the carry into the part before the point. A
Quick that is the result of a calculation often lies just above or below
a whole number, so a program printed `2.` where it should print `3`.

**What happened**

The program sets the 32 bits of a Quick directly, to get these values
exactly: 1.00002, 0.00002, 2.99998 and 32767.99998.

```
Statement pq{x.l}
  q.q=0 : Poke.l &q,x
  NPrint "quick ",Hex$(x)," <",q,"> <",Str$(q),">"
End Statement
pq{$00010001} : pq{$00000001} : pq{$0002FFFF} : pq{$7FFFFFFF}
End
```

The original prints

```
quick 00010001 <1.> <1.>
quick 00000001 <.> <.>
quick 0002FFFF <2.> <2.>
quick 7FFFFFFF <32767.> <32767.>
```

**With the port**

```
quick 00010001 <1> <1>
quick 00000001 <0> <0>
quick 0002FFFF <3> <3>
quick 7FFFFFFF <32768> <32768>
```

A negative value that rounds to 0, such as -0.00002, prints `0` as well,
without a minus sign.

Issues: [A Quick whose fraction rounds to 0 kept the point (#234)](https://github.com/fubar-coder/bb2-c/issues/234),
[a Quick whose fraction rounds up lost the carry (#233)](https://github.com/fubar-coder/bb2-c/issues/233).

### `Format` rounded wrongly and lost the minus sign {#format}

`Format` sets a format string, such as `"###.##"`, for the numbers that
`Print`, `NPrint` and `Str$` write. The original had four faults there.
A carry that rounded out of the first digit was lost, so 999.96 with
`"####.#"` became ` 000.0`. A number below 1 was cut instead of
rounded. A negative number lost its minus sign when the format string
had no `-` or `+`. And a number with more digits before the point than
the format has places gave a `?` followed only by spaces.

**What happened**

```
f.f=999.96 : Format "####.#" : NPrint "a <",f,">"
f=0.567 : Format ".##" : NPrint "b <",f,">"
Format "###" : NPrint "c <",-7,">"
Format "##" : NPrint "d <",1234,">"
Format ""
End
```

The original prints

```
a < 000.0>
b <.56>
c <  7>
d <?  >
```

**With the port**

```
a <1000.0>
b <.57>
c < -7>
d <?34>
```

The carry makes a new first digit, and if that digit does not fit, the
number gives a `?` like any other number that is too large. The minus
sign takes the place in front of the number, also in front of the zeros
of `0` places: -7 with `"000"` prints `-07`. A number that is too large
prints the `?` and then its lowest digits.

Issues: [Format lost a carry out of the first digit (#235)](https://github.com/fubar-coder/bb2-c/issues/235),
[Format did not round numbers below 1 (#236)](https://github.com/fubar-coder/bb2-c/issues/236),
[Format without "-" or "+" lost the sign (#237)](https://github.com/fubar-coder/bb2-c/issues/237),
[Format with too many digits printed "?" and only spaces (#238)](https://github.com/fubar-coder/bb2-c/issues/238).

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

### `LoadFont` lost the style {#loadfont-style}

`LoadFont` takes a style as its last argument: 1 underlined, 2 bold, 4
italic, 8 extended, or a sum of them. The original wrote this value
one byte too far into the font object, the one that describes the font
to the system. The style itself stayed 0, and the value went into the
flags of the font, which tell the system where to look for it. A font
loaded with a style came out without it.

**What happened**

```
DEFTYPE .l
LoadFont 1,"topaz.font",9,1
a=Addr IntuiFont(1)
NPrint "style ",Peek.b(a+6)," flags ",Peek.b(a+7)
End
```

The program loads topaz 9 underlined and reads the style and flags bytes
of the font object. The original prints

```
style 0 flags 1
```

**With the port**

The style is set and the flags are 0:

```
style 1 flags 0
```

A style above 255 counts only with its low byte.

Issue: [LoadFont wrote the style into the font flags (#197)](https://github.com/fubar-coder/bb2-c/issues/197).

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

## File requesters

### `FileRequest$` left the old lengths in the path and the file name {#filerequest-lengths}

`FileRequest$` takes two string variables, the path and the file name to
start with, and after OK writes the ones the user chose back into them,
so that the next call starts there again. The original wrote the new
characters but left the lengths of both variables as they were. `Len`,
`Print` and every string expression with these variables then saw the
old length: a path that was empty before stayed empty for them, and a
name that was longer before kept characters of the old one at its end.
The result of `FileRequest$` itself was right.

**What happened**

```
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$=""
f$="x"+Chr$(0)+"yz"
r$=FileRequest$("FQ",p$,f$)
NPrint "ok ",Len(r$)," [",r$,"]"
NPrint "path ",Len(p$)," [",p$,"]"
NPrint "file ",Len(f$)
End
```

The path starts empty, and the file name is `x`, a 0 byte and `yz`, 4
characters, of which the requester shows the `x`. The program runs in the
drawer `a` of the volume `CORPUS:`, and the user clicks PARENT, which
with an empty path goes to the parent of the current directory, and then
OK. The original prints

```
ok 8 [CORPUS:x]
path 0 []
file 4
```

The path holds `CORPUS:` but has the length 0, and the file name `x`
still has the length 4.

**With the port**

```
ok 8 [CORPUS:x]
path 7 [CORPUS:]
file 1
```

After OK both variables have the lengths of their new contents. Cancel
and the close gadget leave them as they were.

Issue: [FileRequest$ changed the path and file name without setting their lengths (#153)](https://github.com/fubar-coder/bb2-c/issues/153).

### Long file names overwrote memory {#filerequest-long-names}

The file requester keeps the path in a field of 160 bytes and the file
name in one of 64 bytes, so it has room for a path of 159 and a name of
63 characters. The original copied names and paths into these fields
without looking at their length. On the classic Amiga file system a name
has at most 30 characters, but the newer file systems (FFS with long
names, PFS, SFS) and the host directories of an emulator allow much
longer ones. A click on a file name of 64 or more characters in the list
wrote it behind the file name field, and OK then copied on through memory
until the program crashed. A long path, or a path the program's string
variable had no room for, overwrote memory the same way.

The port refuses a name or a path that does not fit. It also respects
the room the program gives: a name longer than the `MaxLen` of the
file name variable, or a path longer than the `MaxLen` of the path
variable, is refused too. The requester then stays open, keeps what its
fields held, and shows "Name too long" or "Path too long" in the title
bar of the screen until the user clicks the next button, field or line.

**What happened**

```
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
d$="CORPUS:fqname/"
If WriteFile(0,d$+"a123456789b123456789c123456789d123456789e123456789f123456789g123456789") Then CloseFile 0
MaxLen p$=160
MaxLen f$=64
p$="CORPUS:fqname"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint Len(r$)," [",r$,"] file ",Len(f$)," [",f$,"]"
End
```

The program creates a file with a name of 70 characters and opens the
requester in its drawer. The user clicks the name in the list and then
OK. The original copies all 70 characters into the 64-byte field, and OK
crashes the program.

**With the port**

The click is refused, and the screen's title bar says so:

![The port's file requester after a click on a name of 70 characters: the screen title bar reads "Name too long", the FILE field still holds x](name-too-long.png)

OK then returns the file the requester held before:

```
15 [CORPUS:fqname/x] file 1 [x]
```

A program that wants names of up to 63 characters gives its file name
variable a `MaxLen` of 64 and its path variable one of 160, as the
page of [freqlib]({{< relref "/docs/libraries/commands/freqlib" >}})
shows. A program whose screen has no visible
title bar does not see the message, but the name is refused all the
same. The editor's file requester is the same code and has the same
limits.

Issue: [The file requester copied names longer than 63 characters without a limit (#1042)](https://github.com/fubar-coder/bb2-c/issues/1042).

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

With runtime error checking, a size outside the limits of a window with a
sizing gadget, as here, is the error "Can't Size Window", and the program
above stops at `WSize`
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#wsize-limits" >}})).

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

### `WCls` left the top line of the window {#wcls-top-line}

`WCls` clears the used window with colour 0 or the colour given. The
original cleared it from its second pixel line on, so the top line kept
what was drawn there. This shows most in a window without a border,
where that line is the first line of the contents.

**What happened**

The program opens a window without a border on a screen of its own,
clears it with colour 2 and reads the colours of the pixels 5,0 and 5,1
through the graphics library:

{{% details title="The test program" closed="true" %}}

```
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"wcls",1,2
Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
Window 0,20,30,100,50,$0800,"",0,1
DefaultOutput
rp=RastPort(0)
x=5 : y=0 : Gosub pix : a=v : y=1 : Gosub pix : b=v
NPrint "before ",a," ",b
WCls 2
x=5 : y=0 : Gosub pix : a=v : y=1 : Gosub pix : b=v
NPrint "after ",a," ",b
CloseWindow 0
CloseScreen 0
End

; colour of the pixel x,y of rp -> v
pix:
  GetReg d0,gfx : MOVE.l d0,a6
  GetReg d0,rp : MOVE.l d0,a1
  GetReg d1,y
  GetReg d0,x
  JSR -318(a6)
  PutReg d0,v
Return
```

{{% /details %}}

The original prints

```
before 0 0
after 0 2
```

The top line is still colour 0.

**With the port**

The whole window is cleared:

```
before 0 0
after 2 2
```

Issue: [WCls only cleared from the second line (#314)](https://github.com/fubar-coder/bb2-c/issues/314).

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

### File requesters {#file-requester-fixes}

`FileRequest$` opens the file requester of Blitz Basic 2; the editor's
file requester is the same code and has the same fixes, except for the
[lengths of the path and the file name](#filerequest-lengths), which only
`FileRequest$` sets. `ASLFileRequest$`, `ASLPathRequest$` and
`ASLScreenRequest` open the requesters of the system's `asl.library`.

* A file name of 64 or more characters, or a path of 160 or more, was
  copied into the requester's fields without a limit and overwrote
  memory; the requester refuses it and says "Name too long" or "Path too
  long" in the screen's title bar
  ([Long file names overwrote memory](#filerequest-long-names),
  [#1042](https://github.com/fubar-coder/bb2-c/issues/1042)).
* `FileRequest$` on a screen of 645 lines or more, such as a 1280×1024
  RTG Workbench, wrote far behind its data and could crash the program
  after the requester closed; the list shows at most 64 lines and the
  window is at most 597 lines high
  ([#160](https://github.com/fubar-coder/bb2-c/issues/160)).
* Every list entry of the requester had room for a name of 34
  characters, so a longer name, on a file system with long names, was
  written behind it, and an entry for which no memory was left was
  written from address 0 on and crashed the system; each entry gets the
  room its name needs, and an entry without memory is left out of the
  list ([#159](https://github.com/fubar-coder/bb2-c/issues/159)).
* The CD button made the drawer shown the program's current directory and
  released the directory the program had been started in, which belongs
  to the shell that started it, and the program ended in the new drawer.
  CD still changes the current directory while the program runs, but the
  start directory is kept and is current again when the program ends,
  unless the program has changed the current directory itself since
  ([#157](https://github.com/fubar-coder/bb2-c/issues/157)).
* After CD, every later click into the path gadget, the field with the
  path, made the system copy the path into the first 64 KB of memory, in
  that requester and in all later ones, because CD damaged the gadget's
  data instead of resetting its cursor; CD resets the cursor and the scroll position of the path
  ([#345](https://github.com/fubar-coder/bb2-c/issues/345)).
* PARENT with an empty path and CD, which find the path of the current
  directory, left every parent directory in use until the next reboot,
  so that such a disk could not be renamed or ejected cleanly; the
  directories are released
  ([#158](https://github.com/fubar-coder/bb2-c/issues/158)).
* DRIVES read the system's list of volumes, assigns and devices without
  locking it, so a disk change, an assign or a mount at that moment could
  make the requester read freed memory; it locks the list while it reads
  it, and shows the same entries
  ([#156](https://github.com/fubar-coder/bb2-c/issues/156)).
* Every call opened the DOS, Intuition and graphics libraries of the
  system and never closed them, so their use counts rose with every
  requester; they are closed at the end of each call
  ([#154](https://github.com/fubar-coder/bb2-c/issues/154)).
* `ASLFileRequest$` and `ASLPathRequest$` without `asl.library`, such as
  on Kickstart 1.3, returned a string whose length was the address of the
  title, so an assignment copied that many bytes; they return the empty
  string, as Cancel does
  ([#62](https://github.com/fubar-coder/bb2-c/issues/62)).
* `ASLFileRequest$` and `ASLPathRequest$` asked `asl.library` to free a
  requester at address 0 when the requester could not be allocated; they
  free only one that was allocated
  ([#403](https://github.com/fubar-coder/bb2-c/issues/403)).
* Every `ASLFileRequest$` and `ASLPathRequest$` kept a copy of its title
  and pattern in memory until the program ended; the copies are freed
  after each request
  ([#63](https://github.com/fubar-coder/bb2-c/issues/63)).
* `ASLPathRequest$` wrote the file name of the requester, after OK, to an
  address it had not been given: in the form with a position, the value
  of `x`, so `x=20` overwrote two exception vectors of the processor; it
  writes nothing there, and the result is the same
  ([#61](https://github.com/fubar-coder/bb2-c/issues/61)).
* `ASLPathRequest$` without a position and size, after an
  `ASLFileRequest$` with a pattern, showed a pattern gadget with that
  pattern; it shows none
  ([#64](https://github.com/fubar-coder/bb2-c/issues/64)).
* `ASLScreenRequest` freed the requester and then returned its address,
  so the program read the chosen mode from freed memory; it returns the
  address of a copy of the requester's fields, which stays valid until
  the next `ASLScreenRequest`
  ([#60](https://github.com/fubar-coder/bb2-c/issues/60)).

### Strings

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
* `Instr` with a start position of 0 or less compared the bytes before
  the string, so an empty search string was found at position 0 or at
  the negative start (`Instr(a$,"",-5)` gave -5); a start position below
  1 searches from the first character, as `Instr` without a start
  position does, and with runtime error checking it is the error
  "Instr Startpos MUST be greater than zero"
  ([#287](https://github.com/fubar-coder/bb2-c/issues/287)).
* `Instr` with a start position near $7FFFFFFF went on searching in
  memory far from the string, until it found the text there or hung; it
  returns 0
  ([#1030](https://github.com/fubar-coder/bb2-c/issues/1030)).
* `Cvi` always read two bytes of its string and `Cvl` and `Cvq` four, so
  a shorter string gave whatever an earlier string expression had left
  behind it; the missing bytes count as 0, so `Cvl("ab")` is $61620000
  and `Cvi("")` is 0. With runtime error checking a string that is too
  short is the error "Cvi needs a String of at least 2 Characters", or
  the same for `Cvl` and `Cvq` with 4
  ([#288](https://github.com/fubar-coder/bb2-c/issues/288)).
* `Peeks$` with a negative length copied up to 4 GB into the string
  buffer and crashed the program; it returns the empty string, as a
  length of 0 does, and with runtime error checking it is the error
  "Can't Peeks$ a negative number of Characters!"
  ([#214](https://github.com/fubar-coder/bb2-c/issues/214)).

### Input

* `Inkey$(0)` returned a string of a random length; it returns the empty
  string ([#128](https://github.com/fubar-coder/bb2-c/issues/128)).
* `Edit` with a default value in a window overwrote the last character
  of the default with a space, so `Edit(9.5,5)` offered `9.` and gave 9
  when the user just pressed Return, and `Edit(25,4)` gave 2; the whole
  default is shown, and Return gives 9.5 and 25
  ([#129](https://github.com/fubar-coder/bb2-c/issues/129)).
* `Edit$` at the end of its input, such as a file given to the program
  with `<` in the shell, returned a string of the length -1; it returns
  the empty string, and `Edit` there returns 0
  ([#132](https://github.com/fubar-coder/bb2-c/issues/132)).
* The runtime error of `Inkey$` from the default input read "Can't Input
  Character from Defualt Input"; it reads "Default"
  ([#193](https://github.com/fubar-coder/bb2-c/issues/193)).
* `Inkey$` with a negative count tried about 4 billion times to read a
  character and added every character it got behind the end of the
  string buffer; with runtime error checking a negative count is the
  error "Can't Inkey$ a negative number of Characters!"
  ([#1031](https://github.com/fubar-coder/bb2-c/issues/1031)).
* With runtime error checking, the first `PopInput` or `PopOutput` with
  no switch of the channel left to undo, such as one at the start of the
  program, passed the check and left the program without a channel; the
  error came only at the pop after it. It is reported at the first such
  pop
  ([#1032](https://github.com/fubar-coder/bb2-c/issues/1032)).

### Arrays and sorting

* A `Dim` of an array of 4 GB or more, such as `Dim a.l(32767,32767)`,
  computed a wrong, tiny size and gave an array of a few bytes; it stops
  with the error "Out of Memory!", and with runtime error checking with
  "Array Size out of Range"
  ([#58](https://github.com/fubar-coder/bb2-c/issues/58)).
* A dimension of an array or list has at most 65535 elements, the indexes
  0 to 65534. `Dim a(65535)` and `Dim List a(65535)` gave an array or
  list of no elements, and every access to it lay outside its memory, but
  runtime error checking refused only an index of 65536 and more; it
  refuses 65535 too, with the error "Array Dimension out of Range"
  ([#1037](https://github.com/fubar-coder/bb2-c/issues/1037)).
* `Sort` on a string array read the length of an element that was never
  given a value from address -4 and its characters from address 0, so
  such elements sorted to places that depended on that memory; they
  count as empty strings and come first, or last after `SortDown`
  ([#276](https://github.com/fubar-coder/bb2-c/issues/276)).
* `Sort` on a word array took the word after the middle element as the
  value it compares with, so for an array of one element it read the word
  behind the array and searched the memory behind and possibly before it;
  it takes the middle element. The sorted array is the same
  ([#275](https://github.com/fubar-coder/bb2-c/issues/275)).

### Lists

* `Dim List` with size 0 wrote behind its memory block; nothing is
  written there ([#59](https://github.com/fubar-coder/bb2-c/issues/59)).
* `SortList` put keys more than 2^31 apart in the wrong order, so
  2000000000 came before -2000000000; it sorts every key in ascending
  order ([#201](https://github.com/fubar-coder/bb2-c/issues/201)).

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
* A toggle `GTButton` whose gadget could not be made wrote to address 14;
  only a gadget that was made is changed
  ([#171](https://github.com/fubar-coder/bb2-c/issues/171)).
* The flags 64 (disabled) and 128 (immediate) of `GTShape` had no effect;
  they work
  ([#990](https://github.com/fubar-coder/bb2-c/issues/990)).
* A `GTShape` gadget got the label of the gadget made before it; it has
  none ([#172](https://github.com/fubar-coder/bb2-c/issues/172)).
* After `GTFreeGadget` of the last gadget of a GadTools list, the next
  gadget was linked to the freed one and was neither shown nor freed; it
  becomes part of the list
  ([#962](https://github.com/fubar-coder/bb2-c/issues/962)).

### Gadgets

* `SelectMode` had no effect, so `SelectMode 1` did not give new gadgets
  a box as their highlight; it does
  ([#952](https://github.com/fubar-coder/bb2-c/issues/952)).
* `Free GadgetList` did not free the text buffers of string gadgets and
  the data of prop gadgets, which stayed allocated until the program
  ended; they are freed with the list
  ([#163](https://github.com/fubar-coder/bb2-c/issues/163)).
* `StringGadget` after `Borders Off` wrote to the addresses 0, 2, 36 and
  38; it writes nothing there
  ([#164](https://github.com/fubar-coder/bb2-c/issues/164)).
* `StringGadget` with a `Maxlen` of 0 cleared 65536 bytes of memory and
  crashed the program; with runtime error checking it is the error
  "Maxlen MUST be greater than zero"
  ([#165](https://github.com/fubar-coder/bb2-c/issues/165)).
* A `ShapeGadget` of a shape of depth 0 got a broken image; with runtime
  error checking such a shape cannot be made, as `InitShape` refuses a
  depth of 0
  ([#167](https://github.com/fubar-coder/bb2-c/issues/167)).

### Menus

* A menu, item or subitem with an empty text read its text from address
  0, and the item got the width of whatever was there; the text is empty,
  and an empty item is as wide as its gaps
  ([#981](https://github.com/fubar-coder/bb2-c/issues/981)).
* `MenuState` of a single menu changed the wrong bit, so a menu could not
  be disabled or enabled on its own; it can
  ([#216](https://github.com/fubar-coder/bb2-c/issues/216)).
* `MenuChecked` of any entry but the first item and subitem of a menu
  walked the menu lists again and gave the state of another entry, or
  read unrelated memory; it gives the state of the entry asked for
  ([#217](https://github.com/fubar-coder/bb2-c/issues/217)).
* `ShapeItem` and `ShapeSub` ignored small flag values such as toggle,
  exclude and checked; they take their flags as `MenuItem` does
  ([#218](https://github.com/fubar-coder/bb2-c/issues/218)).
* Item flags such as $10000 were taken as 4, a checked toggle item that
  excludes all others; they are taken as given
  ([#219](https://github.com/fubar-coder/bb2-c/issues/219)).
* Replacing a subitem moved the subitems behind it by a wrong amount, so
  they covered it or left a gap; they move by the change of its height
  ([#220](https://github.com/fubar-coder/bb2-c/issues/220)).

### Blitz mode boxes

The Blitz OS commands give a Blitz mode program pages of boxes as its
user interface
([Blitz OS]({{< relref "/docs/libraries/commands/blitzoslib" >}})).

* `AddBox` of a string box 8 to 15 pixels wide, whose text has room for
  one byte, wrote 65536 spaces behind that byte; it writes none
  ([#84](https://github.com/fubar-coder/bb2-c/issues/84)).
* On a page of more than 1024 boxes, the box numbers from 1024 on reached
  the memory before the box table or, from 2048 on, the boxes from 0 on,
  and `BoxSelected` returned a wrong number; every box number reaches its
  own box, and `BoxSelected` returns it
  ([#81](https://github.com/fubar-coder/bb2-c/issues/81)).
* `BoxString` of an answer of spaces only, such as that of a string box
  never typed into, removed the spaces beyond the start of the answer
  and could copy up to 65535 bytes into the string buffer; it returns the
  empty string
  ([#85](https://github.com/fubar-coder/bb2-c/issues/85)).
* A box added again with `AddBox` without a text printed the memory from
  address 0 on when it was drawn; it prints nothing
  ([#89](https://github.com/fubar-coder/bb2-c/issues/89)).
* Cursor up or down in `BlitzEvent` with no box selected, such as right
  after `InitPage`, read low memory as a box and could write to it; it
  does nothing
  ([#90](https://github.com/fubar-coder/bb2-c/issues/90)).
* In a string box, Backspace at the start of the text and control keys
  such as Return, Esc and Tab turned the cursor off and left it off; it
  is drawn again
  ([#91](https://github.com/fubar-coder/bb2-c/issues/91)).
* `DeSelectBox` left the box selected, so it was still drawn as selected
  and `BoxSelected` still returned it; the selection is cleared, and
  `BoxSelected` returns -1
  ([#94](https://github.com/fubar-coder/bb2-c/issues/94)).
* `BoxPrint Box#,Bank#,Line` replaced the line feed at the end of the
  printed line by a 0 in the bank itself; the bank stays as it is
  ([#87](https://github.com/fubar-coder/bb2-c/issues/87)).
* A list box made with `AddBox` and a text, or given a text with
  `SetBoxTitle`, got no entries: it showed nothing, and `SetBox` on it
  always failed with "Value Out of Range"; it gets one entry for each part
  of the text
  ([#989](https://github.com/fubar-coder/bb2-c/issues/989)).
* `SpotBox` on a page that was not made with `InitPage`, was freed or has
  no boxes searched 65536 boxes in the memory behind the page and could
  return one found there; with runtime error checking it is the error
  "UnInitialised Page"
  ([#83](https://github.com/fubar-coder/bb2-c/issues/83)).
* `SelectBox Box#` with a number the page does not have selected and drew
  whatever lay behind the box table; with runtime error checking it is
  the error "Illegal Box# For Page"
  ([#82](https://github.com/fubar-coder/bb2-c/issues/82)).

### Windows and screens

* `WindowTags` with a long tag list overwrote code of the library; a long
  list is copied only in part and the rest is linked
  ([#317](https://github.com/fubar-coder/bb2-c/issues/317)).
* A screen mode with a monitor part, such as PAL or NTSC, opened on the
  default monitor; the whole mode is used
  ([#250](https://github.com/fubar-coder/bb2-c/issues/250)).
* `Screen`, `ScreenTags`, `WbToScreen` and `FindScreen` on a screen
  object that had used `ShowBitMap`, on Kickstart 3.0 or later, crashed
  the program when they freed the old screen; it is freed properly
  ([#247](https://github.com/fubar-coder/bb2-c/issues/247)).
* `FindScreen`, `WbToScreen` and `CloseScreen` on an object that had only
  found a screen another object had opened closed that screen, and the
  other object was left with a closed screen; a found screen is only
  forgotten
  ([#248](https://github.com/fubar-coder/bb2-c/issues/248)).
* `ScreenTags` with the address of a tag list of more than 15 tags
  overwrote code behind the library; up to 15 tags are copied and the
  rest of the list is linked, as for `WindowTags`
  ([#249](https://github.com/fubar-coder/bb2-c/issues/249)).
* `ScreenTags` with 16 or more tag and data pairs as arguments wrote
  behind its tag buffer, over code of the libraries; with runtime error
  checking more than 15 pairs are the error "Too Many Tags", as for
  `WindowTags`
  ([#1039](https://github.com/fubar-coder/bb2-c/issues/1039)). A longer
  list can be given as the address of a tag list.
* `Menus`, `MenusOn` and `MenusOff` reached only the windows in the
  lower half of the window numbers, so `MenusOff` left the menus of the
  others on; they reach every window
  ([#310](https://github.com/fubar-coder/bb2-c/issues/310)).
* `WindowTags` without tags opened the window with the extra tags of the
  previous `WindowTags`, such as its position and size; it uses none
  ([#316](https://github.com/fubar-coder/bb2-c/issues/316)).
* `WindowTags` with 16 or more tag and data pairs as arguments wrote
  behind its tag buffer, over data and code of the library; with runtime
  error checking more than 15 pairs are the error "Too Many Tags"
  ([#961](https://github.com/fubar-coder/bb2-c/issues/961)). A longer
  list can be given as the address of a tag list.
* A `Window` with the super bitmap flag `$80` but without a `BitMap#`
  got the bitmap of the last `Window` that named one; only the form with
  `BitMap#` gives a super bitmap window
  ([#319](https://github.com/fubar-coder/bb2-c/issues/319)).
* `WScroll` and `WPrintScroll` set the drawing colour of the window to 0;
  it stays as it was
  ([#315](https://github.com/fubar-coder/bb2-c/issues/315)).
* `Edit$` in a window with a proportional font took the normal width of the font
  for every character from code 160 on, such as the
  umlauts, so the cursor and the typed text were in the wrong place; every
  character has its own width
  ([#321](https://github.com/fubar-coder/bb2-c/issues/321)).
* After a gadget down of a string gadget, `Event` reports a gadget up at
  once, which programs rely on. When another event came in between, the
  real gadget up of that gadget was reported as well, two for one entry;
  it is reported once
  ([#311](https://github.com/fubar-coder/bb2-c/issues/311)).
* The extra menu event of a menu choice with several items picked had
  its qualifier from memory that was never set, so `EventQualifier` of it
  was random; it has the qualifier of the menu choice
  ([#312](https://github.com/fubar-coder/bb2-c/issues/312)).
* `Qualifier` set only the lower half of its long result, so the upper
  half held whatever was there before; the whole result is set
  ([#313](https://github.com/fubar-coder/bb2-c/issues/313)).

### Fonts

* Every `LoadFont` on the same IntuiFont object lost the memory of the
  copy of the previous font name; the old copy is freed. See also
  [`LoadFont` lost the style](#loadfont-style)
  ([#198](https://github.com/fubar-coder/bb2-c/issues/198)).
* `LoadBlitzFont` of a font that is not 8 by 8 pixels stopped with the
  error "Can't Open Font" and left the font open after the program; it
  closes the font before the error
  ([#150](https://github.com/fubar-coder/bb2-c/issues/150)).

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
* `NumDays` of a string too short for its day or month, such as `""`,
  read on into the bytes behind the string and could return a date made
  of them; such a string gives 0
  ([#1029](https://github.com/fubar-coder/bb2-c/issues/1029)).
* `QAngle` gave a wrong angle for points 32768 or more apart in x or y;
  it gives the right one
  ([#1003](https://github.com/fubar-coder/bb2-c/issues/1003)).
* `Sgn` of a float was always 1; it is -1, 0 or 1 by the value
  ([#301](https://github.com/fubar-coder/bb2-c/issues/301)).
* `Val` of a string longer than 32768 characters gave 0 or read only a
  part of it; it reads the whole string
  ([#303](https://github.com/fubar-coder/bb2-c/issues/303)).
* `Val` of a number with 10 or more decimal places gave a wrong fraction,
  and from 32 places on dropped it, so `Val("1.5000…")` with 32 places
  gave 1; the first 9 places count and the others are skipped
  ([#304](https://github.com/fubar-coder/bb2-c/issues/304)).
* A Quick 0 assigned to a float gave a negative zero instead of 0; it
  gives 0 ([#146](https://github.com/fubar-coder/bb2-c/issues/146)).
* The Quick -32768 assigned to a float gave +32768; it keeps its minus
  sign ([#147](https://github.com/fubar-coder/bb2-c/issues/147)).
* A division of longs or quicks by 0 gave the largest positive number for
  every dividend, so -5/0 was 2147483647; it gives the largest number with
  the sign of the dividend, so -5/0 is -2147483647, and it is still no
  error without runtime error checking. With runtime error checking, it
  stops with "GURU! - Division by Zero", as a Word division by 0 does
  ([#200](https://github.com/fubar-coder/bb2-c/issues/200); see
  [A Long or Quick division by 0]({{< relref "/docs/compared-with-original/new-errors#long-division-by-zero" >}})).
* A multiplication of longs or quicks whose product does not fit added
  part of the lost high bits back into the result, so `196608*30000` gave
  1603272705 instead of 1603272704, and the quick `300*300` came out
  1/65536 too large (a long was off by at most 65534, a quick by at most
  0.25); it gives the product with the bits that do not fit cut off, and a
  product that fits is the same as before
  ([#199](https://github.com/fubar-coder/bb2-c/issues/199)); see
  [Getting the old products](#old-products).
* `Str$` and `UStr$` of a string gave a number made from the string's
  address; they give the string
  ([#242](https://github.com/fubar-coder/bb2-c/issues/242)).
* A float below 1E-17 printed in fixed point form, with `FloatMode -1`
  or a format string set by `Format`, overwrote a table of the library,
  so numbers of 1E18 and more printed wrong digits afterwards (5E+18 as
  `9.5E+19`); they print right
  ([#241](https://github.com/fubar-coder/bb2-c/issues/241)).

#### Getting the old products {#old-products}

A program that lets a long or quick product overflow on purpose and needs
the same numbers as with the original, for example a random number
generator, can compute them with these two functions. Each multiplication
inside them has factors below 65536, which the original and the port both
compute exactly, so they give the original's products with either
library. A call is much slower than the `*` operator.

```
; a*b as the original gave it, for longs
Function.l OldMul{a.l,b.l}
  n.l=0
  If a<0 Then a=-a : n=1-n
  If b<0 Then b=-b : n=1-n
  ah.l=(a LSR 16) AND $FFFF : al.l=a AND $FFFF
  bh.l=(b LSR 16) AND $FFFF : bl.l=b AND $FFFF
  c1.l=ah*bl : c2.l=al*bh
  p.l=al*bl+(((c1+c2) AND $FFFF) LSL 16)+((c1 LSR 16) AND $FFFF)+((c2 LSR 16) AND $FFFF)
  If n Then p=-p
  Function Return p
End Function

; the same for quicks
Function.q OldQMul{a.q,b.q}
  ra.l=Peek.l(&a) : rb.l=Peek.l(&b) : n.l=0
  If ra<0 Then ra=-ra : n=1-n
  If rb<0 Then rb=-rb : n=1-n
  ah.l=(ra LSR 16) AND $FFFF : al.l=ra AND $FFFF
  bh.l=(rb LSR 16) AND $FFFF : bl.l=rb AND $FFFF
  hh.l=ah*bh
  p.l=(((al*bl) LSR 16) AND $FFFF)+ah*bl+al*bh+(hh LSL 16)+((hh LSR 16) AND $FFFF)
  If n Then p=-p
  r.q=0 : Poke.l &r,p
  Function Return r
End Function

NPrint OldMul{196608,30000}   ; 1603272705, as with the original
```

### Graphics

* `DecodePalette` and `LoadPalette` cycled a colour range that the
  picture marks as disabled, and ran ranges backwards without their
  reverse flag; a range cycles only when enabled, backwards only when
  marked so
  ([#1002](https://github.com/fubar-coder/bb2-c/issues/1002)).
* `NextFrame` of an anim with a single change read its next change from
  address 0, and so did the first `NextFrame` of an anim without any; such
  an anim plays once and stays on its last frame
  ([#401](https://github.com/fubar-coder/bb2-c/issues/401)).

### Blitz mode displays

These are the slices and their copper list, the copper lists of the
display library and the fades. Two more fixes of them change what an
existing program shows:
[`CustomSprites` with an offset](#customsprites-ccoffset) and
[`CustomColors` with a `thru256flag` of 0](#customcolors-thru256flag).

* `CustomCop` with an empty string, or one of 1 to 3 bytes, copied about
  four thousand million instructions from the string over all memory
  behind the new copper list; such a string inserts only the wait for its
  line
  ([#404](https://github.com/fubar-coder/bb2-c/issues/404)).
* `CustomCop` in a program whose Slice maximum in the compiler options is
  0 changed memory in 64-byte steps over 4 MB from the place of the slice
  objects on; it changes nothing there
  ([#406](https://github.com/fubar-coder/bb2-c/issues/406)).
* `CustomCop` took the copper instruction `$01FE1234` in its string for
  the mark `Slice` uses inside the copper list for a wait at line 256,
  and changed it and the instruction after it; the string goes into the
  copper list as given
  ([#405](https://github.com/fubar-coder/bb2-c/issues/405)).
* `Show`, `ShowF` and `ShowB` on a slice made without the BPLCON1
  instruction (bit 14 of its flags) wrote to a random address; they
  change nothing outside the slice
  ([#407](https://github.com/fubar-coder/bb2-c/issues/407)).
* The sprite channels of a copper list that show no sprite, also those of
  `CustomSprites`, had no sprite data address, so the hardware fetched
  their sprites from chip memory address 0 and showed garbage where that
  memory was not 0; they show an empty sprite
  ([#413](https://github.com/fubar-coder/bb2-c/issues/413)).
* `DisplayBitMap CopList#,BitMap#` without a position set the start
  position of `DisplayScroll` to values the program had left over from
  earlier calculations; it starts at 0,0
  ([#411](https://github.com/fubar-coder/bb2-c/issues/411)).
* `CustomChunky` with a split position reset the sprite resolution of
  the copper list, so that `DisplaySprite` then put a high or super high
  resolution sprite with an x handle too far to the left; such sprites
  keep their place
  ([#949](https://github.com/fubar-coder/bb2-c/issues/949)).
* `DisplaySprite` of a sprite whose channels run from the last channels
  of the copper list into those of `CustomSprites` wrote over other
  instructions of the list; it is the runtime error "Not Enough Sprite
  Channels", also without runtime error checking
  ([#414](https://github.com/fubar-coder/bb2-c/issues/414);
  [`DisplaySprite` across the two sprite banks]({{< relref "/docs/compared-with-original/new-errors#displaysprite-two-banks" >}})).
* `FadeIn` and `FadeOut` of palette 2 or a higher one made the red or the
  green of the first colour run ahead of the others in the steps of the
  fade; all colours fade evenly
  ([#134](https://github.com/fubar-coder/bb2-c/issues/134)).
* With runtime error checking, `FadeIn` and `FadeOut` took a palette whose
  colours lie in the lowest 64 KB of memory as uninitialised, and gave
  two errors the wrong message: a palette that was never made was "Not
  enough colours in Slice", a colour beyond the colours of the slice
  "Uninitialized Slice". The palette is accepted, and the messages are
  "Uninitialized Palette" and "Not enough colours in Slice"
  ([#135](https://github.com/fubar-coder/bb2-c/issues/135),
  [#330](https://github.com/fubar-coder/bb2-c/issues/330),
  [#331](https://github.com/fubar-coder/bb2-c/issues/331);
  [The checks of `FadeIn` and `FadeOut`]({{< relref "/docs/compared-with-original/new-errors#fade-checks" >}})).

### Shapes and sprites

These are the commands of shapes, their changes such as `Scale` and
`Rotate`, sprites, stencils and the collisions of sprites.

* `LoadShapes`, `DecodeShapes` and `LoadSprites` with a raised object
  maximum (1025 shapes or 2049 sprites and more) loaded only the first
  object; they load the whole range
  ([#999](https://github.com/fubar-coder/bb2-c/issues/999)).
* Big shapes went wrong in the shape library. `LoadShape` of a picture
  with planes of 32 KB or more wrote the planes from the second one on in
  front of the shape's memory; they are in place. `SaveShapes`,
  `LoadShapes`, `DecodeShapes` and `CopyShape` handled only part of a
  shape of 64 KB or more, so a file the original saved from such a shape
  holds only part of it; the whole shape is saved, loaded and copied. A
  single plane of 64 KB or more does not fit the shapes format, which
  keeps the size of a plane in 16 bits: `LoadShape` of such a picture
  wrote it over the memory behind a far too small shape; such a picture
  is the runtime error "Can't Read/Write Shapes File", also without
  runtime error checking
  ([#263](https://github.com/fubar-coder/bb2-c/issues/263);
  [LoadShape of a picture with very large planes]({{< relref "/docs/compared-with-original/new-errors#loadshape-large-plane" >}})).
* `LoadShape` of an IFF ANIM put the frames that come after the last
  shape object into the memory behind the table of shapes; it stops at the
  last shape object
  ([#265](https://github.com/fubar-coder/bb2-c/issues/265)).
* `LoadShape` of an IFF ANIM took the XOR mode of the first frame for
  every frame, so an animation that mixes XOR frames and others came out
  wrong; each frame has its own mode
  ([#266](https://github.com/fubar-coder/bb2-c/issues/266)).
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
* `Rotate` made a rotated shape one pixel too large and lost its edge
  pixels, so a quarter turn of a 21×9 shape gave 10×21; it rounds to the
  nearest pixel and gives 9×21
  ([#1004](https://github.com/fubar-coder/bb2-c/issues/1004)).
* `XFlip`, `YFlip`, `Rotate` and `Scale` went wrong on shapes with planes
  of 32 KB or more, or with more than 65535 rows in all planes together:
  `YFlip` flipped nothing, and `XFlip` of a 16×8200 shape of 8 planes
  flipped only 64 rows; they work on the whole shape
  ([#274](https://github.com/fubar-coder/bb2-c/issues/274)).
* `XFlip` of a shape whose width is not a multiple of 16 lets the blitter
  shift the rows and then mirrors them with the processor, which started
  before the blitter had finished, so the result depended on the timing;
  the processor waits for the blitter
  ([#271](https://github.com/fubar-coder/bb2-c/issues/271)).
* `Scale` to a width or height of less than one pixel made that one pixel
  from more pixels than the shape has, from the next rows and the memory
  behind the shape, and could give it the wrong colour; it is made from
  the pixels of the shape only
  ([#273](https://github.com/fubar-coder/bb2-c/issues/273)).
* `GetaSprite` with runtime error checking had the width limits of 4 and
  16 colour shapes the wrong way round: in `SpriteMode 0` it refused a 4
  colour shape of 65 to 128 pixels, which the 8 sprite channels can show,
  and accepted a 16 colour one, which then failed at every `ShowSprite`
  with "Sprite Channel Unavailable". A 4 colour shape may be 8 sprites
  wide (128 pixels in `SpriteMode 0`), a 16 colour shape 4 (64 pixels);
  a wider one is the error "Sprite too Wide"
  ([#988](https://github.com/fubar-coder/bb2-c/issues/988)).
* `GetaSprite` in `SpriteMode 1` or `2` of a shape whose width is not a
  multiple of the sprite width took bytes of the next row of the shape
  into the right edge of its last sprite; that part of the sprite is
  empty
  ([#282](https://github.com/fubar-coder/bb2-c/issues/282)).
* `SpriteMode` with a mode above 2 took the sprite width from the memory
  behind its table of widths; with runtime error checking it is the error
  "Illegal SpriteMode"
  ([#281](https://github.com/fubar-coder/bb2-c/issues/281)).
* `SaveSprites`, `LoadSprites` and the freeing of a sprite took the size
  of its image from the current `SpriteMode`, so after a change of the
  mode `SaveSprites` wrote too many or too few bytes and `LoadSprites`
  read them so; a sprite keeps the size of the mode it was made in
  ([#280](https://github.com/fubar-coder/bb2-c/issues/280)).
* `ShowSprite` of a sprite cut off at the top of the slice showed the
  wrong lines in `SpriteMode 1` and `2`, and with some values of `Y` read
  the image from anywhere in memory; it skips the lines that are cut off
  ([#283](https://github.com/fubar-coder/bb2-c/issues/283)).
* `SColl` with channel 0 or 1 as the second channel, such as
  `SColl(4,0)`, tested the collision of other channels; the order of the
  channels does not matter. Two channels of the same pair, such as
  `SColl(0,1)`, have no collision of their own: with runtime error
  checking they are the error "Sprite channels must be of different
  pairs"
  ([#110](https://github.com/fubar-coder/bb2-c/issues/110)).
* `Stencil` of a bitmap of more than 1024 lines, or wider than 1024
  pixels, filled the stencil only in part; it fills the whole stencil
  ([#284](https://github.com/fubar-coder/bb2-c/issues/284)).

### Drawing and polygons

These are the drawing commands on bitmaps and the polygon commands for 3D
graphics. Two more fixes of them change what an existing program draws:
[`RenderToBitmap` with a dither bitmap](#rendertobitmap-dither) and
[`Box` with `BitMapOrigin`](#box-origin).

* `Circle` and `Circlef` wrote the points of a circle that reaches above
  the top edge of the bitmap far behind its planes or into the memory
  behind the fill buffer; such points are clipped, like those below the
  bottom edge
  ([#392](https://github.com/fubar-coder/bb2-c/issues/392)).
* `AllocFill` as a statement took the size of the fill buffer from two
  words of the string buffer instead of the used bitmap, so it made a
  buffer of the wrong size or none; it makes the buffer for the used bitmap
  ([#390](https://github.com/fubar-coder/bb2-c/issues/390)).
* `Poly` with fewer than 2 points drew 65535 lines from the memory behind
  the coordinates; with runtime error checking it is the error "Poly
  needs at least 2 Points"
  ([#396](https://github.com/fubar-coder/bb2-c/issues/396)).
* `Polyf`, `BitMapPoly`, `ChunkyPoly` and `RenderPoly` drew a polygon of
  up to 16 points wrongly when one of its sides had 11 to 15 edges; it is
  drawn right. More than 16 points overwrote the code of the library and
  crashed the program; with runtime error checking they are the error
  "Too Many Points"
  ([#395](https://github.com/fubar-coder/bb2-c/issues/395)).
* `Polyf` drew polygons whose corners lie far outside the bitmap (from
  about 8000 pixels away) wrongly or not at all; they are drawn
  ([#1000](https://github.com/fubar-coder/bb2-c/issues/1000)).
* `Polyf`, `BitMapPoly`, `ChunkyPoly` and `RenderPoly` stopped with a
  "Division by Zero" guru for some degenerate polygons with a side that
  starts outside the bitmap on a vertical edge; such a polygon is not drawn
  ([#1033](https://github.com/fubar-coder/bb2-c/issues/1033)).
* `FloodFill` with a border colour filled wrongly on a bitmap wider than
  1024 pixels or taller than 1024 lines; it fills these bitmaps as it
  fills smaller ones
  ([#986](https://github.com/fubar-coder/bb2-c/issues/986)).
* `ChunkyVlin` with `CLIP` drew a line at an x at or beyond the width of
  the chunky area into other rows or behind the area; such a line is
  clipped away, as `ChunkyPlot` with `CLIP` does
  ([#987](https://github.com/fubar-coder/bb2-c/issues/987)).
* `ChunkyVlin` without `CLIP` and a `y1` greater than `y2` wrote about
  65536 words, one per chunky line; with runtime error checking it is the
  error "ChunkyVlin y1 MUST NOT be greater than y2"
  ([#417](https://github.com/fubar-coder/bb2-c/issues/417)).
* `RenderPoly` before any `RenderToBitmap` or `RenderToChunky` drew the
  polygon into memory it took for a bitmap; it draws nothing
  ([#420](https://github.com/fubar-coder/bb2-c/issues/420)).
* `RenderPoly` after `RenderToChunky` took a colour above 14 from the
  memory behind its table of 15 colours; with runtime error checking it
  is the error "Illegal Colour"
  ([#422](https://github.com/fubar-coder/bb2-c/issues/422)).
* The dither form of `BitMapPoly` and `RenderPoly` took the planes from
  the second one on from behind the dither bitmap when its planes were
  smaller than those of the bitmap drawn on; every plane comes from the
  dither bitmap
  ([#1035](https://github.com/fubar-coder/bb2-c/issues/1035)).

### Bitmaps

* `BitMap` with a depth of 0 or more than 8 stopped, with runtime error
  checking, with the error text "No currently used object!", or an empty
  text in an executable; the text is "Illegal BitMap Depth", as for
  `CludgeBitMap`
  ([#1040](https://github.com/fubar-coder/bb2-c/issues/1040)).
* A `BitMap` or `CludgeBitMap` of width or height 0 was accepted, and
  `CopyBitMap` of it copied about four thousand million words through the
  memory; with runtime error checking a width or height below 1 is the
  error "Illegal BitMap Size"
  ([#78](https://github.com/fubar-coder/bb2-c/issues/78)).
* A bitmap whose height times depth is 65536 or more, such as
  `BitMap 0,16,8192,8`, got too little memory, and its planes pointed
  behind it into memory of others; it gets all of its memory
  ([#76](https://github.com/fubar-coder/bb2-c/issues/76)).
* `ShapesBitMap` of a shape with planes of 32 KB or more put each further
  plane of the bitmap in front of the previous one, so drawing on it wrote
  in front of the shape; the planes follow each other as in the shape
  ([#79](https://github.com/fubar-coder/bb2-c/issues/79)).
* `BitMap` set only the plane pointers up to its depth and left the
  others of the object as they were, pointing into the freed memory of an
  earlier bitmap of more planes; they are cleared
  ([#80](https://github.com/fubar-coder/bb2-c/issues/80)).
* `SaveBitmap` of a bitmap whose flags are set (not one made with
  `BitMap`, which sets none) read the depth together with the flags and
  wrote hundreds of planes per line, from memory behind the bitmap, into
  an invalid file; it writes the planes the bitmap has
  ([#180](https://github.com/fubar-coder/bb2-c/issues/180)).
* `LoadBitMap` and `LoadScreen` of a picture with more planes than a
  bitmap whose flags are set wrote the extra planes through the memory
  behind the bitmap; they load only the planes the bitmap has
  ([#188](https://github.com/fubar-coder/bb2-c/issues/188)).
* `CludgeBitMap` with a depth of 0 wrote 65536 plane pointers behind the
  bitmap object, and `InitShape` made a shape of depth 0. The drawing and
  blitting commands, such as `Plot`, `Polyf`, `BBlit` and `QBlit`, then
  ran up to 65536 times over memory behind the object. With runtime error
  checking, `CludgeBitMap` refuses a depth of 0 or more than 8 with
  "Illegal BitMap Depth", as `BitMap` does, and `InitShape` a depth of 0
  with "Illegal Shape Depth"
  ([#393](https://github.com/fubar-coder/bb2-c/issues/393),
  [#74](https://github.com/fubar-coder/bb2-c/issues/74),
  [#243](https://github.com/fubar-coder/bb2-c/issues/243)).

### Sounds

* `DecodeSound` uses sound data that lies in chip memory where it is, and
  marks the sound so that this memory is not freed. The mark stayed when
  the next sample was put into the same sound object, by `DecodeSound` of
  other memory, `LoadSound` or `InitSound`, so that sample was never freed
  before the program ended; it is freed when the object is reused or
  freed ([#68](https://github.com/fubar-coder/bb2-c/issues/68)).

### Tasks and message ports

* `TaskName$` of a task without a name copied the text at address 0; it
  is the empty string
  ([#297](https://github.com/fubar-coder/bb2-c/issues/297)).
* `CreateMsgPort` did not notice when the program had no free signal
  left and made a port whose messages never wake the program; this is
  the runtime error "Failed to allocate signal for Port"
  ([#298](https://github.com/fubar-coder/bb2-c/issues/298)).
* `DeleteMsgPort` freed a port with the messages still waiting at it, so
  their senders never got them back; they are replied first
  ([#299](https://github.com/fubar-coder/bb2-c/issues/299)).
* `DeleteMsgPort` took any address as a port, so a port that
  `CreateMsgPort` had not made was freed as if it had, which corrupts
  memory, and runtime error checking reported only a memory error after
  the damage; with runtime error checking such a port is the error "Port
  Not Created By CreateMsgPort", before anything is changed
  ([#1041](https://github.com/fubar-coder/bb2-c/issues/1041)).

### Program start

* A program started from the CLI with an argument line longer than 255
  characters copied the whole line into a buffer of 256 bytes and
  overwrote the memory behind it; the program gets the first 255
  characters
  ([#308](https://github.com/fubar-coder/bb2-c/issues/308)).
* A program started from the Workbench makes its drawer the current
  directory, and did not make the old current directory current again
  at its end; it does
  ([#307](https://github.com/fubar-coder/bb2-c/issues/307)).
* A program started from the Workbench opened `dos.library` and never
  closed it, so every start raised its use count for good; it is closed
  at the end
  ([#306](https://github.com/fubar-coder/bb2-c/issues/306)).

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
* A program that uses a transcendental function, such as `Sin`, `Sqr` or
  `^` on floats, hung at its start when `mathtrans.library` could not be
  opened, like the [missing AmigaOS libraries](#missing-os-library)
  above; the program starts, and only a call of such a function crashes
  ([#202](https://github.com/fubar-coder/bb2-c/issues/202)).

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
* Without runtime error checking, `StartTracker` of an empty slot
  returned the slot number, a true value, although nothing plays; it
  returns 0 (False)
  ([#451](https://github.com/fubar-coder/bb2-c/issues/451)).
* Without runtime error checking, `GetModuleName$` of an empty slot gave
  a string of ten undefined characters per slot number; it is the empty
  string
  ([#437](https://github.com/fubar-coder/bb2-c/issues/437)).
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
