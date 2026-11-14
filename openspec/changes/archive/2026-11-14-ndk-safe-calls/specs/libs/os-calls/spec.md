## ADDED Requirements

### Requirement: An NDK call with a parameter in a3 or a4 keeps the register
A library SHALL call an NDK function that takes a parameter in a3 or a4
only in a form that returns with a3 and a4 unchanged. The libraries do not
use `-fbaserel`, so this covers the a3 and a4 parameters of the NDK's
inline headers; the NDK's a5 protection stays as it is. The form SHALL
save the register inside the call (not on the stack) and SHALL NOT change
the condition codes.

#### Scenario: a3 after an OS call
- **WHEN** a library routine calls `EasyRequestArgs` with an argument array and returns
- **THEN** a3 holds what it held at the routine's entry, not the argument array

#### Scenario: a3 and a4 after AddAppIcon
- **WHEN** a library routine calls `AddAppIconA` (DiskObject in a3, tag list in a4)
- **THEN** a3 and a4 hold their values from before the call

### Requirement: A raw call of an affected function does not compile
For every NDK function with an a3 or a4 parameter that has no safe form,
the build SHALL make a call in library code a compile error that names the
function. The check SHALL be done by the compiler in the compile step of
the object (no extra pass, tool or stamp file) and SHALL have a test with
a call that must fail to compile.

#### Scenario: A library calls a poisoned function
- **WHEN** a library source calls a function that has no safe form, for example `BuildSysRequest`
- **THEN** compiling the object fails with an error naming `BuildSysRequest`

#### Scenario: A library uses none of them
- **WHEN** a library includes the shadow header and calls no affected function
- **THEN** its compiled code is the code the unshadowed NDK header gives, minus the unused varargs wrappers

### Requirement: Unused varargs wrappers are not emitted
A library that includes an NDK `proto/` header SHALL NOT contain the
header's `__X_va` varargs wrappers that it does not call. A wrapper it
calls SHALL compile to the same code as the NDK's own, and the shadowing
SHALL NOT produce a warning.

#### Scenario: One wrapper used
- **WHEN** a library includes `proto/intuition.h` and uses one varargs form
- **THEN** its object holds that wrapper only, byte-identical to the NDK's, and the other wrappers are absent

### Requirement: Only the libraries are affected
The shadow headers SHALL be on the include path of the libraries only.
blitz2, TED and the host tools SHALL see the NDK headers unchanged.

#### Scenario: Compiler and editor build
- **WHEN** `blitz2` or `ted` is built
- **THEN** no shadow header is included
