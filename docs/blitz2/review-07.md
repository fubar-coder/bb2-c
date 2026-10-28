# Review 07 - src/libcall.c vs blitz2.s 10684-12135

Result: no translation errors found. `tools/checkfile.sh src/libcall.c` is clean.
I checked every routine against the asm: branch conditions (signed vs unsigned,
.b/.w/.l), dbf counts, emitted opcodes and register fields, and which flags
come back from the FFP calls (SPFix V/Z). I compared the templates
(prepd1/putidata1/prepint, freemax.., tstmode, lsldn/maxchk/maxdo/stackopuse/
stackop, .theretest) with `tools/asmlisting.py` and they match byte for byte.
I also checked the calls into other chunks (addoff/addoff2 preserve a0,
findtoke preserves a1, datastart, uselib, pokecode2, doallocmem, callregs).

## Findings

- 11743-11753 toreg (max-item path, debug): after `bsr tokejsr` (maxerr) the asm reads `-2(a1)` again. If maxerr's sub had register parameters, the nested toreg/uselib would overwrite a1. C uses `e`, which it computed before the call. The two agree as long as maxerr has no register list, which looks likely (doajsr keeps a1, findtoke keeps a1). -> REPORTED (informational, no change; asm latent issue), low
- 12121 strcom `.lt`: the asm does `cmp #1,d0` on the whole word, so the high byte left over from earlier code decides the result. C always returns "less". This is already marked as DIFF and the claim is correct. -> REPORTED (documented DIFF, no change), low
- 11880 evalconst2: C enters the generated code with a6 = MathBase, d2 = 3 and the other registers 0. The asm entered with whatever registers it had (a6 is probably execbase after flushc). This is already marked as DIFF. It is harmless as long as the const-mode libs do not depend on those registers. -> REPORTED (documented DIFF, no change), low

## BUG(orig)/DIFF claims verified as true
- consttype2 `.quick2` uses d0 without loading it on the paths "type 0 + float" and "type w + float" (it still holds the char). A zero or negative fraction also leads to numdofloat.
- muld1: d1 == 256 emits `lsl #8` twice (count field 0 = 8, giving x65536).
- fetchpoint2: "1." without fraction digits is a syntaxerr, and with a zero fraction the E exponent is not parsed (`.pskip`).
- doajsr2: a nested makebtst clears the global `inline`.
- pushput: when the buffer overflows, pushat is not freed.
- toreg `.sendmd` tests bit 7 for An (makeregop tests bit 4), so the a6 test there cannot be reached.
- makeint drops its return address and goes to numdofloat; C does this with a FALSE return plus `goto numdofloat`.
- doincdir: no length check, and ':' is copied.

## Routines checked and found correct
fetchdata (.datado), fetchfrac, fetchnum, fetchpoint, fetchpoint2, dothee,
getsign, facts, fetchdec, dfetch, fetchnum3, makeint, fetchnum2, constotype,
consttype2, numdofloat, pushstart, pushdo, pushput, muld1 (.once/.domul),
makeinits, prepd1/putidata1/prepint, makessize, makevsize, makesize,
findhilib, findlolib, makefinits, freemax templates, chkstak, tokejsr2,
tokejsr, userjsr, cutejsr, makelibsub, makelsub2, makelsub3, doajsrrout,
doajsr, doajsr2, makebtst, getbbase, tstmode, blitzoff/blitzmode init,
fetchregs, toreg (.sendmd, .notnmax, max-item shifts, .skippy1/.theretest,
.norm), lsldn/maxchk/maxdo/stackop templates, makeregop, ifchar, evalconst,
evalconst2/evalconst3, oldlibat, doincdir, makeiname, readinc, readpend,
imakeend, closeinc, freexincs, freeincs, pokedata2l, pokedata2, pokedata2b,
getsp/getsp2, numcom, strcom.
