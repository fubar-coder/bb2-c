# blitz2-c – Blitz Basic 2 compiler v1.95 in C

C translation of `blitz2.s` (Blitz Basic 2 v1.95, Acid Software 1995,
Devpac assembler, ~16 000 lines + includes) from
`archive/sources/blitz2-1.95/`; the C code is in `programs/blitz2/`.

Purpose: **readability for the compatibility comparison with AmiBlitz3.** Hence
fidelity beats elegance: same algorithms, same data layouts, same
quirks, the original labels as C names, line references
`/* blitz2.s:NNNN */` at every function.

## Building

Toolchain: bebbo `m68k-amigaos-gcc` in `/opt/amiga` (libnix, `-noixemul`,
68000); building the toolchain on Arch/CachyOS:
bb2 `toolchain.md` (not taken over).

```
cmake --preset amiga
cmake --build --preset amiga                    # -> build/amiga/blitz2/blitz2
cmake --build --preset amiga --target headers   # (bb2 only, not taken over)
cmake --build --preset amiga --target blitz2_ref # reference build and vasm listing of the original
```

Global variables are addressed a4-relative (`-fbaserel`, option
`BLITZ2_BASEREL`, on by default): shorter instructions, fewer relocations. All
entries from outside (editor callbacks, `stopcode`, `trap #15`) load a4 in
`glue*.c` (`GLUE_LOAD_A4`). Release builds are linked without symbols.
Size: 96 KB instead of 127 KB (original 2.10: 86 KB); CD reference run and
test corpus unchanged afterwards.

## Structure

Each file corresponds to a line range of `blitz2.s` (in source order):

| File | blitz2.s | Contents |
|---|---|---|
| `src/main.c` | 1–1911, wbheader, asmmac | startup, token list, edstruct, editor wrappers `ed_*`, menus, program start, asm check routines |
| `src/compile.c` | 1912–3290 | `compile`, `dopass`, macro expansion, STOP requester, statement dispatch (`atokens`) |
| `src/stmts.c` | 3291–4999 | statements, direct mode, object buffer, residents, asm EA parser |
| `src/asmlibs.c` | 5000–6739 | inline assembler, parameters, `uselib`, loading the libraries |
| `src/parse.c` | 6740–8450 | statics/data, `savefile`, `getchar`, `err` + error messages, names, variables |
| `src/eval.c` | 8451–10683 | code emitter `poke*`, `convtype`, expression evaluation, operators |
| `src/libcall.c` | 10684–12135 | numbers, lib calls, includes |
| `src/tokens1.c` | 12136–13326 | built-in commands part 1 (DefType … If/While) |
| `src/tokens2.c` | 13327–14655 | built-in commands part 2 (Then … Dim) |
| `src/data.c` | 14656–end, options*.src, typereq, calcreq | globals, types, requesters, options, `.xtra` |
| `src/structs.h` | – | all heap nodes, offsets checked against the original via `static_assert` |
| `src/glue.c`, `src/glue_direct.c` | – | **the only assembler**: register thunk `callregs`, entry stubs for editor/exec, VBR, trap #15 handler of the direct mode |

Each `NAME.c` has a `NAME.h` (contract for the other parts);
`src/blitz2.h` includes all of them.

## Reading aids

* `BUG(orig): …` – bug/quirk of the original, deliberately kept.
* `DIFF: …` – deliberate deviation, only where C cannot do the same
  (stack tricks, register frames); the comment says what the original did.
* Registers that permanently carry state in the original are globals:
  `srcptr` = a5 of the parser, `numval`/`numfloat` = d3/d6 of the number
  fetch (file-local in libcall.c). Register results that are only read by the caller became return
  values or out parameters (e.g. the name length d2 of `getvname`),
  condition codes as results return values (the mapping is in the
  respective header).
* Code templates (machine code that the compiler copies into the generated program)
  are writable `UWORD` arrays with the original bytes, generated
  from the vasm listing (bb2 `tools/asmlisting.py`) and checked against the binary.
* `CONVENTIONS.md` – rules of the translation; `docs/blitz2/` – analyses per section
  (`survey-NN.md`), cross references, change log, reviews.

## State of checking

After translation, each section was checked by an independent pass
routine by routine against the assembler (flags, branches,
generated opcodes, templates, data layouts, contracts between the parts);
findings and corrections in `review-NN.md`. Data areas (token list,
requesters, templates, error texts) were compared byte for byte with the
original by script. Runtime behaviour has **not** been checked.

## IDE

Open the CMake project directly in CLion (the toolchain is set
automatically). `.clangd` makes clangd parse the sources as m68k.

## Tools

* CMake target `blitz2_ref` – vasm listing of the original (addresses + opcodes),
  `build/amiga/programs/blitz2/ref/blitz2.lst`.
* `tools/asmlisting.py` – bytes/templates of a label range as a C array (bb2 only).
* `tools/xrefs.py` – which labels of each section other sections use (bb2 only).
* `tools/req2c.py` – Intuition requester data → NDK structs (bb2 only).
* `cmake -P tools/checkfile.cmake src/NAME.c` – compile one file with the flags from
  `build/amiga/compile_commands.json` (usable in parallel; bb2 only).

## Running

Not tested. The program is a replacement for `blitz2:blitz2` and needs
the same environment as the original: `blitz2:ted` (editor, loaded via LoadSeg
and calls back into the compiler via `edstruct`),
`blitz2:deflibs`, `blitz2:acidlibs`, `blitz2:userlibs/`,
`blitz2:defaultdbug` – all in `archive/cd/ultimate-blitz-basic-2.1/BLITZ2/`. Test in Amiberry: assign `blitz2:`
to a copy of that directory and replace `blitz2` there with
`build/amiga/blitz2/blitz2`.

Limitations: the generated code is partly executed at compile time
(constant folding, `evalconst2`) – so the compiler only runs
on a 68k Amiga or emulator. The binary on the CD is v2.10; the
source is v1.95.
