---
title: windowslib
description: The bugs of Blitz Basic 2 in the command library windowslib that
  the BB2 C port fixes.
sidebar:
  order: 63
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/windowslib/
---

::fixed-bugs-library

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

<details>
<summary>The test program</summary>

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

</details>

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

#### `WSize` beyond the size limits of a window {#wsize-limits}

**What happened.** Runtime error checking of the original tested only
the edge of the screen, not the size limits of the window.

**With the port.** With runtime error checking, a size outside the limits
of a window with a sizing gadget, as in the program above, is the error
"Can't Size Window", and the program stops at `WSize`.

**What to change.** Size the window within the limits given with
`SizeLimits` before it was opened.

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

<details>
<summary>The test program</summary>

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

</details>

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

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Windows and screens

* `WindowTags` with a long tag list overwrote code of the library; a long
  list is copied only in part and the rest is linked
  ([#317](https://github.com/fubar-coder/bb2-c/issues/317)).
* `Menus`, `MenusOn` and `MenusOff` reached only the windows in the
  lower half of the window numbers, so `MenusOff` left the menus of the
  others on; they reach every window
  ([#310](https://github.com/fubar-coder/bb2-c/issues/310)).
* `WindowTags` without tags opened the window with the extra tags of the
  previous `WindowTags`, such as its position and size; it uses none
  ([#316](https://github.com/fubar-coder/bb2-c/issues/316)).
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

#### `WindowTags` with too many tag pairs {#windowtags-too-many-tags}

**What happened.** `WindowTags` copies the tag and data pairs given as
arguments into a buffer of the library without a limit. With 16 pairs or
more it wrote behind the buffer, over data and code of the library:

```
DEFTYPE .l
WbToScreen 0
t=$80000064
WindowTags 0,$000E,"Tags",t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10
DefaultOutput
NPrint "opened"
CloseWindow 0
End
```

The 16 pairs all set the left edge of the window. Compiled with runtime
error checking, the original printed `opened`.

**With the port.** With runtime error checking, more than 15 pairs are
the error "Too Many Tags"; the program above stops at `WindowTags`.

**What to change.** Give a long list of tags as the address of a tag
list, `WindowTags Window#,Flags,Title$,&TagList`: that form takes any
number of tags.

Issue: [WindowTags with tag pairs overflowed its tag buffer (#961)](https://github.com/fubar-coder/bb2-c/issues/961).

::fixed-bugs-library{part="elsewhere"}
