# Ideas: language extensions after the port

These are ideas from a discussion on 2026-10-31. Nothing here is decided or
planned; open items are in `TODO.md`. Everything here comes after the
current work: readable names, documentation, the error check area fixes,
and fixing the original bugs. Those make the compiler's code clear enough
to extend.

New language features mean a "BB2 next" variant, not the faithful port.
New keywords need token numbers, and tokenised sources should stay
exchangeable with the original.

## Order and dependencies

0. **Varargs OS calls (ReAction):** possibly the first step (section 0,
   discussion of 2026-10-31). A compiler change, so "BB2 next". Usable
   for ReAction only together with the constants of 1.
1. **C header import:** independent, needs no compiler change for types
   and constants (functions do need library numbers, below), useful at
   once.
2. **Objects (OOP):** a prerequisite for generics with constraints.
3. **Compilation units:** independent of 1 and 2, but the most
   infrastructure (object format, link step, libraries). Much smaller if
   the Blitz libraries become hunk objects (below).
4. **Generics:** last, building on 2.

Shared by 0, 1 (function libraries), 2 (`Extends`, `New`) and 4
(`List<T>`): new token and library numbers. A rule for how "BB2 next"
assigns them without breaking the exchange of tokenised sources with the
original might be the first building block of all.

## 0. Varargs OS calls (ReAction)

ReAction (the BOOPSI classes of AmigaOS 3.2) builds its windows as trees
of objects with nested `NewObject()` calls and tag lists. BB2 supports
only the tag list forms well (`NewObjectA_`, `SetAttrsA_` in deflibs).

**Today** (`stmts.c` `stamigalib`, blitz2.s:3879): every argument of an
OS call is evaluated left to right onto the stack, then popped into the
registers in reverse. So the arguments are on the stack already, but in
the wrong order for a tag list (tag 0 at the highest address).

- **Fix of the order:** after pushing, a small loop at run time reverses
  the n longwords; then `a7` is the tag list. The alternatives do not
  fit: evaluating right to left does not suit the parser; reserving a
  frame first and storing into it fails because `peval` pushes its own
  intermediate values.
- **Nesting** works by itself: an inner call runs completely inside
  `peval` (frame, call, stack cleanup), its `d0` is the next argument.

**Open: where the varargs go.**

- **A (preferred so far):** a property of the OS call entry in deflibs
  ("the last register = a pointer to the remaining arguments"), with
  names of their own such as `NewObject_`, `SetAttrs_`,
  `OpenWindowTags_`. Fits the existing model; the call owns its stack
  frame.
  - The register byte of an entry uses bit 4 (address register) and bits
    0-2 (number), `$ff` ends the list. An old compiler would read a new
    flag bit as a plain register and silently generate wrong code: such
    a deflibs needs the new compiler.
- **B:** a general expression such as `Tags(...)` that gives a tag list
  on the stack to any existing `...A_` call. More flexible (e.g. a tag
  list kept in a variable?), but the lifetime of a stack area in the
  middle of a `peval` evaluation is tricky, and it needs a new keyword
  token.

**Further open questions:**

- **Strings as tag values:** what does `peval` with `T_LONG` give for
  `"Ok"` or `a$` (an address of 0-terminated data, or an error)? To
  check in the code.
- **Issue #344** (d2-d5 not saved for an Amiga library call in an
  expression): nested `NewObject_` expressions make it the normal case,
  so varargs depend on fixing it.
- **`TAG_END`:** written by the programmer, as in C, or appended by the
  compiler?
- **`DoMethod`** (`RA_OpenWindow`, `RA_HandleInput` in
  `reaction/reaction_macros.h`) is no LVO but an amiga.lib function:
  needs a routine of its own, or `CallHookPkt` (utility.library) with
  the class of the object (`OCLASS(obj)`). Its message is a varargs
  list as well.
- **The class libraries** (`WINDOW_GetClass()`, `LAYOUT_GetClass()`,
  ...): FD files are in the NDK (`window_lib.fd`, `layout_lib.fd`, ...),
  so they would become deflibs-like libraries, with library numbers.
- **The tag constants** (`WINDOW_Layout`, `GA_Text`, ...) exist only as C
  headers (`gadgets/` of the NDK: 30 `.h`, only 3 `.i`), as chains of
  `#define NAME (OTHER + 0x...)` across headers (`REACTION_Dummy` is
  `TAG_USER + 0x5000000`): needs the C header import (1), at least its
  constants.
- **`reaction_macros.h`** (`LayoutObject`, `ButtonObject`, `End`) could
  become Blitz macros in a resident (residents hold macros).

```
 [varargs OS calls]  -----+
   (compiler, deflibs)    |
                          +--> [ReAction usable]
 [constants from .h] -----+          ^
   (host tool)                       |
 [class libraries from FD] ----------+   (library numbers)
 [DoMethod routine] -----------------+
```

## 1. C header import

Part of it exists: `tools/blitzres` writes residents with constants from
FD, EQU and LVO files. A C header import would add:

- **Structs → NEWTYPE:**
  - field offsets by the m68k rules (2-byte alignment);
  - unions, bitfields, nested structs, pointers.
  - The resident format already holds types.
  - **Unions** fit: a field in a resident has its offset stored, so
    overlapping offsets are possible (although NEWTYPE syntax cannot
    write them).
  - **Bitfields** do not: there is no bit type. Possibly the enclosing
    word plus constants for the masks.
- **Constants:** `#define NAME value`, enums; a small preprocessor for
  macros with expressions.
- **Functions:** a prototype is not enough. Amiga library calls need the
  LVO offset and the registers, from `.fd`/`.sfd` or the `proto/inline`
  headers. `blitzres` already reads FD files.
  - A resident holds only types, macros and constants. OS calls are
    entries of type 6 in a library like deflibs (`docs/libs/deflibs.md`,
    `BLITZ_OSCALL`), so imported functions mean a deflibs-like library
    with a **library number and tokens** (the token question above).
- **Implementation:** a host tool in C with its own small declaration
  parser. libclang is an alternative (Apache-2.0 with LLVM exception,
  compatible with GPL-3), but it is a heavy dependency.
- **The compiler:** unchanged for types and constants. The result is a
  resident that `blitz2` already loads.
- **A test exists already:** `amigalibs.res` of the CD is in effect an
  imported NDK (359 types, 98 macros, 4551 constants; fields as in the C
  headers, `ln_Succ` a pointer to type `Node`). Generating it again from
  the NDK and comparing with the CD's file checks the import. The
  comparison also shows the renaming rules: e.g. `IntuiMessage` has
  `_Qualifier`, `_MouseX` (probably names that clash with BB2 tokens);
  a general import needs the same rule.
- **Open: the input.**
  - **Assembler includes (`.i`):** `STRUCTURE`/`APTR`/`UWORD`/`LABEL`
    give the offsets mechanically, no C preprocessor; `blitzres` reads
    `.i` already (`--equ`). Enough to reproduce `amigalibs.res`.
  - **C headers (`.h`):** needed for third party libraries and for
    ReAction (its tag constants exist only as `.h`, section 0).
  - Possibly both: `.i` first, with `amigalibs.res` as the test, then
    `.h`.

## 2. Objects (methods, inheritance, virtual calls)

"Objects" already means the library objects in BB2 (`Shape`, `Screen`,
...). Here it means object-oriented programming. The base exists:
NEWTYPE with field access through `\`, and procedures with locals. New
would be:

- **Methods:** procedures with an implicit `this` (the object as the
  first parameter), called e.g. as `a\Draw{x,y}`. That must fit with
  today's `\` field access (parser).
- **Inheritance:** `NEWTYPE .B Extends .A`. The fields of `.A` are the
  start of `.B`, so a pointer to `.B` works as an `.A`.
- **Virtual methods:** a pointer to a method table in the object; the
  compiler emits the tables as data.
- **Creation:** `New`/`Free` through memlib, possibly constructors.

This mainly touches the parser, the type handling and the code generation
of procedures (`parse.c`, `compile.c`, `stmts.c`).

## 3. Compilation units (compile and link as separate phases, vlink)

Today `blitz2` writes the executable directly:

- it pulls in the libraries itself (`fixjsrs`);
- it computes the object maxima for the whole program (`calcmaxs`);
- `INCLUDE` is textual.

Separate units need:

- **Object files:** `HUNK_UNIT` with `HUNK_EXT`, i.e. exported symbols
  and external references. vlink links Amiga hunk objects.
- **Visibility:** `Public`/`Extern` for variables, procedures and labels.
  Globals live in one block with fixed offsets today; across units the
  offsets must be resolved at link time (relocations).
- **Maxima and start-up:**
  - the object counts (`calcmaxs`) and the start-up initialisation are
    merged over all units at the end;
  - so are the debugger and line information.
- **The Blitz libraries:** vlink does not understand the Blitz library
  format, so they become hunk objects (next section).

## 3a. Blitz libraries as hunk objects

Our build already makes the libraries as hunk objects. gcc writes a
`.c.obj` per file; only `libs/blitzlib.ld` and the post-processing turn
them into the one-hunk Blitz library format. The intermediate objects
would get metadata instead of being converted.

- **Metadata (tokens, `!subs`, library number, object definitions,
  maxima):**
  - a data section of its own, e.g. `HUNK_DATA` named `.blitzmeta`, that
    no symbol refers to;
  - the compiler and TED read it from the object file by its name
    (`HUNK_NAME`);
  - linking with garbage collection drops it, in debug and normal builds
    alike, so it never ends up as debug information.
  - **Not `HUNK_DEBUG`:** a debug link would carry it into the program
    as debug information.
  - A companion file per library (like today's token lists) would be the
    alternative.
- **init/finit:** vlink's constructor/destructor collection (vbcc's
  convention: functions named `__INIT_<prio>_<name>` and
  `__EXIT_<prio>_<name>` become lists the start-up code calls). The
  priority replaces today's order of the libraries.
- **Commands as symbols:**
  - each command is an exported symbol, e.g. `_blitz_<lib>_<token>`, and
    the compiler emits references instead of today's `fixjsrs`;
  - with one section per routine (`-ffunction-sections`) and vlink's
    garbage collection only what is used gets linked. Today a library
    always comes whole.
- **Input validation (the error check area):**
  - **its own sections:** the check routines (`!subs` such as
    `prescreen`) and the error texts go in sections of their own, e.g.
    `.text.blitzcheck.prescreen` via `__attribute__((section(...)))`;
  - **why not `.text`:** garbage collection works per section, so in the
    same `.text` section as the command, every reference to the command
    keeps the checks;
  - **with "Runtime Errors":** the compiler emits a call to the check
    before the command; the reference keeps the section;
  - **without:** there is no reference, and the linker drops the checks.
    That is today's leaving out of the error check area, with no special
    logic;
  - **the 73 jumps per set** from normal code into the error check area
    (the `blitzlibcheck` warnings) would simply be references and keep
    the check sections, silently. So they must be fixed first (`TODO.md`).
    A check that a link without "Runtime Errors" leaves no
    `.text.blitzcheck.*` section would succeed `blitzlibcheck`.
- **Final link:** vlink merges the many small sections into one code hunk
  (its option to merge sections).
- **Limits:**
  - **register conventions:** the Blitz conventions (a3/a4/a5 fixed,
    parameters in d0-d5) stay an agreement between compiler and library;
    the linker does not check them;
  - **the original libraries:** they exist only in the Blitz format and
    need a converter, Blitz library → hunk object. `blitzlibcheck` and
    `blitzlibcompare` already read the format;
  - **compatibility:** `blitz2` must read both formats, so old `userlibs`
    keep working.
- **To check in the vlink documentation:**
  - the exact garbage collection option (`-gc-all`/`-gc-empty`);
  - the constructor naming and option;
  - the option to merge sections;
  - whether unreferenced data sections are dropped as expected.

## 4. Generics (like C#, not C++ templates)

Without a JIT, C#-style generics come down to **shared code plus type
information at run time**:

- **Shared code:** a method of `List<T>` is compiled once and works with
  a type descriptor (size, copy, compare, and for `.s` strings the memory
  handling). C++, in contrast, makes a copy of the code per type.
- **Type checking at compile time:** `List<.w>` only takes words.
- **Constraints** (`where T : IComparable`) need interfaces, i.e. objects
  (2).
- **Value types:** C# specialises the code per value type. Without a JIT
  the choice is boxing (slower) or still one specialisation per size.

This needs a real type system with type parameters. BB2 has none today:
types are suffixes (`.b .w .l .q .f .s`) and NEWTYPEs.
