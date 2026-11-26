---
title: rievallib
weight: 21
description: The bugs of Blitz Basic 2 in the RWE userlib rievallib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rievallib {#rievallib-fixes}

These fixes concern
[rievallib]({{< relref "/docs/libraries/userlibs/rievallib" >}}), the
evaluation of integer expressions in strings by Reflective Images.
`ConvToPostFix` converts an expression such as `2+6*3` into postfix
form, `2 6 3 * +`, and `PFEvaluate` evaluates that form.

Converting an expression:

* `ConvToPostFix` looks past spaces in front of a `-`: after an
  operator, after `(` or at the start, as in `4* -2` or ` -5`, the `-`
  is a sign. The original looked only at the character just before it
  and took the `-` after a space for a subtraction without a first
  value, so `PFEvaluate` failed with error 1
  ([#633](https://github.com/fubar-coder/bb2-c/issues/633)).
* `ConvToPostFix` keeps a character that cannot belong to an operand,
  so `PFEvaluate` reports it as error 4. The original dropped it
  without an error and split the operand: `1,2` became the two values
  `1 2`, and `12x+3` gave 15
  ([#632](https://github.com/fubar-coder/bb2-c/issues/632)).
* An expression that ends in `.` ends there; the original went on
  converting the memory behind the string
  ([#631](https://github.com/fubar-coder/bb2-c/issues/631)).

Numbers and operators:

* `PFEvaluate` checks the digits of a number: a character that is no
  digit of its base is error 4. The original took every character for
  a digit, so `12x` gave 192, `%2` 2 and `$g` 16
  ([#634](https://github.com/fubar-coder/bb2-c/issues/634)).
* Negative hexadecimal and binary numbers have their value: `-$10` is
  -16 and `-%1` is -1. The original read `$` and `%` after a `-` as
  decimal digits and gave -24410 and -2451
  ([#643](https://github.com/fubar-coder/bb2-c/issues/643)).
* `<` is "less than" with sign, as documented and like `>`. The
  original's `<` meant "less than or equal" without sign, so `2 2 <`
  was true and `-1 1 <` false
  ([#638](https://github.com/fubar-coder/bb2-c/issues/638)).
* `*` gives the low 32 bits of a product that does not fit into a
  long; the original also got the low word wrong, so
  `1000000 1000000 *` gave -727379962 instead of -727379968
  ([#639](https://github.com/fubar-coder/bb2-c/issues/639)).
* `^` is the integer power: `3^0` is 1, `0^5` is 0, `2^-1` is 0, and
  exponents of any size count. The original returned the base for the
  exponents 0 and below, 1 for the base 0, and counted only the low 16
  bits of the exponent, so `3^65538` gave 9. 0 with a negative
  exponent is error 3 "Divide by 0 error!"
  ([#641](https://github.com/fubar-coder/bb2-c/issues/641)).
* `PFEvaluate` works with any number of values pending; from 128
  values on, the original failed every operator with error 1 "Need 2
  arguments for expression!"
  ([#645](https://github.com/fubar-coder/bb2-c/issues/645)).

Registers and memory:

* `PFEvaluate` accepts only `d0` to `d7`, `a0` to `a7` and `pc` as
  registers, and any other name is error 4. The original took the
  character after `a` or `d` as the number of the register without a
  check, so `d9` or `dx` read other values of the register block, a
  `d` at the end read on behind the string, and `d01` was `d0` and the
  number 1 ([#642](https://github.com/fubar-coder/bb2-c/issues/642)).
* A register inside `{}`, such as `{d0}`, reads the memory at the
  register's value; the original mostly failed with error 5 "No close
  bracket on indirect!"
  ([#637](https://github.com/fubar-coder/bb2-c/issues/637)).
* `{address}.b` and `{address}.w` read only the byte or the word, also
  at an odd address on a 68000; the original always read a long, which
  crashed a 68000 at an odd address
  ([#636](https://github.com/fubar-coder/bb2-c/issues/636)).
* A program run a second time from the editor starts without the
  register block, the error number and the indirect address of the
  first run. The original kept all three, so the registers were read
  from memory freed at the end of the first run
  ([#1087](https://github.com/fubar-coder/bb2-c/issues/1087)).

Results and errors:

* An expression that leaves more than one value, such as `1 2` or
  `1 2 + 3`, is error 4 "No result value!"; the original returned the
  last value without an error
  ([#635](https://github.com/fubar-coder/bb2-c/issues/635)).
* `PFEvaluate` of a string of spaces returns 0 with error 4; the
  original returned an address in the program
  ([#644](https://github.com/fubar-coder/bb2-c/issues/644)).
* `PFErrorText` after a `PFEvaluate` without an error returns ""; the
  original could return a string of 64 KB or more, depending on what a
  register held before the call
  ([#640](https://github.com/fubar-coder/bb2-c/issues/640)).

{{% fixed-bugs-library part="elsewhere" %}}
