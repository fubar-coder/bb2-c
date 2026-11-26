---
title: gtlib
weight: 28
description: The bugs of Blitz Basic 2 in the command library gtlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

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

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

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

{{% fixed-bugs-library part="elsewhere" %}}
