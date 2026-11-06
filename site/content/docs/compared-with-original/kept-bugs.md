---
title: Bugs the port keeps
weight: 4
description: Why the BB2 C port reproduces the bugs of Blitz Basic 2, how they are recorded, and the tests that fail on purpose.
---

Before this page you should know which bugs have been fixed ([Fixed
original bugs]({{< relref "/docs/compared-with-original/fixed-bugs" >}})).

If a program behaves strangely with the BB2 C port, the first question is
whether it behaves the same way with the original. Often it does: the port
keeps the bugs of Blitz Basic 2 on purpose.

## Why the bugs stay

The port was made to be exactly Blitz Basic 2.10. Only then can its
compiler, editor and libraries be compared with the originals byte for
byte, and only that comparison shows that the translation is right. A
"small improvement" made on the way would have hidden the translation's
own mistakes among intended changes.

So the rule is: a bug of the original is reproduced and written down, not
fixed. Fixing one is a separate decision, taken for one bug at a time,
with its own issue and a test, and listed on the page of fixed bugs.

## Where they are recorded

Every known bug of the original has an issue on GitHub with the label
[bug-orig](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-orig),
which describes it and, where possible, names a small program that shows
it. In the source code, the place that reproduces it carries a comment
that starts with `BUG(orig)` and names the issue.

If you suspect a bug, search these issues first. If it is there and still
open, the port behaves as the original did, and the issue tells you how
to work around it.

## Tests that fail on purpose

Many of these bugs have a test. Such a test checks the *correct*
behaviour, so it fails as long as the port reproduces the bug, and it
passes once the bug is fixed. Most of them run in an emulator, with the
original compiler and libraries and with the port's, and compare what
both do. Each comes out as one of:

| Result | Meaning |
|---|---|
| REPRODUCED | The port shows the bug exactly as the original does: the normal state of a kept bug. The test fails. |
| FIXED | The port behaves correctly. The test passes. |
| DIFFERS | The port neither behaves correctly nor like the original: a mistake of the port. |
| BROKEN | The original already behaves correctly: the test does not show a bug. |

Because they fail on purpose, these tests are not part of the normal test
runs; they have a run of their own
([Tests]({{< relref "/docs/project-structure/tests" >}})).

## Bugs of the port itself

A mistake of the port, where the port behaves differently from the
original without it being intended, is a different matter: it is not
kept, it is fixed. Such bugs have the label
[bug-port](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-port).
A known one also gets a test that fails until the fix, and it runs
together with the tests of the original's bugs.
