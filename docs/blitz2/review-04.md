# Review 04: src/asmlibs.c vs blitz2.s 5000-6739

Reviewer 04. I compared every routine in the range against the C, instruction by instruction:
condition codes, signed vs unsigned compares, loop counts, register survival, emitted words and
struct offsets (checked against structs.h). I re-derived the templates (ptype, fbase, pushadd,
pushlast) with `tools/asmlisting.py` and they match byte for byte. `tools/checkfile.sh src/asmlibs.c`
is clean (rc=0, no output). I made no changes to the file.

## Findings

- 6553 fixlibs: the comment `BUG(orig): system libs number their tokens from the (negative) lib number`
  states something the code alone cannot prove. For system libs (libnum < 0) the asm starts the
  token-node numbering at the lib number. 14/16(lib) are not set; they come from the file. Whether
  that is a bug or intended (system tokens at the top of the token space) cannot be decided from
  this range. The C behaves exactly like the asm. -> REPORTED (wording only, behaviour correct; up to
  the coordinator whether to soften it to a plain note), severity low

There are no high or medium findings.

## Markers verified

- BUG(orig) insasm: the node size is stored as a byte (`move.b d2,(a0)+`), so it truncates when
  asmlen > 239. True.
- BUG(orig) loadblitzlibs: only ':' is checked, but the default libsdir `'blitz2:userlibs/'` (line
  750) already ends in '/', which gives "//". True.
- BUG(orig) loadalib: `cmp.l #12,d0 / bcs` is unsigned, so a Read result of -1 passes. True.
- DIFF doea/makemode: mode 15 does `addq #4,a7 ; rts` in makemode. The C returns TRUE and doea
  returns early, which is equivalent. Both the a1<libat path and the other path drop the frame.
- DIFF fixlibs: the C restores errjmp, while the asm left errstack/errcont pointing at `.cont`. The
  C keeps the same continuation semantics (memlib stays set if only ffplib is missing).

## Routines checked and found correct

asmconst, copyconst, asmit (size parse, operand fetch, movea/addi redirection via temp1 high word,
check-routine call and swap incl. extraword long swap and asmlen swap, flag 1-5 handling, skipsrc,
src/dest doea, putit size field, libat guard), asmoff, asmfixer (all patch modes 5-11/16/17, flags
1/3/4, unsigned bhi checks, `.noflag` -> isabsl for 11), doea (mode->reg folding for modes >= 7,
nibble shifts), chkbyte, chkword, insasm, makemode, countpees, fetchlibps (start/rep/trailing logic,
a2 reuse per repeat, `a2 += d2`), fetchpees4/2/3/fetchpees (pmode save/restore, userp, notparerr),
handlep (pmode high byte), putap, pushap, putapincreg, putalp, handlevara, handlearr, sendtype,
baseadd, highadd, getap, resetlibs, usevars, uselib, uselib3 (cfetchmode, used bit, dirmode, max
block varoff, copy loop incl. overflow path, reloc loop incl. errchk skip / bmi skip / first-tokejsr
capture, handlejsrs pointer = entry-4, var slot + template patches for 65235/64535, uselib2 order),
fixjsrs (batch sentinels for list growth), tokecode (form walk, direct vs stub decisions, .penis
address, cutejsr/makelibsub args), freetlist, handlejsrs, maketjsr, uselib2, findlibnoerr (NULL =
asm Z for the caller at 15390), findlib, findtoke (unsigned range; firsttoken is UWORD, so the
equality walk is safe), loadlibgroup, loadlibs, loadblitzlibs (path building incl. empty libsdir;
namebufffib aliases namebuff, so namebuff+8 = fib_FileName), loadalib, fixlibs (sub table walk, token
nodes, maxinfo/nummaxs, error continuation), skiplibreg, aligna0, aligna1, freeblitzlibs, freelibs,
freemacs, freeasms, pokedata1b, pokedata1l, pokedata1.

Data: lc, ptype, pmode, fbase/pushadd/pushlast, tokeslist, deflibsat, acidlibsat, blitzlibsat,
ffplib. All initial values and encodings match.

Cross-chunk contracts checked: getea (mode, reg), evalconst3 (eos via *c), ampersand2, fetchvar,
eval/evalu2, cutejsr/makelibsub, addoff2, endininfo, the asmchkfn result convention (matches
main.c's abcdchk/addchk translation of the CC-only returns), LIBF_USED/LIBF_CONSTOK = byte-12 bits
7/6, rescode2 = &rescode[4].
