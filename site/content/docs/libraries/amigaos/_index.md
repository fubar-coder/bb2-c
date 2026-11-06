---
title: AmigaOS libraries
weight: 2
description: The AmigaOS functions a program can call directly, one page per AmigaOS library or resource, with their arguments and registers.
---

The second library file the compiler loads, `deflibs`, declares the
functions of the AmigaOS libraries and resources, so that a program can
call them like commands, with an underscore after the name:

```
Delay_ 50                     ; wait one second (50 ticks)
```

Each page below belongs to one AmigaOS library or resource, named as
the program opens it when it starts. It lists the calls in alphabetical
order. The pages are made from the `deflibs` the build produces, so they
always match what the compiler sees.

## Reading a call

**Arguments** is the help text of the call, exactly as the editor shows
it: the names of the arguments, as `(name,accessMode)` for `Open_`. What
the function does and what its arguments mean is described in the
documentation of AmigaOS itself.

**Registers** are the processor registers the compiler loads the
arguments into, in the order of the arguments: `d1, d2` for `Open_`
means that `name` goes into d1 and `accessMode` into d2. A call returns
its result in d0, which is the value of the call when it is used as a
function.

**Offset** is the position of the function in the library's jump table,
the number the AmigaOS documentation calls its offset (LVO). The
compiler calls the function at this offset from the library's base.

Some AmigaOS libraries are declared by two `deflibs` libraries, the
second adding newer functions; their calls are on one page.

## The libraries

{{< library-list what="calls" >}}
