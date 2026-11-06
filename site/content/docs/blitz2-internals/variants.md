---
title: Library variants
weight: 4
description: Sets of the libraries with other library numbers - what a variant is, the variants that exist, and how to make one.
---

Before this page you should know why library numbers matter
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).

A *library variant* is the same set of libraries with other library
numbers. Since a command's token is made from its library's number, a
variant gives some commands other tokens, and a source tokenised for one
variant means something else with another. Variants exist for two
reasons: to make room for a library that needs a number already taken,
and to build library sets that other Blitz software was written for.

## One file of numbers

Every library number of the port comes from one file per variant,
`libnums.h`. Nothing else names a number: the header of each library, the
values one library requests from another, and the calls between libraries
all use the names defined there. Change a number in that file, and the
build changes it everywhere it is used.

A variant is a directory under `libs/variants/` with its own `libnums.h`.
That file usually includes the one of another variant and changes only
what differs:

```c
/* libnums.h of the variant myvariant: bb2-2.10 with fadelib moved to 3 */
#ifndef MYVARIANT_LIBNUMS_H
#define MYVARIANT_LIBNUMS_H
#include "../bb2-2.10/libnums.h"
#undef LIB_fadelib_header
#define LIB_fadelib_header 3
#endif
```

## The variants that exist

| Variant | What it is |
|---|---|
| bb2-2.10 | the numbers of Blitz Basic 2.10, the default; its libraries are `acidlibs` and `deflibs` in the `blitz2` directory |
| rwe-1999 | bb2-2.10 plus the numbers of the Red When Excited userlibs ([The RWE userlibs]({{< relref "/docs/blitz2-internals/rwe-userlibs" >}})) |
| rwe-1999-eval | rwe-1999 with one library moved, to free a number one RWE userlib needs |
| rwe-1999-ahi | rwe-1999 with the newer of two RWE sound libraries that share a number |

## Making your own

{{% steps %}}

### Write the numbers

Create a directory `libs/variants/myvariant/` with a `libnums.h` like the
one above. Configure again so the build finds it:

```
cmake --preset amiga
```

### Build it

Every variant except the default one is built into
`build/amiga/variants/<name>/`, with the targets `<name>-acidlibs` and
`<name>-deflibs` (they are also part of the normal build):

```
cmake --build --preset amiga --target myvariant-acidlibs myvariant-deflibs
```

The build checks that no number occurs twice in the variant, as it does
for the default libraries.

### See what changed

The tool `blitztokens` lists the tokens a compiler and its libraries give,
in the order blitz2 builds them. Compare the lists of the default
libraries and of the variant:

```
build/amiga/host/tools/blitztokens/blitztokens build/amiga/blitz2/blitz2 \
    build/amiga/variants/myvariant/acidlibs build/amiga/variants/myvariant/deflibs
```

In this example the commands of the fade library move from `CE81` and
following to `8181` and following: their library is now number 3.

{{% /steps %}}

To use a variant, put its `acidlibs` and `deflibs` into a `blitz2`
directory instead of the default ones, together with the userlibs built
for that variant. Its userlibs are built into
`build/amiga/variants/<name>/userlibs/`.

{{< callout type="warning" >}}
A source tokenised with one variant shows other commands, or `?????`,
when it is loaded with another. Keep sources and the variant they were
written for together.
{{< /callout >}}

The default variant can be changed too, with
`-DBLITZ_DEFAULT_VARIANT=<name>` when configuring; then its libraries are
built into the `blitz2` directory. The tests compare the default
libraries with those of the original Blitz Basic 2, so with another
default those comparisons report the changed numbers.
