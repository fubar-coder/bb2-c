---
title: What the build checks
weight: 3
description: The checks every result of the build must pass before it counts as built, and what each one refuses.
next: /docs
---

Before this page you should know the two sides of the build
([Project structure]({{< relref "/docs/project-structure" >}})).

The build of the BB2 C port does not trust its own results. Every library
file, every joined library set and the compiler and editor themselves are
checked in the same build step that makes them. If a check fails, the
result counts as not built: the step fails, nothing that depends on it is
built, and the next build tries again. You never end up with a `blitz2`
directory that contains a file that failed a check.

## The checks

| Check | Applies to | Refuses |
|---|---|---|
| The library format | every library file | a file with more than one hunk, relocations outside its hunk, a damaged header or table of commands, or a reference from the normal code into the error check area |
| Library numbers | `acidlibs`, `deflibs` of every variant, every userlib | two libraries with the same number in the set that is loaded together |
| The compiler's link | blitz2 | anything that would keep it from starting on Kickstart 1.3, as the original does |
| The editor's link | ted, and the helper of blitz2-ab3 | a wrong layout of the program file, parts of the program file that Kickstart 1.3 would not clear, and helpers that need a newer Kickstart |
| The editor's data | ted | data (fonts, menus, requesters, texts) that differs from the reference build of the original editor |
| The editor's code labels | ted | a code label of the original editor that is not in the C code exactly once |
| The reference builds | the original compiler and editor, assembled | a listing that does not match the assembled program |

## What this means for you

**When you write a library**, the first check is the one you meet: a
library must be one hunk with its header and table at the very start,
and its normal code must not use anything in the error check area,
because without runtime error checking that area is not copied into the
program. If you put a check routine or a message there and call it from
normal code, the build refuses the library.

**When you choose a number**, the number check compares your library
with the library files it will be loaded with. The message names both
libraries:

```
biglib: library number 187 twice: .../userlibs/mylibs/twicelib.obj (library 1) and .../blitz2/acidlibs (library 42)
```

**When you change the compiler or the editor**, the link checks keep them
running on Kickstart 1.3, and the checks of the editor keep it equal to
the original in its data and complete in its code.

Each check has a test that feeds it deliberately broken input, so a check
that stops working is noticed too. The checks are described for
developers in the repository's
[development notes](https://github.com/fubar-coder/bb2-c/blob/main/docs/development.md).
