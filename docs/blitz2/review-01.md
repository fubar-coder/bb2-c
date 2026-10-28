# Review 01 - src/main.c (blitz2.s 1-1911, wbheader, asmmac) + src/glue.c

## Findings

- asmmac:464 bitchk: Dn source path did `d1 |= d2<<9` without changing d2; the asm shifts d2 itself (`lsl #8,d2; lsl #1,d2; or d2,d1`). The shifted d2 goes back to asmit ("all changes get used in assembly"). It has no effect today because the srcshift of BCHG/BCLR/BSET/BTST is 15/15, so doea ignores d2. -> FIXED (low)
- asmmac:495 aslchk2: same as above for the register-count form (`lsl #8,d2; lsl #1,d2; or d2,d1; bset #5,d1`). -> FIXED (low)
- wb:47-60 main (WB branch): the comment said a failed read leaves "<name>.info" in progfile. That is only true when Open fails; after a short Read progfile holds the partial .info bytes. Comment only. -> FIXED (low, cosmetic)
- glue.c callregs: the X flag is never captured, so the result never has CCR_X (glue.h defines it) set. No caller tests CCR_X today. -> REPORTED (glue.c is coordinator-owned; fix needed only if someone needs X) (low)

## Checked and found correct

Data / tables (checked by script against the asm where possible):
- tokens list (tok1..tok59, toktnum..toktnum33): names, help strings, numbers, links, even padding, last next=0
- asmmac `a` table, all 118 entries (opcode, src/dest masks, shift bytes, size word, checker, flag) and the `c` EndAsm node (fnum+118 -> toktnum)
- edstruct (28 bytes, chars ':".;'), menus (10 items + trailing 0, PRINT TOKENS only if !final), menuadds
- objects..addrega2: 274 words identical to the vasm listing except exg a4,a5 = $CB4C (Devpac, intended); all label #defines in main.h match the listing offsets
- tsthead/tstsize1/tstsize2/tstend/tstsize3/tstdone
- nomemreq + borders/texts/gadgets, verreq (+ version digits "1.95", release text), undobuffer (long aligned), libsdir, dpath/dname/xpath/xname sizes
- atokens (compile.c, 92 entries) and cifbras (tokens1.c): match 1294-1351

Routines:
- main/wbheader (wb_header, fromcli, wb_exit, dosopen: DIFFs as documented), begin + fail chain (fail3/fail2a/fail2b/fail2/fail1/failme/fail0)
- ed_* wrappers (screenclose, screenopen2, openreq, closereq, getinput, error, detokallz, tokall, ezfreq, refgads, getinput2, tokone, openio, closeio, actgad), pointeron, sleepon
- menu (menu_c), menu_d, loaddmod, nodload, dloaderr, menu1, docloseed, donocli, flushc
- menu0, menu10, procstart (fake seglist layout/alignment), runtheprog/runthing, calcvbr, flash, flash2, flushlibs, menu8
- checklib, makelibs, makeblitzlibs (sorted insert, compare and case folding match the asm), frlibslist
- asmmac: noerror/eaerror/szerror/swapem, movemchk (16x lsl/roxr reverse), movepchk, movechk, exgchk, roxlchk/rolchk/lslchk/aslchk, abcdchk (asm Z=1/d0 untouched = noerror for asmit), addchk, andichk
- glue.c: callregs (stack offsets, Scc capture/decode N/Z/V/C, a6 write-back), REGSTUB (menu/loadxtra/savextra/stopcode; restores all regs incl. handler changes, `tst.l d0`. ted ignores the flags after these callbacks), ffpcall, getvbr/vbr_super (movec vbr,d0 = $4e7a,$0801; rte; calcvbr only calls it on 020+)
