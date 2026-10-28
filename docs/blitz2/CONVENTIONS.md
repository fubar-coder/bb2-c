# Blitz2 asm → C: translation conventions (phase 2)

Source: `../quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s`
(Blitz Basic 2 v1.95, Devpac). Target: m68k-amigaos-gcc 16 in `/opt/amiga`,
68000, libnix (`-noixemul`). Purpose: a **readable** C version for comparing
behaviour with AmiBlitz3, so **fidelity beats elegance**: same algorithms, same
data layouts, same quirks; the original label names stay as C identifiers.

## 1. Files and owners

| # | lines | file | contents |
|---|---|---|---|
| 01 | 1-1911 (+ wbheader, asmmac) | `src/main.c/.h` | startup, token list, edstruct, editor wrappers, menus, run program, libs dir |
| 02 | 1912-3290 | `src/compile.c/.h` | compile, dopass, macro expansion, stop requester, statement dispatch |
| 03 | 3291-4999 | `src/stmts.c/.h` | statements, direct mode, allox, residents, asm EA parser |
| 04 | 5000-6739 | `src/asmlibs.c/.h` | inline assembler, params, uselib, lib loading |
| 05 | 6740-8450 | `src/parse.c/.h` | statics/data, savefile, getchar, err + error stubs, names, vars |
| 06 | 8451-10683 | `src/eval.c/.h` | pokes, convtype, expression evaluator, operators, elements |
| 07 | 10684-12135 | `src/libcall.c/.h` | number fetch, lib call generation, includes |
| 08 | 12136-13326 | `src/tokens1.c/.h` | built-in tokens part 1 |
| 09 | 13327-14655 | `src/tokens2.c/.h` | built-in tokens part 2 |
| 10 | 14656-end (+ options2.src.new, typereq, options.src, calcreq) | `src/data.c/.h` | globals, types, requesters, options, .xtra |

Shared, owned by the coordinator (edit only as described in §9):
`src/blitz2.h` (umbrella), `src/structs.h`, `src/glue.h`, `src/glue.c`, `CMakeLists.txt`, `cmake/`.

Your notes: `notes/survey-NN.md` (yours and everybody else's – read the
others when you call into their code). Cross references:
`notes/xrefs.md` lists, per chunk, every label used from another chunk
(`name(defline<-using chunks)`); all of those must be in your header.

## 2. Process

**Phase A – header only.** Write `src/NAME.h`: declarations of everything in
`notes/xrefs.md` for your chunk (plus whatever else you think others need).
Do not write NAME.c yet. Check with `cmake --build --preset amiga --target headers` (it fails until all 10
headers exist; to test yours alone use
`echo '#include "structs.h"
#include "glue.h"
#include "NAME.h"' | /opt/amiga/bin/m68k-amigaos-gcc -noixemul -std=gnu11 -Isrc -fsyntax-only -xc -`).
Then reply briefly and stop. You will get a "go" message when all headers
are integrated.

**Phase B – translation.** Read all other headers (`src/*.h`), then write
`src/NAME.c` and build it with `cmake --build --preset amiga` until it compiles without
errors (warnings: fix what is reasonable). Never edit another chunk's .c.

## 3. Naming

* Every top-level asm label keeps its name: `getchar3`, `dofor`, `firstlabel`.
* If the name clashes with a C keyword, a libc / libnix / NDK name or macro,
  append `_`: `int_`, `getchar_`, `read_`, `open_`, `close_`, `exit_`,
  `signal_`, `wait_`, `lock_`, `free_`, `remove_`, `abs_`, `div_`, `nop_`,
  `cmp2_` … (the compiler tells you; check with the header test).
* Local labels (`.loop`) become structured control flow. Where a `goto` is
  clearer, name it `l_loop` (label without the dot).
* Equates: `#define` with the asm name in UPPER case only if the lowercase
  name is also a label; otherwise keep the asm spelling (`#define localloc
  0xc000`).

## 4. Comments and traceability

* Each function starts with `/* blitz2.s:LINE label – original comment */`.
  Long functions get `/* @LINE */` markers at the main blocks.
* Keep the original comments (English). Add short explanations where the
  asm did something tricky.
* `BUG(orig): …` = quirk/bug of the original kept on purpose (keep them all!).
* `DIFF: …` = deliberate deviation, only where C cannot do the same thing
  (stack tricks, register frames). Say what the asm did.

## 5. Types and state

* Exec types (`UBYTE/UWORD/ULONG/BYTE/WORD/LONG/APTR/BPTR`). Globals keep the
  asm size (`dc` = 16 bit, `dc.l` = 32 bit). Pick signedness by use.
* **Addresses in the object code buffers** (`pc`, `pcat`, `data1`, `data2`,
  `libat`, label pc …) are `ULONG`, like in the asm. Raw accesses with the
  macros `B_(a)`, `W_(a)`, `L_(a)` from structs.h. Heap nodes use the structs
  from structs.h.
* Byte/word aliasing in the asm (`move.b x,var+1`, `sne debugga`,
  `bset #7,optreqga15+13`) → explicit masks on the full variable
  (`debugga = cond ? 0xff00 : 0` where the word value matters).
* Gadget option flags: `optreqgaNN.Flags & GFLG_SELECTED` (= "+13 bit 7").
* **Persistent registers become globals** (defined by chunk 05 in parse.c):
  `UBYTE *srcptr;` = asm **a5** of the parser (source char pointer).
  Other registers are parameters / return values.
* **Current char/token** (asm d0 from getchar*): `UWORD`, tokens are
  `0x8000|n`. getchar, getchar2, getchar3, reget … return it.
  End of statement (asm: Z=1 after getchar3/reget):
  `BOOL eos(UWORD c)` (parse.h) – valid right after the call.
* **Condition codes as results** → return values (`BOOL`, `int` with
  <0/0/>0, or a pointer that is NULL when "not found"). Document the mapping
  in the header comment (`/* asm: Z=1 if found, a2=node */`).
* Several results (d2 type + d3 offset, a2 node + d1 …): return one, the rest
  via out-parameters, or a small struct (in structs.h). Document it.
* Registers that callers rely on being preserved/modified → make it explicit.
* `regat`, `pc`, `nomemleft`, `linenumat` … are already globals in the asm.

## 6. Control flow

* `bra other` (tail call) → `return other(...);` or `other(...); return;`.
* Fallthrough into the next label → call it and return (comment
  `/* falls through into X */`).
* Statement handlers (`atokens` etc.) are entered after getchar3:
  `void dofoo(UWORD c);` (typedef `tokhandler` in structs.h). They use
  `eos(c)` if the asm tested Z/N on entry.
* Routines that drop their caller's return address (`addq #4,a7`), reset the
  stack (`move.l passstack,a7`) etc.: see your survey section G; agree via
  the header (return flag, or a longjmp target owned by the caller's chunk).
* **err**: `NORETURN void err(const char *msg)` (parse.h). The asm's
  `bsr err / dc.b 'text'` stubs become
  `NORETURN void syntaxerr(void) { err("Syntax Error"); }` with the same
  names. Error recovery (`errstack`/`errcont`) is
  `extern jmp_buf *errjmp;` + `extern const char *errmsg;` (parse.h):
  ```c
  jmp_buf jb, *olderr = errjmp;
  errjmp = &jb;
  if (setjmp(jb)) {           /* asm: errcont label, a7 = errstack */
      errjmp = olderr;
      ...continuation...
      return;
  }
  ...
  errjmp = olderr;            /* DIFF: asm never restored errstack */
  ```
  `errmsg` holds the message (asm: d0 in the direct-mode/quiet path).
* Dispatch tables live with their dispatcher: `atokens`, `modetable` → 02;
  `convtable`, `operators` → 06; `cifbras` → 08; `menuadds` → 01.

## 7. Code templates (code the compiler copies into the generated program)

* Never take the address of compiled C code as a template. Templates are
  **writable** `UWORD` arrays defined in the chunk where the label is, with
  the exact bytes of the original. Generate them from the original listing:
  ```
  python3 -I tools/asmlisting.py build/amiga/blitz2.lst block START END
  ```
  (one array named START covering START..END, plus `#define` for every label
  inside, e.g. `#define usef (&use[5])`). `carray NAME START END` gives a
  const array. The listing is from vasm; references to labels show as 0
  (relocations) – fill or comment them. Compare with the bytes you verified
  against the shipped binary (e.g. Devpac encodes `exg a4,a5` as `$CB4C`).
* Put `extern UWORD X[];` and the `#define`s for inner/end labels in your
  header, so `pokecode(use, usef)` works everywhere.
* Templates that are patched in place (`move d1,preplibst+2`) stay patched
  in place, as in the asm (other chunks read them).

## 8. Platform access

* exec/dos/intuition: `#include <proto/exec.h>` etc. and the normal
  functions (`AllocMem`, `Open`, `ActivateWindow` …). Library bases:
  `SysBase`, `DOSBase` from libnix; `MathBase`, `IntuitionBase` (asm `maths`,
  `int`) are defined by chunk 01.
* Editor (`comdata`) calls: wrappers `ed_*()` declared by chunk 01 in
  main.h; `doallocmem()` (comdata+88) is declared by its owner chunk.
* mathffp: `ffpcall(lvo, d0, d1, &ccr)` with raw FFP bit patterns (glue.h);
  never C floating point.
* Running generated code / calling register-based code:
  `callregs(fn, &regs)` (glue.h).
* **Inline asm only in `src/glue*.c`.** If your chunk needs some (directtrap,
  procstart …), create `src/glue_<topic>.c` (68000 code, MIT syntax, symbols
  with `_`). Keep it minimal and explain it.
* `flushc()` before executing generated code (owner 01).

## 9. Shared headers

* `structs.h`: node layouts with `OFFS()` asserts. If a field is wrong,
  missing, or you need a new struct/typedef, edit structs.h (Read it fresh,
  minimal Edit, never change an offset without evidence from the source) and
  log one line in `notes/changes.md`.
* Your header contains only `extern`s, prototypes, `#define`s, and
  `extern UWORD template[]`s. Typedefs/structs go into structs.h.
* Phase B: if another header lacks something you need, add the declaration
  there with `/* added by chunk NN */`, matching the asm semantics in that
  chunk's survey, and log it in `notes/changes.md`; the coordinator checks
  that it gets implemented.
