---
title: The language
weight: 4
description: A reference of the Blitz Basic 2 language as the compiler of the BB2 C port accepts it - lines and names, types, expressions, program flow, procedures, directives, the inline assembler and Blitz objects.
prev: /docs/how-blitz2-works/residents
next: /docs/language/types
---

This chapter describes the language that blitz2, the compiler of the BB2
C port, compiles: how a program is written down, which types and
operators it has, and what the words do that the compiler knows itself.
It is a reference, not a course; to learn Blitz Basic 2 from the start,
the Blitz Basic 2 manual by Acid Software is still the best way in. Where
the port behaves differently from the original compiler, the section says
so and links the page that explains the difference.

## Built-in words and library commands

Blitz Basic 2 has two kinds of words. A few dozen *built-in words* belong
to the compiler itself: the statements of program flow (`If`, `While`,
`For`, `Select`, `Goto`), the declarations (`Dim`, `NEWTYPE`, `DEFTYPE`,
`Statement`, `Function`), the directives (`INCLUDE`, `CNIF`, `Macro`),
the words for Blitz objects (`Use`, `Free`) and the mnemonics of the
inline assembler. This chapter describes them.

Every other command comes from a library, even such basic ones as
`Print`, `Left$` or `AddItem`. Those are listed with their forms in the
[command reference]({{< relref "/docs/libraries/commands" >}}). For the
compiler both kinds look the same in a source: a token, as
[Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})
explains.

## A program, line by line

A program is a sequence of lines, and a line holds one or more
statements separated by colons. A semicolon starts a comment, which runs
to the end of the line:

```
a=5 : b=a*2      ; two statements on one line
NPrint a,b
```

Outside a string, everything after the `;` is ignored. There is no line
number and no `REM`.

**Names.** Variables, labels, constants, procedures, types and macros
have names made of letters, digits and the underscore `_`. A variable
name starts with a letter. A name has at most 127 characters, and upper
and lower case are different letters: `myship` and `MyShip` are two
variables.

A name must not be a word the editor knows. When you leave a line, the
editor replaces every word that is a built-in word or a library command
by its token, whatever its case: `loc`, `neg` or `frac` in a name become
the commands `Loc`, `NEG` and `Frac`, and the line gives a "Syntax
Error". A longer word that only begins with a command, such as
`printer`, stays a name.

**Labels.** A label marks a place in the program, for `Goto`, `Gosub`,
`Restore` and the inline assembler. There are three ways to write one: a
name alone as a statement, the same with a colon, or a name after a
full stop:

```
begin                 ; a label
loop: a+1             ; a label, then a statement
.main                 ; a label in the editor's list of labels
```

The form with the full stop also appears in the editor's list of labels
on the right of the screen, where a click jumps to it. A label may also
begin with an underscore.

A name that begins with an apostrophe is a *local label*: it belongs to
the last ordinary label before it, and its full name is that label's
name, the apostrophe and the rest. Under `.main`, the label `'skip` is
`main'skip`, so the same short name can be used again under another
label. A jump names the full name: `Goto main'skip`.

## The pages of this chapter

{{< cards >}}
  {{< card link="types" title="Types, variables and constants" subtitle="The number types and strings, type suffixes and the default type, number literals, constants, Data and Read." >}}
  {{< card link="newtypes-arrays-lists" title="NewTypes, arrays and lists" subtitle="Types of your own, pointers, arrays of any type, and list arrays." >}}
  {{< card link="expressions" title="Expressions and operators" subtitle="Operands, the operators and their order, comparisons, and the type in which an expression is computed." >}}
  {{< card link="program-flow" title="Program flow" subtitle="If, the loops, Select, Goto and Gosub, leaving a loop early, waiting, interrupts and error handlers." >}}
  {{< card link="procedures" title="Procedures" subtitle="Statements and functions with parameters, local and shared variables." >}}
  {{< card link="directives" title="Directives and macros" subtitle="Include files, conditional compiling, macros, and the settings a source can make itself." >}}
  {{< card link="inline-assembler" title="Inline assembler" subtitle="68000 instructions in a program, GetReg and PutReg, operands, labels, Dc, Ds and Dcb, and calls of library routines." >}}
  {{< card link="objects" title="Blitz objects" subtitle="Screens, windows, bitmaps and the other numbered objects: Use, Free, Used, Addr, Maximum." >}}
{{< /cards >}}

Parts of this chapter are based on the Blitz Basic 2 manual by Acid
Software, checked against the compiler of the port.
