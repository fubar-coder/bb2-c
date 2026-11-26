---
title: rithreadlib
weight: 45
description: The bugs of Blitz Basic 2 in the RWE userlib rithreadlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rithreadlib {#rithreadlib-fixes}

These fixes concern
[rithreadlib]({{< relref "/docs/libraries/userlibs/rithreadlib" >}}),
the threads of Reflective Images: parts of a program that run as DOS
processes of their own.

* Two `CreateThread` in a row start each thread with its own code,
  registers and memory. The original handed them over in single
  variables, which a thread read only when it began to run, so a second
  `CreateThread` before that gave both threads the second one's code
  and memory ([#838](https://github.com/fubar-coder/bb2-c/issues/838)).
* At its end, the program waits until no thread runs any more. The
  original waited only once: after all threads had ended once and new
  ones were started, it could end while they still ran in its freed
  code ([#837](https://github.com/fubar-coder/bb2-c/issues/837)).
* `CreateThread` does not count a thread that could not be started for
  lack of memory, so `ActiveThreads` stays right; with the original, the
  program could hang at its end, waiting for that thread
  ([#1086](https://github.com/fubar-coder/bb2-c/issues/1086)).
* The end of the program frees the signal the library takes at its
  start. The original kept it, so after 15 runs from the editor, or
  from the same shell, `CreateThread` did nothing
  ([#839](https://github.com/fubar-coder/bb2-c/issues/839)).

{{% fixed-bugs-library part="elsewhere" %}}
