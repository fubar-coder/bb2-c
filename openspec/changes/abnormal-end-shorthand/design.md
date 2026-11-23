# Design

## Context

`tools/lib/testconf` reads the `[ends."SELECTOR"]` tables: `testconf.c`
checks their form (a `reason` string, every other key a non-empty list),
`testassert.c` checks each element (`check_ends`: "QUERY PREDICATE", the
query one that varies with the end) and builds, per cell, the alternatives
(`tc_ends_for`, `add_alt`). `testjudge.c` judges them; `testreg.c` checks
that the check queries name checks of the source. The file format is the
TOML subset of `tools/lib/testtoml/testtoml.h`: strings, integers,
booleans and one-line lists of strings; no inline tables.

## Decisions

### 1. The syntax: `abnormal = ["any", ...]`

The decision's example was `abnormal = "any"`. A string value cannot carry
the extra assertions that 15 of the 58 tests need in every alternative (a
check that the crash can hide or the bug can fail, `check.NS.NAME != ok`),
and the subset has no inline tables. The chosen form keeps every
alternative a list, so:

- one form covers the 43 plain tests (`abnormal = ["any"]`) and the 15 with
  checks (`abnormal = ["any", "check.NS.NAME != ok", ...]`), the extra
  assertions in the same place as today;
- the type rule of the table stays ("every key but reason is a non-empty
  list"), and `testreg`'s check of the check names sees the extra elements
  unchanged (it skips elements without a blank, such as `any`);
- no valid table changes its meaning: an element without a blank was
  refused before ("QUERY PREDICATE expected"), and a list without `any`
  stays an ordinary alternative, also under the key `abnormal`.

A string `abnormal = "any"` is refused with a message that names the list
form. Rejected: a second key for the extras (`abnormal_with = [...]`): it
would take a name that is an ordinary alternative today.

### 2. The expansion keeps the names and the order

The built-in expands, in the place of its key, to the alternatives
`abnormal`, `abnormal_after_case`, `abnormal_after_run`,
`abnormal_run_only`, each with `session != done`, `rc not exists`,
`requester count >= 0`, its `end.case` and `end.run` assertions and then
the extra elements, in the order the 58 tests wrote them. So the judging
and the report's names stay as they were; only the file line of the
assertions is now the one line of `abnormal`.

### 3. What is refused

- `any` other than as the first element of the key `abnormal`;
- a string value of `abnormal` (the message names the list form);
- another bare word (no blank) as the first element of `abnormal`: the
  message names the built-in form;
- an extra element on `session`, `rc`, `requester`, `end.case` or
  `end.run` (the built-in fixes them);
- the keys `abnormal_after_case`, `abnormal_after_run`, `abnormal_run_only`
  in a table that declares the built-in (they would be two alternatives of
  the same name).

## Findings

(recorded during the work)
