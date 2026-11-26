---
title: rizonejoylib
weight: 50
description: The bugs of Blitz Basic 2 in the RWE userlib rizonejoylib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rizonejoylib/]
---

{{< fixed-bugs-library >}}

## Changes you may notice

### `NewZoneTable` cleared the zones of the current table {#newzonetable-current-table}

The zones of
[rizonejoylib]({{< relref "/docs/libraries/userlibs/rizonejoylib" >}})
are kept in zone tables. A program starts with table 0, and
`NewZoneTable(table#,num_zones)` creates one of the tables 1 to 15. The
library's guide says that the zones of the new table are cleared and
that the new table does not become the current one.

**What happened.** The original cleared the zones of the current table
instead, and left in the new table whatever its memory held. A program
that had set up the zones of table 0 lost them when it created a second
table:

```
SetZone 0,10,10,20,20
r=NewZoneTable(3,300)
NPrint Zone(15,15)
```

With the original this printed `-1`; with the port it prints `0`, the
zone the point lies in.

**With the port.** `NewZoneTable` clears the zones of the new table and
leaves the current table alone. When it replaces the current table, the
new one stays the current table.

**What to change.** A program that set its zones again after
`NewZoneTable`, or cleared the new table with `UseZoneTable` and
`ZoneInit`, works as before. One that relied on the current table being
cleared clears it with `ZoneInit` itself.

Issue: [NewZoneTable initialises the current table instead of the new one (#875)](https://github.com/fubar-coder/bb2-c/issues/875).

### Circular zones of rizonejoylib {#zone-circles}

`SetZone num,x,y,radius` of
[rizonejoylib]({{< relref "/docs/libraries/userlibs/rizonejoylib" >}})
defines a circular zone around `x,y`. On a 68020 or later processor,
such as the one of an A1200, the library tests the zones with its 68020
mode unless `SetZoneMode 0` switches it off.

**What happened.** The 68020 mode compared the squared distance of a
point from the centre with the squared radius in 16 bits only. A point
far outside the circle could hit it, and a circle with a radius of 182
or more missed points inside it. In both modes, `SetZone` kept the
squared radius in 16 bits, so a radius of 256 or more gave a smaller
circle: 256 one of radius 0, 300 one of radius 156. Two examples, the
first in the 68020 mode:

```
SetZoneMode -1
ZoneInit
SetZone 1,0,0,10
NPrint Zone(256,3)
```

The original printed `1`, the point 256 pixels to the side lying in the
circle of radius 10; the port prints `-1`.

```
ZoneInit
NPrint SetZone(1,1000,1000,256)
NPrint Zone(1000,1000)
```

The original printed `-1` and `1`; the port prints `0` and `-1`.

**With the port.** The two modes give the same results for every zone
that `SetZone` makes, and circles with a radius up to 255 work in both.
`SetZone` refuses a radius outside 0 to 255: it returns 0 (False) and
stores nothing. A point more than 32767 pixels from the centre of a
circle can still hit it, as in the original
([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet#bugs-that-stay" >}})).

**What to change.** Keep the radius at 255 or below; cover a larger
area with several zones.

Issues:

* [68020 zone test compares radius² and distance² of circles as signed words (#880)](https://github.com/fubar-coder/bb2-c/issues/880)
* [SetZone num,x,y,radius stores radius² as a word: radius 256 and above wraps (#867)](https://github.com/fubar-coder/bb2-c/issues/867)

### Zones above the top of the screen {#zone-negative-y}

In a zone table of
[rizonejoylib]({{< relref "/docs/libraries/userlibs/rizonejoylib" >}}),
a circular zone is marked with -1 in place of the bottom edge `y2` of a
rectangle. Zones may lie partly or wholly above the top of the screen,
at negative y.

**What happened.** The 68020 mode took every zone with a negative `y2`
for a circle, so a rectangle above the top of the screen was tested as
a circle around its corner. `SetZone` stored a rectangle whose bottom
edge is -1 as it is, so it counted as a circle in both modes. In the
68020 mode:

```
SetZoneMode -1
ZoneInit
SetZone 7,0,-200,10,-100
NPrint Zone(5,-150)
NPrint SetZone(5,-100,-50,50,-1)
```

The original printed `-1` and `-1`: it did not find the point in
rectangle 7 and stored rectangle 5. The port prints `7` and `0`.

**With the port.** Only a `y2` of -1 marks a circle, in both modes.
`SetZone num,x1,y1,x2,y2` refuses a rectangle whose bottom edge is
exactly -1: it returns 0 (False) and stores nothing. Rectangles with a
bottom edge of -2 or less are rectangles in both modes.

**What to change.** A rectangle that ends at row -1 ends at row -2 or
row 0 instead.

Issue: [A rectangle with y2 = -1 (68020 mode: any negative y2) is tested as a circle (#866)](https://github.com/fubar-coder/bb2-c/issues/866).

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rizonejoylib {#rizonejoylib-fixes}

These fixes concern
[rizonejoylib]({{< relref "/docs/libraries/userlibs/rizonejoylib" >}}),
the zones and joystick commands by Reflective Images. The fixes of
[`NewZoneTable`](#newzonetable-current-table), of
[circular zones](#zone-circles) and of
[zones above the top of the screen](#zone-negative-y) are described
above.

* `NewZoneTable`, `UseZoneTable` and `FreeZoneTable` refuse a negative
  table number. The original took -1 for the entry in front of its list
  of tables: `UseZoneTable -1` chose a table that was none, and
  `FreeZoneTable -1` and `NewZoneTable -1` freed the current table and
  went on using it
  ([#872](https://github.com/fubar-coder/bb2-c/issues/872)).
* `ZoneInit` and `SetZone` refuse a negative zone number. The original
  wrote such a zone in front of the zone table, over the number of
  zones the table holds and the memory before it
  ([#873](https://github.com/fubar-coder/bb2-c/issues/873)).
* `SetZone` and `ZoneInit` reach every zone of a table of more than 8192
  zones. In the original, zone 8192 was zone 0 again, and every zone
  above it the one 8192 below
  ([#878](https://github.com/fubar-coder/bb2-c/issues/878)).
* `ZoneTest` from zone 256 on tests the zones it is asked for. The
  original took the number modulo 256 for the zone it tested and
  reported it under the number asked for: `ZoneTest(260,5,5)` tested
  zone 4 and returned 260. In a range, a
  negative start zone counts as one beyond the table, the last zone,
  and a negative end zone gives -1
  ([#881](https://github.com/fubar-coder/bb2-c/issues/881)).
* In the 68000 mode, `Zone` and `ZoneTest` find points in zones more
  than 32767 pixels wide or high. The original missed them: a zone from
  y -32768 to 32767 did not contain the point 5,-1
  ([#879](https://github.com/fubar-coder/bb2-c/issues/879)).
* `GetZoneX1`, `GetZoneY1`, `GetZoneX2` and `GetZoneY2` return the
  values of a zone also in small tables, and -1 for a zone beyond the
  table or without a table. The original did not check the zone number,
  returned -1 for every zone from `GetZoneY1` in a table of up to 4
  zones, from `GetZoneX2` in a table of 2 and from `GetZoneY2` in a
  table of up to 6, and read address 0 without a table
  ([#870](https://github.com/fubar-coder/bb2-c/issues/870)).
* When there is no memory for zone table 0 at the start of the program,
  `SetZone` returns 0 (False) and `ZoneTable` returns 0. The original
  returned -1 for both, as if the zone had been set and as if -1 were
  the address of a table
  ([#876](https://github.com/fubar-coder/bb2-c/issues/876)).
* `FreeZoneTable` removes the table it frees from the library's list.
  The original left it there: `UseZoneTable` still chose the freed
  table, and the end of the program freed it a second time, in every
  program that used `FreeZoneTable`
  ([#868](https://github.com/fubar-coder/bb2-c/issues/868)).
* When `NewZoneTable` replaces a table and then gets no memory for the
  new one, the table of that number no longer exists: the result is 0,
  and if it was the current table, table 0 becomes the current one. The
  original kept the freed table in its list, where `UseZoneTable` chose
  it and the end of the program freed it again
  ([#874](https://github.com/fubar-coder/bb2-c/issues/874)).
* `NewZoneTable` accepts a table of one zone, as the guide allows. The
  original refused it and took 2 to 65535 zones
  ([#1136](https://github.com/fubar-coder/bb2-c/issues/1136)).
* A program run a second time from the editor starts with zone table 0
  alone. The original found the tables of the run before, which had
  been freed: `UseZoneTable` chose them, and the end of the program
  freed them again
  ([#871](https://github.com/fubar-coder/bb2-c/issues/871)).
* A program run a second time from the editor starts with the reading
  of the four player adaptor switched on, as the guide says. After a
  `JAdaptorStatus Off`, the original started the next run with it off
  ([#1132](https://github.com/fubar-coder/bb2-c/issues/1132)).

{{% fixed-bugs-library part="elsewhere" %}}
