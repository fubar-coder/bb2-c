---
title: riapplib
weight: 7
description: The bugs of Blitz Basic 2 in the RWE userlib riapplib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `AddAppMenu` returned an address {#addappmenu-result}

[riapplib]({{< relref "/docs/libraries/userlibs/riapplib" >}}) makes a
program part of Workbench with App windows, App icons and App menu
items. `AddAppMenu id,text$` adds an item to the Tools menu of
Workbench. Like `AddAppWindow` and `AddAppIcon`, it returns -1 when the
item was added and 0 when not.

**What happened.** The original returned the address of the menu item
in the system instead of -1. A program that tested the result as a
truth value worked, but one that compared it with -1, as with the other
two commands, took every added item for a failure:

```
If AddAppMenu(0,"My tool")=-1
  NPrint "added"
Else
  NPrint "not added"
EndIf
```

With the original this printed `not added` although the item was in
the Tools menu; with the port it prints `added`.

**What to change.** A program that tested the result with `<>0` or as a
truth value works as before. One that kept the address to pass it to
workbench.library itself gets -1 now; the library removes the item with
`DelAppMenu` and at the end of the program.

Issue: [AddAppMenu returns the AppMenuItem address instead of -1 (#518)](https://github.com/fubar-coder/bb2-c/issues/518).

### `AppEventID` returned 0 without an App event {#appeventid-no-event}

[riapplib]({{< relref "/docs/libraries/userlibs/riapplib" >}}) numbers
its App windows, App icons and App menu items from 0 to 15. After
`AppEvent` has fetched a message, `AppEventID` gives the id of the
object that sent it. Without a current message, before the first
`AppEvent` or after one that found no message, it returns -1, as the
library's guide says: "-1 means that no AppEvent occurred".

**What happened.** The original returned 0 without a message, which is
also the id of the first object of each kind. A program that read
`AppEventID` without testing the result of `AppEvent` could not tell
"no message" from "a message of object 0". This program has one App
icon with the id 0 and asks `AppEventID` in every frame:

```
If AddAppIcon(0,"Drop here","SYS:System/Format")=0 Then End
Repeat
  VWait
  r=AppEvent
  If AppEventID=0 Then NPrint "icon 0"
Until Joyb(0)
DelAppIcon 0
```

With the original it printed `icon 0` in every frame, whether files
were dropped on the icon or not. With the port it prints `icon 0` only
for a message of the icon.

**What to change.** A program that tests the result of `AppEvent`
before it reads `AppEventID` works as before. One that took 0 for "no
message" tests for -1, or tests `AppEvent` first.

Issue: [AppEventID returns 0 without an event; the manual says -1 (#1102)](https://github.com/fubar-coder/bb2-c/issues/1102).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in riapplib {#riapplib-fixes}

These fixes concern
[riapplib]({{< relref "/docs/libraries/userlibs/riapplib" >}}), the App
windows, App icons and App menu items of Workbench by Reflective
Images. Each of them sends the program a message when the user drops
files on it or chooses it; `AppEvent` fetches the message, and
`NextAppFile`, `AppFile` and `AppLock` tell which files it holds. The
fixes of [the result of `AddAppMenu`](#addappmenu-result), of
[the result of `AppEventID`](#appeventid-no-event), of
[the text that follows `NextAppFile` and `AppFile`]({{< relref "/docs/compared-with-original/fixed-bugs/libraries#rwe-text-after-string-function" >}})
and of [the local variables after `AddAppIcon`](#more-fixes) are
described elsewhere on this page.

* Programs that use the library run on Kickstart 1.x. The App commands
  need Workbench 2.0 and do nothing there, as on any system without the
  Workbench libraries they need. The original called a function of
  Kickstart 2.0 before it checked for them, and every program with the
  library crashed at its start
  ([#1143](https://github.com/fubar-coder/bb2-c/issues/1143)).
* `NextAppFile` and `AppFile` leave Workbench's lock of the file's
  drawer in the message. The original gave the lock back to the system,
  so reading the same file again with `AppFile` gave a wrong path, and
  `AppLock` returned 0
  ([#521](https://github.com/fubar-coder/bb2-c/issues/521)).
* `AppFile` with a number of 0 or less returns an empty string. The
  original made a path of whatever lay in memory in front of the list of
  files ([#520](https://github.com/fubar-coder/bb2-c/issues/520)).
* `AppLock` returns 0 for a message without files, such as the one of
  an App menu item chosen without icons selected. The original returned
  a value from memory that was no lock
  ([#522](https://github.com/fubar-coder/bb2-c/issues/522)).
* `AppLock` returns the lock of the drawer of the file that
  `NextAppFile` or `AppFile` read last, as the guide says, and the first
  file's lock before either has read one. The original always returned
  the first file's lock, so for the other files of a message it gave
  the wrong drawer
  ([#1103](https://github.com/fubar-coder/bb2-c/issues/1103)).
* `DelAppWindow` of a window that is no App window returns 0, as
  `DelAppMenu` and `DelAppIcon` do for an id without an object. The
  original returned -1 and passed an empty App window to the system
  ([#523](https://github.com/fubar-coder/bb2-c/issues/523)).
* An `AddAppIcon` that fails on an id in use, after it has removed the
  old icon, frees the image it loaded for the new one, and nothing of
  the old icon is left. The original kept both, and `DelAppIcon` or the
  end of the program removed the old icon a second time
  ([#517](https://github.com/fubar-coder/bb2-c/issues/517)).
* At the end of the program the library answers the current message and
  every message still waiting, so Workbench gets them back. The original
  answered the waiting ones only if `AppEvent` had fetched a message
  last ([#525](https://github.com/fubar-coder/bb2-c/issues/525)).
* An App window, icon or menu item with an odd id that is still there
  at the end of the program is removed normally. On a 68000 the original
  crashed the machine there
  ([#524](https://github.com/fubar-coder/bb2-c/issues/524)).
* When workbench.library, dos.library or icon.library cannot be opened,
  the library closes the others again, and `AppMsgPort` returns 0. The
  original left them open and returned the address of a port it had
  deleted ([#526](https://github.com/fubar-coder/bb2-c/issues/526)).
* A program run a second time from the editor starts without an App
  message. The original started with the last message of the run
  before: `AppMessage`, `AppEventType` and the other functions read it,
  and the first `AppEvent` answered it a second time
  ([#1099](https://github.com/fubar-coder/bb2-c/issues/1099)).
* With runtime error checking, an id of `AddAppMenu`, `DelAppMenu`,
  `AddAppIcon` or `DelAppIcon` outside 0 to 15 stops with the runtime
  error "App id must be 0 to 15", and a window number of `AddAppWindow`
  or `DelAppWindow` outside 0 to 15 with "App window number must be 0
  to 15". Without it, such a number overwrites memory, as in the
  original ([#1097](https://github.com/fubar-coder/bb2-c/issues/1097)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `AddAppIcon` set the base of the local variables to 0: inside a
  Statement or Function the local variables used after the command were
  read and written in the wrong place, and the procedure crashed when it
  returned. Used as a function in a string expression, it also let the
  rest of the expression be written into the data of the icon. Local
  variables and string expressions work after `AddAppIcon`
  ([#516](https://github.com/fubar-coder/bb2-c/issues/516)).

{{% fixed-bugs-library part="elsewhere" %}}
