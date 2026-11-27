---
title: ldivlib
description: The bugs of Blitz Basic 2 in the command library ldivlib that the
  BB2 C port fixes.
sidebar:
  order: 35
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/ldivlib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

#### A Long or Quick division by 0 {#long-division-by-zero}

**What happened.** A Word division by 0 stopped a program compiled with
runtime error checking with "GURU! - Division by Zero". A Long or Quick
division by a variable that holds 0 went on without a message and gave
the largest positive number for every dividend, so -5/0 was 2147483647:

```
a.l=7 : b.l=0
NPrint "before"
c.l=a/b
NPrint "after ",c
End
```

Compiled with runtime error checking, the original printed `before` and
`after 2147483647`.

**With the port.** With runtime error checking, a Long or Quick division
by 0 stops with "GURU! - Division by Zero", as a Word division does; the
program above stops at line 3. Without runtime error checking, the result
is the largest number with the sign of the dividend, here 2147483647, so
-5/0 is -2147483647, and the program goes on.

**What to change.** Test the divisor before the division where it can
be 0.

Issue: [Division by 0 returned $7FFFFFFF regardless of the sign (#200)](https://github.com/fubar-coder/bb2-c/issues/200).

::fixed-bugs-library{part="elsewhere"}
