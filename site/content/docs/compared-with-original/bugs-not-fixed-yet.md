---
title: Bugs not fixed yet
weight: 4
description: The known bugs of Blitz Basic 2 that the BB2 C port still shows, where they are recorded, and the tests that check the correct behaviour.
aliases:
  - /docs/compared-with-original/kept-bugs/
---

Before this page you should know which bugs have been fixed ([Fixed
original bugs]({{< relref "/docs/compared-with-original/fixed-bugs" >}})).

If a program behaves strangely with the BB2 C port, the first question is
whether it behaves the same way with the original. Often it does: the
bugs of Blitz Basic 2 are fixed one at a time, area by area, and a bug
that is not fixed yet shows in the port as it did in the original.

## Where they are recorded

Every known bug of the original has an issue on GitHub with the label
[bug-orig](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-orig),
which describes it and, where possible, names a small program that shows
it. The open issues with that label are the bugs not fixed yet; a fixed
one also gets the label
[fixed-in-port](https://github.com/fubar-coder/bb2-c/issues?q=label%3Afixed-in-port)
and is listed on the page of fixed bugs. In the source code, the place of
a bug not fixed yet carries a comment that starts with `BUG(orig)` and
names the issue.

If you suspect a bug, search these issues first. If it is there and still
open, the port behaves as the original did, and the issue tells you how
to work around it.

## Bugs that stay

A few issues are closed without a fix:

* Some issues have the label
  [wontfix](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-orig+label%3Awontfix).
  These bugs are real, but fixing them would add more complexity than the fix is worth.
  They stay as they are in the original.
  An example is PANIC in the stop requester of a program run from the editor: the program is removed without any cleanup, so its files stay open and its memory and screens stay allocated ([#24](https://github.com/fubar-coder/bb2-c/issues/24)).
* Some issues have the label
  [not-a-bug](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-orig+label%3Anot-a-bug).
  Their behaviour turned out to be intended, or harmless in practice.

## Tests of the correct behaviour

Many of these bugs have a test. A test checks the *correct* behaviour,
taken from the documentation of the command or from the issue, not what
the original does. Most of them run a program in an emulator with the
port and compare what it does with the expectation and with a recorded
observation of the original: what the original compiler and libraries
did with the same program, recorded once and kept in the repository. So
the tests themselves do not need the original; only a new recording
does. Each test comes out as one of:

| Result | Meaning |
|---|---|
| REPRODUCED | The port shows the bug as the original does: the normal state of a bug not fixed yet. The test fails. |
| FIXED | The port behaves correctly. The test passes. |
| DIFFERS | The port neither behaves correctly nor like the original: a mistake of the port. |
| BROKEN | The original already behaves correctly: the test does not show a bug. |

Because they fail as long as the bug is there, these tests are not part
of the normal test runs; they have a run of their own. When a bug is
fixed, its test becomes a *fixed test* in the same change: from then on
it runs with the normal emulator tests, checks only the port against the
expectation, and must come out FIXED
([Tests]({{< relref "/docs/project-structure/tests" >}})).

## Bugs of the port itself

A mistake of the port, where the port behaves differently from the
original without it being intended, is a different matter: it is always
fixed. Such bugs have the label
[bug-port](https://github.com/fubar-coder/bb2-c/issues?q=label%3Abug-port).
A known one also gets a test that fails until the fix, and it runs
together with the tests of the original's bugs.
