---
title: datelib
description: The bugs of Blitz Basic 2 in the command library datelib that the
  BB2 C port fixes.
sidebar:
  order: 16
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/datelib/
---

::fixed-bugs-library

## Dates

### `Date$` showed 1 January after a leap year as 32 December {#date-after-leap-year}

`Date$` turns a day number, the days since 1 January 1978 that
`SystemDate` returns, into a date string. For one day in every four years
it gave a date that does not exist: the 1 January after each leap year,
such as 1 January 1981 or 1 January 2025, came out as the 32nd of
December of the leap year. A program that shows today's date showed it
wrong on that day.

**What happened**

```
NPrint Date$(1095)
NPrint Date$(1096)
NPrint Date$(1097)
End
```

Day 1096 is 1 January 1981. The original prints

```
31/12/1980
32/12/1980
02/01/1981
```

`Days`, `Months` and `Years` after that `Date$` return 32, 12 and 1980
as well. Day 17167, 1 January 2025, comes out as `32/12/2024`.

**With the port**

```
31/12/1980
01/01/1981
02/01/1981
```

`Days`, `Months` and `Years` return 1, 1 and 1981, and day 17167 is
`01/01/2025`.

Issue: [Date$ showed the 1st of January after a leap year as "32/12" (#120)](https://github.com/fubar-coder/bb2-c/issues/120).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

* `Date$` and `NumDays` made every hundredth year a leap year, so dates
  before 1 March 1900 and from 1 March 2100 on were a day off per
  century; they follow the Gregorian calendar, in which 1900, 2100, 2200
  and 2300 are not leap years
  ([#1001](https://github.com/fubar-coder/bb2-c/issues/1001)).
* `WeekDay` was two days off for a few dates, such as 15 and 16
  September 1973; it is right for every date
  ([#121](https://github.com/fubar-coder/bb2-c/issues/121)).
* `NumDays("31/12/1977")` gave 65535 instead of -1, and the day 65535, in
  the year 2157, came back from `NumDays(Date$(65535))` as 131071; both
  are right
  ([#122](https://github.com/fubar-coder/bb2-c/issues/122)).
* `NumDays` of a date with an empty field, such as `"/5/1990"`, took the
  empty field as 47; it returns 0, as for other dates it cannot read
  ([#123](https://github.com/fubar-coder/bb2-c/issues/123)).
* `NumDays` of a month outside 1 to 12 returned a number taken from
  unrelated memory; it returns 0
  ([#124](https://github.com/fubar-coder/bb2-c/issues/124)).
* `NumDays` of a year with fewer than four digits at the end of the
  string, such as `"1/1/78"`, read past the end of the string, and the
  result depended on the memory behind it; the year ends with the
  string, so `"1/1/78"` is the year 78, and an empty year gives 0
  ([#125](https://github.com/fubar-coder/bb2-c/issues/125)).
* `NumDays` of a string too short for its day or month, such as `""`,
  read on into the bytes behind the string and could return a date made
  of them; such a string gives 0
  ([#1029](https://github.com/fubar-coder/bb2-c/issues/1029)).

::fixed-bugs-library{part="elsewhere"}
