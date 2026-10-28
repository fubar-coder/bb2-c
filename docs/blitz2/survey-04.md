# Survey chunk 04 : blitz2.s 5000-6739

Content: inline assembler back half (asmconst, asmit, asmfixer, doea, makemode, insasm),
library-parameter fetching (countpees, fetchlibps, fetchpees*, handlep & friends),
library usage/linking (resetlibs, uselib*, fixjsrs, tokecode, maketjsr, findlib, findtoke),
library loading/freeing (loadlibgroup, loadlibs, loadblitzlibs, loadalib, fixlibs, free*),
pokedata1*.  calcstatic (6740) starts the next chunk.

Memory layout of the output buffer (as implied by the limit checks):
`pcat <= pc < libat <= lib < data1at <= data1 < data2at`. Overflow never errors directly:
it sets `nomemleft=-1` and keeps advancing the pointer (pass is retried with more mem).
All output pointers (`pc`, `lib`, `data1`, `40(lib)`) are HOST addresses inside the buffer.

## A. Routines

ASM EA mode numbers (d3=src mode, d5=dest mode; from getea, 4783 in chunk 03):
0 Dn, 1 An, 2 (An), 3 (An)+, 4 -(An), 5 d(An), 6 d(An,Xi), 7 abs.w, 8 abs.l, 9 d(PC),
10 d(PC,Xi), 11 #imm, 12 CCR, 13 SR, 14 USP, 15 movem reglist (d2/d4 = mask).
Pseudo modes only used in firstasm nodes: 16 = word immediate / dc.w, 17 = dc.b byte.
Reg value: d2 (src) / d4 (dest). Size code `asmsize`: 0=.b 4=.w 8=.l (it is a SHIFT count).

| routine (line) | in | out | notes |
|---|---|---|---|
| `asmconst` (5000) | a5 src ptr; `asmbuff` (namebuff/namebuff2); `numincs`,`macmask` | asmbuff filled: +0.w flag, +2 text\0 or +2.l src-ptr; `asmlen2` = stored len; d0 = terminator char ('(' ',' or eol) | flag = numincs\|macmask. If flag!=0 (include/macro: source text not persistent) -> tail `bra copyconst` (text copy). Else saves a5 on stack, copyconst; if len<=4 or local label used (`lc`) -> `.leave`: flag word NOTed (=$FFFF -> "direct"), `addq #4,a7` drops the SAVED a5 (not the return addr); else store saved a5 (pointer into source) at +2, `asmlen2=4` ("indirect", flag 0). CCs on return meaningless. |
| `copyconst` (5028) | a0 dest, a5 | a0 at NUL, d1=len incl NUL -> `asmlen2`; `lc`=loclabch if local label seen; d0 terminator | Reads via getchar3 till eol(Z) / '(' / ',' (not inside quotes). Tokens (d0<0) stored as 2 bytes hi,lo. `loclabch` -> prefixes `lastbiglab` string (nolocerr if empty). |
| `asmit` (5072) | d0 = asm token ($8000+fnum..); a5 | pc advanced; opcode at old pc | Entered by `bra asmit` (3115). alignpc; entry = asmpnts[(d0-$8000-fnum)]; idx*4 kept in `temp1` (word). Parses `.b/.w/.l` (none -> asmsize=4 unchecked; else size nibble 15 -> illsizeerr). getea src -> d2/d3 (asmbuff=namebuff), extraword->extraword2, asmlen2->asmlen; if ',' getea dest -> d4/d5 (asmbuff=namebuff2). No operand: d3=-1. Then assembles (see below). Errors: syntaxerr, illsizeerr, illeaerr (never return). a1 = running ext-word ptr, a4 = opcode addr. |
| `asmoff` (5278) | a1 = host addr needing reloc | - | pc temporarily := a1, `addoff` (adds pc-pcat to firstoff list), pc restored. |
| `asmfixer` (5286) | `firstasm` list, `nomemleft`, option gadget `optreqga15+13` bit7 ("make smallest"), `smallpass` | patches output | End-of-pass fixups. Skips if nomemleft; if smallest-option set runs only when smallpass!=0. constmode=-1, regat=0, prepstack; per node chkstop, a5=text (16(n) direct or (16(n)).l if 15(n).b==0), asmtype=0, evalconst3 -> d3 value, asmtype!=0 if relocatable. NOTE `move.l asmbuff,a2 / beq` tests Z of evalconst3 (movea sets no CC): eol, '(' or ',' ok else syntaxerr. Patches by mode, see E. constmode=0 at end. Errors illdiserr/illtraperr/illimmerr. |
| `doea` (5386) | d1 opcode accum, d2 reg, d3 mode, d4 extraword, d6 allowed mask (bit per mode), d7 shift byte (lo nib=mode shift, hi nib=reg shift, 15=none); a0 entry, a1 ext ptr | d1 \|= bits; a1 advanced | `btst d3,d6` else illeaerr; bsr makemode; modes>=7: reg=min(mode-7,4), mode=7. Mode 15 returns from INSIDE makemode past doea (stack trick). Clobbers d0,d2,d3,d7. |
| `chkbyte` (5423) / `chkword` (5429) | d3.l | - | range -128..127 / -32768..32767 else illdiserr. |
| `insasm` (5435) | a0 entry (14(a0) flag), a1 patch addr, d3 mode, `asmbuff`,`asmlen`,`linenumat` | node prepended to `firstasm` | preserves d0-d2/a0, a1; clobbers a6 (=exec). doallocmem(16+asmlen, 1). Flag word stored 0 if negative. |
| `makemode` (5474) | d3 mode, d2/d4, a1, asmsize | a1 advanced | <5: nothing. 5,7,9: insasm,+2. 6,10: insasm, write extraword d4 at a1 (low byte = disp, patched later), +2. 8: insasm,+4. 11: asmsize<8 -> insasm with d3=16 (restored to 11), +2; else insasm(11),+4. 12-14: nothing. 15: write d2 (reg mask) word, then `addq #4,a7 ; rts` -> returns to doea's CALLER. 16/17 nothing. Writes guarded by `a1<libat`. |
| `countpees` (5519) | a5 | d1 = number of params (top-level ',' count+1; 0 if eol at once) | storeloc ... tail `bra resloc` (restores a5,d0,lastchar). Nests ( { / ) }; stops at eol or unmatched ')'; eol while nested -> syntaxerr. Quoted chars ignored. Clobbers d3,d4. |
| `fetchlibps` (5563) | d1 = (formhi<<8)\|actual count; formhi bits0-2 = n repeatable, bits 4-7 (`(d1>>12)&15`) = index of first repeat (comment says $78>>3, code uses >>12); a2 = type byte table | `numreps`; params emitted; d0 = last char | fetch `start` params (fetchpees2), need ','; then while remaining>=n: fetchpees4(n) re-using same a2 (types), numreps++; numreps==0 -> syntaxerr; a2+=n; remaining trailing -> ',' then tail `bne fetchpees2`. |
| `fetchpees4` (5620) / `fetchpees2` (5623) / `fetchpees3` (5624) / `fetchpees` (5626) | d0 count (fetchpees3: `and #15`), a2 types, d2 = group push mode (4: 1, 2: 0) | d0 = char after last param; a2 advanced; d1 preserved | Multiple entry points, fallthrough chain. Saves d1, old pmode (recursion), sets pmode word=d2. Per param: type byte (0 -> `userp`), handlep; between params need ',' else notparerr. |
| `handlep` (5653) | d2 type byte: bit7 addr-of-var (low 3 bits type), bit6 push, bit5 array info (+bit4 high, bit3 type); a5 | value in D(regat) or pushed; d0 next char | Writes `move.b d1,pmode` = HIGH byte of the pmode word (bit6 -> $40). bit7 -> handlevara, bit5 -> handlearr (both tail), else getap then falls into putap. |
| `putap` (5675) | d2 type, pmode word | | pmode!=0 -> pushap else putapincreg. |
| `pushap` (5679) | d2 type, regat | emits `move.l Dregat,-(a7)` ($2F00\|regat, from `pushdn` template), or `.w` (\|$1000) if type<3; stackuse +=4 / +=2 | |
| `putapincreg` (5689) | | regat++ | |
| `putalp` (5695) | | | d2=3 (long) then putap. |
| `handlevara` (5698) | d2&7 expected type | | ampersand2 (addr of var -> D(regat), d2 = var type, bit14 = pointer). type 7 kept; bit14 or 0 -> 3. Expected!=0: must equal else mismatcherr. Expected 0 ("any"): emits `move.w #type,-(a7)` (ptype opcode + type word), stackuse+=2. Then putalp. |
| `handlearr` (5730) | d2 flags | d0 = next char (tail getchar3) | baseadd; bit4 -> highadd; bit3 -> sendtype. |
| `sendtype` (5742) | regat, a2 type node, a3 var node (from fetchvar) | emits `moveq #type,Dregat` then pushap(d2=2) | 7(a3) bit0 -> pointer -> type 3; else 4(a2).l>=256 -> arrerr1; type = 7(a2).b. |
| `baseadd` (5759) | a5 at "name(" | emits `move.l d16(a5),Dregat` (fbase opcode\|regat<<9, then 4(a3)); putalp | getvname must set flagmask bit1 (array) else syntaxerr; fetchvar; next must be ')'; `added`!=0 -> noarrayerr. |
| `highadd` (5781) | a3 var, varmode, pmode, regat | emits move.l off(a5),Dregat with off = 4(a3) + 8(a3)*4 (negated part if varmode!=0); then `add.l (a7),Dregat` (pushadd) if pmode!=0 or regat>=7, else `add.l D(regat-1),Dregat` (pushlast); sets pmode hi byte=1; putalp | |
| `getap` (5822) | d2 | d2 result type | d2&7 == 0 -> tail evalu2 else tail eval. |
| `resetlibs` (5826) | firstlib | every lib: 10(l)=-1, clear 12(l) bit7 | |
| `usevars` (5835) | | | d1=allocvars(65534), falls into uselib. |
| `uselib` (5837) | d1 lib number | a1 = lib struct | findlib (noliberr if absent) then falls into uselib3. |
| `uselib3` (5841) | a1 lib, d1 lib# | a1 | If `cfetchmode`: lib flag bit6 required else badconerr; nothing else. Else bset bit7 (if already used or 10(a1)!=-1 -> rts). `dirmode` -> undo, dirliberr. Max-type lib (word at code-2 !=0): recursive uselib(64335), allocate varoff (even) -> stored at code-6, +4, +4 more if long at code-14 !=0. Copies lib code words from code start (a1+18(a1)) to `lib` (end = a1+6(a1), or a1+44(a1) if 44!=0 and !debugga), 40(a1)=old lib. Relocates (see E/G), handlejsrs for token-jsr relocs. Var slot: 26(a1)==0 -> 10(a1)=0; else 10(a1)=varoff (even), varoff += 2 (26<0) or 4, patches templates for libs 65235/64535 (D), usevars. Then uselib2(22(a1)), uselib2(28(a1)) with d2=own lib# (dependencies). Preserves everything except a1 (a0/a2/d0-d3 saved). Recursive. |
| `fixjsrs` (6003) | tokeslist | patches all token-jsr longs | Called at end of compile (2665, before asmfixer). Processes tokeslist and each node's offset list in "batches" (sentinels d2/d3 = previous heads) because tokecode->maketjsr can prepend new nodes/offsets while iterating. Per node batch: tokecode(4(n),6(n)) -> d7; writes d7.l at every offset node's 4(o) address (skip writes if nomemleft). |
| `tokecode` (6047) | d1 token, d2 form number (bit14 = Blitz-mode form) | d7 = host address to jsr (either lib routine direct or a freshly generated stub at pc ending with rts $4E75) | findtoke (marks lib used). blitzmode bit7 := form bit14. Direct address = sub offset -18(lib)+40(lib) when: no reg params, both errchk longs 0, offset even, first routine word not $Axxx. Otherwise generates stub: lasta6=-1, `jsr cutejsr` (a2 lib, a3 form) or `jsr makelibsub` (a2 lib, d1 sub offset) [both chunk 06/07, 11459/11466], then pokewd $4E75. tokeerr on bad flags (bits 2/3 of 1(a3)) / form beyond end / simple token with form!=0. Clobbers a0,a2,a3,d0-d2. |
| `freetlist` (6138) | tokeslist | tokeslist=0 | FreeMem offset nodes (8) and toke nodes (12). |
| `handlejsrs` (6158) | a1 lib, a2 ptr to first reloc entry that is a token-jsr, d2 = count of remaining reloc entries | | Preserves a1-a6/d3-d5. For entries whose source word 0(lib,off) is negative (and not in errchk part unless debugga): d5 = word&$7FFF (form#, bit14 Blitz), d4 = token# (word at off+2), d3 = output addr (off-18+40) -> maketjsr. |
| `maketjsr` (6189) | d3 output addr to patch, d4 token, d5 form | a2 lib, a3 sub (tail `bra findtoke`) | Find/create tokeslist node {tok,form}, prepend offset node {addr}. Also called from 3606 (chunk 02). a6=exec, clobbers d0,d1,a0. |
| `uselib2` (6237) | **Z flag** from caller's `move.l off,d1` (Z = no list); a1 lib, d1 offset, d2 own lib# | a1 preserved | list at a1+d1+6: words of lib numbers, 0-terminated, each followed by a skipped word; uselib each != d2. Recursive via uselib. |
| `findlibnoerr` (6253) | d1 lib# | a1 lib or 0; d1=-1 if found (d1 unchanged + Z=1 via a1 compare if not) | used at 15390. |
| `findlib` (6264) | d1 | a1 | noliberr (never returns) if missing. |
| `findtoke` (6277) | d1 token | a2 lib, a3 sub node; d1 = 14(a2) (clobbered!) | lib with 14<=d1<16(a2); walks sub chain via 2(a3) from 48(a2), decrementing d1. Then uselib3 for that lib (d1/a1 preserved around it). notokerr if none. Syslib/userlib branches identical (old code commented). |
| `loadlibgroup` (6322) | d0 length (0 -> return 0), d1 filename, a3 = address of tail link field | d0 buffer (0 if open failed), a3 = last linked struct | Open(1005)/AllocMem/Read/Close, then walks records and links structs (see E). No alloc/read failure checks. Clobbers d2-d7,a0-a2,a6. |
| `loadlibs` (6374) | acidlibs/deflibs lengths, acidname/defname | firstlib chain; acidlibsat, deflibsat, blitzlibsat | a3=&firstlib; acid group, def group; clr.l (a3); blitzlibsat=a3 (tail link addr); falls into loadblitzlibs. Called at startup (1414). |
| `loadblitzlibs` (6392) | blitzlibsat, libsdir, libslist | tail `bra fixlibs` | Also entry from menu8 (1781). Path in undobuffer = libsdir + '/' (unless ends ':' or empty) + dirname + '/' + filename. For each enabled libslist entry: Lock(-2), Examine/ExNext into `namebuff` as FileInfoBlock; skips dirs (fib_DirEntryType>=0) and *.info (endininfo Z=1); loadalib each. Stack: (a7)=name pos, 4(a7)=dir pos. |
| `loadalib` (6470) | d1 filename, a6=dos, a3 tail | a3 = new struct; d0 junk | Open/Read 32 hdr -> optstuff, struct size = optstuff+28 longs; alloc; read; link; 6(s)=size; read 12 bytes (HUNK_RELOC32 hdr) -> reloc count optstuff+4 -> 32(s), alloc count*4 -> 36(s), read. Tail `jmp close(a6)`. Open fail -> rts silently. CLOBBERS a5 (and d0-d7). |
| `fixlibs` (6525) | firstlib, lasttoken | 14/16/18 of each lib, sub links, token nodes appended after lasttoken; nummaxs; memlib, ffplib | see E. End: sets `errstack=a7, errcont=.cont, ezerr=-1` so a missing memlib(65530)/ffplib(64935) noliberr resumes at `.cont` (rts) instead of aborting. |
| `skiplibreg` (6618) | a1 | a1 past reg list | words: nonzero -> skip it + one more word; 0 terminates (skipped). |
| `aligna0` (6624) / `aligna1` (6629) | a0/a1 | aligned up to even | uses d0 via exg (restored). |
| `freeblitzlibs` (6635) | blitzlibsat | (blitzlibsat)=0 | FreeMem each struct (6(s)) and reloc table (32(s)*4 at 36(s)); tail `beq frlibslist` (1898, frees libslist 38-byte nodes). |
| `freelibs` (6653) | | firstlib=0, (lasttoken).l=0 | freeblitzlibs, then FreeMem deflibsat (deflibs bytes) and acidlibsat. |
| `freemacs` (6680) | lastresmac (link field after resident macros) | | frees non-resident macros: text 4(m) of len 8(m).w (if !=0), node size 12(m).b. |
| `freeasms` (6701) | firstasm | firstasm=0 | node size = 14(n).b. |
| `pokedata1b` (6714) | d1.b | data1 += 1 | limit data2at -> nomemleft. clobbers a4. |
| `pokedata1l` (6725) | d1.l | data1 += 4 (hi word first) | swap/pokedata1/swap, falls into pokedata1. |
| `pokedata1` (6729) | d1.w | data1 += 2 | clobbers a4. |

### asmit assembly details (5129-5276)
asm table entry `e` (asmmac macro `a`; asmpnts[] points just past name\0\0,even):
+0 opcode.w, +2 src mode mask.w, +4 dest mode mask.w, +6 src shift.b, +7 dest shift.b
(each (regshift<<4)|modeshift, 15=none), +8 size.w ((sizefieldshift<<12) | nib0 .b code |
nib1 .w code | nib2 .l code; nib 15 = size illegal; top nib $F = no size field),
+10 check routine.l (0=none), +14 flag.w.
Flow:
1. no operands (d3<0): both masks must be 0 else illeaerr -> putit.
2. flag<0 (e.g. $8003): bit0 && dest An && src!=USP -> use entry idx+2 (e.g. ADD->ADDA); else bit1 && src #imm -> entry idx+1 (ADDI). a0 reloaded to the new entry, d1 = its opcode (all later fields come from the new entry).
3. buff1=namebuff, buff2=namebuff2. If e+10: `jsr (a3)` with d1 op,d2,d3,d4,d5, d6=asmsize, a0,a1. Return CC: N -> d0==-2 ? illsizeerr : illeaerr; Z -> ok; else (>0) swap src/dest (d2<->d4,d3<->d5, swap the two words of long `extraword` (extraword/extraword2 adjacent), buff1<->buff2, asmlen<->asmlen2). Check routines live in asmmac (abcdchk, addchk, aslchk, ...).
4. asmbuff=buff1; flag (14(e)): 1 (addq/subq/shift count) and src #imm -> skipsrc; 2 (Bcc): src must be abs.l(8) -> becomes d(PC)(9); 3 (moveq),4 (trap) -> skipsrc; 5 (DBcc): dest must be 8, d1\|=d2 (Dn), d5=9.
   skipsrc: insasm(src imm, patch addr a1 = after opcode, flag kept) and move dest into d2/d3 as the only ea (encoded with DEST mask/shift).
5. src: doea(d6=e+2, d7=e+6, d4=extraword2); dest: asmbuff=buff2, asmlen=asmlen2, doea(e+4, e+7, d4=extraword). If no dest, dest mask must be 0.
6. putit: if size field: d1 \|= ((e+8 >> asmsize)&15) << (e+8>>12). Store d1 at a4 if a4<libat else nomemleft. pc=a1.

## B. Register conventions
- a5 = source char pointer (getchar3/getchar2), d0 = current char/token (bit15 = token), Z=1 at end of statement (0, ':' or `Else` token $8000+34). storeloc/resloc save a5+d0.
- In asm code: a0 = asm table entry, a1 = next output word ptr (host), a4 = opcode addr; d1 = opcode being built; d2/d3 src reg/mode; d4/d5 dest reg/mode.
- In param fetch: `regat` = next free data register for results (D(regat)); a2 = types table; d2 = type byte.
- Lib code: a1 or a2 = lib struct; a3 = sub/form ptr or list tail ptr; a6 = exec (4.w) or `dos`.
- pokewd emits d1.w at pc (outside range); pokel d1.l.

## C. Globals defined in range
| label | size | init | purpose |
|---|---|---|---|
| `lc` (5026) | w | 0 | local-label-char seen in asmconst text (forces direct text) |
| `ptype` (5617) | code 4 bytes | `move.w #0,-(a7)` ($3F3C,$0000) | template: only opcode word read |
| `pmode` (5618) | w | 0 | push mode; HIGH byte written by handlep/highadd (per-param push), LOW/word by fetchpees (group push); tested as word |
| `fbase` (5818) | code 4 | `move.l 0(a5),d0` ($202D,$0000) | template opcode |
| `pushadd` (5819) | code 2 | `add.l (a7),d0` ($D097) | template |
| `pushlast` (5820) | code 2 | `add.l d0,d0` ($D080) | template |
| `tokeslist` (6227) | l | 0 | head of token-jsr fixup list |
| `deflibsat` (6318) | l | 0 | deflibs group buffer |
| `acidlibsat` (6319) | l | 0 | acidlibs group buffer |
| `blitzlibsat` (6320) | l | 0 | ADDRESS of the link field where per-dir blitz libs start (tail of group libs, or &firstlib) |
| `ffplib` (6616) | l | 0 | lib struct of 64935 (ffp) |

Globals used but defined elsewhere (for header): asmbuff, asmlen, asmlen2, extraword, extraword2 (must stay adjacent, accessed as one long), asmsize, asmtype, buff1, buff2, firstasm, constmode, temp1 (long; also dummy link in fixlibs), numincs, macmask, lastbiglab, quoteflag, nomemleft, pc, pcat, libat, lib, data1at, data1, data2at, firstlib, lasttoken, cfetchmode, dirmode, varoff, debugga(.b test), stackuse, userp, numreps, added, varmode, flagmask, blitzmode, lasta6, regat, smallpass, optreqga15, linenumat, namebuff/namebuff2 (+8 pad = FIB), optstuff, undobuffer, libsdir, libslist, acidlibs, deflibs, acidname, defname, nummaxs, errstack, errcont, ezerr, memlib, lastresmac, firstoff (via addoff2), dos, comdata, asmpnts.

## D. Machine-code templates / emitted words
- Emitted constants: `$4E75` rts (tokecode stub end); `$7000|regat<<9|type` moveq (sendtype); `$2F00|regat` / `$3F00|regat` push (pushdn template, 1092, ORed); `$3F3C,type` (ptype); `$202D|regat<<9, off` (fbase); `$D097|regat<<9` (pushadd); `$D080|regat<<9|(regat-1)` (pushlast).
- Library code itself is copied word-by-word from the loaded lib struct into the output (uselib3).
- Self-modifying patches of other chunks' templates in uselib3 (word = lib var offset 10(a1)):
  lib 65235 (stringslib1): `movestdn+2`, `pusha3+2`, `pusha32+2`, `pulla3+2`, `geta3+2` (d16 of 0(a5));
  lib 64535: `dataget+2`, `dataput+2`, `rescode+6`, `rescode2+2`.
  -> in C these become variables read when emitting those templates.
- asmfixer patches in generated code (a1 = node+4 addr): see E firstasm.

## E. Structures
**firstasm node** (insasm; alloc 16+asmlen via doallocmem; freed by freeasms with size 14.b):
0 next.l | 4 patch addr.l (host) | 8 mode.w (5-11,16,17) | 10 flag.w (asm entry flag, 0 if negative) | 12 linenumat.w | 14 alloc size.b (truncates if asmlen>239!) | 15 type.b (0 = indirect: 16.l = pointer into source text; else direct) | 16 text\0 (or ptr).
asmfixer patch by mode d1 (value d3, a1=addr): 5,7 chkword -> (a1).w; 6 chkbyte -> 1(a1).b; 17 -> 1(a1).b unchecked; 8 (and 11 w/o flag) -> asmtype? asmoff(a1); (a1).l; 9 d3-=a1, chkword -> (a1).w; 10 d3-=a1, chkbyte -> 1(a1).b; 11/16 with flag: 1 -> 1..8 else illimmerr, `(d3&7)<<9` ORed into word at a1-2 (opcode); 3 -> chkbyte, byte at a1-1; 4 -> <=15 else illtraperr, ORed into byte a1-1; 16 no flag -> (a1).w unchecked.

**asmbuff layout** (namebuff/namebuff2): 0 flag.w (0 indirect / nonzero direct; insasm copies only LOW byte as type) | 2 text\0 or ptr.l.

**tokeslist node** (12 bytes): 0 next.l | 4 token.w | 6 form.w (bit14 Blitz) | 8 offsets list.l. **offset node** (8): 0 next.l | 4 host addr.l to patch.
**firstoff node** (addoff2, 8): 0 next.l | 4 offset.l (relative to pcat); numoffs++.

**lib struct** (firstlib chain; loaded image of hunk 0 of a lib file):
0 next.l | 4 lib#.w (signed: <0 = system lib 65535.., >=0 user lib) | 6 struct size bytes.l (set by loader) | 10 var offset.w (-1 unused, 0 none) | 12 flags.b (bit7 used, bit6 allowed in const mode) | 14 first token.w (user: lib#*128+1, set by fixlibs) | 16 last token+1.w (user: fixlibs) | 18 offset to code start.l (fixlibs) | 22 dep list 1 offset.l | 26 var size flag.w (0 none, <0 word, >0 long) | 28 dep list 2 offset.l | 32 reloc count.l | 36 reloc table ptr.l | 40 output addr of copied code.l | 44 offset where errchk-only code begins.l (0 none) | 48 sub table.
Dependency list at lib+off+6: {lib#.w, pad.w}*, 0.w.
Sub table (fixlibs, 48..): repeated sub nodes, terminated by negative word:
  sub: 0 flags.w | 2 next sub.l (filled by fixlibs) | 6 ...
  - flags==8: token without code -> token node follows directly.
  - flags&15==0: system sub (no token): reglist (skiplibreg), 12 bytes. token# still counted.
  - flags bit2: amiga-lib call: 4 bytes, bytes until one with bit7 set, align, token node.
  - else forms: { form.w (lo byte nparams, hi byte: bits0-2 n repeatable, bits4-7 first repeat), nparams type bytes, align, reglist {w,w}* 0.w, 12 bytes: +2 errchk1.l, +6 routine offset.l (rel. to lib struct; odd = flag), +10 errchk2.l (offsets rel. to the 0 terminator word) }*, negative word, token node.
  - simple (flags==0 path in tokecode, i.e. (a3).w==0): +6 reglist 0.w, +8 .l, +12 routine offset.l, +16 .l.
  - token node (linked into global token list after `lasttoken`): next.l, token#.w, name\0, help\0, even.
After terminator word: pad.w, maxflag.l; if !=0 a 22-byte max block precedes code (code-14 .l, code-6 .w varoff written, code-2 .w nonzero marker) and nummaxs++.
Relocs: longs = offsets from struct start of longs to relocate; offsets <18 ignored; >=44(lib) skipped when errchk stripped. Relocated: long += 40(lib)-18(lib); offset (addr-pcat) added via addoff2. Entries whose target word is negative are token jsrs: word0 = $8000|form (bit14 Blitz), word1 = token# -> handlejsrs/maketjsr; final value written by fixjsrs.

**lib file** (loadalib): AmigaDOS exe: 32-byte header (only +28 = hunk longs used), hunk body = lib struct, then 12 bytes (HUNK_RELOC32 $3EC, count, hunk#), count longs.
**lib group file** (loadlibgroup; acidlibs/deflibs): records {len.l, 32-byte header (+28 = struct longs), struct, 12-byte reloc hdr (count at struct_end+4), relocs} ; next record at after-len + len; len 0 terminates. Struct at rec+4+32; 36 points into the buffer (not freed separately).
**libslist node** (38 bytes, frlibslist): 0 next.l | 4 enabled.w | 6 dir name.
**macro node** (freemacs): 0 next.l | 4 text.l | 8 text len.w | 12 node size.b.

## F. External calls
- `doallocmem` (3379): `jmp comdata->88` — editor callback, AllocMem semantics (d0 size, d1 flags=1 MEMF_PUBLIC, a6=exec loaded before), returns d0; results never checked here. Freed with exec FreeMem (`jsr freemem(a6)`, a6=4.w) directly.
- dos (a6=`dos`): Open(d1 name, 1005), Read, Close, Lock(d1,-2), Examine, ExNext, UnLock.
- endininfo (3522): Z=1 if a0 string ends with ".info".
- other chunks: getchar3, reget, bakup, storeloc, resloc, getea, alignpc, addoff/addoff2, evalconst3, chkstop, prepstack, pokewd, ampersand2, getvname, fetchvar, eval/evalu2, cutejsr/makelibsub (code generators), err handlers (syntaxerr, illsizeerr, illeaerr, nolocerr, illdiserr, illtraperr, illimmerr, notparerr, mismatcherr, arrerr1, noarrayerr, badconerr, dirliberr, tokeerr, noliberr, notokerr; all `bsr err` + inline text, never return).

## G. Special handling for C
- makemode mode 15: `addq #4,a7; rts` returns past doea -> in C makemode returns a flag, doea returns early.
- asmconst saves a5 on stack and drops it in `.leave` (simple local var in C).
- Asm check routines called via function pointer in asm table (+10) with all regs as inputs and a 3-way CC result (N / Z / >0) and d0==-2 -> C function returning int (<0 err, 0, >0 swap), input struct for d1-d6; may modify d1 (opcode).
- tokecode `jsr (a0)` -> plain call of cutejsr/makelibsub.
- uselib2 takes Z from caller (`move.l off,d1`) -> test off==0.
- asmfixer `beq` uses Z from evalconst3 across a movea -> evalconst3 must return eol status.
- fixlibs installs an error continuation (errstack/errcont/ezerr) -> setjmp-like recovery or use findlibnoerr.
- pmode accessed as byte (high) and word; extraword/extraword2 swapped as one long; temp1 written as word (high half of long).
- namebuff is used as a FileInfoBlock (needs 260 bytes, 4-aligned: namebuff+namebuff2+8 pad, cnop 0,4); clobbers namebuff2. In C use a real aligned FIB.
- loadalib clobbers a5 (harmless at that time).
- Self-modified templates in other chunks (D).
- Relocation writes and uselib copy operate on raw big-endian words/longs in the output buffer -> needs byte-exact pointer arithmetic (fine on m68k gcc).
- findtoke returns with d1 changed (= first token of lib).
- asm node size stored as byte (bug-compatible or fix).
- libsdir default ends with '/', loadblitzlibs still appends '/' (only ':' checked) -> "//" (parent dir in AmigaDOS) unless config path lacks slash; keep behaviour.
