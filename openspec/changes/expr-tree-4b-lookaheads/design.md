# Design

## Context

The series and its check: `expr-tree-1-typing/design.md`; the tree built before emission: `expr-tree-4a-emit-after-build/design.md`.

## Goals / Non-Goals

**Goals:** no source rescans and no code copies in expression compiling.

**Non-Goals:** the statements' own look-aheads outside expressions.

## Decisions

### 1. Parameter counts from the node

`countpees`'s callers get the count from the call node; where a statement (not an expression) calls it, it stays.

### 2. Intrinsics as node properties

The conditions the state machine collects are computed on the node at build time; the inline code is emitted at the place the state machine emits it today.

### 3. Assignments in emission order, not by copying

The left side of `dolet2` is built first (reading order: variables created as today), the right side next; emission is right side, then the address code, then the store: the bytes the copy produces today. Relocations of the address code are added at its emission, so the adjustment `pushput` makes goes away.

### 4. A later array index narrowed on the node

`arrevalchk` narrows a later index of an array (`blitz2.s:8809`); after `expr-tree-4a-emit-after-build` it still patches the code already written for that index (the actions `idxmark` and `idxnarrow`; that change's design, "Findings"). The narrowing is decided on the index's node at build time and the index is emitted in its narrowed form, with the bytes of today.

## Risks / Trade-offs

- [The copied address code today carries relocations whose order in the LIFO list differs from emission order] -> the check compares executables including their relocation hunks; task 1.1 has assignments with relocated left sides (arrays of a NewType in a global, string arrays).
- [`doselect`'s copy has its own protocol] -> cases in 1.1.

## Planning choices (mine, not the user's)

- `Point: ab3-mode-g5 and buffer-access-annotations edit tokens2.c / tokens1.c / asmlibs.c beside this change, no order set — L2 Possible / I2 Medium / E2 Reasoned → Plan`.

## Findings (implementation)

- Task 1.1 found an open original bug (compiler-let-left-a6, issue to be filed): the left side's address code of an assignment is compiled before the right side and moved behind it, but `lasta6` stays as the left side's code left it; a library function with its base in a6 in the index (`Abs` of a Word: ffplib's) makes the right side's call skip its load of a6, and the right side's code runs first (`a(Abs(-j))=Abs(-5)` stores -5 with mathieeesingbas.library's base in a6, crashes with another base). 1.95, 2.10 and the port behave the same (REPRODUCED). `expr-lookahead` therefore has no case that combines such calls on both sides.
- Decision 4 as built (task 2.4): `evalbuild` decides the narrowing of a later index (`laterindex`, without runtime error checking) right after the index is built (`idxnarrow`), from the build's records instead of the positions of the written code: the constant that is the index's only code (`bcst`, its code before it the index's start: the original's `constat == at`, a `move.l` of a Long or Quick) gets `ELF_NARROW` and `constemit` writes the `move.w`; the conversion of one Byte, Word or Quick element at the index's depth that is its last code (`bconv`: `conv.end == pc`) gets `ELF_NARROW` and `contcode` writes it without its `ext.l` (nothing for a Byte or Word, `quicktob2` for a Quick); the rounding of a raw Quick at the end is recorded without its `ext.l` (`rawcutact`'s y). The actions `idxmark` and `idxnarrow`, the emitter's facts they read (`conv`, `rawcutend`, `rawcutdepth`, `constat`, `constend`, `islongconst`) and the facts kept around a `~` constant (`struct facts`, `setfactsact`, `emitfacts`) are gone. Each later index records two actions fewer (48 bytes of the arena on the 68000); `evalbuild`'s frame is 4 bytes larger (one more saved register), on every level of nested evaluations.
- Decision 3 for `doselect` (task 2.3, in part): `Select`'s expression is a unit whose code `bakevalubefore` writes after the code of `selclr` (the `clr.l -(a7)` of a string), between the build and the run of the expression (`evalunit`, the unit `maineval2` begins outside an expression); its relocations are added where its code is written, behind the clr, which has none, so their offsets and their order are those `pushput` gave them. `pushstart`, `pushdo` and `pushput` stay for `dolet2`.
- Conflict for `dolet2` (decision 3, task 2.3 not done for it): the copy's bytes depend on the left side being compiled before the right side. The emitter's `lasta6` flows from the left side's code into the right side's (the original bug above), and the relocations of the left side's code are in the program's list below those of the right side; written in the order the decision names (right side, then the address code), the right side's code loads a6 where the left side's index loaded it first, and the relocations change their order: the executables of such assignments change. Byte-identical emission in that order needs the left side's emitter state replayed before the right side (its code run behind `pc` and set back, as `divzerobuild` does) and its relocations moved below the right side's: more machinery than the copy. The emission order of decision 3 is the fix of compiler-let-left-a6; proposed: make 2.3 for `dolet2` that bug's fix commit (its changed executables named), after its issue.
