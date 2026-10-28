# Review 05: src/parse.c vs blitz2.s 6740-8450

Reviewer 05. I compared each routine with the asm. Templates came from
build/amiga/programs/blitz2/ref/blitz2.lst (zarrchk, arrmul1..listarfix, brchkcode0). I checked
the objects[] offsets in main.h (numtoa2, endarray, pusha2, pulla2, movea0,
adda0a0, movea5a0, movea0a0, addqa0, leaa0a0, addrega2), and they all
match. I compared objicon byte for byte with obj.info (383 bytes, same). I
also extracted all 127 error stub strings by script and compared them with
the asm `dc.b` text: they are identical. opterr's string is the
concatenation of the unpatched opchars/opchars2, which is correct.

## Findings

- 7746 findlast: BUG(orig) in the asm. `.loop beq .here / move.l d0,a2 / bra .loop` re-tests the flags left by the first `move.l (a2),d0`, because movea sets no flags. So the asm loops forever on a non-empty list, while the C walks to the last node. findlast is never called anywhere in blitz2.s, so this is dead code. -> FIXED (documented as BUG(orig)/DIFF in a comment, no code change), low
- 7188 gchar2: the DIFF claim (a token with low byte < 32 inside quotes makes the asm index gs with a negative word) is true. The asm `move.b` also replaces only the low byte. This cannot happen with tokenised source. -> no action, low
- 6800 savefile: the DIFF claims are true. The asm never restores errstack/errcont; C restores errjmp. The asm writes up to 3 bytes past the label node in the symbol hunk; C pads with zeros. The `BUG(orig): never cleared` note on `anyerrs = -1` is literally true but may be intentional (it forces state after Create Executable). The behaviour matches the asm. -> no action, low (cosmetic)
- 7042 getchar .cmake: `BUG(orig): cmake stays set` after syntaxerr is true. The asm `bne syntaxerr` comes before `not cmake`, and its movem frame is abandoned. -> no action
- 7723 makename4: `BUG(orig): no length check` is true. -> no action
- 7822 findlab: `BUG(orig)`: with len==0, dbf with d0=-1 runs 65536 times. The C loop (cnt=0xffff, `cnt-- == 0`) does the same count. -> no action
- 6895 savefile .dodeb: `d5-d4 never true` is correct. d4 is a multiple of 4, so lsr/lsl #2 gives the same value. -> no action

I found no translation errors that change behaviour.

## Checked and found correct

calcstatic, calcstatic2, datastart, mywrite, execerr, savefile (reloc fixup
`tst.w`/bmi, hunk header/code/reloc32 writes via tsthead layout, debug hunk
label filter `refs&1 && refs!=1`, icon Lock(-2)/Open/Write), addoff, addoff2,
killoffs, getchar (order: lastchar/0 -> token $801c-$801f before comflag
test, .incnest, .celse only at connest 0, .cend signed `bpl` with WORD
connest and WORD *concomsp stack, nociferr; .done2 comflag skip, '~' cmake
incl. 24-byte cmakebuff, makelong returning the NUL -> 1 marker, namebuff
save/restore, markers 2 -> usedfrom and 1 -> cmakea5; prevloc/oldqflag set
on every re-entry), getcharb (token = 2 bytes, '"' toggles, ';' outside
quotes skips to NUL and sets lastchar 0), getchar2, gs table, getchar3,
gchar2/eos (Z flag after `move.b gs` == mapped value 0; $8022 and ':'
only outside quotes), reget, bakup, storeloc, resloc, err (dirmode quiet
path without pointeron; ezerr skips cfetchmode/constmode/droplocals/anyerrs/
menuret/endstop but not pointeron or the editor report), asmerr2/asmerr
(asmbuff is a pointer variable, line at 12), opterr, interr2, noliberr
(offset 21), notokerr (offset 19), macenderr, all plain error stubs,
tstalpha (unsigned bhi/bcs), tstnum, tstnum2, tstlab (loclabch 39),
makename2, makename, makename3, makename4, addhere2, addhere (size byte =
nameoff+len+1), findlab (size byte at nameoff-1, continue-from-node on
mismatch, *last on miss), find wrappers (d7 values 19/5/9/13/19/11/33/17/15),
freeprocvs, freevars, freelabels, freepends, freeprocs, freetypes
(`cmp #$ff,a3` == 0xff), fetchvar, fvarback, fvarlocal (varoff/locvaroff
swap), fvaradd (byte type skips alignment, `addq` only in global mode,
NewType var = 4 bytes, local offsets negative), fvarfound, addd0a0 (unsigned
`< 9`, 8 -> addq #8 field 0, template patched in place, clobbered d3
returned), makelmul (all 11 opcodes and register fields, displacement
off+2/off = caller's 4(a7) varoff slot), calcvar (list `()` path, .notalist
stack frame offsets: 8(a7) varmode, 2(a7) ndims, (a7) varoff +4/-8 local,
debug zarrchk/arrerr, `.ardone` add.l Dr,a2 + listarfix; .normalar
pointer/`\` decisions $8000/$c000 + .pfix; .notarr simpvar/simpvar2;
`\field` loop with lastoffset, `[n]` path with push/pull a2, brchkcode
patch and the 3 pokes, muld1, addrega2; .pointer movea0/movea0a0/adda0a0
and restart at .loopm1 vs .realdone -> .loop0 without d3 reset; .done low
byte merge + addd0a0).

checkfile: tools/checkfile.sh src/parse.c reports no errors or warnings.
