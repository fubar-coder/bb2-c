---
title: Where AB3 mode differs from AmiBlitz3
weight: 5
description: Cases where AmiBlitz3 v3.14.0's own compiler gives another result than AB3 mode, mostly bugs of AmiBlitz3 that AB3 mode does not copy.
next: /docs/blitz2-internals
---

Before this page you should know what AB3 mode supports ([Language
additions]({{< relref "/docs/ab3-compared/language" >}})).

AB3 mode is compared with AmiBlitz3 v3.14.0 by compiling the same sources
with both, on the same emulated machine, and comparing what the programs
do. Where the results differ, the reason is almost always a bug of
AmiBlitz3's compiler, and AB3 mode does what AmiBlitz3 is meant to do
rather than repeat the bug. This page lists those cases, so that a
program that works with AB3 mode but not with AmiBlitz3, or the other way
round, is not a surprise. The features AB3 mode lacks are on the previous
page.

## Compiling from the command line

| Case | AmiBlitz3 | AB3 mode |
|---|---|---|
| A compile error in some sources | return code 0 and "A unknow error happend!" | return code 10 |
| A text include whose last line has no line end | the last line is lost | the last line is compiled |
| A line of 250 characters or more in the main source | the rest of the source is lost | the line is cut, the following lines are compiled |
| A program compiled without the debugger present | no runtime error checking, `#__debug` is 0, `Stop` is left out | runtime error checking as the options say; `Stop` as in Blitz Basic 2 |
| Data in the `Chip` section that points to other labels | wrong relocations | correct relocations |

## Strings

| Case | AmiBlitz3 | AB3 mode |
|---|---|---|
| `\\R05*` | six stars | five stars |
| `\\R03ab` | `AAAab` | `aaab` |
| `\\R00x` | runs over the string | an empty text |
| An unknown escape, such as `\\qq` | the rest of the line in upper case, and a warning | the error "Unknown Escape Sequence" |
| A name that only begins like a known escape, such as `\\__THIS_FUNCXYZ` | taken for the known name, the rest lost | the error "Unknown Escape Sequence" |
| `\\__THIS_POINTER` | an address in AmiBlitz3's own memory | the error "Not supported: \\\\__THIS_POINTER" |
| `\\__THIS_INCLUDE` after an include has ended | still the name of that include | the name of the file the line is in |
| `\\__CODELINE` in lines joined with `@@` | the number of the first line, later lines one less | each line's own number |
| Escapes in `Dc.b` in lower case | copied as written | replaced in any case |
| `f.f=@"AB"` | the four bytes stored unconverted | the number converted to floating point |
| `a$=@"AB"` | garbage | "Can't Convert Types" |

## Macros and line joining

| Case | AmiBlitz3 | AB3 mode |
|---|---|---|
| A macro line with two `Then`s | only the inner `If` is closed | both are closed |
| A macro line closed with the word `EndIf` | "'End' condition without condition" | `EndIf` closes the `If` like `End If` |
| `@@` after an operator, as in `1 + @@` | a syntax error | the next line is joined |
| `@@` on an `End Macro` line | ignored | the error "Joining lines with @@ does not work after End Macro" |

## Procedures

| Case | AmiBlitz3 | AB3 mode |
|---|---|---|
| A call with seven or more parameters inside an argument of another such call | the outer call's parameters from the seventh on are overwritten | each call keeps its own |
| `name{}` of a procedure whose seven or more parameters are all optional | "Syntax error!" | every parameter gets its default |
| A Function with a string parameter from the seventh on | a wrong result | the right result |
| A call that ends before the first optional parameter | the compiler hangs | "Not Enough Parameters" |
| `Function Return` of a value that is not of the declared NewType | refused | accepted (the check is not made yet) |
