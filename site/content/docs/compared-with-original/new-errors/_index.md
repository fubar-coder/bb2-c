---
title: Sources that compile differently
weight: 10
description: Sources that Blitz Basic 2 accepted and the BB2 C port refuses or compiles to something else, the new runtime errors of the libraries and the forms only the port accepts, with what the original did and how to change them.
---

Almost every source compiles with the BB2 C port exactly as with Blitz
Basic 2. The exceptions all come from fixed bugs: the original accepted
these sources, but compiled them to a wrong or random result, or crashed
while compiling them. The port refuses them with an error message, or
compiles them to what they say. A division by a constant 0 is an example:

```
; original: compiles, prints 2147483647    port: "Division by Zero"
d.l=7/0 : NPrint d
```

The original computed the division while compiling, did not look at the
divisor and stored the largest Long number. The port finds the divisor 0
and says so.

Each case on the following pages shows a short program to copy, with
comments that give what the original did and what the port does, then
the reason for the difference and, where a source needs it, what to
change. Before these pages you should know the fixed bugs of the compiler
([Compiling]({{< relref "/docs/compared-with-original/compiler" >}})).

## Compile errors

The compiler finds these cases. A source that relied on the original's
behaviour gets an error message at the line, or another result.

{{< cards >}}
  {{< card link="expressions" title="Operators and expressions" subtitle="=<, a ~ in a string, Dcb, division by a constant 0, Expression too Complex, shift counts." >}}
  {{< card link="names-and-types" title="Names, procedures and NewTypes" subtitle="Accented letters, constants without a name or value, long names, procedure ends, NewTypes." >}}
  {{< card link="files-macros-assembler" title="Include files, macros and inline assembler" subtitle="Text after a path, long paths, End in a macro, the Macro Buffer, long operands." >}}
{{< /cards >}}

## Runtime errors and refused forms

A few fixes of the libraries work the same way when the program runs: a
command that the original carried out with a wrong argument stops with a
runtime error. And a few sources go the other way: the original refused
them, and the port compiles them.

{{< cards >}}
  {{< card link="runtime-errors" title="Runtime errors of the libraries" subtitle="Commands that wrote over memory or hung with a wrong argument stop with an error." >}}
  {{< card link="refused-forms" title="Forms the original refused" subtitle="Bit operators with a fraction, PlayWait without a tape, InstallFNS with a count." >}}
{{< /cards >}}
