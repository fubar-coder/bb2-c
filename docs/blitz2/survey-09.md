# Survey 09: blitz2.s 13327-14655 (token handlers, part 2)

This range covers dothen, ongoto/gosub, goto, gosub, return, Statement/Function (procs),
End *, NewType, Wend/EndIf, Stop/Cont, For/Next, Let, Dim. It runs from `dothen` up to
`;-----data`. It is entered from the `atokens` table (line 1298). Dispatch pushes the handler
address and does `bra getchar3`, so each handler is entered with
**d0 = the next char/token after the keyword and Z = getchar3 result (Z=1 on EOL, ':', 0
or Else)**. Only `doend` actually tests the incoming Z.

Token numbers used here are `$8000+n`, where n is the index in the token list at line ~300:
$8001 NewType, $8002 End, $8005 Goto, $8006 Gosub, $8007 Return, $8008 Statement,
$8009 Function, $800b If, $800c While, $800e Select, $8018 To, $8019 Step, $8030 SetInt,
$8000+tnum = List, $8001+tnum = SetErr. `tnum` is a `set` symbol: it starts at fnum=60 and is
incremented inside `asmmac` once per asm mnemonic, so it is the final value after the asm table.

Common helpers used everywhere (all outside this range):
- getchar3/reget return d0 and Z.
- bakup rewinds a5 to prevloc.
- pokewd(d1.w), pokel(d1.l), pokecode(a0,a1) and pokecode2 write at `pc`.
  - **pokewd clobbers a4** and word-aligns `pc` first.
  - When the output buffer overflows they set `nomemleft=-1` and keep advancing `pc`.
- addoff records a relocation at the current `pc`.
- tokejsr/tokejsr2(d1=token) emit a library call. tokejsr2 also sets lasta6=-1.
- uselib(d1=lib#) marks a library as used.
- eval(d2=type) leaves the result in d0, the type in d2 and the terminating token in d0.
  bakeval = bakup + eval.
- doallocmem(d0=size, d1=flags) goes through comdata+88. Nodes are freed with exec FreeMem.

## A. Routines

| name (line) | in | out / effect | notes |
|---|---|---|---|
| dothen (13327) | — | bakup; lastchar=':' (so the next reget sees end of statement); rts via ifdone | **doif (13272, chunk 08) falls through into it** after `bset #7,12(a0)` (one-line If). Also `bra dothen` from doelse (12291) and atokens "Then" |
| ifdone (13329) | — | rts | branch target from doif (13325) |
| ongotocode..ongotocodef, ongosubcode.., ongcode.. (13331-13366) | — | — | **templates, never executed by the compiler** (section D). `goset` (13362) is also read as a long template |
| doongo (13370) | d0 = first char of expression | parses `On e Goto/Gosub l1,l2...`; emits the template + label table; patches the template; rts | bakeval with d2=2 (word). Token after the expression must be $8005 or $8006, else syntaxerr. With Gosub and debugga it calls chkstak and uses ongcode. Pushes the skip offset (word) and the template start (`pc`) on the a7 stack, pops them at the end. Label loop: getchar3, `pc-=2`, makealab, `pc+=2`, addoff, pokel 8(a2). The pc-2 is there because makealab records the reference at pc+2. If !nomemleft it patches 10(tmpl)=labcnt and 2(tmpl+skipoff)=labcnt*4+2 |
| dogosub (13429) | d0 = label first char | with debugga: chkstak + poke long `goset` (move #'gS',-(a7)); then d1=gosubcode opcode ($4eb9); bra dogoto2 | |
| dogoto (13439) / dogoto2 (13440) | d1 = opcode word (dogoto2) | makealab (calls makename2, which calls bakup, so the name is re-read); pokewd opcode; addoff; tail `bra pokel` with 8(label) (0 if not yet defined, fixed later by handlelab2) | 2 entries. gotocode = $4ef9 |
| eos (13448) | procmode | emits `jmp abs.l` (addoff, pokel with garbage d1 left by doallocmem); adds a pending node {next, addr-of-long} to firstpend; tail bra pokel | procmode==0 → badpenderr. Also the atokens entry for "AsmExit" ($8011) |
| stateret (13466) | — | bsr eos; bra getchar3 | Statement Return |
| funcret (13469) | thisproc | eval with d2=5(thisproc) (return type; d0=value); bra eos | Function Return |
| doreturn (13477) | debugga | debugga: tail tokejsr chkret ($c305); else falls to normret | |
| normret (13483) (.noerrchk) | — | poke $4e75 (rts); tail bra pokewd | also called from endstate |
| dostate (13494) | d0 | if d0==$8007 → stateret. Else d1=-1 (statement) → dofunc2 | |
| dofunc (13498) | d0 | if d0==$8007 → funcret. Else bakup; sizespec2 (d1 = return type word: .type / $ =7 / default); getchar3; falls into dofunc2 | |
| dofunc2 (13504) | d1 = procmode value (-1 statement, else type code 1..7); d0 = first char of name | full proc header, see detail 1 below; ends `bra getchar3` | jsr errchx (errors if inside select/repeat/setint/if/proc/for) |
| linkput (13623) | a0 = output address; linka4+2 already patched | if !nomemleft: saves pc, sets pc=a0, pokes `link a4,#n` (linka4 long) + tokejsr2 clrloc ($cc00), restores pc | clobbers a4,d1 |
| procfixer (13636) | thisproc | rewrites the 16-byte nop area at 14(proc) and emits the statics init at the current pc, ending with rts; may change 14(proc) (entry point). See detail 2 | called from endstate only |
| domwait (13715) | — | pokecode mwait..mwaitf (template at 1155: `btst #6,$bfe001 / bne mwait`) | tail |
| donewtype (13719) | d0 must be '.' | makename (beq laberr); findtype in alltypes. If found it must be a $ff placeholder (else typeerr); if not found, addhere. Then 4(type)=0, thistype=a2, prevtype=0, typelen=0, linemode=0. reget: Z → rts, else → typemode | linemode=0 makes smode (2941) route following lines to modetable[0]=typemode |
| typemode (13740) | d0 = first char/token of the NewType line (called through smode/modetable after reget, or from donewtype) | defines one field; rts. $8002 (End) → doend2 | See detail 3. smode did `addq #4,a7` (dropped dostatement's return), so typemode's rts returns to doline |
| doendif3 (13830) | d1 = expected kind (0=If, 1=While) | a0 = firstif node; checks it; falls into doendif2 | noiferr / badenderr |
| doendif2 (13837) | a0 = if node, d1 = kind | While: emits `bra.w` back to 4(node). Then patches the bcc displacement at 8(node)-2 = pc-8(node)+2 (if !nomemleft); rts | ≥32768 → bigiferr. External callers: 2802 (un-then at EOL), doelse 12289 |
| dowend (13860) | — | bakup; falls to dowend2 | atokens "Wend" |
| dowend2 (13861) | — | d1=1; bra doendif | |
| mydoendif2 (13863) | — | bakup; falls to mydoendif | atokens "EndIf" |
| mydoendif (13864) | — | d1=0; falls to doendif | |
| doendif (13865) | d1 | doendif3; freetheifz; bra getchar3 | |
| freetheifz (13869) | a0 = firstif node | firstif=(a0); exec FreeMem(a0,14) as a **tail `jmp freemem(a6)`** | also jsr'd from 2803 |
| doend2 (13875) | — | getchar3, falls into doend | |
| doend (13876) | **Z flag**, d0 | Z=1 (bare End): dirmode → baddirerr, else tail tokejsr endjmp ($c800). Otherwise loops `.doend3` + reget while not EOL | `.doend3` dispatch is detail 4 |
| dostop (13975) | — | dirmode → baddirerr; !debugga → badstoperr; tail tokejsr $d501 | |
| docont (13983) | — | jmp badconterr (never returns) | |
| nop (13985) | — | poke $4e71 | no references found |
| freefors (13988) | firstfor | frees every node (size = byte 16(node)) and clears firstfor | jsr'd from 2007 |
| endstate (14001) | thisproc, procmode, firstpend, firstlocal, memlib | emits the proc epilogue and finalises the proc; bra getchar3 | See detail 5. procmode==0 → badpenderr |
| droplocals (14104) | firstlocal, firstglob | allocates a 12-byte node {next, firstlocal, firstglob} on alllocals and clears both heads. If debugga && !nomemleft: writes them through proclocals into the 2 longs emitted after trap #4 | |
| donext (14130) | d0, Z (via reget) | Next [var[,var]]: see detail 6; tail `bra reget` | dirmode → baddirerr; firstfor==0 → noforerr |
| forset (14221), thebra (14223) | — | — | templates (D) |
| dofor (14228) | d0 = first char of var | see detail 7; tail `bra reget` | dirmode → baddirerr |
| thebgt (14336) | — | — | template |
| dolet (14338) | d0 | letstart=a5; getvname; falls to dolet2 | atokens "Let" |
| dolet2 (14340) | namebuff/d2 from getvname, d0 = char after the name | assignment code; **out: d0 = terminating token, d2 = destination type/flags, d3 = var offset** (dofor uses these); rts | See detail 8. External entry: handleascii `jmp dolet2` (2984) |
| alignletpc (14498) | pcodd | if pcodd is odd: pcodd=0, poke `addq #1,a2` ($524a). d1 preserved | |
| add1a2 (14508) | — | — | template word $524a |
| dodim (14513) | d0 | regat+=1 (never undone here); uselib 65434 (arrays); falls into dodim2 | |
| dodim2 (14520) | d0 | per array: optional List ($8000+tnum: listadd=8, uselib 65530); nonew=0; getvname (must have '(' → flagmask bit1, else syntaxerr); fetchvar; redim or new; dimwit; getchar3; ',' → loop; rts | Redim: temp1=8(a3) (#dims); temporarily varoff=4(a3)+4 (global) or locvaroff=4(a3)-4 (local) around dimwit, then restored |
| dimwit (14571) | a3 = var, a2 = type, temp1, listadd | emits the array creation, see detail 9; tail bra tokejsr | redimerr if the dimension count differs |
| setles (14654) | — | — | template word $383c (move #imm,d4) |

### Detail 1: dofunc2 (proc header)
- prolineat=linenumat; procnum = ++procnum2; procmode=d1; locvaroff=0.
- makename2; d0 must be '{'.
- memlibstat = 12(memlib); `bclr #15,12(memlib)` (memlib "used" flag reset, so we can see if the proc uses it).
- emit `jmp abs.l` ($4ef9 + addoff + pokel garbage). This long is patched at End by endstate (jump around the proc).
- findproc: found → dupprocerr. addhere (a2 = new proc); 14(proc)=pc; thisproc=a2.
- pokecode2 nops8..nopsf: 8 × $4e71 (16-byte placeholder for procfixer).
- debugga: maketrap(4) emits `trap #4`; proclocals=pc; pokel ×2 (inline data with garbage $00004e44, filled by droplocals).
- a4 = proc+22 (param-type byte array); 5(proc) = procmode low byte; d6 = param count, d5 = string-param count.
- for each param until '}': getvname, fetchvar (a3 = var, a2 = type).
  - String (4(type)==7): stbuff[d5] = {word d6 (reg #), word 4(a3) (offset)}; d5++; type byte 7; no code.
  - Otherwise emit `move.? dN,off(a4)`: $2940 (.l), $3940 (.w), $1940 (.b), OR'd with d6 (reg), then word 4(a3).
    - Pointer (flagmask bit0) → .l, type 3.
    - 4(type)≥256 → illprocperr.
    - size from 6(type): >2 .l, ==2 .w, else .b.
    - a4 is saved around pokewd.
  - d6++; d6≥7 → toovarerr (max 6 params d0-d5); `(a4)+` = type byte; ',' → next.
  - The closing token must be '}', else syntaxerr.
- 4(proc) = d6 | d5<<4.
- for each string param (i = d5-1..0; stbuff walked forward):
  - funcst+2 = 8+4*i, poke `move.l 8+4i(a4),-(a7)`
  - poke movedna3|reg (`move.l dR,a3`)
  - least+2 = off, poke `lea off(a4),a2`
  - tokejsr lastring ($c700)
- bra getchar3.

### Detail 2: procfixer
a0 = 14(proc) = start of the 16 nop bytes.
- 6(proc) (link size) ≠ 0 → linka4+2 = 6(proc).
  - 8(proc)==0: 14(proc)=a0+6; linkput(a0+6). Total 10 bytes: link (4) + jsr clrloc (6).
  - else: linkput(a0); a0+=10; → .cont.
- 6==0:
  - 8==0: 14(proc)=a0+16; rts. The nops are skipped entirely and no rts is emitted.
  - else → .cont.
- .cont (if !nomemleft):
  - pc=a0; emit `jsr abs.l` ($4eb9, addoff, pokel garbage).
  - overwrite that long with the old (real) pc; restore pc. The jsr goes to the statics code emitted next.
- .contskip:
  - if 8(proc) bit15: tokejsr2 newmem ($c004), then d4 = 8(proc)&$7fff.
  - d4==0 → skip.
  - else n = 4(proc)&15 (#params):
    - n≥5 → poke `move.l d4,-(a7)` $2f04.
    - d3=locdatast; bsr datastart (emits `move.l #d3,a2` + reloc, `move #d4-1,d4`).
    - tokejsr2 localstat ($c501).
    - n≥5 → poke `move.l (a7)+,d4` $281f.
- finally poke rts $4e75 (tail bra pokewd).

### Detail 3: typemode (NewType field)
- bakup. d6 = flag: '*' → 1, '@' → 5, else 0; flagmask=d6.
- makename (beq syntaxerr). findvar in fields list (thistype+4): found → dupofferr; else addhere (append).
- push the field. Type suffix:
  - `.name` → findtype in alltypes. Not found and pointer: addhere a placeholder type with 8=linenumat, 4.l=$ff. Not found and not pointer: notypeerr.
  - `$` → stringtype.
  - none → prevtype (0 → notypeerr).
- If type ≠ bytetype (or string): typelen = even.
- d3 = 8(type) (size); prevtype=type; 10(field)=type; 4(field)=typelen. Pointer → d3=4.
- `[` → evalconst2 (d3 = value, d6 = float flag → fpconerr); `]` required (else syntaxerr); getchar3; 8(field)=n; d3*=n (mulu); flagmask|=2.
- typelen += d3 (≥32768 → toolongerr); 6(field)=flagmask; rts.

### Detail 4: doend .doend3 (local routine; ends `bra getchar3` or tail)
- $8001 End NewType:
  - linemode≠0 → modeerr.
  - typelen = even; 8(thistype)=typelen; linemode=-1; getchar3.
- $800b → mydoendif.
- $800c → dowend2.
- $8008 → endstate.
- $8009 End Function:
  - poke `moveq #0,d0` $7000.
  - if 5(thisproc)==7 (string): poke putstlen (`move.l d0,-(a7)` $2f00).
  - then endstate.
- $8030 End SetInt:
  - procmode≠0 → interr3.
  - debugga: maketrap(3). If !nomemleft: *(intcleanat)=pc and tokejsr gouse ($c30c).
  - intstring==0 → interr. If <0: poke `$7200|intlevel` (moveq #lvl,d1), tokejsr oldint ($c103), intlevel=0.
  - intstring=0. pokel intfin (`movem.l (a7)+,d2-d7/a2-a4`, 4cdf 1cfc), pokel $70004e75 (moveq #0,d0; rts).
  - if !nomemleft: *(intjmpat)=pc (written as `pc(pc)`, i.e. the same `pc` variable).
  - getchar3.
- $800e → doendsel (jmp, chunk 08).
- $8001+tnum → `jmp doendseterr`.
- else syntaxerr.

### Detail 5: endstate
1. If !nomemleft: for each firstpend node, *(4(node)) = pc (eos jumps land here).
2. debugga → maketrap(5).
3. calcstatic2(a2=firstlocal) gives d3 = data addr, d4 = #static struct locals. 8(proc)=d4; locdatast=d3.
4. If 12(memlib) <0 (memlib used in the proc): `bset #7,8(proc)` (statics word bit15) and tokejsr2 freelast ($c005). Otherwise 12(memlib)=memlibstat.
5. Push 14(proc) (the original entry; procfixer may change it). 6(proc)=locvaroff & ~3 (bclr #1/#0 of byte 7).
6. String function (5(proc)==7): poke getstlen (`move.l (a7)+,d0` $201f).
7. 6(proc)≠0: uselib 64635; poke `unlk a4` ($4e5c).
8. n = 4(proc)>>4 (#string params):
   - n==1: poke fix1 (`move.l (a7)+,(a7)`).
   - n>1: moverts+2=4n, poke `move.l (a7),4n(a7)`. Then if 4n≤8: poke `fixq|((4n&7)<<9)` (addq #4n,a7); else fixstack+2=4n, poke `lea 4n(a7),a7`.
9. normret; procfixer.
10. pop a0 (old 14(proc)); if !nomemleft: -4(a0)=pc (patches the jmp-around from dofunc2).
11. procmode=0, thisproc=0, procnum=0; droplocals; freepends; bra getchar3.

### Detail 6: donext
- Z (bare Next): a2=firstfor → .unknown2.
- Else, for each `var`: getvname (d2 = len). It must equal the innermost firstfor name: len == 16(node)-18 and the bytes match namebuff vs 17(node) (else noforerr). Then .unknown2; reget; EOL → rts; ',' → getchar3 and repeat; else syntaxerr.
- .unknown2 (local subroutine):
  - jsr popnext (debug: tokejsr snxtchk/+1 for long).
  - type=12(node). Template: nextb (1), nextw (2), nextf (5; tokejsr getffpbase first), else nextl. pokecode; pokel thebra ($6000fffe).
  - unlink firstfor.
  - If !nomemleft, patch the bgt at 8(node)-2 with pc-8(node)+2 (loop exit = here), and the bra at pc-2 with -(pc-4(node)-6). The bra target is 4(node)+4: it skips the template's first `move.l n(a7),a2`.
  - ≥32768 → bigforerr.
  - jsr popnext3 (emits the stack drop: addq #8 / lea 12, or 10/14 with debug).
  - FreeMem(node, 16(node)); bra reget.

### Detail 7: dofor
1. forlineat=linenumat; letstart=a5; getvname; fortemp=d2.
2. findfor: found → fornexterr. addhere2(firstfor) inserts at head; push node.
3. debugga: chkstak; pokel forset (`move #'fO',-(a7)` 3f3c664f).
4. d2=fortemp; dolet2. Checks:
   - d0 must be $8018 (To), else syntaxerr.
   - commode must be 0, else syntaxerr.
   - d2.b≥6 → badforerr.
5. If !(d2 bit15): pokewda5s leaamp (`lea d3(a5),a2`, a4 if varmode) + d3 word.
6. poke pushindex (`move.l a2,-(a7)`). 12(node)=d2&255.
7. eval fin. Push opcode = pushd0l ($2f00) if type≥3, else pushd0wd ($3f00); poke it (and keep it on the stack).
8. Step:
   - d0==$8019 (Step): eval the step.
   - else default step: type<4 `moveq #1,d0`; type==4 (quick) moveq #1 + `swap d0`; type 5 `move.l #$80000041,d0` (FFP 1.0).
   - poke the push opcode again.
9. 4(node)=pc. Compare template: forcompb/w/l, or for float forcompf + tokejsr getffpbase + forcompf2 (`jsr -42(a6)` = SPCmp).
10. pokel thebgt ($6e00fffe); 8(node)=pc; bra reget.

Runtime frame (word types): 0(a7) step, 2 fin, 4 index ptr. Long types: 0 step, 4 fin, 8 ptr. +2 more for 'fO' in debug.

### Detail 8: dolet2
1. pushstart; regat=1; fetchvar (a3 var, a2 type); calcvar (d2 type/flags, d3 offset; may emit address code); regat=0; pushdo (cuts that code out to a heap buffer); commode=0; pcodd=0.
2. **No '=' → a5 = letstart-1** (re-reads the name, so `a+1` means `a=a+1`).
3. .nextcom, string (d2.b==7):
   - save d2/d3, lastoffset, varmode; setsvars; eval with d2=7; restore.
   - poke moved0a3 (`move.l d0,a3` $2640).
   - commode: pokecode pulla2st (`move.l 4(a7),a2; move.l (a7)+,(a7)`); else pushput (re-inserts the cut address code).
   - restore d2/d3. If !bit15: leaa5d3a2+2=d3, pokela5s (`lea d3(a5),a2` / a4 if varmode).
   - uselib 65135; tokejsr lastring (+1 if varmode==0, global alloc).
   - ',' → poke addq4a2 ($584a), → .nextstr. Else rts.
4. Non-string:
   - bit14 (pointer) → d2.b=3. d2.b==0 (struct) → noleterr.
   - eval with d2&$ff (saving d2/d3, varmode, lastoffset); !commode → pushput.
   - d1=$2b40 (`move.l d0,d16(a5)`), varmode → bclr #9 ($2940, a4).
   - bit15 (address in a2): commode → poke pulla2 ($245f); d1=$2480 (`move.l d0,(a2)`). If ',' then d1=$24c0 ((a2)+), bsr .code, → .nextstr.
   - .code: byte → pcodd++, d1^=$3000. Else alignletpc; type<3 → d1|=$1000 (word). pokewd d1; if d1 bit7==0 also poke the d3 offset word. rts.
5. .nextstr (struct comma list):
   - lastoffset==0 → nocomerr. commode=-1; varcodelen=0; poke pusha2 ($2f0a).
   - field = *(lastoffset): 0 → comerr; lastoffset=field.
   - field flag bit1 (array) → nocomerr. d2 = 4(10(field)) (type code).
   - Pointer field: if code≥256 then d2=0; then bset 14.
   - Non-pointer: code≥256 → nocomerr.
   - bset 15 → .nextcom.

### Detail 9: dimwit
1. nonew=-1. Poke `move.l #esize,d0`: $203c, $0000, esize = 4 if pointer, else 8(type); +listadd. listsize = esize.
2. temp2=0; push a3. For each dimension:
   - temp2++; eval with d2=3; debugga → tokejsr arrchk ($c30f); tokejsr arrmult ($c402).
   - local: locvaroff-=4, putarr+2=locvaroff. Global: putarr+2=varoff, varoff+=4.
   - pokela5d putarr (`move.l d0,off(a5)`, a4 if procmode).
   - ',' → loop; ')' required (else syntaxerr).
3. pop a3. Check the dimension count:
   - temp1<0 → set 8(a3)=temp2 (#dims).
   - temp1 bit14 (list) → temp2 must be 1 (else redimerr).
   - else temp1 must equal temp2 (redimerr). Then `move extraword,(varoff or locvaroff)`. This is odd but harmless: the caller restores it. Keep it literally.
4. doarr+2=4(a3), doarr+6=putarr+2; pokel ×2 (`move #varoff,d2 / move #lastdim,d3`).
5. d1 = newarr ($c400) +1 if procmode. List: poke setles word ($383c) + listsize (`move #lsize,d4`), d1+=3.
6. tail bra tokejsr.

## B. Register conventions
- d0 = current char/token (from getchar3/reget/eval); Z = EOL status on handler entry.
- a5 = source char pointer (bakup → prevloc). dolet2 rewinds it to letstart-1.
- a2 = struct/list item from find*/addhere/fetchvar (fetchvar: a3=var, a2=type).
- d2 = name length after getvname/makename (also eval type in/out). d3 = var offset (after calcvar).
- d2 type code: low byte 1=b, 2=w, 3=l, 4=quick, 5=FFP float, 7=string, 0=struct. Bit15 = address already in a2; bit14 = pointer.
- `pc` is a **global long** (output pointer), not the CPU PC; it is saved/restored on the a7 stack to patch earlier code.
- a4 is clobbered by every poke (pokewd uses it).
- a6 = 4.w (exec) for FreeMem.

## C. Globals defined in range
| name | size | init | purpose |
|---|---|---|---|
| labcnt (13368) | w | 0 | On..Goto label count |
| localslist (13488) | l | 0 | unused here ("all locals for procs") |
| proclocals (13489) | l | 0 | output addr of the 2 debug longs after trap #4 |
| procnum (13490) | w | 0 | current proc # (0 = main); used by makealab/handlelab2 for scope checks |
| procnum2 (13491) | w | 0 | proc counter (cleared at 2494) |
| prolineat (13492) | w | 0 | line of the current proc (errchx error report) |
| forlineat (14225) | w | 0 | line of the last For (errchx) |
| fortemp (14226) | w | 0 | For var name length |
| pcodd (14509) | w | 0 | byte-field counter for (a2)+ alignment |
| listadd (14510) | w | 0 | 8 if Dim List, else 0 |
| listsize (14511) | w | 0 | list element size |

Templates: goset, forset, thebra, thebgt, add1a2, setles (section D).

Externals written here (defined elsewhere): firstif, firstpend, firstfor, alllocals, firstlocal, firstglob, thisproc, procmode, memlibstat, locdatast, letstart, commode, regat, varmode, lastoffset, varcodelen, temp1/2, nonew, varoff, locvaroff, stbuff (ds.l 8), typelen, linemode, thistype, prevtype, flagmask, lastchar, intstring, intlevel. The data templates funcst, least, linka4, moverts, fixstack, putarr, doarr and leaa5d3a2 get **their immediates patched in place** (+2 / +6) before poking.

## D. Machine-code templates (bytes checked with vasm -devpac -no-opt; all branches must be .w)
- **ongotocode..ongotocodef** (30 bytes):
  `b07c 0001 6d00 0014 b07c NNNN 6e00 000c d040 d040 207b 0004 4ed0 6000 SSSS`, followed by the emitted label table (labcnt longs, each relocated with addoff).
  - patch +10 = labcnt (cmp #n).
  - skip offset gc1skip-ongotocode = 26; patch +28 = labcnt*4+2 (bra past the table).
  - blt/bgt go to the bra.
- **ongosubcode..f**: identical, but `4e90` (jsr (a0)) instead of 4ed0; skip offset 26.
- **ongcode..f** (debug gosub, 34 bytes):
  `b07c 0001 6d00 0018 b07c NNNN 6e00 0010 d040 d040 207b 0008 3f3c 6753 4e90 6000 SSSS`; skip offset 30, patch +10 and +32.
- **goset** = `3f3c 6753` (move #'gS',-(a7)), pokel'd by dogosub (debug).
- **forset** = `3f3c 664f` ('fO').
- **thebra** = `6000 fffe`, patched later at disp.
- **thebgt** = `6e00 fffe`.
- **add1a2** = `524a`. **setles** = `383c` (+ listsize word). **nop** routine emits `4e71`.

Raw words poked in this range: $4ef9 jmp.l, $4eb9 jsr.l, $4e75 rts, $7000 moveq #0,d0, $7001, $7200|n, $203c + $80000041, $2f04, $281f, $2940/$3940/$1940|reg (param store), $2b40/$2940, $2480/$24c0 (±$1000/^$3000 for size), $70004e75. maketrap n = `$4e40|n` (trap #3 end SetInt, #4 proc entry + 8 bytes inline data, #5 proc end).

## E. Heap/linked structures
- **firstif node, 14 bytes** (allocated in doif; walked and freed here):
  - +0 next.
  - +4.l pc at If/While start (target of the Wend back-branch).
  - +8.l pc after the bcc.w (displacement at +8-2).
  - +12.w kind: low byte 0=If, 1=While; bit15 = one-line If..Then (bset #7,12).
- **firstpend node, 8 bytes** (eos): +0 next, +4.l address of the jmp long to patch with the epilogue pc. Freed by freepends.
- **firstfor node** (addhere2, d7=17):
  - +0 next.
  - +4.l pc of the compare code (loop top).
  - +8.l pc after bgt.w.
  - +12.w var type.
  - +16.b alloc size (=18+namelen).
  - +17 name\0.
- **label (firstlabel, d7=19)** (makealab):
  - +0 next.
  - +4 refs list; bit0 set = illegal (see handlelab2).
  - +8.l pc (0 = undefined).
  - +12 data ptr.
  - +16.w line/procnum.
  - +18.b size, +19 name.
  - ref node 12 bytes: {next, addr(=pc+2), procnum.w, line.w}.
- **proc (firstproc, d7=33)**:
  - +0 next.
  - +4.b #params | #stringparams<<4.
  - +5.b return type ($ff statement, else type 1..7).
  - +6.w link size (locvaroff &~3).
  - +8.w #static struct locals; bit15 = needs newmem/freelast.
  - +14.l entry pc.
  - +22.. param type bytes (≤6).
  - +32.b size, +33 name. Offsets 10-13 and 18-21 are not used here.
- **type (alltypes, d7=11)**:
  - +0 next.
  - +4.l builtin type code (<256), or newtype field-list head, or $ff = forward pointer placeholder.
  - +6.w = low word of the code.
  - +8.w size (placeholder: linenumat).
  - +10.b size, +11 name.
- **var/field (d7=15)**:
  - +0 next.
  - +4.w offset.
  - +6.w flagmask: bit0 pointer, bit1 array, bit2 '@'; vars: bit0 also = list array (dim).
  - +8.w #dims / array count.
  - +10.l type ptr.
  - +14.b size, +15 name.
- **alllocals node, 12 bytes**: {next, firstlocal, firstglob} (droplocals; freed by freeprocvs 7850).
- stbuff: 8 longs {w reg, w offset} for string params.
- memlib (lib struct): +12.w flags, bit15 = used.

## F. External calls
- exec FreeMem (-210) via `move.l 4.w,a6; jsr/jmp freemem(a6)`: freetheifz (size 14), freefors (size byte), donext.
- Allocation always goes through doallocmem, which calls **comdata+88** (editor alloc: d0=size, d1=1 MEMF_PUBLIC → d0=ptr).
- Blitz lib tokens via tokejsr/tokejsr2:
  - chkret $c305, clrloc $cc00, newmem $c004, localstat $c501, freelast $c005, lastring $c700(+1).
  - endjmp $c800, gouse $c30c, oldint $c103, arrchk $c30f, arrmult $c402, newarr $c400(+1,+3,+4).
  - getffpbase $c900, $d501 (Stop), stchk via chkstak, snxtchk via popnext.
- uselib: 65434 arrays, 65530 memlib, 64635 (unlk/link support), 65135 (strings), 65335 statics (in calcstatic2).
- Runtime-only (in templates): FFP SPCmp -42(a6), SPAdd -66(a6).

## G. Special handling for C
- **doif falls through into dothen.** doelse uses `bra dothen` (tail call).
- **Handlers receive the CCR (Z) from dispatch.** doend tests it, so pass "eol" explicitly in C (or re-test d0).
- smode drops its caller's return address (`addq #4,a7`) before jumping to typemode. typemode must therefore end the statement (C: return a flag, or longjmp-free restructuring).
- Error routines never return (syntaxerr, *err). docont and `jmp doendseterr/doendsel` are tail jumps.
- `pc` save/restore on the stack to patch earlier output (doongo, linkput, procfixer, endstate); the stack also carries intermediate values across calls (doongo, dofor, dofunc2, endstate). Use C locals.
- **Self-modifying data templates:** immediates in funcst/least/linka4/moverts/fixstack/putarr/doarr/leaa5d3a2 are patched before poking. In C, build the words directly (e.g. `pokel(0x2F2C0000|disp)`).
- The templates in D are copied out byte for byte. Keep them as const uint16_t arrays with the documented patch offsets (in C, don't take the address of compiled code).
- pokewd clobbers a4. dofunc2 relies on saving it, which is a non-issue in C.
- pokel of garbage d1 values (eos, dofunc2, procfixer, the debug longs) are always overwritten later unless nomemleft. Emit 0.
- `nomemleft` guards every back-patch.
- Devpac branch-size assumption: templates use word branches (6000/6d00/6e00 + disp).
- No traps/movec/hardware access in compiler code. Only the mwait template reads $bfe001 at runtime.
