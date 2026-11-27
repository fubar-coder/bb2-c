---
title: gameiolib
description: The bugs of Blitz Basic 2 in the command library gameiolib that the
  BB2 C port fixes.
sidebar:
  order: 27
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/gameiolib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* `Gameb(0)` left the control lines of the game pad as if it had read
  port 1: it changed a line of port 1 and left that line of port 0 low.
  It sets back the lines of port 0 only
  ([#415](https://github.com/fubar-coder/bb2-c/issues/415)).

::fixed-bugs-library{part="elsewhere"}
