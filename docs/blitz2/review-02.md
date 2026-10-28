# Review 02 - src/compile.c vs blitz2.s 1912-3290

## Findings

- 2096-2103 nextline: after `move.l (a0),a0 / move.l a0,thisitem / lea 9(a0),a5` the asm returns the Z flag of the `move.l`, so a NULL next node means Z=1 ("no more lines"). The C always returned TRUE. -> FIXED (`return thisitem != NULL`), severity low (only happens if linesleft and the list do not agree)
- 2271/2280-2290 expandmax: on the `` `{expr} `` path d0 = d3 (long). `move numarg,d0` then replaces only the low word, so `divu #10,d0` divides (d3 upper word << 16 | numarg). If the overflow flag is set, d0 is left unchanged. The C divided numarg alone. This shows up for `` `{n} `` with n >= 65536 and a low word of 0 or >= $8001. -> FIXED (the upper word is kept, and the divu overflow case is handled), severity low

## DIFF/BUG markers checked (claims true)
- compile: divu with numlines==0 or overflow -> $FFFF (DIFF ok)
- dopass: first nextline result untested (BUG(orig) ok); `pokel(0)` instead of stale d1 (DIFF ok, it is overwritten later or suppressed by nomemleft)
- expandmax: no macrobuff end check and no overflow check for the `@ digits (BUG(orig) ok)
- smode: return flag instead of addq #4,a7 (DIFF ok, equivalent: the mode handler returns to doline)
- handlelib: countpees(lastchar) (DIFF ok, countpees restores the char)
- handleconst: empty name not checked (BUG(orig) ok)

## Checked and found correct
compile, qfree, freeemup, fralltypes, frallvars, freeifs, nextline (include and main stripe, sign extension of the word adds), hexascii, expandmax (scan, quote skip, params, dbf loop count with inner subq, backtick escapes, .notnum ranges, param copy, rest of line, myoline free), freembuff, findit, isitopen, isitclose, stopreq/Border/IntuiText/Gadget data (pens, chains, XY arrays, Requester padding), compiling/executing strings, stopcode, endstop, setcvars (all fields), openstopreq, dopass (order, titleadd[1..4], debug libs, passjmp re-entry = passnxt, end sequence, gotocode/endjmpat), eopcode, pointchk, errchx2, errchx (order), chxerr4, calcmaxs (bit15 test, offsets -6/-4/-2, lsl.w/mulu), doline (Else, ':' loop, un-then), createlab (d2 incl. loclabch, unchanged-length -> syntaxerr), prepstack, chkrealstop, chkstop, gostop, maketrap, debugstuff (record layout, link patch), undodebug, setsvars, smode, dostatement, handlechar, handleascii, handleeol, handlelocal, handlemouse, handlelabel, handlelab2 (ref patching, Restore bit0, FreeMem 12), handleconst, handleproc (signed rettype test, params at +22), handletoken (asm range, no-debug list 13/18/19/26/35..42/56/57, remap tnum-fnum, fnum+10/15, index range -> flash), atokens table (92 entries, order matches 1298-1351), modetable, handlelib, dolibtoke (form search bcs/divu remainder, flags<<8, preplibst/movenumrep/fixlibst patching, addq encoding $504F|(n&7)<<9, save/restore), sizespec2 ('$' skips bakup, >=256 unsigned long test), sizespec ($7000|type, regat++)

## Notes (not errors)
- dolineabort stays set if an error longjmps out between domacro setting it and doline testing it. In practice this cannot happen, because domacro only does getchar3 afterwards.
