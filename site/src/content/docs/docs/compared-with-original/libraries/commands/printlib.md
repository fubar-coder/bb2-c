---
title: printlib
description: The bugs of Blitz Basic 2 in the command library printlib that the
  BB2 C port fixes.
sidebar:
  order: 46
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/printlib/
---

::fixed-bugs-library

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

Issues:

* [A Quick whose fraction rounds to 0 kept the point (#234)](https://github.com/fubar-coder/bb2-c/issues/234)
* [A Quick whose fraction rounds up lost the carry (#233)](https://github.com/fubar-coder/bb2-c/issues/233)

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

Issues:

* [Format lost a carry out of the first digit (#235)](https://github.com/fubar-coder/bb2-c/issues/235)
* [Format did not round numbers below 1 (#236)](https://github.com/fubar-coder/bb2-c/issues/236)
* [Format without "-" or "+" lost the sign (#237)](https://github.com/fubar-coder/bb2-c/issues/237)
* [Format with too many digits printed "?" and only spaces (#238)](https://github.com/fubar-coder/bb2-c/issues/238)

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

* `Str$` and `UStr$` of a string gave a number made from the string's
  address; they give the string
  ([#242](https://github.com/fubar-coder/bb2-c/issues/242)).
* A float below 1E-17 printed in fixed point form, with `FloatMode -1`
  or a format string set by `Format`, overwrote a table of the library,
  so numbers of 1E18 and more printed wrong digits afterwards (5E+18 as
  `9.5E+19`); they print right
  ([#241](https://github.com/fubar-coder/bb2-c/issues/241)).

::fixed-bugs-library{part="elsewhere"}
