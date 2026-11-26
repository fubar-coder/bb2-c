# Spec Delta

## MODIFIED Requirements

### Requirement: One page per library
The website SHALL render the committed data in its build, without a
step before it: one page per command library, per AmigaOS library (the
deflibs libraries that share a base on one page) and per userlib, in
sections of the Libraries chapter. A page SHALL show every command with
its name, kind, help text as the original has it, result type and forms
with their argument types, and every AmigaOS call with its arguments,
registers and offset. The Libraries chapter SHALL link the API
reference.

#### Scenario: Looking up a command
- **WHEN** a reader opens the page of windowslib
- **THEN** `Window` is listed with its help text, its three forms with their argument types, and the page's table of contents leads to it

#### Scenario: A long argument list of an AmigaOS call
- **WHEN** a reader opens the page of an AmigaOS library whose calls have long argument lists without spaces
- **THEN** the list may break after each comma and the table is not wider than the content column

#### Scenario: The anchors of a command stay
- **WHEN** a link made before the move to the current generator points to the heading of a command on a library page
- **THEN** the heading has the same id on the page built now
