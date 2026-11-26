---
title: mathtranslib
weight: 38
description: The bugs of Blitz Basic 2 in the command library mathtranslib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `^` lost the sign of a negative base {#power-negative-base}

The operator `^` raises its left operand to the power of its right one.
It always computes in Float, whatever the types of its operands, and the
compiler hands that calculation to the first entry of mathtranslib.

**What happened.** The original gave the power of the base's absolute
value, also when the exponent is a whole odd number, where the result
must be negative: `(-2)^3` was 8 instead of -8. This program shows it
with variables of different types:

```
vm.w=-2 : vf.f=-2.5 : vn.w=-3 : nw.w=3
rf.f=0 : rl.l=0
rf=vm^3 : NPrint rf
rf=vm^-1 : NPrint rf
rf=vf^nw : NPrint rf
rl=vf^nw : NPrint rl
rf=vm^2 : NPrint rf
rf=vn^1.5 : NPrint rf
End
```

**With the original**

```
8
.5
15.62499
15
4
5.196152
```

**With the port**

```
-8
-.5
-15.62499
-15
4
5.196152
```

A negative base with a whole odd exponent, also a negative one such as
-1, gives a negative result, and a whole even exponent a positive one.
The exponent may be a whole number, a Quick or a Float: what counts is
its value, so a Float 3 is odd as well. With a fraction in the exponent
a negative base has no real power, and the port gives the power of the
absolute value as the original did: `(-3)^1.5` is 5.196152, the same as
`3^1.5`. The minus sign binds more tightly than `^`, so `-3^nw` is the
power of -3 as well: -27 with the port, 27 with the original. Only
`-(3^nw)`, the negated power of 3, was -27 in both.

The fix is in the port's mathtranslib, so a program compiled with the
libraries of the Blitz Basic 2 CD keeps the original result. AB3 mode
uses AmiBlitz3's libraries and gives AmiBlitz3's result, which drops the
sign as the original did.

**What to change.** A program that relied on the positive result, for
example to avoid `Abs`, writes `Abs(x)^n`.

Issue: [`^` with a negative base drops the sign: (-2.5)^3 gives 15.625 (#1189)](https://github.com/fubar-coder/bb2-c/issues/1189).

{{% fixed-bugs-library part="elsewhere" %}}
