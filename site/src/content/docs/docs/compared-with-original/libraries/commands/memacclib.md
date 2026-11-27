---
title: memacclib
description: The bugs of Blitz Basic 2 in the command library memacclib that the
  BB2 C port fixes.
sidebar:
  order: 40
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/memacclib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Strings {#other-strings}

#### `Peeks$` with a negative length {#peeks-negative}

**What happened.** The original took the length of `Peeks$` as an
unsigned number: `Peeks$(Address,-1)` copied about 4 GB into the string
buffer, over the memory behind it, and the program crashed.

**With the port.** A negative length gives the empty string, as a
length of 0 does. With runtime error checking, it is the error "Can't
Peeks$ a negative number of Characters!".

**What to change.** Use a length of 0 or more.

Issue: [Peeks$ with a negative length copied up to 4 GB (#214)](https://github.com/fubar-coder/bb2-c/issues/214).

::fixed-bugs-library{part="elsewhere"}
