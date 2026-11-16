---
title: Important Fixes & Breaking Changes
weight: 1
width: wide
description: All fixed bugs of Blitz Basic 2 in one table, sorted by what they can change in an existing program, with the few fixes that change correct programs first.
prev: /docs/compared-with-original
next: /docs/compared-with-original/fixed-bugs
---

Before this page you should know what the chapter means by a fixed bug
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).

The BB2 C port fixes the bugs of Blitz Basic 2 one at a time. Nearly all of
these fixes only matter to a program that ran into the bug: it crashed,
hung or got a wrong result, and with the port it does not. A few fixes,
though, can change a program that worked. This page lists every fixed bug
in one table, so that you can see which fixes can concern your sources
without reading all the pages of fixed bugs.

{{< fix-overview part="breaking" >}}

## The five levels

Every fix has one of five levels. The level answers one question: can a
program that ran correctly with Blitz Basic 2 behave differently with the
port, and how hard is that to notice before it happens? A new runtime
error ranks above a new compile error, since the compiler shows the one
at once and the other only comes when the program runs into it.

{{< fix-overview part="levels" >}}

A fix of level 3 or 4 is also described, with an example and what to
change in a source, on the page
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors" >}}).

## All fixes

Each row names the topic, the library or the part of Blitz Basic 2 that
is concerned (the compiler or the editor TED), and the commands. The
sentence of a row leads to its description on the pages of fixed bugs,
and the issue to its details and its test on GitHub. The table starts
sorted by level and, inside a level, by issue number. With JavaScript,
the column headers sort the table and the two lists above it show only
one level or one topic.

{{< fix-overview part="table" >}}
