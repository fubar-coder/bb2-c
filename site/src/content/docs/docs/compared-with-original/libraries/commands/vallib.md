---
title: vallib
description: The bugs of Blitz Basic 2 in the command library vallib that the
  BB2 C port fixes.
sidebar:
  order: 61
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/vallib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

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
* `Val` of a hexadecimal, binary or negative number read one character
  behind the string and took a digit there as part of the number; it
  reads only the string
  ([#302](https://github.com/fubar-coder/bb2-c/issues/302)).

#### `Frac` of a string {#frac-string}

**What happened.** `Frac` took only a float. A Quick was turned into a
float first, so a Quick with an integer part of 256 or more lost the low
bits of its fraction: the Quick 30000 + 10/65536 gave 0. The original
refused a string argument when compiling:

```
f.f=1.25 : NPrint "float ",Frac(f)
a$="1.5"
NPrint "string ",Frac(a$)
End
```

The original stopped with "Can't Convert Types" in line 3.

**With the port.** `Frac` takes a value of any numeric type, as `Int`
does, and the program compiles. It gives the exact fraction of a Quick,
0 for an integer type, and the same as the original for a float. With
runtime error checking, a string is the error "Illegal Type", as it is
for `Int` in the original and the port: the program above prints
`float .25` and stops at line 3. Without runtime error checking, `Frac`
of a string gives 0.

**What to change.** Turn the string into a number first, for example
`Frac(Val(a$))`.

Issue: [Frac of a Quick loses low fraction bits for integer parts of 256 and more (#1064)](https://github.com/fubar-coder/bb2-c/issues/1064).

::fixed-bugs-library{part="elsewhere"}
