---
title: NewTypes, arrays and lists
weight: 2
description: Types of your own with NEWTYPE, fields and paths, pointers, arrays with Dim, and list arrays.
---

Before this page you should know the types and how a variable gets one
([Types, variables and constants]({{< relref "types" >}})).

Besides its six types, Blitz Basic 2 lets you define types of your own,
called NewTypes, and keep values of any type in arrays and lists. This
page shows how to define them and how to reach their parts.

## NewTypes

A NewType is a group of fields under one name, like a record or a C
structure. It is defined between `NEWTYPE .name` and `End NEWTYPE`, one
field per line or several separated by colons:

```
NEWTYPE .Person
  name$
  age.b
  height.q
End NEWTYPE

NEWTYPE .Point
  x.w:y:z          ; y and z are Words as well
End NEWTYPE
```

A field without a suffix has the type of the field before it. A field can
also have the type of another NewType defined earlier.

A variable gets the NewType as its type like any other type, with the
suffix (`a.Person`) or with `DEFTYPE .Person a`. Its fields are reached
with a backslash, and a path through several levels with several:

```
a.Person\name="Harry"
a\age=20
NPrint a\name," is ",a\age

NEWTYPE .Object
  position.Point
  speed.Point
End NEWTYPE
DEFTYPE .Object ship
ship\position\x=100
```

A string field is named without the `$` when it is used: `a\name$` is the
compile error "Garbage at End of Line".

An assignment with a list of values fills consecutive fields, starting
with the one named: `ship\position\x=100,1,2` sets `x`, `y` and `z`, and
`a.Person\name="Harry",20,2.1` all three fields of `a`.

**Array fields.** A field can be an array of a fixed number of elements,
given in square brackets. Unlike `Dim`, the number is the count of
elements, so `[4]` holds the elements 0 to 3, and it must be a
constant. An array field has one dimension and can be of any type:

```
NEWTYPE .Record
  name$
  address.s[4]     ; four strings, also written address$[4]
End NEWTYPE
DEFTYPE .Record p
p\address[0]="10 St Kevins Arcade"
p\address[3]="New Zealand"
```

Without an index, `p\address` is element 0. An array field of 0
elements takes no room: it lies on the same bytes as the field after it,
so two names can share one place in memory. A field of 65536 elements or
more is the compile error "Type too Big"
([An array field of 65536 elements or more]({{< relref "/docs/compared-with-original/new-errors#newtype-field-count" >}})).

**SizeOf.** `SizeOf .Point` is the size of a NewType in bytes, and
`SizeOf .Point\z` the offset of a field from its start. Fields are laid
out in order; a field of two bytes or more starts at an even offset.

A NewType cannot contain a field of its own type (the compile error
"Illegal Type"), but it can contain a pointer to it, which is how linked
structures are built
([A NewType that contains itself]({{< relref "/docs/compared-with-original/new-errors#newtype-contains-itself" >}})).
A field may have a NewType that is defined further down the source; its
size is then known only from that definition on
([A field of a NewType defined later]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#newtype-defined-later" >}})).

## USEPATH

When a part of a program works on one structure, the paths get long.
`USEPATH` sets a text that the compiler puts in front of every name that
begins with a backslash:

```
USEPATH ship\position
\x=160            ; ship\position\x=160
\y=\y+1           ; ship\position\y=ship\position\y+1
```

`USEPATH` acts while the source is compiled, not while the program runs:
the path in effect is the one of the last `USEPATH` above the line in the
source, wherever the program jumps.

## Pointers

A variable whose name starts with `*` is a pointer: it holds the address
of a value of its type, and its fields are reached through it as if it
were the value itself:

```
DEFTYPE .Point *p
*p=&ship\speed       ; the address of the field speed
*p\x=3               ; changes ship\speed\x
```

A pointer is a Long, and `&` gives the address of a variable
([Expressions and operators]({{< relref "expressions#addresses" >}})). A
pointer is also the way to reach the structures of AmigaOS: with a
NewType that matches the structure, `*exec.ExecBase=Peek.l(4)` reads
the fields of exec's base. The pointer `*p` is a variable of its own,
next to a plain `p`.

A NewType field written `@name` instead of `*name` is a BCPL pointer (a
BPTR, as AmigaDOS uses them): it holds the address divided by 4, and a
path through it multiplies the value by 4. The form is meant for fields:
a variable written `@name` cannot be read in an expression, the compiler
reports "Syntax Error"
([#1199](https://github.com/fubar-coder/bb2-c/issues/1199)).

## Arrays

`Dim` creates an array. Its type comes from the suffix of the name, or
the default type without one, and it can have any number of dimensions:

```
Dim a(10)            ; 11 Quicks, a(0) to a(10)
Dim grid.w(3,4)      ; 4 × 5 Words
Dim names$(20)       ; 21 strings
Dim aliens.Alien(100); 101 NewType values
n=50 : Dim b.l(n)    ; the size can be computed while the program runs
```

The number in `Dim` is the highest index, so an array has one element
more than the number says, starting at 0. An array must be created with
`Dim` before it is used; a `Dim` of an array that exists already creates
it again, with all its elements 0 or empty. The elements of an array of
a NewType are reached with the path behind the index: `aliens(5)\x`.

A dimension holds at most 65535 elements, so its highest index is 65534.
Without runtime error checking an index is not checked: an index outside
the array reads or writes other memory. With runtime error checking an
index outside the array is the runtime error "Array Subscript out of
Range"; how an index is computed is on the page
[From source to executable]({{< relref "/docs/how-blitz2-works/compiling#array-indexes" >}}).

The array `a()` is a variable of its own, next to a plain variable `a`.

## Lists

A list array, or list, is an array whose elements are used in an order
the program keeps, with a current element. It is created with `Dim List`
and has one dimension; a second one is the compile error "Illegal number
of Dimensions":

```
Dim List aliens.Alien(100)     ; room for 101 elements, none used yet
```

A new list is empty. The commands of the list library add elements,
move the current element and remove it; the most used are `AddItem`,
`AddFirst` and `AddLast`, which add an element and make it the current
one, `ResetList` and `NextItem`, which step through the list, and
`KillItem`, which removes the current element. The functions among them
return -1 when they succeeded and 0 when the list is full or at its end.
The current element is the array name with empty brackets:

```
While AddItem(aliens())        ; fill the list
  aliens()\x=Rnd(320)
Wend
ResetList aliens()
While NextItem(aliens())       ; every element once
  aliens()\x+1
Wend
```

All list commands are on the page of the
[list library]({{< relref "/docs/libraries/commands/linklistlib" >}}).
The elements of a list are linked in their order, which need not be their
order in memory, so a list is used through these commands and not with
an index.

Based on the Blitz Basic 2 manual by Acid Software.
