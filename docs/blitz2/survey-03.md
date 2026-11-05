# Survey chunk 03 - blitz2.s lines 3291-4999

Range: `subs` (3292) .. end of `getrest` (4998). `asmconst` (5000) is NOT in range.
Assembled with no branch optimisation: forward `beq`/`bra` without `.s` are `.w` (4 bytes).
Template bytes below were checked against the shipped `blitz2` binary.

Notation: "Z-in" = handler entered with flags from getchar3 (Z=1 if d0 is 0 / ':' / Else token).
`err` routines (`xxxerr`) = `bsr err` + inline text; never return (longjmp via errstack/errcont).

## Global context needed to read this chunk

- `pc` (14900) is a GLOBAL `dc.l`: current output pointer into the object buffer (absolute address),
  NOT the CPU program counter. `move.l pc,x` = "remember current output address".
- `pokewd` (8476): emits d1.w at `pc` (word aligns pc first, the skipped pad byte is not
  written: BUG(orig) #969, the port writes it as 0), sets `nomemleft=-1` on overflow
  (pc >= libat), keeps pc advancing; updates `bigpc`. `pokel`: emits d1.l (high word first).
  `pokecode` a0=start,a1=end: copies words [a0,a1) (always at least one word).
  `pokewda5s`: pokewd, but clears bit0 if `varmode` (turns `(a5)` into `(a4)` base reg).
- `addoff` (6997): records reloc at `pc-pcat` (call BEFORE poking the long). `addoff2`: records d2.
  Both alloc 8-byte node {next.l, offset.l} on `firstoff`, `numoffs++`; preserve all regs.
- `tokejsr` (11434): d1 = internal token number -> emits call to that lib routine. Preserves d2/a3.
  This chunk assumes it emits exactly a 6-byte `jsr abs.l` (see popreturn template).
- Statement dispatch (3164): `move.l atokens[tok-1],-(a7) ; bra getchar3` -> every doXXX is entered
  with d0 = first char/token after the keyword and Z from getchar3; they return by rts to the
  dispatcher's caller (`dostatement`). Many end with a tail `bra getchar3` to leave d0 = next char.
- `blitzmode` (11637) is a WORD; code here uses `bset/bclr #n,blitzmode` = BYTE ops on its HIGH byte:
  bit7 of high byte (= word sign) set => Blitz mode, bit0 of high byte (= word bit8) => "mode
  statement seen". `btst #7,blitzmode` tests sign.
- `debugga` (2518) is `dc` (word) but used as a byte flag (`sne/sf/tst.b` on its high byte).

## A. Routines

### Statement handlers (all entered Z-in, d0=next char; return rts)

| name (line) | notes |
|---|---|
| dorunerrson (3294) | `debugga.b = (optreqga10+13 bit7) ? $FF : 0` (gadget SELECTED flag of "runtime errors" option). Also called at end of pass (2616). |
| dorunerrsoff (3300) | `debugga.b = 0`. |
| dousepath (3306) | Z -> syntaxerr. Copies d0 bytes (token => d0<0 => syntaxerr) into `usedpath` via getchar3 until end; terminates with byte 2 (getchar sees 2 => `a5=usedfrom`, see getvname `\`). No length check (128). Returns d0 = terminator char. |
| doexchange (3335) | Exchange a,b. excget; needs ','; `srctype.b=d2`; emit `pusha2` ($2F0A); getchar3; excget; types equal else excerr2; emit `pulla1` ($225F); tail `bra pokecode` swapb/w/l by d2.b (<2 byte, =2 word, else long). |
| doallocmem (3379) | NOT a statement: `jmp comdata+88` (editor `extalloc` = `jsr AllocMem(a6)`). in: d0 size, d1 MEMF flags, **a6 = ExecBase required**. out d0 = mem. On failure editor's `outmem` longjmps to editor (never returns 0). Called from ~8 places outside. |
| dowbstartup (3383) | tail `tokejsr wbstart($bf00)`. |
| dopop (3386) | d0 = token after Pop: $8006 (Gosub) -> popreturn; $8016 (For) -> popnext2; $800E (Select) -> popselect2; $800B(If) $800C(While) $8014(tok 20) $8000+tnum+8 (Repeat) -> just `bra getchar3`; else syntaxerr. |
| dorepeat (3504) | `replineat=linenumat`; alloc 8 (a6=exec), node {next.l=firstrep, pc.l=pc}; push on `firstrep`. No getchar (d0 left as is). Popped by dountil (elsewhere); errchx checks firstrep. |
| dolibjsr (3571) | `tst d0` then gettokeps; emit $4EB9, addoff, long `((d5|$8000)<<16)|d4` (placeholder: Amiga-lib token ref, resolved elsewhere). |
| doblibjsr (3582) | same with `|$C000` (Blitz-lib). |
| dosysjsr (3593) | evalconst, d3 < $10000 else tokeerr; tail tokejsr d1=d3. |
| dotokejsr (3599) | gettokeps (uses N from getchar3, no tst); emit $4EB9; d3=pc (operand addr); if Blitz mode `bset #14,d5`; maketjsr (records d3 in tokeslist); addoff; tail pokel d1 (d1 = leftover from findtoke = lib first-token #; placeholder). |
| dovwait (3610) | Z -> emit `moveq #0,d0` ($7000); else d2=2, bakeval (word), emit `fvwait` ($5340 subq #1,d0). Then tail tokejsr intvwait ($c104). |
| doblitz (3632) | blitzmode high byte |= $81; tokejsr goblitz ($c200); if debugga emit `move.w #-1,blitzoff(a5)` = $3B7C,$FFFF,getbbase(); tail `bra debugmode`. |
| doamiga (3649) / doqamiga (3654) | d1=goamiga($c201)/goqamiga($c202) -> `toamode` (3658, shared tail, 3rd entry): high byte bit0 set, bit7 clr; tokejsr d1; if debugga emit `clr.w blitzoff(a5)` = $426D,getbbase(); tail debugmode. doamiga also called at end of compile (2644) if in Blitz mode. |
| debugmode (3626) | helper: if debugga.b: tail tokejsr $d500; else rts. |
| doclrint (3674) | evalconst, d3<14 else interr4; emit `moveq #lvl,d0`; tail tokejsr clrint ($c101). |
| doclrerr (3688) | tail tokejsr clrerr ($d202). |
| doerrfail (3700) | tail tokejsr errfail ($d205). |
| doseterr (3703) | procmode!=0 -> errerr1; inerr!=0 -> errerr2; `inerr=~inerr` (=$FFFF). Emit `$203C` (move.l #x,d0) + addoff + long (garbage), `errcode` = addr of that long; tokejsr seterr ($d201); emit `$4EF9` (jmp) + addoff + long (garbage), `errjmp` = addr of that long; if !nomemleft: `*errcode = pc` (handler starts here). rts (no getchar). |
| doendseterr (3691) | jumped to from doend (13966, token $8001+tnum "End SetErr"). !inerr -> errerr3; inerr=0; if !nomemleft `*errjmp = pc`; tail getchar3. |
| dosetint (3728) | d1=-1 then falls into `setint2` (3730; 2nd entry, d1 = intstring value; no outside caller found). errchx; `intstring=d1`; `intlineat=linenumat`; evalconst lvl<14 else interr4; `intsused=$FF..` (st, byte); `intlevel=lvl`. Emit: `moveq #lvl,d0`; `$223C`+addoff+long (move.l #handler,d1; operand addr saved on stack); tokejsr setint($c100); `$4EF9`+addoff+long (jmp past handler, `intjmpat` = operand addr, patched by End SetInt at 13958); if !nomemleft patch move.l operand = pc; emit `intstart0` long ($48E7 3F38). If debugga: `maketrap(2)` (= $4E42 trap #2), addoff, `intcleanat=pc`, pokel (garbage, patched later 13931), pokewd intlevel, tokejsr gosup($c30b). Then `uselib(65235)`; tokejsr newint($c102); `lasta6=-1`; `intline=linenumat`. rts. |
| doputreg (4665) | PutReg Rn,var. getreg2 (d4=0..15); ',' else syntaxerr; emit $48E7FFFE (movem.l d0-d7/a0-a6,-(a7)), $2F00|d4 (move.l Rn,-(a7)); getchar3; getvname/fetchvar/calcvar; pointer(bit14) or type 0 or 7 -> type 3; if !bit15 emit $45ED,d3 (lea d3(a5),a2 - note: plain pokewd, no a4 fix); emit $201F (move.l (a7)+,d0); store: type1 $1480 (move.b d0,(a2)), type2 $3480 (move.w), else $2480 (move.l); tail pokel $4CDF7FFF. |
| dosetreg (4707) | SetReg Rn,expr. getreg2; ','; regmovems(d4); d2=3 eval (long, result in d0 at runtime); reg 0: nothing; 1-7: $2000|n<<9 (move.l d0,Dn); 8-15: $2040|(n-8)<<9 (movea.l d0,An); tail dounmove. |

### Pop helpers (also called from outside with a2 preset)

| name | in/out |
|---|---|
| popreturn (3403) | no debug: emit `.sfix` $584F (addq #4,a7). debug: pokecode `.rfix..rfixf` (10 bytes: $0C6F $6753 $0004 $6700 $0008 = `cmp.w #"gS",4(a7); beq.w +8`), tokejsr chkret ($c305), emit `.rfix2` $5C4F (addq #6,a7). Tail getchar3. |
| popnext2 (3424) | firstfor==0 -> noforerr; a2=firstfor; getchar3; popnext; falls into popnext3. |
| popnext3 (3430) | in a2 = for struct (12(a2).w = var type). Emits stack drop: debug: type<3 `pokel nextfixw2` (lea 10(a7),a7) else `pokel nextfixl2` (lea 14); no debug: type<3 `pokewd nextfixw` (addq #8,a7) else `pokel nextfixl` (lea 12). Called from donext (14211). Templates live at 985-991 (other chunk). |
| popnext (3446) | in a2. If debugga: tokejsr snxtchk($c306) (+1 if type>=3); preserves a2. Out d0 = 12(a2). Called from 14163. |
| popselect2 (3460) | firstsel==0 -> eselerr; a2=firstsel; getchar3; falls into popselect. |
| popselect (3465) | in a2 = select struct, 12(a2).w = type (d2). Debug: tokejsr casechkw ($c313, +1 if type>=3). type<3: d1=2, <7: d1=4, (+2 if debug), emit `endsel | d1<<9` (endsel=$504F addq #8,a7 -> addq #n,a7). type>=7 (string): pokecode endselstr..endselstrf (or ..endselstrf2 if debug), tail tokejsr $c003 (global freemem). Called from doendsel (13061). Preserves d0 only if tokejsr does. |

### Helpers

| name (line) | in ; out ; notes |
|---|---|
| excget (3363) | getvname/fetchvar/calcvar; bit14 (pointer) -> d2.b=4; d2.b==0 -> excerr; if !bit15: emit leaamp word via pokewda5s ($45ED or $45EC), then d3. out d2 type, d0 next char. |
| endininfo (3522) | in a0 = C string ; out Z=1 if ends with ".info" (case-insens: chars `or #32`, NUL->' ', compares last 6 as ".info "). Clobbers a0 (past NUL), a1, d0. Callers 1842, 6454. Uses `infochk` 6-byte shift window. |
| gettokeps (3541) | in d0 + N flag (caller does `tst d0`). N=1 (token): `(d0 & userand $7F80)==0` -> tokeerr; d4 = d0&$7FFF; getchar3. N=0: evalconst, d3<=65535 && d6==0 else tokeerr; d4=d3. Optional `,form`: evalconst2 same checks -> d5 (else d5=0). out d4=toke, d5=form, d0 next char. |
| errchx (2719, NOT in range; used) | - |
| intvar block | see C. |
| directtrap (3805) | **trap #15 exception handler** (vector installed at VBR+$BC, line 1705). Supervisor: frame (a7)=SR, 2(a7)=PC. PC += 18 (skips 18 bytes inline params after the `trap #15`), saves PC to `ret15add`, SR to `ret15cc`, sets frame PC=`directdo`, `rte`. |
| directdo (3819) | user mode, never "returns" normally. Builds fake rtr frame [ccr.w, ret15add.l], movem.l d0-d7/a0-a6 (60 bytes). `errcont=dircont`, `errstack=a7`, `dirmode=-1`. Reads params backwards from ret15add: -2 .w -> blitzmode; -6 .l cleared (result); -10 -> firstglob; -14 -> firstlocal; -18 -> a0 = source line ptr. procmode = (firstglob|firstlocal)?$FFFF:0. Calls comdata+56 (`tokone`, a0=line, tokenises in place), a5=line. `pc=dirpcat`, nomemleft=0, lasta6=-1, `doline`, procmode=0, errchx; emit `$4EF9, ret15add` (jmp back). nomemleft -> nodirmem (err). Else overwrite stacked return PC (60+2(a7)) with `dirpcat` => rtr enters generated code. |
| direxit (3864) | dirmode=0; stacked ccr = ret15cc; movem restore; flushc; `rtr`. |
| dircont (3871) | errcont target (err .quiet path: d0 = inline error text ptr, a7=errstack). Store d0 at ret15add-6; bra direxit (returns to ret15add, i.e. after the 18 param bytes). |
| stamigalib (3879) | entered by `bne` from handlelib (3174) for statement tokens of Amiga-lib type; a3 = toke sub struct, d0 = current char. uselib(6(a3).w) -> a1 lib; push 10(a1).w (libbase a5-offset), 8(a3).w (LVO), a3+10 (reg byte list, terminated by byte<0; bit4 set = An with bits0-2, else Dn). For each reg byte: d2=3 peval (pushes long at runtime), separated by ','; if fewer args than regs -> syntaxerr. No args: getchar3. Then emit pops in reverse reg order: $201F|n<<9 (move.l (a7)+,Dn) or $205F|n<<9 (movea.l (a7)+,An). Patch & emit templates `libbase` (move.l d16(a5),a6 = $2C6D,off) and `libjsr` (jsr d16(a6) = $4EAE,lvo) - **self-modifies libbase+2/libjsr+2** (C: build words). |
| alloxinc (3936) | sizes = max* * 1.5 -> bra newallox. Called 1992; out NE if allocated. |
| alloxadj (3956) | sizes = used: bigpc-pcat (+1024 unless makeexec), lib-libat, data1-data1at, data2-data2at; falls into newallox. Called 1964. |
| newallox (3969) | makeallox(d2..d5); NE -> rts; else tail makeiallox (retry with old max*) -> its flags. (So NE even if fallback succeeded.) |
| freeallox (3974) | a6=exec; if objlen: objlen=0, tail FreeMem(pcat,objlen). Called 1462. |
| makeiallox (3982) | d2..d5 = maxpc,maxlib,maxdata1,maxdata2; falls into makeallox. Called 1415, 15407, 15595. |
| makeallox (3988) | in d2=pc,d3=lib,d4=data1,d5=data2 sizes. anyerrs=-1; freeallox (a6=exec stays); round each up to even; doallocmem(total, $10001 PUBLIC|CLEAR). Fail: Z=1 (dead in practice). OK: objlen,max*=sizes, pcat,libat,data1at,data2at,allat=end; NE. Clobbers d0,d1,d6,a1,a6. |
| menu3 (4034) | under `ifeq final`; final=-1 -> NOT assembled. Debug: print token names/help to PAR:. Skip in C (or #if 0). |
| makelong (4074) | in d2.l unsigned, a0 buf ; writes decimal digits (no leading zeros, "0" for 0), NUL; out a0 -> NUL. Clobbers d1-d4. 6 outside callers. |
| menu5 (4105) | menu entry "Make Resident" (menuadds table 741). Requires firstmacro or firsttype or a label with 4(a2)==1, else rts. comdata+24 (`ezfreq` file requester: a2=title restext, a0=respath, a1=resname; Z=1 cancel, d0=filename). Open(d0,1006); sleepon; **killoffs** (discards compile reloc list); write 32-byte `tsthead`; d6=0 (file data offset); restypes, resmacs, resconsts; pad to long with bytes from `hello`; if numoffs: write `tstend` 12 bytes ($3EC, tstsize3=numoffs, 0), each offset node's 4(a2) (LIFO order), `zero`; write `tstdone` ($3F2); patch longs at file offset 20 and 28 with size/4 (via `temp1`, Seek OFFSET_BEGINNING=-1); Close; tail pointeron. a6=dos throughout (writes use `jsr write(a6)`). |
| writelist (4214) | a2=first, d5=len-byte offset; writeitem each. (unused) |
| writeitem (4223) | in a2 item, d5 = offset of length byte, d6 file offset, d7 fh, a6 dos. len=(b[d5]+1)&~1; if next!=0: addoff2(d6) and temporarily next=d6+len (assumes next item written right after); write len bytes; restore next; d6+=len. |
| resoff (4241) | d2=d6; tail addoff2. (unused) |
| writeoffs (4244) | a2 = first field (newtype member) list. For each: d1 = index of 10(a2).l (type ptr) in `alltypes` chain (0-based; list length if absent); temporarily store d1.w in HIGH word of 10(a2); writeitem (d5=14 set by caller); restore. |
| countem (4265) | count list a2 -> temp1.w, write 2 bytes, d6+=2. tail `jmp write(a6)`. |
| resmacs (4278) | macros: count.w, then per macro: struct (len 12(a3) rounded) with 4(a3) := offset of text (if 8(a3)!=0; reloc d5+4) and next := offset after text (if !=0; reloc d5); then text (8(a3) rounded). Restores pointers. |
| restypes (4324) | types: count.w, per type: fields via writeoffs(d5=14), then type struct (len 10(a3) rounded) with 4(a3) := offset of its first field (reloc d6+4). Type `next` written raw (fixed by loader). |
| resconsts (4353) | count.w of firstlabel items with 4(a2)==1, then each (len 18(a2) rounded), next := following offset if !=0 (reloc). |
| loadres (4393) | **setjmp-like**: errstack=a7, errcont=`.next` (mid-loop resume label), ezerr=-1. freeemup; freeres (a6=dos). For 8 entries of `resbuff` (68 bytes: name[64], seglist.l @64), name nonempty: LoadSeg -> 64(a5); 0 -> reserr (err -> resumes at .next with a5/d6 preserved!). a3 = BADDR(seg)+4; fixtypes, fixmacs, fixconsts. Then errcont=`freeres` (clash error -> freeres then rts to loadres's caller); checkclash(firsttype,11), (firstmacro,13), (firstconst,25). ezerr NOT reset. Called 15408, 15599. |
| checkclash (4441) | a3 = first item, d7 = name offset. For each a4: copy name to namebuff (d2=len); findlab from a3 repeatedly; found item != a4 -> clasherr. Note: firstconst is always empty (findconst 7797 has no caller; constants are in firstlabel, name at 19, resident constants appended via lastrescon), so the (firstconst,25) check never sees anything: clashing constants between residents are never detected (the offset 25 vs 9 is irrelevant). Clobbers a0-a2,a4,d0-d2. |
| freeres (4460) | a6=dos; lastrestype=&firsttype, lastresmac=&firstmacro, lastrescon=&firstlabel, and clears those heads; rescnt=0; falls into freeres2. Called 1457. |
| freeres2 (4473) | 2nd entry (called 15345): UnLoadSeg each resbuff[i].seg, clear it. a6 must be dos. |
| fixconsts (4483) | a3 = cursor: count.w; link *lastrescon=a3; follow next count-1 times; lastrescon=last; a3 += len(last) (18). Last item's next left as written (points past data). out a3 advanced. |
| fixmacs (4500) | same for macros: link via lastresmac; a3 past last struct+text (12(a3), 8(a3)). |
| fixtypes (4521) | a3 cursor: count.w; per type: follow field next chain to last field (next==0), skip it (len 14(a3)) -> type; *a2=a3 link, a2=type; skip type (len 10). Breaks if a type has no fields. 2nd pass over new types: for each field 10(a1).w = index -> idx<7: walk alltypes idx times; else idx-=rescnt then walk; store ptr in 10(a1).l. rescnt += count; lastrestype=last. |
| runerropts (4577) | push 7 words optreq2gaN+12 (gadget flags), comdata+0 openreq(a0=optreq2); loop comdata+8 getinput until d7==-1 && d6>=8; d6==8 OK (`lea 14(a7),a7` drop saved), d6>8 Cancel (restore 7 words). Called 15560. |
| makeasmtab (4609) | at startup (1366). a0 = tokens list, skip lnum-1 nodes; for tnum-fnum nodes: asmpnts[i] = address after name\0 help\0, word-aligned (asm mnemonic data from asmmac). Clobbers a0-a2,d0. |
| getimm (4631) | asmconst; d5=11. |
| regmovems (4638) | in d4 = reg 0..15: emit $48E7, mask $FFFF with bit (15-d4) clr; `unmove` = $FFFF with bit d4 clr. |
| dounmove (4651) | emit $4CDF, unmove (tail pokewd). |
| getreg (4730) / getreg2 (4731) | getreg: getchar3 first. d0|32: 'd' -> tail getan3; 'a' -> getan3, d4+=8; else syntaxerr. out d4 0..15, d0 next char, Z from getchar3. |
| getan (4740) / getan2 (4742) / getan3 (4744) | getan: getchar3, lowercase; getan2: d0=='a' else syntaxerr; getan3: getchar3, digit 0-7 -> d4 else syntaxerr; tail getchar3. |
| getmovem2 (4752) / getmovem (4753) | in d4 = ASCII digit of first reg (getmovem2 adds 8 = An), d0 '-' or '/'. Builds mask (bit n = d0..d7=0..7, a0..a7=8..15) via getreg/tstend2; out d4=mask, d5=15. |
| getea (4783) | asm EA parser. in a5 src (a4=a5 saved for backtrack). getchar3 first. out d5 mode, d4 reg/mask, d0 next char; extraword for index; expression text via asmconst. d5: 0 Dn,1 An (sp->7),2 (An),3 (An)+,4 -(An),5 d16(An),6 d8(An,Xn),7 abs.w,8 abs.l,9 d16(PC),10 d8(PC,Xn),11 #imm,12 CCR,13 SR,14 USP,15 reglist. Fallback `.notccr`: a5=a4, asmconst, then `.w/.l/(An/(pc`. Called 5115, 5124. |
| tstend2 (4954) | Z=1 if d0 in {0, ':', ','}. |
| getrest (4961) | in d0 after `d(An` / `d(PC`: ')' -> d5=5; ',' Xn[.w/.l] ')' -> extraword = (A?$8000)|reg<<12|(.l?$800), d5=6. tail getchar3. |

## B. Register conventions

- d0 = current char/token (word; token = $8000|n, lib tokens have bits in $7F00), flags from getchar3.
- a5 = source char pointer (parser); getea saves it in a4 for backtracking.
- d1 = word/long to emit (pokewd/pokel), or token for tokejsr / lib number for uselib.
- d2 = expected/obtained type (eval in: 1 byte,2 word,3 long; calcvar out: low byte type, bit15
  = address in a2 generated, bit14 = pointer); d3 = constant value (evalconst) / var offset from a5;
  d6 = 0 int / !=0 float after evalconst.
- asm parsing: d4 = reg / mask, d5 = ea mode code.
- resident writer: d7 = file handle, d6 = current file offset, d5 = length-byte offset, a6 = DOSBase.
- resident loader: a3 = cursor in loaded hunk, a5 = resbuff entry, d6 = loop count.
- a6 = ExecBase required by doallocmem (editor extalloc does `jsr AllocMem(a6)`).

## C. Globals defined in range

| label | size | init | purpose |
|---|---|---|---|
| usedpath (3304) | ds.b 128 | 0 | UsePath prefix, terminated by byte 2 (used by getvname '\', 2497) |
| swapb..swapl, pulla1 | code | - | templates (D) |
| srctype (3333) | dc.w, used as byte (high) | 0 | Exchange first var type |
| firstrep (3501) | .l | 0 | Repeat stack list head {next, pc} |
| replineat (3502) | .w | 0 | line of last Repeat (error report) |
| infochk (3520) | ds.b 6 | - | endininfo window |
| fvwait, cgoblitz, cgoamiga, intstart0, intfin | code | - | templates |
| inerr (3684) | .w | 0 | in SetErr block flag |
| errjmp (3685) | .l | 0 | addr of JMP operand to patch at End SetErr |
| errcode (3686) | .l | 0 | addr of move.l #x,d0 operand |
| intlineat (3726) | .w | 0 | SetInt line (errchx) |
| intallox (3790) | .w | 0 | unused |
| intdata1 (3792) | .l | 0 | where in data1 a5 is kept for ints (used 2491, 11205) |
| intjmpat (3794) | .l | 0 | JMP operand to patch at End SetInt |
| intline (3795) | .w | 0 | line for interr2 |
| intstring (3796) | .w | 0 | 0 none, >0 setint no string space, <0 with string space |
| intlevel (3799) | .w | 0 | interrupt level |
| intcleanat (3800) | .l | 0 | debug trap#2 operand to patch |
| ret15add (3802) | .l | 0 | trap#15 return addr (after params) |
| ret15cc (3803) | .w | 0 | trap#15 saved SR |
| prtname (4031) | str | 'PAR:' | only if final==0 |
| divisors (4071) | 10 x .l | 1..10^9 | makelong |
| restext (4098) | str | 'Name of Resident File to Create' | requester title |
| respath (4100) | 192 bytes | "" | requester path buf |
| resname (4102) | 192 bytes | "" | requester name buf |
| hello (4212) | 4 bytes | 'ZAP',0 | padding source |
| unmove (4636) | .w | 0 | movem mask for dounmove |

Used but defined elsewhere (for header): pc, pcat, bigpc, libat, lib, data1, data1at, data2, data2at,
allat, objlen, maxpc/maxlib/maxdata1/maxdata2 (options.src, .l each), makeexec, anyerrs, nomemleft,
debugga, blitzmode, blitzoff, procmode, varmode, linenumat, lasta6, intsused, firstfor, firstsel,
firstif, firstmacro, firsttype, firstlabel, firstconst, alltypes, firstoff, numoffs, tokeslist,
lastrestype/lastresmac/lastrescon (initialised to &heads), rescnt, resbuff (options.src, 8x68),
ezerr, errstack, errcont, dirmode, dirpcat, firstglob, firstlocal, comdata, temp1, zero, namebuff,
asmpnts (ds.l tnum-fnum), extraword, tokens, optreqga10, optreq2, optreq2ga1..7, tsthead/tstsize1/
tstsize2/tstend/tstsize3/tstdone, endsel/endselstr/endselstrf/endselstrf2, nextfixw/w2/l/l2,
pusha2, leaamp, libbase, libjsr.

## D. Machine-code templates (verified bytes)

| label(s) | bytes | how used |
|---|---|---|
| swapb..swapbf | 1012 1491 1280 | pokecode (move.b (a2),d0; move.b (a1),(a2); move.b d0,(a1)) |
| swapw..swapwf | 3012 3491 3280 | pokecode |
| swapl..swaplf | 2012 2491 2280 | pokecode |
| pulla1 | 225F | pokewd (move.l (a7)+,a1) |
| popreturn .rfix..rfixf | 0C6F 6753 0004 6700 0008 | pokecode; then tokejsr (6-byte jsr) |
| .rfixf | 4EB9 0000 0000 | not copied (placeholder for jsr size) |
| .rfix2 | 5C4F | pokewd (addq #6,a7) |
| .sfix | 584F | pokewd (addq #4,a7) |
| fvwait | 5340 | pokewd (subq #1,d0) |
| cgoblitz | 3B7C FFFF 0000 | only first word poked; then $FFFF, getbbase() |
| cgoamiga | 426D 0000 | only first word; then getbbase() |
| intstart0 | 48E7 3F38 | pokel (movem.l d2-d7/a2-a4,-(a7)) |
| intfin | 4CDF 1CFC | pokel at 13950 (End SetInt) |
| libbase (1161, external) | 2C6D xxxx | `move.l 0(a5),a6`; stamigalib writes +2 = base offset |
| libjsr (1163, external) | 4EAE xxxx | `jsr 0(a6)`; +2 = LVO |
| endsel (external) | 504F | OR'd with n<<9 -> addq #n,a7 |
| nextfixw/w2/l/l2 (ext.) | 504F / 4FEF000A / 4FEF000C / 4FEF000E | popnext3 |

Immediate opcodes emitted directly: $7000|n moveq #n,d0; $203C move.l #,d0; $223C move.l #,d1;
$4EF9 jmp abs.l; $4EB9 jsr abs.l; $48E7FFFE / $4CDF7FFF movem all; $2F00|r move.l Rn,-(a7);
$45ED lea d(a5),a2; $201F/$205F|n<<9 pops; $2000|n<<9, $2040|n<<9 moves; $1480/$3480/$2480 stores;
$4E40|n trap (maketrap); $48E7 / $4CDF + mask.

Patched later (absolute pointers into object buffer, recorded in errcode/errjmp/intjmpat/intcleanat,
`firstrep.pc`): long operands written as garbage then overwritten, guarded by `nomemleft==0`.
Every emitted abs.l operand gets `addoff` (reloc) before poking.

## E. Heap / linked structures

- firstrep node (alloc 8, MEMF_PUBLIC): 0 next.l, 4 pc.l (loop top address).
- offset node (addoff2, 8): 0 next.l, 4 offset.l (from pcat or resident file offset); head firstoff, count numoffs.
- tokeslist (maketjsr, 12): 0 next.l, 4 token.w, 6 form.w (bit14=Blitz), 8 offsets list.l; offset node (8): 0 next, 4 address.l.
- for struct (firstfor): 12.w var type. select struct (firstsel): 8.l bne list, 12.w type.
- label (firstlabel): 0 next, 4 value.l (bit0 = resolved, ==1 => constant saved to resident), 8 ref list, 16.w line, 18.b struct len, 19 name.
- macro (firstmacro): 0 next, 4 text ptr, 8.w text len, 12.b struct len, 13 name.
- type (firsttype / alltypes): 0 next, 4 first field ptr, 10.b struct len, 11 name. alltypes chain starts at builtin `bytetype` (7 builtins).
- field (type+4 list): 0 next, 10.l type ptr (resident file: high word = type index), 14.b len, 15 name.
- const (firstconst): name at 9 (findconst) - but the list is never filled (findconst has no caller); constants are firstlabel items.
- resbuff entry (68): 0 name[64], 64 seglist BPTR.
- lib struct (from uselib a1): 10.w libbase a5-offset, 12.b flags (bit7 used, bit6 const-ok).
- toke sub (handlelib a3): 1.b flags (bit2 Amiga-lib stmt, bit0 statement), 6.w lib #, 8.w LVO, 10.. reg bytes (end <0).
- Resident file = loadable executable: hunk_header {$3F3,0,1,0,0, size/4 @20}, {$3E9, size/4 @28},
  data [types section][macros section][consts section] pad to 4 ("ZAP"), optional
  {$3EC, numoffs, 0, offsets..., 0}, $3F2. Each section = count.w + items; internal pointers are
  file offsets fixed by LoadSeg relocation.

## F. External calls

- exec: AllocMem via comdata+88 (a6=exec), FreeMem (freeallox `jmp freemem(a6)`), CacheClearU -636 in flushc (external).
- dos (`calldos` = `move.l dos(pc),a6; jsr lvo(a6)`): Open(1006 MODE_NEWFILE), Write, Seek(-1 OFFSET_BEGINNING), Close, LoadSeg, UnLoadSeg.
- comdata (editor ted.s table): 0 openreq(a0=req), 8 getinput (out d7 class -1=gadget, d6 gadget id), 12 error(a0=text; used by err), 24 ezfreq(a0 path,a1 name,a2 title; d0 name, Z=cancel), 56 tokone(a0=line; tokenise in place), 72 pointeron, 76 sleepon, 88 extalloc. (Others: 4 closereq,16 detokallz,20 tokall,28 refgads,32 getinput2,36 combitmap,40 myscreen,44 extint,48 colors,52 halfcolors,60 openio,64 closeio,68 actgad,80 mywindow2,-4 screenopen2,-8 screenclose.)
- Runtime library tokens referenced (tokejsr): wbstart $bf00, setint $c100, clrint $c101, newint $c102, intvwait $c104, goblitz $c200, goamiga $c201, goqamiga $c202, chkret $c305, snxtchk $c306(+1), gosup $c30b, casechkw $c313(+1), $c003 global freemem, $d500 debug mode, seterr $d201, clrerr $d202, errfail $d205; uselib 65235.
- Uses (out of range): getchar3, bakup/bakeval/eval/peval/evalconst/evalconst2, getvname, fetchvar, calcvar, findtoke, findlab, maketjsr, uselib, getbbase, tokejsr, maketrap, errchx, doline, flushc, killoffs, freeemup, asmconst, err family.

## G. Needs inline asm / special handling

- directtrap: real trap #15 exception handler (supervisor, `rte`, frame layout, 68010+ format word
  ignored). Must be asm. directdo/direxit/dircont: saved-register frame + `rtr` that jumps INTO
  generated code (dirpcat) with original regs restored; generated code jmps back to ret15add.
  Keep as asm stub; the middle part (param read, tokone, doline, errchx, emit jmp) can be a C
  function `int direct_compile(params*)` returning error text / 0, with errcont longjmp.
- err mechanism (errstack/errcont = longjmp): dircont, loadres (resume INSIDE the loop at `.next`
  with a5/d6 intact; then errcont=freeres aborting loadres to its caller). C: setjmp per loop iteration.
- stamigalib patches `libbase+2`/`libjsr+2` (self-modifying templates) -> compute words in C.
- Statement handlers get flags (Z/N) from getchar3 via the dispatcher; C handlers must receive
  d0 and recompute (`Z = d0==0||d0==':'||d0==$8000+34`; N = d0<0).
- Byte ops on word globals (debugga, blitzmode, srctype, intsused st) - keep big-endian byte semantics
  (e.g. blitzmode high byte), `move d0,temp1` writes high word of a long.
- writeoffs stores word into high half of a pointer field (big-endian dependency, fine on m68k).
- runerropts keeps 7 words on stack; trivial in C.
- doallocmem never returns NULL (editor longjmps out on failure); a6=exec requirement disappears in C.
- menu3 excluded (final=-1).
