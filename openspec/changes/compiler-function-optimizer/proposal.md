# Proposal

## Why

Programs built with includes carry every procedure of every include, called or not. AmiBlitz3's function optimizer removes the unreachable ones and measured 20-54 % smaller executables on include-heavy AB3 programs (`~/.local/share/bb2-c-work/analyse-ab3-optimizers-tasktrap.md`, section 1): AB3HelpViewer 195 -> 90 KB, the AB3 compiler itself 481 -> 383 KB. Since 2026-11-16 the project's goal is functional BB2 compatibility without visible side effects, so an optimization that no program can observe is welcome in both modes. blitz2 and blitz2-ab3 have no such optimizer; `FuncOptimize` is only a header key.

## What Changes

- **What is removed:** blitz2 and blitz2-ab3 leave out procedures (Statements and Functions, also those of includes) that the main program cannot reach through calls. That covers dead call chains and dead recursion, in the reachability form of AmiBlitz3 develop (`mark_funccall`/`propagate_calls`), not v3.14.0's one-level-per-pass form.
- **When it runs:** exactly when the compiler makes small code today. With "Make Smallest", the second pass leaves the unreachable procedures out. "Create Executable" always makes small code (as `menu1` does); a run from the editor does so only when the option is set. A procedure the program never calls therefore stays callable from the debugger's direct mode in a normal editor run, e.g. a procedure for debug output. There is no other switch; the AB3 header key `FuncOptimize` does not switch it.
- **What is kept:** a procedure stays when removing it could be observed. That is when it:
  - contains `Data`;
  - defines a constant, NewType or macro;
  - contains text-level directives (`INCLUDE`, `XINCLUDE`, `INCBIN`, conditional compilation, `DEFTYPE`, a global `Dim`);
  - has a label referenced from another context, by BASIC or by inline assembler.
- **Libraries:** they stay all or nothing, as `uselib` makes them today. A library is linked when a kept procedure or the main program uses it.
- **Results:** programs give the same output, results and errors. Executables become smaller and code addresses change; the prev comparison names them as intended.

## Capabilities

### New Capabilities

- `compiler/function-optimizer`: which procedures the compiler leaves out of a program compiled as small code, what it always keeps, and when it runs.

### Modified Capabilities

(none)

## Impact

- **Compiler:**
  - `programs/blitz2/src/compile.c`: the pass loop (`compile`, `dopass`), `handleproc`, the procedure definition (`dofunc2`), and label references;
  - `programs/blitz2/src/eval.c`: `afunction` and the label references;
  - the inline assembler's label references (`asmlibs.c`).
  - Both modes.
- **Tests:**
  - host or ab3_cli/blitz2 suites for every keep rule and for dead chains and recursion;
  - corpus and CD comparisons, where executables change and outputs stay;
  - ted_ab compile cases;
  - the AB3 oracle where AmiBlitz3 shows the same size class.
- **Docs:** `docs/ab3/variant.md` (G8 FuncOptimize), the compiler docs, and the site page on compiler differences.
