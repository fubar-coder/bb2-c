# Spec Delta

## ADDED Requirements

### Requirement: The general abnormal end is built in
An `[ends]` table SHALL accept the alternative `abnormal` whose list starts
with the word `any`: the runner SHALL judge it as the four alternatives of
the general abnormal end (before the end of the case, behind it, behind the
end of the run, behind the end of the run without the case), each with a
session not done, no exit code and any number of requesters, and each with
the further elements of the list.

#### Scenario: The short form holds where the four alternatives held
- **WHEN** an open bug's test declares for `libs=cd` `abnormal = ["any"]` and the original crashes after the end entry of its case
- **THEN** the cell passes with the alternative `abnormal_after_case`, as it did with the four alternatives written out

#### Scenario: Further elements apply to each of the four
- **WHEN** the test declares `abnormal = ["any", "check.a.x != ok"]` and the original crashes before the end of its case with the check `x` passed
- **THEN** the case fails, naming each of the four alternatives with its failed assertion

#### Scenario: An unknown word
- **WHEN** a test declares `abnormal = ["all"]`, `abnormal = "any"`, or the word `any` under another key or not as the first element
- **THEN** the runner fails naming the file and line

#### Scenario: An existing alternative keeps its meaning
- **WHEN** a test declares `abnormal = ["session == timeout", "rc not exists"]`
- **THEN** it is one named alternative with these two assertions, as before
