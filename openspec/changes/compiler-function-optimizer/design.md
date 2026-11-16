# Design

## Context

- **The pass loop:** `compile()` (`programs/blitz2/src/compile.c:225-250`) runs `dopass()`. With "Make Smallest" (`optreqga15` selected) it runs a second pass (`smallpass`), which writes the final, shorter code.
  - `menu1` ("Create Executable", `main.c:853-869`) forces the option for that compile.
  - A run from the editor uses the option as set; its default is off.
- **Procedure lookups:** procedures are looked up in exactly two places, `handleproc` (`compile.c:1359`, a call as a statement) and `afunction` (`eval.c:2608`, a call in an expression). There is no address-of and no call by address for a procedure.
- **Procedure definitions:** they are compiled by the procedure code around `dofunc2`.
- **Libraries:** they are linked by `uselib()` when a token is compiled, so a library that only skipped code uses is not linked.
- **AmiBlitz3 develop** marks call edges in `mark_funccall` and computes reachability from the main program in `propagate_calls`. It protects nothing beyond that; the analysis shows silent Data shifts and "not found" errors (`analyse-ab3-optimizers-tasktrap.md`, 1.4).

## Goals / Non-Goals

**Goals:**
- Reachability over the call graph, in one extra analysis of data from the first pass, applied in the second pass.
- Keep rules that make the removal invisible to the program.

**Non-Goals:**
- Removing code inside procedures, or removing library commands.
- A new pass or a new switch.
- Optimizing the main program.

## Decisions

1. **Collect in pass 1, skip in the small pass.**
   - In pass 1 every procedure gets a node. Each lookup in `handleproc`/`afunction` adds an edge from the current context (the main program or `thisproc`) to the callee. Each keep reason marks the procedure as a root.
   - Between the passes, a reachability walk from the main program and the roots marks the kept procedures.
   - In the small pass the definition of an unmarked procedure is skipped to its `End Statement`/`End Function` without emitting code. Labels inside it are not defined, and nothing can reference them, by the keep rules.
   - *Alternative:* AmiBlitz3 v3.14.0 removed one level per pass and needed up to 10 passes. Reachability needs none extra.
2. **Keep rules as roots, decided syntactically in pass 1.**
   - A procedure is a root when its body has `Data`, a constant or NewType or macro definition, an include or `INCBIN`, conditional compilation, `DEFTYPE`, or a global `Dim`.
   - It is also a root when one of its labels is referenced from another context. That covers BASIC references (`Goto`/`Gosub`/`Restore`/`?label`, `SetErr`/`SetInt` targets, including AB3 mode's forward references with `OPTIMIZE 4`) and inline assembler references.
   - A root is kept with everything it calls.
   - Erring on the side of keeping is always allowed.
3. **The same in both modes.** BB2 mode and AB3 mode share the code. The AB3 header key `FuncOptimize` stays a key that is read and written, but it does not switch the optimizer.
4. **Errors.** A program with an error in an unreachable procedure still reports it: pass 1 compiles every procedure. Only the small pass skips.
5. **Debugger line info.** The skipped procedures have no code, so no line info. That is the same as for code that was never compiled. Runtime-error messages of kept code are unchanged.

## Risks / Trade-offs

- **[A keep reason is missed, and a program compiles differently or reads other Data.]** → One test per keep rule, in both modes, compared with the result without "Make Smallest". The CD corpus and corpus_cd_ab3 are compiled both ways and the outputs compared.
- **[Pass 1 and the small pass see different sources]** (conditional compilation that depends on pass state). → A procedure with conditional compilation is a root.
- **[Code addresses change and expose a layout-dependent bug in a program.]** → This is the same exposure "Make Smallest" already has. It is accepted.
- **[Memory for the graph on large sources.]** → One node per procedure and one edge per distinct call; if memory runs out, keep everything, as AmiBlitz3 does.
