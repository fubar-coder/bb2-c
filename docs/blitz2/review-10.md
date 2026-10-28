# Review 10 - src/data.c vs blitz2.s 14656-16015 (+ options2.src.new, typereq, options.src, calcreq)

convtable/operators skipped (reviewer 06).

## Method for the data
blitz2.s was assembled with vasm (-Fhunk, then objcopy to a binary; it matches
build/amiga/programs/blitz2/ref/blitz2.lst byte for byte). data.c was compiled with -S, and every
data/bss label in it was checked item by item against the asm image at the
label of the same name. Plain values had to match exactly. Every pointer
(`sym+off`) had to resolve to the same address as the asm long (relocation
values in the image). Result: 585 labels identical, covering all requesters,
gadgets, StringInfos, PropInfo, Images, Borders/XY lists, IntuiTexts and
strings, the type nodes and all of mydata. Exceptions, all intended:
`extrawords`/`lasta6qf` (merged asm labels), and optreqin4..7/in36/in37,
which gcc drops because nothing references them (they are not reachable from
the asm chain either). maxpc..stringwork, uimageloc/dimageloc, firsttype and
namebuff2 are macros onto verified fields.

## .xtra layout
loadxtra/savextra order: vers(1) ga9.Flags low byte(1), 14 Flags words,
6 LongInts, resbuff 544, xpath+xname 384, argreqla8 128 (only if vers!=0 on
load), then {libnum.w, max.w}*. C writes/reads the same bytes in the same
order. optssave 982 bytes, same order.

## Findings
- 15869(asm) menu7: `d3 = -d3` on LONG is UB for $80000000 (asm neg.l keeps it) -> FIXED (negate as ULONG), low
- 15386(asm) loadxtra: asm reads xpath+xname with ONE Read of 384; C does two Reads of 192 (xpath/xname are separate arrays in main.c). It differs only for a file truncated exactly after xpath: asm goes on to the vers/CLI part, C goes to .done2. The bytes of normal files are identical -> REPORTED (needs xpath/xname to be one block in main.c), low
- 15211(asm) menu6: findtype ends with reget() (parse.h contract), which reads at srcptr. In the asm a5 is just whatever the editor left, and in C srcptr may be stale or NULL. The asm has the same harmless read -> REPORTED (not a translation error, note only), low

## BUG/DIFF claims checked
- typename BUG(orig) (31-char name without NUL): true.
- loadxtra ezerr never reset: true. DIFF errjmp restore: fine.
- maxit DIFF (stops at NULL): unreachable in the asm, fine.
- menu7 DIFF errjmp restore: fine.
- optreqprep BUG(orig) AllocMem unchecked: true.

## Checked and correct
mydata globals, alltypes/type nodes/dummytype, all requester data (optreq2,
typereq, optreq, calcreq, argreq), arrow images, resbuff, zerotypes, makebody,
countlist, showlist3, showlist, showlist2, typeshow, reflist, allofem, menu6,
typename, optspnts/optssave, loadxtra_c, savextra_c, menu4, menu4c, optsdone,
menu4canc, newres, makeres, makemax, maxit, optreqprep, optreqdone, menu7
(incl. radio stride 44 = calcreqga3-calcreqga2, verified contiguous), menu9.
