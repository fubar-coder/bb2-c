# Review 06 - src/eval.c vs blitz2.s 8451-10683 (+ convtable/operators 14693-14829)

## Findings

- 9555-9559(asm) shpoke: asm loads `moveq #3,d2` before `convtypef` when the type is not quick, so the result type of LSL/ASL/LSR/ASR on byte/word operands is LONG. The C returned the original type t (the comment called this "NB(orig)", which was wrong). Later code would then treat a long value as a word, e.g. a spurious ext.l after `w LSL 2`. -> FIXED (returns T_LONG unless t == T_QUICK). Severity: high
- 10169/10585(asm) revmask: the header comment pointed at blitz2.s:10002, which is wrong; the real locations are the `.loopsw` loops. -> FIXED (comment only). Severity: low
- 8693(asm) convtype: the DIFF is real. With from == 0 the asm reads before convtable (in the comdata/dir* longs) and jumps there; the C does nothing. No caller passes 0 that I could find. Kept as a DIFF. -> REPORTED (nothing to fix). Severity: low

## Markers checked (claims true)
- pevalu BUG(orig): it is the same as peval, because d3 is reloaded.
- eval2 BUG(orig): "=<" compiles as "<>".
- amigalib BUG(orig): the d4/d5 bset in .loop2 has no effect. d4 was already poked and d5 was saved on the stack.
- fetchasm: `not asmtype` toggles.
- makealab2: 16(a2) holds linenumat for an undefined label.
- doern: has no getchar.
- maineval: regat=6 is not restored.
- pushprec DIFF: it ends in the same pserr. Only the timing differs.
- eval5 DIFF (callregs, a5 = &maths - varoff, varoff is a signed WORD): this matches `sub.w d0,a5`.
- lasta6 restore: the C (word, then long) has the same final effect as the asm.

## Data tables
- convtable: 7x7 entries, identical to 14693-14699.
- operators: 25 entries plus the 0 terminator, identical in order, handlers and precedences (and the '<>', '<=', '>=' word constants). opeq..opge = [15]..[20] is correct.
- Templates were compared with the listing: overchk .code1-3, ceos, somecode, ststfix/pushut, mseq..cmp2, addrcode, usedcode (all inner label offsets), deca3.

## Routines checked and found correct
pokewda5s, pokewda5d, pokela5d, pokela5s, pokel, pokelc, pokewd, pokebyte, pokecode,
pokecode2, bytetoword, bytetolong, wordtolong, bytetoquick, wordtoquick, bytetofloat,
bytetof2, fdo2, wordtobyte, overchk, wordtofloat, longtobyte, longtoword, longtoquick,
longtofloat, quicktobyte, quicktob2, quicktoword, quicktolong, quicktofloat, qtof2,
floattobyte, floattoword, floattolong, floattolong2, floattoquick, stringtolong,
convtypef, convtype, getvname, dopusha3, dopulla3, sbtolong, arrevalchk, arrszchk,
sharreval, arreval, bakpevalu, pevalu, bakpeval, peval, bakevalu, evalu, evalu2,
bakeval, eval, maineval, maineval2, eval3, eval4, eval5 (incl. constant folding),
eval2, doop, pushprec, popprec, cmpit, cpoke, do{eq,ne,lt,le,gt,ge}{cmp,cmp2,b,w,l,f,s},
docflib, docflib2, doplusb/w/l/f, doplus2, doflib, addstrings, dominusb/w/l/f,
dotimesb/w/q/l/f, domylib, dopow, nocando, domodb/w/l/q/f, dodivb/w/l/q/f,
doandb/w/l, doorb/w/l, doeorb/w/l, doeor2, dols*/dolsr*/doasr* (now correct via shpoke),
dothebit, bittst, bitset, bitclr, bitchg, ritetod0, ritefromd0, getelement, negate,
bracket, notit, fetchpi, getelement2, fetchsize, fetchon, fetchoff, chkhex, chkbin,
fetchhex, fetchbin, fetchconst, ampersand, ampersand2, doern, fetchit, doused,
domaximum, doaddr, function, libfunction, amigalib, tstbras, pokemovem, savereg,
makesbase, litstring, makealab, makealab2, makelit, qmark, fetchqasm, fetchasm,
variable, varcont, savem, makefjsr, afunction, deca3, stvar, variable2.

tools/checkfile.sh src/eval.c: clean (no errors/warnings).
