---
title: editlib
weight: 20
description: The bugs of Blitz Basic 2 in the command library editlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/commands/editlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Input

* `Inkey$(0)` returned a string of a random length; it returns the empty
  string ([#128](https://github.com/fubar-coder/bb2-c/issues/128)).
* `Edit` with a default value in a window overwrote the last character
  of the default with a space, so `Edit(9.5,5)` offered `9.` and gave 9
  when the user just pressed Return, and `Edit(25,4)` gave 2; the whole
  default is shown, and Return gives 9.5 and 25
  ([#129](https://github.com/fubar-coder/bb2-c/issues/129)).
* `Edit$` at the end of its input, such as a file given to the program
  with `<` in the shell, returned a string of the length -1; it returns
  the empty string, and `Edit` there returns 0
  ([#132](https://github.com/fubar-coder/bb2-c/issues/132)).
* `Inkey$` with a negative count tried about 4 billion times to read a
  character and added every character it got behind the end of the
  string buffer; with runtime error checking a negative count is the
  error "Can't Inkey$ a negative number of Characters!"
  ([#1031](https://github.com/fubar-coder/bb2-c/issues/1031)).

{{% fixed-bugs-library part="elsewhere" %}}
