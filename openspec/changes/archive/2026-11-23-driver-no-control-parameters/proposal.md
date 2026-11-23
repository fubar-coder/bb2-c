# Proposal

## Why

The requirement "The driver takes no parameters" is wider than its reason: the driver takes no control data (which cases run, where the results go; they stay compiled in, design decision D5), but the run setting `runargs` passes arguments to the tested behaviour (`tests/bug-orig/libs/wbstartuplib-clistring-overflow.test.toml` starts the program with a 306-character argument). The user's answer Q-A6, "Option (a)" (`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Answers to Q-A5, Q-A6, Q-A7, Q-A8, Q-A9"), asks for the narrower wording.

## What Changes

- Requirement "The driver takes no parameters" of `testing/basic-test-runner` (MODIFIED): no control data as parameters; `runargs` arguments are for the tested behaviour and the driver ignores them. Its scenario stays; one scenario for `runargs` is added.

## Impact

Text of one requirement; no code, no test and no result changes.
