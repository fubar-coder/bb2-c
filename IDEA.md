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

1. **C header import:** independent, needs no compiler change, useful at
   once.
2. **Objects (OOP):** a prerequisite for generics with constraints.
3. **Compilation units:** independent of 1 and 2, but the most
   infrastructure (object format, link step, libraries). Much smaller if
   the Blitz libraries become hunk objects (below).
4. **Generics:** last, building on 2.

## 1. C header import

Part of it exists: `tools/blitzres` writes residents with constants from
FD, EQU and LVO files. A C header import would add:

- **Structs → NEWTYPE:**
  - field offsets by the m68k rules (2-byte alignment);
  - unions, bitfields, nested structs, pointers.
  - The resident format already holds types.
- **Constants:** `#define NAME value`, enums; a small preprocessor for
  macros with expressions.
- **Functions:** a prototype is not enough. Amiga library calls need the
  LVO offset and the registers, from `.fd`/`.sfd` or the `proto/inline`
  headers. `blitzres` already reads FD files.
- **Implementation:** a host tool in C with its own small declaration
  parser. libclang is an alternative (Apache-2.0 with LLVM exception,
  compatible with GPL-3), but it is a heavy dependency.
- **The compiler:** unchanged. The result is a resident that `blitz2`
  already loads.

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
