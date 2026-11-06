---
title: Language additions
weight: 1
description: The parts of AmiBlitz3's language that AB3 mode supports - new syntax, line joining, escapes, character literals and AmiBlitz3's procedures.
prev: /docs/ab3-compared
---

Before this page you should be able to compile an AmiBlitz3 source from
the Shell ([Compiling from the command line]({{< relref "/docs/ab3-mode/command-line" >}})).

This page goes through the additions of AmiBlitz3's language that AB3
mode compiles, each with a short example. All examples were compiled with
blitz2-ab3 and run; the output shown is theirs.

## New syntax

Some additions are switched on by AmiBlitz3's *new syntax* mode, which a
source selects with the directive `OPTIMIZE` and the value 4 (or a value
that contains it, such as 5). Many AmiBlitz3 sources do so in their first
lines. The page [Directives: OPTIMIZE, SYNTAX and runtime
errors]({{< relref "/docs/ab3-compared/directives" >}}) explains
`OPTIMIZE`.

With new syntax on:

* **Escapes in strings** and **character literals** work (below).
* **A macro call can take its parameters without braces**: `!mac a,b`
  instead of `!mac{a,b}`.
* **A label needs no colon**: a name at the start of a statement,
  followed by a command, defines a label (`loop NPrint "x"`). The
  shortcut assignments of Blitz Basic 2 such as `a LSL 2` still work.
* **A label of another procedure** may be referenced before it is
  defined.

## Escapes in strings

With new syntax on, two backslashes start an escape inside a string:

```
OPTIMIZE 4
NPrint "[\\41\\42] [\\R03*] a\\nb"
NPrint "[\\__THIS_FUNCTION]"
```

```
[AB] [***] a
b
[GLOBAL]
```

| Escape | Gives |
|---|---|
| `\\` and two hex digits | the character with that code: `\\41` is `A` |
| `\\N` | a line feed |
| `\\R`, two decimal digits and a character | the character that many times: `\\R03*` is `***` |
| `\\__THIS_FUNCTION` | the name of the procedure the string is in, `GLOBAL` outside one |
| `\\__THIS_INCLUDE` | the name of the include file the string is in, empty in the main source |
| `\\__CODELINE` | the number of the line the string is on |
| `\\__VER_MAJOR`, `\\__VER_MINOR`, `\\__VER_PATCH`, `\\__VER_BUILD` | the version and build number from the source's embedded header |
| `\\__DATE__`, `\\__DATE_GER__` | the date of the compile, as month.day.year or day.month.year |

A single backslash is an ordinary character, and four give two. An
escape the compiler does not know is the error "Unknown Escape Sequence".
Without new syntax, and always in `Data` strings, a string keeps every
character as written. In `Dc.b` strings only the date and version escapes
are replaced, with or without new syntax.

## Character literals

With new syntax on, `@"..."` is a number made of the codes of up to four
characters, the first character in the highest byte. In memory it holds
the characters in the written order, which is what IFF chunk names and
similar four-character codes need:

```
OPTIMIZE 4
id.l = @"ILBM"
NPrint Hex$(id)
```

```
494C424D
```

A byte takes one character, a word two, a long four. More characters than
the target can hold are a syntax error; a string or quick target gives
"Can't Convert Types".

## Joining lines with @@

`@@` at the end of a line continues the statement on the next line, so
long argument lists can be split. It works in AB3 mode with or without new
syntax:

```
NPrint sum{1, @@
           2}
```

Inside an include, outside a macro definition, only `,` followed by `@@`
may join lines.

## Procedures

AmiBlitz3's procedures can do more than Blitz Basic 2's:

* **Optional parameters.** A parameter written with `@` is optional, and
  so is every parameter after it. An omitted number gets −1, an omitted
  string the empty string.
* **Up to ten parameters** instead of six.
* **A Function called as a statement.** `sum{2,3}` alone on a line calls
  the function and discards its result; in Blitz Basic 2 it is "Illegal
  Procedure Call".
* **`FAST` procedures** (`Function.l FAST name{...}`, `Statement FAST
  name{...}`) do not clear their local variables when they start. They
  must not use local strings or NewType variables; the compiler reports
  it if they do.
* **NewType return types**: `Function.mytype name{...}` returns the
  address of a `.mytype` item.

```
Statement greet{name.s, @times.l}
  If times < 0 Then times = 1
  For i.l = 1 To times
    NPrint "Hello ", name
  Next
End Statement
greet{"Amiga"}
greet{"Blitz", 2}
```

```
Hello Amiga
Hello Blitz
Hello Blitz
```

## Other additions

* **Global `SHARED`.** `SHARED count` in the main program shares the
  variable with every procedure, without a `SHARED` line in each.
* **Names may begin with `_`**, as in `_start.l = 5`.
* **Labels written as `.name`** at the start of a statement.
* **`#__include`** is 0 in the main source and 1 inside an include, and
  `CNIF @#name` tests whether a constant is defined. Together they let an
  include protect itself against being included twice.
* **`#__debug`** tells whether runtime error checking is on, as in BB2
  mode ([Extensions]({{< relref "/docs/compared-with-original/extensions" >}})).
* **Code in a comment after `:@`** is compiled, and **a `+` at the start
  of a statement** is accepted (it marks a statement without type check in
  AmiBlitz3).

## Macros

Two rules for macro definitions follow AmiBlitz3: a line of a macro with
a one-line `If ... Then` ends that `If` at the end of its macro line, and
an `End` at the end of a macro line is the statement `End`. In Blitz Basic
2 the first rule does not exist, so a macro written for AmiBlitz3 can give
"Illegaly nested Condition" there.

## Words are matched exactly

AB3 mode turns text into tokens with AmiBlitz3's rules: a word is a
command only if it is written exactly like one, in the same case, and only
a whole word counts. `execbase` is a variable, `ExecBase` the function.
A word directly after `.`, `\`, `#` or a digit is never a command, so a
NewType field such as `\Width` stays a field.
