# Survey 08 - blitz2.s 12136-13326 (built-in token handlers part 1, up to `dothen`)

Range starts with marker label `tokes` (12137, no code, unreferenced) and ends at
`bset #7,12(a0)` in doif2, which FALLS THROUGH into `dothen` (13327, chunk 09).

Handler entry convention (from dispatcher at 3155-3167, `handletoken`): the token
handler (`atokens[tok-1]`) is reached via `move.l a0,-(a7) / bra getchar3`, so on entry
d0 = first char/token after the keyword, Z = getchar3 result (Z=1 if 0, ':' or Else-token
$8000+34), a5 = source pointer, (a7) = return into `doline` (after `bsr dostatement`),
4(a7) = return into `passloop`/`passnxt` (or line 3849 direct mode).
Handlers return with d0 = last char (usually via `reget`/`getchar3`) - doline then checks
`reget` / `illeolerr`.

## A. Routines

Error labels used (all `bsr err` + inline string, never return): syntaxerr notypeerr
mismatcherr freeerr nosuperr noiferr illelseerr noleterr baddaterr notqerr sharederr
dupsherr noincerr readerr alincerr illdirerr dupmacerr macenderr(sets linenumat=macline
first) macbigerr cbraerr eselerr defaerr caseerr unterr1 unterr2.

### Type / MaxLen / Free / Use
- `dodeftype (12139)` : DefType .t [var,...]. in: d0 must be '.'; makename, findtype in
  `alltypes` (d7=11) ; out: no varlist -> `defaulttype`=type struct (a2), rts. With
  varlist: saves defaulttype on stack, sets it, loops getvname/fetchvar (',' -> getchar3),
  restores defaulttype. Leaves d0=last char.
- `maxprep1 (12165)`, `maxprep2 (12166)` : TEMPLATES (not executed), see D.
- `domaxlen (12168)` : MaxLen strvar=expr. getvname/fetchvar/calcvar -> d2,d3; d2.b must
  be 7 (string) else mismatcherr; pushes `varmode`; needs '='. If d2 bit15=0 emits
  `lea d3(a5|a4),a2` (via pokewda5s, which clears bit0 -> a4 when varmode<>0) ; emits
  `move.l a2,d0`; `addq #1,regat` (NOT restored); eval d2=3 (long); tail `bra tokejsr`
  with d1=maxlen ($d000) if saved varmode==0 else maxlen+1.
- `getmaxel (12196)` : common for Free/Use and line 10062 (Addr-like fn). in: d0=token
  (must be negative, else syntaxerr); bclr15 -> findtoke -> a2=lib. `18(lib).l` = offset
  to lib's max-info block, `-2(block)`=0 -> freeerr. eval d2=2 (word index into d(regat)).
  out: a2 = lib+18(lib) (max-info block ptr, fields at negative offsets, see E),
  `mymaxlib` = lib. Emits: if `debugga`: `cmp #max,dR` ($B07C|regat<<9, -4(a2)),
  maxchk long ($65000008 bcs.w +8), `tokejsr maxerr`($c30a) (a2 saved around it);
  then shift index left by -2(a2) bits: repeated `lsl.w #8,dR` ($E148|regat) while
  >=8, remainder `$E148|regat|(n<<9)`. QUIRK: shift count 0 emits lsl #8 (count field 0).
  Clobbers d0-d2.
- `free (12247)`/`freef (12249)` : TEMPLATE (D). Also read at 11360 (`move.l free,d1`).
- `mymaxlib (12251)` : data (C).
- `dofree (12253)` : Free X(i). getmaxel; patches template `free+2` = -6(a2); pokecode;
  d1 = -10(a2).l (sub offset) -> falls into maxjsr.
- `maxjsr (12262)` : in: d1 = lib-sub offset, CCR Z from the preceding `move.l` (Z -> 
  nosuperr). a2=mymaxlib; tail `bra makelibsub` (lib a2, offset d1).
- `douse (12266)` : Use X(i). getmaxel; patches `use+2` = -6(a2), `usef-2` = -6(a2)+4;
  pokecode use..usef; d1 = -14(a2).l; `bra maxjsr` (Z flag from move.l carried).

### If/Else, conditional compile
- `doelse2 (12279)` : getchar3 then falls into doelse. Called by doline (2786) on Else.
- `doelse (12280)` : in: firstif (0 -> noiferr); `13(ifnode).b==1` (While) -> illelseerr.
  Emits `bra.w` placeholder via `pokel` of `$6000<<16 | garbage` (d1 high word = $6000,
  low word = old d1 high word); calls doendif2 (chunk later, 13837) with d1=0, a0=ifnode
  (patches the IF's bcc to here); sets 8(ifnode)=pc (new pending disp = the bra);
  tail `bra dothen` (foreign, 13327).
- `docerr (12293)` : CErr "text". getsp -> namebuff; pushes `namebuff` address and
  `bra err` = faked inline-string error (stack trick, never returns).
- `docnif (12299)` : called by getchar cond-compile code (7123) with bsr. `evalconst`
  (bakup + const eval -> d3), d1=5 (index of cifne), bakup, a0=numcom, `bra compare`.
- `docsif (12306)` : called from 7116. getsp->namebuff, getchar3, collif -> d1 code,
  getsp->namebuff2, a0=strcom, falls into compare.
- `compare (12315)` : in: a0 = compare routine (sets CCR), d1 = cond index 0..11.
  Pushes `cifbras[d1]` (code ptr), d1=-1, `jsr (a0)`, pops & `jmp` to cifXX which tests
  the CCR left by the compare routine. COMPUTED JUMP + CCR passing (G).
- `cifeq cifne ciflt cifle cifgt cifge (12327-12337)` : `bCC cifdo / bra cifdont`
  (cifge falls into cifdont). Signed conditions.
- `cifdont (12339)` : d1=0, falls into cifdo.
- `cifdo (12340)` : push `comflag` word onto `*concomsp++` (no overflow check),
  comflag=d1 (-1 compile / 0 skip); getchar3, bne syntaxerr (must be EOL); tail
  `bra bakup`.
- `collif (12942)` : parse relational op. in: d0 = first char; ifchar ('<'=0,'='=1,'>'=2,
  Z=1) else syntaxerr. 1 char: d1=c1, out d0=next char (already read). 2 chars:
  d1=c1+3*(c2+1); (d1&3)==3 -> syntaxerr ('<<','==','>>'); getchar3. out: d1 index
  into cifbras (0 '<',1 '=',2 '>',4 '=<',5 '><',6 '<=',8 '>=',9 '<>',10 '=>'), d0 next.
  Only caller docsif (in range).

### Restore / Read / Data
- `rescode (12348)`, `rescode2 (12349)` : TEMPLATES (D), patched by uselib (5983-5984).
- `dorestore (12351)` : `uselib #64535` (data lib); reget Z -> .resbeg: `tokejsr
  getdstart`($cd00) + tail `jmp pokel` of rescode2 long (`move.l d0,dptr(a5)`).
  Else makealab (label struct a2; creates outstanding ref at pc+2 if undefined) ; emits
  `$2B7C`, `addoff` (reloc for next long), `12(a2).l` (label data ptr), rescode+6 word
  (dptr offset) = `move.l #lab_data,dptr(a5)`. If 4(a2) (ref list) <>0: `addq.l
  #1,4(firstref)` - make ref pc odd = "DATA reference" flag. rts.
- `doread (12384)` : Read v,... uselib 64535. per var: pokel `dataget`; getvname/
  fetchvar/calcvar -> d2 (type|bit14 ptr|bit15 addr-in-a2), d3 offset; bit14 -> d2.b=6;
  d2.b==0 -> noleterr. debug: emit `moveq #(d2&15),d0` ($70xx) + `tokejsr datachk`
  ($c310). Byte: bit15 -> dataletb2; else dataletb (global)/dataletbl (varmode<>0) with
  patched disp at +4. Word(<3)/long(dataletw/dataletl, 3-word tables): bit15 -> poke
  word at +4 (`(a3)+,(a2)`); else global long at +0 / local long at +6 with disp patched
  at +2 / +8. String(7): pokewd stlenget; if bit15=0 pokel leaamp/leaampl with patched
  +2; `tokejsr astring` ($c701, or $c700 if varmode<>0); lasta6=-1; pokecode stalign.
  .next: pokel `dataput`; d0==',' -> getchar3, loop. NOTE relies on d0 preserved across
  tokejsr/pokecode.
- `getsize2 (12483)` : in d0. '.' -> getchar3: 'b'->d1=0,'w'->1,'l'->2 (else syntaxerr),
  rts (no bakup). No '.' -> d1=1, tail `bra bakup`. out d1 size code.
- `dodcb (12501)` = `dodcb2 (12502)` (two names, same addr) : DCB.s n,v. getsize2;
  evalconst2 -> d3 count; ',' required; evalconst2 -> value. b: pokebyte loop; w/l:
  alignpc first then pokewd / 2x pokewd loop. do-while on count (count 0 -> 2^32).
- `dods (12532)` : DS.s n: getsize2, evalconst2, `pc += d3<<size` (no align, no limit
  check). The storage is not written (BUG(orig) #969; the port writes it as 0).
- `insasm2 (12540)` : save d0; a0=dummyasm, a1=pc, asmlen=asmlen2; bsr insasm (adds
  a fixup record for one-pass asm resolve, mode in d3); restore d0. rts.
- `dodc (12548)` : DC.s list. asmbuff=namebuff. Long: d3=11, alignpc, loop {asmconst,
  insasm2, pokel d1}. Word: d3=16, alignpc, loop pokewd. Byte: d3=17, loop {getchar3;
  '"' -> poke chars until '"' (0 -> syntaxerr), getchar3; else bakup, asmconst,
  pc-=1, insasm2, pc+=1 (fixup address = pc-1!), pokebyte}. ',' continues.
  NOTE: asmconst only stores expression TEXT; poked value d1 is a placeholder (whatever
  asmconst left in d1 - text length or 4); real value written later by asm resolver.
- `doeven4 (12600)` : pc=(pc+3)&~3 (and.w on low word pc+2). rts.
- `doeven8 (12605)` : pc=(pc+7)&~7.
- `doeven (12610)` = `alignpc (12611)` : pc=(pc+1)&~1. Also used by asmit (5072), dodcb,
  dodc. doeven4/doeven8/alignpc move pc only, the skipped pad bytes keep what the object
  buffer held (BUG(orig) #969; the port writes them as 0, `docs/libs/deviations.md`).
- `datasize (12615)` : data (C).
- `pdt (12617)` : if debugga: d0 = $4400|datasize.b ('D',type) ; tail `bra pokedata2`.
  else rts. Clobbers d0.
- `dodata (12629)` : Data[.t] v,... uselib 64535; bakup; `jsr sizespec2` -> d1 type;
  datasize=d1. String (7) -> .string. Else push type word; loop {pdt; d2=type;
  fetchdata -> d2 got-type, d0/d3 value; mismatch allowed only want=2&got=3&d3<65536,
  want=1&got=2&d3<256, else baddaterr; want<3 -> pokedata2 (word d0) else pokedata2l;
  reget; ','-> loop}; addq #2,a7. .string: {pdt; push data2; pokedata2l placeholder;
  d1=len=0; getchar3 (Z->done); '"' quoted loop (getchar3, 0->notqerr, until '"' then
  getchar3); ','->done; unquoted: pokedata2b d0, len++, raw `getchar` until 0 or ',';
  done: if !nomemleft store len.l at saved addr; data2=(data2+1)&~1; ','-> repeat}.
  Local `.isdone` unreferenced.

### Shared / Include / Macro
- `doshared (12716)` : procmode==0 -> sharederr. saves procmode, clr procmode. per name:
  getvname (d2 len, flagmask); search `firstglob` (findvar d7=15) for same name AND same
  flag byte (7(a2) vs flagmask+1) -> dupsherr; else addhere (append to list at a2=last);
  fetchvar (global since procmode=0) -> a3=global var; copy 5 words var+4..+13 into
  glob+4..+13. If flagmask bit1 (array): expect ')' then getchar3. ',' -> next.
  Restores procmode. rts with d0.
- `doxinclude (12760)` : makeiname; findxinc (firstxinc, d7=5) Z (already) -> rts. Else
  addhere (d2 saved), `bra doinc2`.
- `doincbin (12772)` : makeiname; dos Lock(namebuff,-2) (0->noincerr); Examine(lock,
  macrobuff as FIB); fib_DirEntryType(4)>=0 -> noincerr (lock leaked); d3=fib_Size(124);
  UnLock; Open(namebuff,1005) (0->noincerr); if pc+size > libat: nomemleft=-1,
  pc+=size; else Read(fh,pc,size) (-1->readerr, fh leaked), pc+=d0. tail `jmp close(a6)`.
- `doinclude (12811)` : dirmode -> illdirerr; makeiname; falls into doinc2.
- `doinc2 (12818)` : in: namebuff, d2 len. findinc Z -> alincerr. addhere2 on firstinc
  (insert at head); clr.w 12(a2), clr.l 4(a2); doallocmem(inclen=1024, 1) -> 4(a2) &
  14(a2) & a5; Open(namebuff,1005) -> 8(a2) (0 -> noincerr); numincs++;
  `move.l passstack,a7 / jmp passnxt` - NEVER RETURNS, unwinds stack to pass loop (G).
- `coolnest (12845)`, `macline (12847)` : data (C).
- `domacro (12849)` : Macro name. macline=linenumat; makename2 (Z->syntaxerr); bakup;
  findmac Z -> dupmacerr; addhere (append, firstmacro d7=13); clr.w 10(a2); a0=macrobuff
  write ptr; d2=0 (nothing yet); coolnest=0. Gather loop with `getcharb` (raw, no
  cond-compile): 0 -> save d2/a0/a2, `jsr nextline` (Z -> macenderr), restore; if
  something got put ':' byte. Token $800d (Macro) -> coolnest++, put word. Token $8002
  (End): getchar3; if $800d: coolnest--, <0 -> done; else put $8002 word then put d0 AS
  WORD (quirk: plain char -> $00,char). Other tokens put word, chars put byte.
  Done: `addq #4,a7` DROPS RETURN ADDRESS (returns past doline to passloop, G). Strip
  trailing ':' bytes (not if preceded by a byte with bit7 set = token low byte; always if
  len==1). 8(a2)=len.w; len<>0: doallocmem(len,1) -> 4(a2), copy. tail `bra getchar3`.
- `macputw (12933)` : put d0 high byte then falls into macputb.
- `macputb (12936)` : d2=-1; *a0++=d0.b; a0>=macrobufff -> macbigerr. in/out a0.

### Loops / Select / If core
- `thetst1..5, posbr, negbr (12962-12969)` : TEMPLATES (D).
- `dounless (12971)` : d2=-1, `bra doifm`. UNREFERENCED (dead; not in atokens).
- `dowhile (12974)` : d1=1 (while), d2=0, `bra doif2`.
- `freereps (12978)` : a2=&firstrep, d2=8, `bra freeslist`. Called from qfree (2000).
- `freesels (12982)` : a2=&firstsel, d2=14, falls into freeslist. qfree (2001).
- `freeslist (12985)` : d3=0, falls into freelist. Also `freeifs` (2028) jmps here.
- `freelist (12989)` : in: a2 = ADDRESS of list head, d2 item size, d3 offset of a sub
  list head inside each item (0 = none), d4 sub item size. Clears head, walks list,
  for each item: if d3 recursive freeslist(item+d3, d4); FreeMem(item,d2). a6=exec on
  exit. Clobbers d0-d1/a0-a2/a6.
- `firstsel (13014)` : data (C).
- `selpush*`, `selcom*`, `selbne`, `endsel` (13015-13030) : TEMPLATES (D). endsel also
  used by popselect (3498).
- `doendsel (13032)` : firstsel 0 -> eselerr; a2=sel; fillbne; for each node in
  4(sel) list: d1=pc-4(node), >32767 unsigned -> cbraerr, store word at 4(node) unless
  nomemleft; unlink & FreeMem 8. Then `jsr popselect` (3465, a2=sel: emits debug check
  + stack cleanup); unlink firstsel, FreeMem 14; tail `bra getchar3`. Also reached from
  13963 (bra).
- `endselstr..endselstrf..endselstrf2 (13070-13075)` : TEMPLATE used by popselect.
- `dodefault (13077)` : firstsel 0 -> defaerr; fillbra; fillbne; clr.l 8(sel);
  d2=12(sel); tail `bra casechk`.
- `fillbne (13086)` : in a2=sel. If 8(sel)<>0: d1=pc-8(sel) (>32767 -> cbraerr), store
  word at 8(sel) unless nomemleft. Does NOT clear 8(sel). Clobbers d0,d1,a0.
- `fillbra (13099)` : in a2=sel. If 8(sel)<>0 (previous case active): pokewd $6000;
  doallocmem(8,1) node, push on 4(sel) list, 4(node)=pc; tail `bra pokewd` with d1 =
  whatever doallocmem left (placeholder disp). Clobbers d0,d1,a0,a3,a6.
- `casechk (13121)` : in d2 type. if debugga: tail `tokejsr casechkw($c313)` (d2<3) or
  casechkl ($c314). else rts.
- `docase (13130)` : firstsel 0 -> caseerr; fillbra; fillbne; d2=12(sel); casechk;
  bakeval with d2=type; emit compare: b selcomb..selcombf, w selcomw..selcombf,
  l(3..6) selcoml..selcomlf, string(>=7) `tokejsr casestrcomp`($ce01). Then pokewd
  `$6600` (bne.w), 8(sel)=pc, tail `bra pokewd` ($6600 again = placeholder disp).
- `pcchk (13169)`, `clrst (13171)` : TEMPLATES (D).
- `sellineat (13173)` : data (C); read by 2735 for error line.
- `doselect (13175)` : debug -> pokel pcchk (`move.w #"sE",-(a7)`); sellineat=linenumat;
  pushstart; bakevalu d2=0 (any type) -> d2; pushdo (pull eval code out); type 7 ->
  pokewd clrst ($42A7); pushput (re-insert code). Push value: b selpushb..selpushbf,
  w selpushw..selpushbf, l(3..6) selpushl..selpushlf, string: pokewd moved0a3 ($2640),
  pokecode selpushs..selpushsf, `tokejsr astring`. Alloc SELECT node 14 (doallocmem
  14,1): next=firstsel, firstsel=node, 4=0, 8=0, 12=d2. rts.
- `doforever (13228)` : firstrep 0 -> unterr1; pokewd $6000; d1=4(rep)-pc (word must be
  negative else unterr2), pokewd; a6=exec; tail `bra unlinkrep`.
- `dountil (13241)` : firstrep 0 -> unterr1; bsr doif (creates IF node + `tst/beq.w`);
  reget bne -> syntaxerr; if !nomemleft: patch word at pc-2 = 4(rep)-(pc-2) (must be
  negative else unterr2); unlink firstif node, FreeMem 14; falls into unlinkrep.
- `unlinkrep (13266)` : in a6=exec. unlink firstrep, tail `jmp freemem(a6)` size 8.
- `iflineat (13271)` : data (C), read by 2741.
- `doif (13272)` : d2=0 (negate flag) falls into doifm.
- `doifm (13273)` : d1=0 (if flag) falls into doif2.
- `doif2 (13275)` : in d1 = 0 If / 1 While, d2 = 0 normal / -1 negate (Unless). Pushes
  d1.w, pc.l, d2.w; iflineat=linenumat; bakevalu d2=0 -> type d2. Emits test: float(5)
  `tokejsr getffpbase`($c900) + pokel thetst5 (`jsr -48(a6)` SPTst); byte thetst1,
  word thetst2, string(7) thetst4, else thetst3 (pokewd). Then pokel posbr (beq.w) or
  negbr (bne.w) if negate. doallocmem(14,1) IF node: 0 next=firstif, firstif=node,
  4 = start pc (popped), 8 = pc (after bcc; disp at pc-2), 12.w = d1 (popped).
  reget Z -> `ifdone` (rts, block If). Else `bset #7,12(node)` (one-liner flag = bit15 of
  word 12) and FALLS THROUGH into `dothen` (next chunk: bakup, lastchar=':', rts).

## B. Register conventions
- a5 = source char pointer (getchar*/bakup/reget). d0 = current char/token (token =
  $8000|n). Z flag from getchar3/reget = end of statement.
- `pc` (global long, 14900) = object code output pointer; pokewd/pokel/pokebyte/
  pokecode write there (pokewd word-aligns first). In C don't confuse with CPU PC.
- d2 = type code after calcvar/eval (low byte: 1 b,2 w,3 l,4 q,5 f,6 addr,7 str; bit14
  pointer; bit15 address already in a2), d3 = var offset (from a5 global / a4 local);
  `varmode` 0=global(a5) else local(a4) (set by fetchvar).
- `regat` = current data register number for generated code.
- d1 = word/long to poke for pokewd/pokel, token number for tokejsr, lib number for
  uselib, size code from getsize2.
- a6 = exec (4.w) before doallocmem/freemem; = `dos` for DOS calls.
- a2 = struct from findlab/addhere (z=1 found; else a2 = last for link); d7 = name
  offset in struct; d2 = name length (from makename/makeiname/getvname).
- tokejsr preserves d2/a3 (and code here assumes d0 too).

## C. Globals defined in range
| name | size | init | purpose |
|---|---|---|---|
| mymaxlib (12251) | long | 0 | lib of last getmaxel (for makelibsub) |
| datasize (12615) | word | 0 | current Data type (pdt) |
| coolnest (12845) | word | 0 | nested Macro depth in domacro |
| macline (12847) | word | 0 | line of Macro start (macenderr) |
| firstsel (13014) | long | 0 | head of SELECT stack |
| sellineat (13173) | word | 0 | line of last Select (errors) |
| iflineat (13271) | word | 0 | line of last If (errors) |
Code templates listed in D are also "globals" (patched in place, so mutable data).
`cifbras` (1294, outside) = 12 code pointers indexed by collif code:
`ciflt,cifeq,cifgt,0, cifle,cifne,cifle,0, cifge,cifne,cifge,0`.

## D. Machine-code templates (Devpac, no OPT -> bare Bcc = .w, d16(An) kept)
| label(s) | words | use / patch |
|---|---|---|
| maxprep1 | $45ED (0000) `lea 0(a5),a2` | only 1st word, via pokewda5s (->$45EC a4) |
| maxprep2 | $200A `move.l a2,d0` | 1st word |
| free..freef | $266D xxxx $D6C0 `move.l x(a5),a3; adda.w d0,a3` | +2 = -6(maxinfo); also read at 11360 |
| use..usef (918, outside) | $266D xxxx $D6C0 $2B4B yyyy | +2 = off, usef-2 = off+4 |
| lslimm (916) | $E148 `lsl.w #8,d0` | or regat, or n<<9 |
| maxchk (11831) | $6500 $0008 `bcs.w .ok` + `jsr 0`($4EB9 0 0) | only 1st long poked; skips the 6-byte jsr from tokejsr maxerr. ASSUMPTION: jsr 0 assembled abs.l; verify |
| rescode (12348) | $2B7C iiii iiii dddd `move.l #0,d(a5)` | +6 = data-ptr offset, patched by uselib(5983); only word0 and word@+6 used |
| rescode2 (12349) | $2B40 dddd `move.l d0,d(a5)` | +2 patched by uselib(5984) |
| dataget/dataput (946/948) | $266D dddd / $2B4B dddd | +2 patched by uselib (5981-82) |
| stlenget (950) | $2F1B `move.l (a3)+,-(a7)` | |
| stalign..stalignf (952) | $524B $C18B $0880 $0000 $C18B | round a3 up to even |
| dataletb..f (960) | $524B $1B5B dddd | +4 = d3 |
| dataletbl..f (964) | $524B $195B dddd | +4 = d3 |
| dataletb2..f (968) | $524B $149B | |
| dataletw (974) | +0 $3B5B dddd, +4 $349B, +6 $395B dddd | +2 / +8 = d3 (global / local) |
| dataletl (980) | +0 $2B5B dddd, +4 $249B, +6 $295B dddd | same layout |
| leaamp / leaampl (1071/1074) | $45ED dddd / $45EC dddd | +2 = d3 (patched in place) |
| moved0a3 (1107) | $2640 | |
| thetst1..4 | $4A00 / $4A40 / $4A80 / $4A9F (`tst.l (a7)+`) | 1 word each |
| thetst5 | $4EAE $FFD0 `jsr -48(a6)` (SPTst) | long |
| posbr / negbr | $6700 $FFFE / $6600 $FFFE (`beq.w/bne.w *`) | disp at pc-2 patched later (doendif2/dountil) |
| selpushb..selpushbf | $C07C $00FF $3F00 | |
| selpushw..selpushbf | $3F00 | |
| selpushl..selpushlf | $2F00 | |
| selpushs..selpushsf | $45EF $0004 `lea 4(a7),a2` | |
| selcomb..selcombf | $C07C $00FF $B057 | |
| selcomw..selcombf | $B057 `cmp.w (a7),d0` | |
| selcoml..selcomlf | $B097 `cmp.l (a7),d0` | |
| selbne | $6600 ($FFFE) | only 1st word; disp placeholder poked separately |
| endsel | $504F `addq.w #8,a7` | popselect ORs n<<9 |
| endselstr..endselstrf..endselstrf2 | $225F $5149 $7009 $D091 / $544F | popselect; f2 variant (debug) adds `addq #2,a7` |
| pcchk | $3F3C $7345 `move.w #"sE",-(a7)` | long |
| clrst | $42A7 `clr.l -(a7)` | |
Inline-built opcodes: `$B07C|r<<9` cmp #imm,dr; `$70xx` moveq #t,d0; `$6000`/`$6600`
bra.w/bne.w + disp; `$6000xxxx` bra.w in doelse.

## E. Heap / linked structures
- IF node (`firstif`, 14 bytes, doallocmem 14,1; freed 14 by dountil/freeifs):
  0 next.l, 4 start pc.l (loop-back for While), 8 pc after cond branch.l (disp word at
  -2; doelse replaces with its bra), 12 flags.w: low byte 0=If 1=While, bit15 one-liner.
- REPEAT node (`firstrep`, 3501, 8 bytes): 0 next.l, 4 pc of Repeat.l.
- SELECT node (`firstsel`, 14 bytes): 0 next.l, 4 head of pending end-of-case bra list.l,
  8 address of pending bne disp word.l (0 = none/default), 12 type.w.
  Bra list node (8): 0 next.l, 4 address of bra disp word.l.
- Max-info block (lib + 18(lib).l), negative offsets: -2.w shift count (log2 elem
  size; 0 -> no max -> freeerr), -4.w max count, -6.w global var offset of current-item
  pointer (used ptr at +4), -10.l Free sub offset, -14.l Use sub offset. (lib: 14.w/16.w
  token range used by findtoke.)
- Label (`firstlabel`, d7=19, via makealab): 0 next, 4 ref list (odd = data ref flag
  per ref pc), 8 pc (0 = undefined), 12 data ptr, 16.w line/proc, 18.b len, 19 name.
  Ref node (12): 0 next, 4 pc of ref (+1 = DATA ref), 8.w procnum, 10.w linenum.
- Var/glob (`firstglob`, `firstvar`, d7=15): 0 next, 4..13 payload copied by doshared
  (4.w offset, 6.w flags, 7.b flag byte bit0 ptr bit1 array, ...), 14.b len, 15 name.
- Type (`alltypes` list, d7=11): 4.l size (<256 for data), 6.w type code, 10.b len,
  11 name.
- Include (`firstinc`, d7=19, head insert): 0 next, 4 buffer.l (inclen=1024), 8 fh.l,
  12.w bytes in buffer, 14 read ptr.l, 18.b len, 19 name. `numincs` counter.
- XInclude (`firstxinc`, d7=5): 0 next, 4.b len, 5 name.
- Macro (`firstmacro`, d7=13): 0 next, 4 text ptr.l (len bytes, doallocmem), 8 len.w,
  10.w cleared, 12.b len, 13 name. Build buffer `macrobuff`..`macrobufff`.
- Conditional compile stack: `concomsp` -> `concomstack` (32 words), pushes `comflag`.
- Data area: `data2` write ptr (bounded by `allat`), Data items stored as words for
  b/w, longs for l/q/f, strings as len.l + bytes + even pad; debug prefix word 'D'<<8|t.

## F. External calls
- exec: FreeMem(a1,d0) via `freemem(a6)` (freelist, doendsel, dountil, unlinkrep).
- Allocation through editor callback `doallocmem` = `comdata+88` (d0 size, d1 flags
  1=MEMF_PUBLIC, out d0 ptr; result never checked here).
- dos (a6=`dos`): Lock(-2), Examine (FIB in macrobuff), UnLock, Open(1005), Read,
  Close (doincbin); Open (doinc2).
- Compiler internals called: makename/makename2/makeiname/makealab/getvname/fetchvar/
  calcvar/findtype/findvar/findlab/findinc/findxinc/findmac/addhere/addhere2/eval/
  bakeval/bakevalu/evalconst/evalconst2/fetchdata/sizespec2/getsp/ifchar/getchar/
  getchar3/getcharb/reget/bakup/nextline/uselib/findtoke/tokejsr/makelibsub/addoff/
  pokewd/pokewda5s/pokel/pokebyte/pokecode/pokedata2/pokedata2l/pokedata2b/asmconst/
  insasm/pushstart/pushdo/pushput/popselect/doendif2/strcom/numcom/err.
- Token numbers (equ, outside): maxlen $d000, maxerr $c30a, datachk $c310, astring
  $c701, getdstart $cd00, casestrcomp $ce01, getffpbase $c900, casechkw $c313,
  casechkl $c314; data lib 64535.

## G. Special handling for C
- `compare`/cifXX: computed jump through `cifbras` and CCR passed from numcom/strcom.
  C: compare fn returns signed int; cifbras -> table of predicate ids. strcom compares
  bytes SIGNED (cmp.b + blt/bgt); numcom = tst.l d3 (signed).
- `maxjsr` relies on Z flag from caller's `move.l` across a label (make it test d1==0).
- `docerr` fakes the err inline-string pointer (push namebuff, bra err).
- `domacro` `addq #4,a7`: drops return into doline -> returns straight to passloop's
  caller frame (skips doline's EOL check / rest of line / un-then). Needs an
  "abort doline" flag/longjmp in C.
- `doinc2`: `move.l passstack,a7; jmp passnxt` - non-local exit to pass loop (longjmp).
- Self-modifying templates patched in place: free, use/usef, dataletb/bl, dataletw/l,
  leaamp/leaampl (here) and dataget/dataput/rescode/rescode2 (by uselib). In C use
  mutable globals (or build the words) - but keep patch semantics since uselib
  patches them once and many users read them.
- maxchk: displacement depends on assembled size of `jsr 0`; verify bytes ($65000008
  assumed).
- Fallthroughs: doif->doifm->doif2->dothen (foreign); freesels->freeslist->freelist;
  cifge->cifdont->cifdo; macputw->macputb; doelse2->doelse; doinclude->doinc2;
  dountil->unlinkrep; dofree->maxjsr; dodcb==dodcb2; doeven==alignpc. Template blocks
  sit between routines (never executed).
- Tail branches into foreign code: tokejsr, makelibsub, pokel, pokewd, pokedata2,
  bakup, getchar3, dothen, doinc2 (local), freeslist, close(a6), freemem(a6).
- pc alignment done by byte/word ops on halves of the long `pc` (pc+2, pc+3).
- Placeholder disp words written with garbage d1 (doelse low word, fillbra after
  doallocmem); write 0 in C (patched later or not, nomemleft case).
- Quirks to preserve: getmaxel shift 0 -> lsl #8; dodcb count 0 -> 4G loop; domacro
  End+non-token writes 2 bytes; doincbin leaks lock/fh on error; domaxlen leaves
  regat incremented; dorestore `.no` path does nothing ("not working" comment).
