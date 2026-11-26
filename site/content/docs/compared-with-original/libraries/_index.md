---
title: Libraries
weight: 5
description: Fixed bugs of Blitz Basic 2 in the command libraries, in AmigaOS calls and in the RWE userlibs, one page per library, with examples of the ones ordinary programs meet.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/, /docs/compared-with-original/new-errors/runtime-errors/]
# the entries of the old page new-errors/runtime-errors, and those that
# moved from this page to the page of their library, by anchor: the page
# at an old URL (layouts/alias.html) forwards a link to its entry
anchorMoves:
  frac-string: libraries/commands/vallib
  long-division-by-zero: libraries/commands/ldivlib
  windowtags-too-many-tags: libraries/commands/windowslib
  screentags-too-many-tags: libraries/commands/screenslib
  stringgadget-maxlen: libraries/commands/gadgetslib
  inkey-negative: libraries/commands/editlib
  vwait-zero: libraries/commands/intlib
  peeks-negative: libraries/commands/memacclib
  cvi-short-string: libraries/commands/stringfunclib
  instr-startpos: libraries/commands/stringfunclib
  dim-too-large: libraries/commands/arrayslib
  dim-65535: libraries/commands/arrayslib
  wsize-limits: libraries/commands/windowslib
  depth-zero: libraries/commands/bitmaplib
  bitmap-depth-text: libraries/commands/bitmaplib
  bitmap-size: libraries/commands/bitmaplib
  poly-points: libraries/commands/2dlib
  too-many-points: libraries/commands/2dlib
  chunkyvlin-ends: libraries/commands/polylib
  renderpoly-colour: libraries/commands/polylib
  loadshape-large-plane: libraries/commands/shapeslib
  loadshape-blit-size: libraries/commands/shapeslib
  loadshape-no-planes: libraries/commands/shapeslib
  loadshape-size-zero: libraries/commands/shapeslib
  shape-too-big: libraries/commands/shapeslib
  createshape-count: libraries/userlibs/bb_blitlib
  bb-blitlib-shape-undefined: libraries/userlibs/bb_blitlib
  blocktoshape-too-large: libraries/userlibs/bb_blitlib
  save-palette-no-colours: libraries/commands/iffmakelib
  saveshape-depth: libraries/commands/iffmakelib
  loadbitmap-missing-file: libraries/commands/bitmaplib
  getasprite-width: libraries/commands/spriteslib
  spritemode-range: libraries/commands/spriteslib
  scoll-same-pair: libraries/commands/collslib
  displaysprite-two-banks: libraries/commands/displaylib
  fade-checks: libraries/commands/fadelib
  setperiod-check: libraries/commands/audiolib
  jumpmed-no-module: libraries/commands/medlib
  freetrackermodule-playing: libraries/userlibs/ciatrklib
  pop-without-push: libraries/commands/inputoutputlib
  createmsgport-signal: libraries/commands/utillib
  deletemsgport-foreign: libraries/commands/utillib
  rexx-numbers: libraries/commands/arexxlib
  spotbox-page: libraries/commands/blitzoslib
  selectbox-range: libraries/commands/blitzoslib
  initpage-numboxes: libraries/commands/blitzoslib
  ezrequest-too-many-arguments: libraries/userlibs/rimisclib
  notify-slot-range: libraries/userlibs/ridoslib
  linked-list-number-range: libraries/userlibs/rilinkedlistlib
  anim-position-outside: libraries/userlibs/rianimlib
  anim7-odd-byte: libraries/userlibs/rianimlib
  bbshadowvalue-set: libraries/userlibs/ribbmaplib
  fns-clip-tab-checks: libraries/userlibs/rifnslib
  rigfxlib-palette-checks: libraries/userlibs/rigfxlib
  rwe-effects-depth: libraries/userlibs/rifxlib
  rifxlib-delay-zero: libraries/userlibs/rifxlib
  zoomyzoom-size: libraries/userlibs/rifxlib
  zoomxy-without-initzoomxy: libraries/userlibs/rifxlib
  colsplit-numlines: libraries/userlibs/ricopperfxlib
  rwe-effect-results: libraries/userlibs/ricopperfxlib
  chunkytoplanar-sizes: libraries/userlibs/richunkylib
  raycastsky-outside: libraries/userlibs/richunkylib
  rimapeditorlib-map-checks: libraries/userlibs/rimapeditorlib
  rimapeditorlib-size-checks: libraries/userlibs/rimapeditorlib
  createmap-width: libraries/userlibs/rimapeditorlib
  snow-list-checks: libraries/userlibs/risnowlib
  shapetomapshape-depth: libraries/userlibs/rimapshapeslib
  group-63-members: libraries/userlibs/rimapshapeslib
  mapshape-number-checks: libraries/userlibs/rimapshapeslib
  wbsprite-checks: libraries/userlibs/riwbspritelib
  compactdisc-checks: libraries/userlibs/ricompactdisclib
  font-printer-bitmap: libraries/userlibs/rilesdebuglib
  addprocessptr-20-codes: libraries/userlibs/rilesdebuglib
---

These bugs are in the libraries that provide the commands of Blitz Basic
2 ([Libraries]({{< relref "/docs/libraries" >}})). A fix in a library
changes only that library, and a program behaves differently only where
it ran into the bug. Each library has a page of its own with its fixes,
listed below. On a library's page, the fixes an ordinary program can run
into come first, each with a short program; the other fixes follow as
lists, one sentence each with a link to its issue. Every fix is marked
with its level, which says what it can change in a program that ran
correctly with the original. The table on
[Important Fixes & Breaking Changes]({{< relref "/docs/compared-with-original/important-fixes" >}})
lists all fixed bugs by these levels.

The outputs shown were recorded in an emulator: the original is the
program compiled with the libraries of the Blitz Basic 2 CD, the port the
same program compiled with the port's libraries. The programs run without
runtime error checking unless the text says otherwise.

Some fixes make a command stop with a runtime error where the original
went on with a wrong argument, wrote over memory or hung (level 3, a new
runtime error). Such a fix follows the list of its group on the
library's page, with a program, what the original did, what the port
does and what to change in a source. Most of these are error checks:
like the other checks of Blitz Basic 2, they come only in a program
compiled with runtime error checking, and without it the program runs as
with the original. An error is shown with the line number the compiler
reported, counted in the program as it is shown.

## Fixes by library

The tables give the number of fixes on each library's page, by level,
and the library's page in the command reference. A fix that concerns
several libraries is on the page of the first of them, and the pages of
the others link to it; a fault shared by many libraries is described
once, in [Fixes in several libraries](#fixes-in-several-libraries).

### Command libraries

{{< fixed-bugs-libraries group="commands" >}}

### RWE userlibs

{{< fixed-bugs-libraries group="userlibs" >}}

### AmigaOS calls

The fixes in the calls of AmigaOS functions are on one page,
[AmigaOS calls]({{< relref "/docs/compared-with-original/libraries/amigaos" >}}).

## Changes you may notice {#noticeable-changes}

With the fixes in this part, a program that ran correctly with the
original still does its job, but something differs that you may notice
and may want to adapt, such as its sound or the events on a tape. The
fixes that change the result of a program follow in
[Changes for existing programs](#changes-for-existing-programs).

In the command libraries:

* [`CustomColors`]({{< relref "/docs/compared-with-original/libraries/commands/displaylib#customcolors-thru256flag" >}}) with a `thru256flag` of 0
  makes the next colour change below line 255 of the display at its own
  line.
* [`Filter On` and `Filter Off`]({{< relref "/docs/compared-with-original/libraries/commands/audiolib#filter-on-off" >}}) switch the audio filter
  the other way round.
* [A tracker module]({{< relref "/docs/compared-with-original/libraries/commands/trackerlib#tracker-break-song-end" >}}) with a pattern break in its
  last song position loops at its song length.
* A MED module with more than 8 tracks no longer runs the commands of
  tracks 8 to 11, as a MED player does
  ([Music players]({{< relref "/docs/compared-with-original/libraries/commands/medlib#music-players" >}})).
* [Four smaller changes](#more-tape-and-keyboard-changes) concern tapes
  of mouse and keyboard events and the keyboard in Blitz mode.
* [Ctrl-C]({{< relref "/docs/compared-with-original/libraries/commands/rundebuglib#ctrl-c-runtime-errors" >}}) ends an executable compiled with
  runtime error checking at its next `Print`, `NPrint`, `Edit$` or `Edit`
  on the console.

In the RWE userlibs:

* The file requester of [`ReqFileRequest`]({{< relref "/docs/compared-with-original/libraries/userlibs/rireqlib#reqfilerequest-default-flags" >}})
  opens with the default flags its library means.
* [`LoadIFF`]({{< relref "/docs/compared-with-original/libraries/userlibs/ripacklib#loadiff-bank-result" >}}) works without `BankLimit` and returns
  0 for a picture it could not unpack.
* [`UnpackIFF`]({{< relref "/docs/compared-with-original/libraries/userlibs/ripacklib#unpackiff-row-width" >}}) reads pictures whose width is no
  multiple of 16 pixels.
* `RXDeletePort` and the end of a program wait up to 5 seconds for the
  replies to ARexx commands sent without waiting
  ([Fixes in riarexxlib]({{< relref "/docs/compared-with-original/libraries/userlibs/riarexxlib#riarexxlib-fixes" >}})).
* [`FNSLength`]({{< relref "/docs/compared-with-original/libraries/userlibs/rifnslib#fnslength-width" >}}) returns the whole width of a text, and
  [the FNS fonts]({{< relref "/docs/compared-with-original/libraries/userlibs/rifnslib#fns-missing-glyphs" >}}) print a character they have no
  glyph for as a space instead of ending the text there.
* In the zones of rizonejoylib,
  [`NewZoneTable`]({{< relref "/docs/compared-with-original/libraries/userlibs/rizonejoylib#newzonetable-current-table" >}}) leaves the zones of the
  current table alone, [circular zones]({{< relref "/docs/compared-with-original/libraries/userlibs/rizonejoylib#zone-circles" >}}) on a 68020 hit
  only the points inside them, and `SetZone` refuses a
  [radius above 255]({{< relref "/docs/compared-with-original/libraries/userlibs/rizonejoylib#zone-circles" >}}) and a
  [rectangle whose bottom edge is -1]({{< relref "/docs/compared-with-original/libraries/userlibs/rizonejoylib#zone-negative-y" >}}).
* The Workbench sprites of riwbspritelib give
  [clear results]({{< relref "/docs/compared-with-original/libraries/userlibs/riwbspritelib#wbsprite-results" >}}): `GetWBSprite` returns -1 whenever
  it gets no sprite, and `MoveWBSprite` returns -1 when it moved one.
* [`DiskMotor`]({{< relref "/docs/compared-with-original/libraries/userlibs/ritrackdisklib#diskmotor-state" >}}) of ritrackdisklib reports whether the
  library has switched the motor of a drive on, instead of -1 at all
  times.
* In the CD library ricompactdisclib, `CDTrackLength 1` no longer counts
  the gap before the first track, `CDVolume` no longer swaps the volume
  and the fade time, and `OpenCD` no longer keeps the SCSI flag, device
  and unit of an earlier `OpenCD`
  ([Track lengths, volume and the SCSI flag]({{< relref "/docs/compared-with-original/libraries/userlibs/ricompactdisclib#compactdisc-results" >}})).
* [`InitSerialIO` and `GetSerial address,bytes`]({{< relref "/docs/compared-with-original/libraries/userlibs/rifastseriallib#fastserial-results" >}}) of
  rifastseriallib return -1 or 0 instead of their arguments.
* [The block commands of riseriallib]({{< relref "/docs/compared-with-original/libraries/userlibs/riseriallib#serial-block-commands" >}}) open the
  serial port when it is closed, and `SerialReceiveBlock` keeps the bytes
  that arrived before it.
* In the debugger helpers of rilesdebuglib,
  [`ExtractLabel`, `GetCustomRegAdr` and `FInverse`]({{< relref "/docs/compared-with-original/libraries/userlibs/rilesdebuglib#rilesdebuglib-changes" >}})
  end a label at a space, find a register only by its whole name, and
  invert the text of `FPrintDetoke` too.
* [`DisAssemble$`]({{< relref "/docs/compared-with-original/libraries/userlibs/ridisasmlib#disassemble-line-length" >}}) of ridisasmlib returns a
  long instruction whole instead of cutting it after 34 columns.

Five more are in the lists of fixes of the RWE userlibs:

* `FNSLoad` works without `BankLimit`
  ([Fixes in rifnslib]({{< relref "/docs/compared-with-original/libraries/userlibs/rifnslib#rifnslib-fixes" >}})).
* `ClearBitmap` clears the last word of its area
  ([Fixes in rifxlib]({{< relref "/docs/compared-with-original/libraries/userlibs/rifxlib#rifxlib-fixes" >}})).
* A colour split from line 0 starts at line 0
  ([Fixes in ricopperfxlib]({{< relref "/docs/compared-with-original/libraries/userlibs/ricopperfxlib#ricopperfxlib-fixes" >}})).
* A Workbench sprite made of a shape whose width is no multiple of 16
  shows the shape's left 16 pixels unsheared
  ([Fixes in riwbspritelib]({{< relref "/docs/compared-with-original/libraries/userlibs/riwbspritelib#riwbspritelib-fixes" >}})).
* `PauseModule` of ciatrklib pauses only a module that plays, and
  `StartTracker`, `StopTracker`, `ChangeTrackerVBlank` and freeing the
  module end a pause: two `PauseModule` no longer continue a module that
  `StopTracker` or the effect `F00` stopped
  ([More fixes]({{< relref "/docs/compared-with-original/libraries/userlibs/ciatrklib#more-fixes" >}})).

### More changes of tapes and the keyboard {#more-tape-and-keyboard-changes}

These four fixes change what a program sees in a smaller way. Each is
described in a list of fixes on its library's page.

* `XStatus` gives only 0, 1 or 2; the original added 32768 right after
  `Record`, `PlayBack` or a simulated event. A program that compared the
  value with `And 255` sees no difference
  ([Tapes and simulated input]({{< relref "/docs/compared-with-original/libraries/commands/brexxlib#tapes" >}})).
* `ClickButton` releases the button it pressed, also after an executed
  `ClickButton 1`, where the original released the left button
  ([Tapes and simulated input]({{< relref "/docs/compared-with-original/libraries/commands/brexxlib#tapes" >}})).
* `RelMouse` makes a relative mouse event, so a tape records it as
  such; the pointer moves as before ([Tapes and simulated input]({{< relref "/docs/compared-with-original/libraries/commands/brexxlib#tapes" >}})).
* `BlitzKeys Off` gives the keyboard input back to the input used before
  `BlitzKeys On` ([Input]({{< relref "/docs/compared-with-original/libraries/commands/blitzkeyslib#input" >}})).

## Changes for existing programs

Unlike the changes above, the fixes in this part change the result of a
program that ran correctly with the original: what it computes, draws or
sends, or which way it takes. Check your sources for the commands they
name.

In the command libraries:

* [`RenderToBitmap`]({{< relref "/docs/compared-with-original/libraries/commands/polylib#rendertobitmap-dither" >}}) with a dither bitmap and
  [`Box`]({{< relref "/docs/compared-with-original/libraries/commands/2dlib#box-origin" >}}) draw on the right bitmap at the right place. Only a
  program written to make up for the bug changes; no known example
  program, demo or game does that.
* [`CustomSprites`]({{< relref "/docs/compared-with-original/libraries/commands/displaylib#customsprites-ccoffset" >}}) takes its offset in the
  documented unit: every program that gives it an offset other than 0
  changes.
* [`LoadShape`]({{< relref "/docs/compared-with-original/libraries/commands/shapeslib#loadshape-blit-size" >}}) stops with a runtime error on
  pictures too large to blit, which it loaded before.
* [`ReplyRexxMsg`]({{< relref "/docs/compared-with-original/libraries/commands/arexxlib#replyrexxmsg-result" >}}) gives the result string to other
  ARexx replies than before.
* [`WriteSerialString` and `ReadSerial`]({{< relref "/docs/compared-with-original/libraries/commands/seriallib#serial-flags" >}}) take the flags
  their help text names.
* [`GetMedVolume`]({{< relref "/docs/compared-with-original/libraries/commands/medlib#getmedvolume-clears" >}}) no longer silences the channel it
  reads, so the next read no longer gives 0.
* [`Translate$`]({{< relref "/docs/compared-with-original/libraries/commands/speaklib#translate-length" >}}) returns its phonemes without an extra
  0 byte.
* [Blitz code with runtime error checking]({{< relref "/docs/compared-with-original/libraries/commands/rundebuglib#supervisor-mode-traps" >}}) that
  runs in supervisor mode outside a `SetInt` routine hangs at its first
  trap.
* [A simulated event]({{< relref "/docs/compared-with-original/libraries/commands/brexxlib#simulated-event-ends-tape" >}}) no longer ends a running
  recording or playback of a tape.
* [`^` with a negative base]({{< relref "/docs/compared-with-original/libraries/commands/mathtranslib#power-negative-base" >}}) and a whole odd
  exponent gives a negative result: `(-2)^3` is -8 instead of 8.

In the RWE userlibs:

* [`BlockToShape`]({{< relref "/docs/compared-with-original/libraries/userlibs/bb_blitlib#blocktoshape-width" >}}) copies the whole of a shape wider
  than 16 pixels.
* [`RTEZGetLong`]({{< relref "/docs/compared-with-original/libraries/userlibs/neilsreqtoolslib#rtezgetlong-previous" >}}) without a default value no longer
  offers and returns the number of the call before.
* [`BankLimit n`]({{< relref "/docs/compared-with-original/libraries/userlibs/riamosfunclib#banklimit-result" >}}) and [`FindVolume`]({{< relref "/docs/compared-with-original/libraries/userlibs/riamosfunclib#findvolume-result" >}})
  return the numbers their manual gives instead of an address, and
  [`D_FindVolume`]({{< relref "/docs/compared-with-original/libraries/userlibs/rilesdebuglib#d-findvolume-result" >}}) of rilesdebuglib returns -1 or
  0 as `FindVolume` does.
* [`PutIconObject`]({{< relref "/docs/compared-with-original/libraries/userlibs/ritooltypeslib#puticonobject-result" >}}) reports an icon it could not
  write and keeps it open.
* [`SetToolValue`]({{< relref "/docs/compared-with-original/libraries/userlibs/ritooltypeslib#settoolvalue-prefix" >}}) no longer replaces a tool type
  whose name only begins with the name given.
* [`StoreNVData`]({{< relref "/docs/compared-with-original/libraries/userlibs/rinonvolatilelib#storenvdata-result" >}}) returns True for a stored item
  instead of 0.
* [`RXRunCommand`]({{< relref "/docs/compared-with-original/libraries/userlibs/riarexxlib#rxruncommand-result" >}}) and
  [`AddAppMenu`]({{< relref "/docs/compared-with-original/libraries/userlibs/riapplib#addappmenu-result" >}}) return -1 for success instead of a
  value the system left or an address.
* [`AppEventID`]({{< relref "/docs/compared-with-original/libraries/userlibs/riapplib#appeventid-no-event" >}}) returns -1 instead of 0 when there
  is no App event.
* A second [`DataTypeCreate`]({{< relref "/docs/compared-with-original/libraries/userlibs/ridatatypeslib#datatypecreate-second" >}}) or
  [`MakeCommodity`]({{< relref "/docs/compared-with-original/libraries/userlibs/ricommoditieslib#makecommodity-second" >}}) is refused while the first
  object or commodity exists.
* [`ILBMPalette`]({{< relref "/docs/compared-with-original/libraries/userlibs/ripacklib#ilbmpalette-offset" >}}) with an offset makes a palette that
  is too small larger instead of replacing it.
* [`FillPalette`]({{< relref "/docs/compared-with-original/libraries/userlibs/rigfxlib#fillpalette-end-zero" >}}) with an end colour of 0 fills up
  to the last colour.
* [`Derez`]({{< relref "/docs/compared-with-original/libraries/userlibs/rifxlib#derez-result" >}}) of rifxlib and
  [`DoColSplit` and `CopperAGACol`]({{< relref "/docs/compared-with-original/libraries/userlibs/ricopperfxlib#rwe-effect-results" >}}) of ricopperfxlib,
  used as functions, return -1 or 0.
* [`GetCCOffset`]({{< relref "/docs/compared-with-original/libraries/userlibs/ricopperfxlib#getccoffset-resetadr" >}}) returns its offset after
  `CopperResetAdr`.
* [`OpenDisk`]({{< relref "/docs/compared-with-original/libraries/userlibs/ritrackdisklib#opendisk-device-name" >}}) of the floppy library ritrackdisklib
  without a device name always opens trackdisk.device instead of the
  device of an earlier `OpenDisk`.
* [`SetIOSize n`]({{< relref "/docs/compared-with-original/libraries/userlibs/rifastseriallib#setiosize-count" >}}) of the serial link library
  rifastseriallib makes `DoSerialIO` transfer n bytes instead of n + 1.
* [`SerialGot`]({{< relref "/docs/compared-with-original/libraries/userlibs/riseriallib#serialgot-count" >}}) of riseriallib returns the number of
  bytes received instead of -1.

## Fixes in several libraries

These fixes concern the same fault in several libraries, and are
described once for all of them.

### Text after a string function lost its last character {#text-after-string-function}

A string expression such as `ProgDir$+"/data"` is built in a work area,
one part behind the other. A function that returns a string writes its
result there and leaves the place where the next part goes. Several
library functions left that place one byte too far, behind the 0 byte
that ends their result. The joined string had the right length, but it
held a 0 byte between the result and the next part, and the last
character of the next part was missing. A program that adds an
extension to a chosen file name, or a directory to the program's own
one, got a name that no file has. In the port the next part follows the
result.

`ProgDir$` of the RWE userlib rimisclib returns the directory of the
program. This program adds `/data` to it and looks at the text behind
the directory:

```blitzbasic
p$=ProgDir$
r$=ProgDir$+"/data"
NPrint Len(r$)-Len(p$)            ; original: 5      port: 5
NPrint Asc(Mid$(r$,Len(p$)+1))    ; original: 0      port: 47
NPrint Right$(r$,4)               ; original: /dat   port: data
End
```

Both add five characters, but in the original the first of them is a 0
byte instead of the `/` (code 47), and the text ends in `/dat`.

#### The functions with this fault {#rwe-text-after-string-function}

The table names each library with its page of fixes, and the issue with
the functions it concerns, linked to the command reference.

| Library | Issue and functions |
|---|---|
| [freqlib]({{< relref "/docs/compared-with-original/libraries/commands/freqlib" >}}) | [#161](https://github.com/fubar-coder/bb2-c/issues/161) |
| [arexxlib]({{< relref "/docs/compared-with-original/libraries/commands/arexxlib" >}}) | [#52](https://github.com/fubar-coder/bb2-c/issues/52) |
| [rimisclib]({{< relref "/docs/compared-with-original/libraries/userlibs/rimisclib" >}}) | [#743](https://github.com/fubar-coder/bb2-c/issues/743) |
| [ridoslib]({{< relref "/docs/compared-with-original/libraries/userlibs/ridoslib" >}}) | [#618](https://github.com/fubar-coder/bb2-c/issues/618) |
| [riapplib]({{< relref "/docs/compared-with-original/libraries/userlibs/riapplib" >}}) | [#519](https://github.com/fubar-coder/bb2-c/issues/519) |
| [neilsreqtoolslib]({{< relref "/docs/compared-with-original/libraries/userlibs/neilsreqtoolslib" >}}) | [#461](https://github.com/fubar-coder/bb2-c/issues/461) |
| [riregistrylib]({{< relref "/docs/compared-with-original/libraries/userlibs/riregistrylib" >}}) | [#783](https://github.com/fubar-coder/bb2-c/issues/783) |
| [rireqlib]({{< relref "/docs/compared-with-original/libraries/userlibs/rireqlib" >}}) | [#784](https://github.com/fubar-coder/bb2-c/issues/784) |
| [riseriallib]({{< relref "/docs/compared-with-original/libraries/userlibs/riseriallib" >}}) | [#795](https://github.com/fubar-coder/bb2-c/issues/795) |

#### Functions that left the place wrong in other ways {#string-function-wrong-place}

These functions left the place of the next part somewhere else. The
table says what the original did, with the issue and the functions it
concerns; in the port the rest of the expression follows the result.

<div class="fixed-bugs-breakcode">

| Library | What was wrong |
|---|---|
| [stringfunclib]({{< relref "/docs/compared-with-original/libraries/commands/stringfunclib" >}}) | `Replace$` with an empty search string left it behind its arguments: `Replace$("ab","","c")+"x"` gave `ab` and a 0 byte instead of `abx`. [#424](https://github.com/fubar-coder/bb2-c/issues/424) |
| [gtlib]({{< relref "/docs/compared-with-original/libraries/commands/gtlib" >}}) | `GTGetAttrs` and `GTSetAttrs`, used as functions, left it on a tag list inside the library; the rest of the expression overwrote that list and the memory behind it. [#328](https://github.com/fubar-coder/bb2-c/issues/328) |
| [neilsreqtoolslib]({{< relref "/docs/compared-with-original/libraries/userlibs/neilsreqtoolslib" >}}) | After `RTEZSaveFile`, the text that followed overwrote the end of the chosen path, or came behind bytes of its arguments. [#465](https://github.com/fubar-coder/bb2-c/issues/465) |
| [rimisclib]({{< relref "/docs/compared-with-original/libraries/userlibs/rimisclib" >}}) | `EZRequest` and `EZRequestArgs`, used as functions, left it at address 0 or on their argument list, so `a$="Answer "+Str$(EZRequest("T","Body","Yes\|No"))` came out wrong or crashed the program. [#1027](https://github.com/fubar-coder/bb2-c/issues/1027) |
| [riwbspritelib]({{< relref "/docs/compared-with-original/libraries/userlibs/riwbspritelib" >}}) | `ChangeWBSprite`, used as a function, left it on a tag list inside the library; the rest of the expression overwrote that list. [#1028](https://github.com/fubar-coder/bb2-c/issues/1028) |

</div>

`Translate$` of speaklib also counted the 0 byte as part of its result
([`Translate$` added a 0 byte to its result]({{< relref "/docs/compared-with-original/libraries/commands/speaklib#translate-length" >}})).

### Runtime errors without the debugger

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
