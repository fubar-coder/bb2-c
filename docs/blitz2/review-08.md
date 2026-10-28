# Review 08 - src/tokens1.c vs blitz2.s 12136-13326

Reviewer 08. Compared routine by routine; templates re-generated with
`tools/asmlisting.py carray` (maxprep1, free, rescode, thetst1..negbr,
selpushb..endsel, endselstr..endselstrf2, pcchk/clrst) - all bytes and the
inner-label offsets in tokens1.h match. `tools/checkfile.sh src/tokens1.c`: clean.

## Findings

No translation errors found. Nothing fixed, nothing reported.

Points that I checked specifically and that are correct:
- getmaxel: `bpl syntaxerr` on bit 15, bclr before findtoke, the shift test before eval,
  mymaxlib set after eval, `cmp #8 / bcs` unsigned, the remainder `n<<9`, and the loop
  exiting with no extra lsl when the count is a multiple of 8. maxchk ($6500 $0008, from
  libcall.c lsldn+2) skips the 6-byte `jsr abs.l` correctly.
- compare/cifXX: numcom/strcom do not touch d1, so cifdo always gets d1=-1 when true. The
  C value -1/0 for comflag is correct. cifbras order and the collif index arithmetic
  (0..10, 3/7/11 rejected) match. ifchar leaves d0 unchanged when there is no match, so
  collif returning the char read before ifchar is correct.
- doelse: doendif2 keeps a0, so `brapc = pc` goes into the same node. byte 13 = low byte
  of kind.
- doshared: flag compare = low byte of the word at 6 (7(a2)). fetchvar gets d0 from
  addhere's `bra reget` (C: reget()). dbf copies 5 words = memcpy 10 bytes from +4.
- doread: dataletw/l layout (+2/+8 disp, +4 a2 form), dataletb/bl disp at +4, astring-1
  for locals, d0 kept across tokejsr/pokecode.
- dodata: range checks use the un-negated d3 (numd3, ULONG, so the compare is unsigned
  like bcc). The long/word poke choice uses the wanted type in both asm paths.
- dodc DIFF claim verified: insasm preserves d1. The asm placeholder is the copyconst
  count, which equals asmlen2 except for the indirect form (asmlen2=4).
- domacro: `.mclean` predecrement/len==1/bit7 order, the coolnest `bmi`, the End+char
  word quirk, the dropped return (dolineabort).
- dountil/doforever: the displacement is taken relative to the disp word, and the
  `tst.w / bpl` check is done as `(WORD)d >= 0`.
- fillbne/doendsel: `bhi` unsigned limit 32767. docase/doselect: type dispatch 1 / 2 /
  <7 / else (type 0 goes the long way, as in the asm).
- BUG(orig)/DIFF markers (domaxlen regat, dodcb count 0, doincbin leaks, domacro quirks
  and empty-text, doelse/fillbra/dodata placeholders, dounless unreferenced): all true.

## Routines checked and found correct
dodeftype, domaxlen, getmaxel, dofree, maxjsr, douse, doelse2, doelse, docerr, docnif,
docsif, collif, compare, cifeq, cifne, ciflt, cifle, cifgt, cifge, cifdont, cifdo,
dorestore, doread, getsize2, dodcb, dods, insasm2, dodc, doeven4, doeven8, doeven,
alignpc, pdt, dodata, doshared, doxinclude, doincbin, doinclude, doinc2, domacro,
macputw, macputb, dounless, dowhile, freereps, freesels, freeslist, freelist,
doendsel, dodefault, fillbne, fillbra, casechk, docase, doselect, doforever, dountil,
unlinkrep, doif, doifm, doif2 (incl. fallthrough into dothen); templates/data: maxprep1/2,
free/freef, mymaxlib, rescode/rescode2, datasize, coolnest, macline, thetst1..negbr,
firstsel, selpushb..endsel, endselstr..endselstrf2, pcchk/clrst, sellineat, iflineat,
cifbras.
