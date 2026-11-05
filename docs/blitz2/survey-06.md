# Survey chunk 06 - blitz2.s 8451-10683

Code emitter primitives (pokewd & co), type conversion, the expression evaluator
(maineval / eval5 precedence climbing with compile-time constant folding), operator
code generators, element fetchers (hex/bin/const/asm/SizeOf/Used/Addr/Maximum,
library + local function calls, variable fetch, literal strings, `?label`).

Type codes used everywhere (d2): 0=unknown/struct addr, 1=byte 2=word 3=long 4=quick
5=float(FFP) 6=address(long-like) 7=string. "regat" = current Dn the expression
result goes into; right operand of a binary op is always D(regat+1).
Opcode helpers: `regat<<9` (written as `lsl #8 / lsl #1`) = dest-reg field,
`regat` alone = source-reg field.

## A. Routines

### Emitters
- pokewda5s (8453): in d1 opcode word; if varmode!=0 clear bit0 (src reg a5->a4); falls to pokewd. 
- pokewda5d (8458): same, tests varmode, clears bit9 (dest reg a5->a4); -> pokewd.
- pokela5d (8463): d1.l; tests **procmode** (not varmode!), clears bit25; -> pokel.
- pokela5s (8468): d1.l; tests varmode, clears bit16; falls into pokel.
- pokel (8472): emit d1.l (high word then low); d1 preserved (swapped twice). Multiple entry: pokelc (8474) = second half only (swap d1 then pokewd) - not referenced elsewhere.
- pokewd (8476): emit d1.w at `pc`. First rounds pc up to even (addq #1 / bclr #0,pc+3). If pc>=libat: no write, nomemleft=-1, pc still +=2. If new pc>bigpc and cfetchmode==0: bigpc=pc. out: a4=new pc (clobbered), d1 kept.
- pokebyte (8496): emit d1.b at pc, no alignment; same libat/nomemleft/bigpc logic; a4 clobbered.
- pokecode (8514): in a0=start,a1=end; emits words [a0,a1) (always >=1 word, do-while); a0 left = end; d1,a4 clobbered.
- pokecode2 (8525): identical semantics (do-while).

### Type conversion (all emit code that converts D(regat) in place; in: regat, fuckpos; clobber d1,d4,d5,a0,a4)
- bytetoword (8533): no-op (rts; dead code after). Bytes are already ext.w'd at fetch (varcont).
- bytetolong (8541): bytetoword; falls to wordtolong.
- wordtolong (8543): emit ext.l Dr ($48c0|regat).
- bytetoquick (8546) -> falls wordtoquick (8548): emit swap Dr ($4840|r), clr.w Dr ($4240|r).
- bytetofloat (8554): bytetolong then falls into bytetof2.
- bytetof2 (8556): d1=-36 (SPFlt); falls into fdo2.
- fdo2 (8557): in d1 = FFP LVO; patches libjsr+2; savereg; pokemovem(d4); tokejsr(getffpbase) ; ritetod0; pokel(libjsr) [jsr lvo(a6)]; ritefromd0; pokemovem(d5). Has a dead `rts` after the tail bra.
- wordtobyte (8570): d1=wtobover; falls into overchk.
- overchk (8573): in d1 = lib token of overflow-check routine. Only if debugga.b!=0 && btst #7,optreq2ga4+13 (options gadget, "overflow checking") && cfetchmode==0. If regat==0: tokejsr(d1) with movem d0-d7/a0-a6 saved around it (compiler regs preserved). Else emit move.l d0,-(a7) ; move.l Dr,d0 ; <tokejsr> ; move.l (a7)+,d0. Uses inline templates .code1/.code2/.code3 (D).
- wordtofloat (8604): wordtolong ; bra bytetof2.
- longtobyte (8607): overchk(ltobover). longtoword (8610): overchk(ltowover).
- longtoquick (8613): overchk(ltowover) ; bra wordtoquick.
- longtofloat (8617): bra bytetof2 (dead rts).
- quicktobyte (8620): overchk(qtobover); falls quicktob2.
- quicktob2 (8624): emit clr.w Dr ; swap Dr. quicktoword (8630): bra quicktob2.
- quicktolong (8632): quicktob2 ; bra wordtolong.
- quicktofloat (8635): d1=qutofl; falls qtof2.
- qtof2 (8639): in d1 = lib token: savereg; pokemovem d4; ritetod0; tokejsr(d1); ritefromd0; pokemovem d5. (Blitz-lib version of fdo2.)
- floattobyte/floattoword (8652/8659): overchk(ftobover/ftowover), floattolong2, then bra longtobyte/longtoword (double overflow check).
- floattolong (8666): overchk(ftolover); falls floattolong2 (8669): d1=-30 (SPFix) ; bra fdo2.
- floattoquick (8673): overchk(ftowover); d1=fltoqu ; bra qtof2.
- stringtolong (8681): emit stol (addq #4,a7 - drop string length) ; bra sbtolong.
- convtypef (8686): fuckpos++ ; convtype ; fuckpos-- (forces savereg to also save d1 when regat==0 because regat+1 is live).
- convtype (8693): in d3=from type, d2=to type (1..7). idx=((d3-1)*7+(d2-1))*4 into convtable (global, outside range, 7x7 code ptrs, 0=nothing, noconerr = error). Computed `jmp (a0)` tail call. clobbers d1,a0. Does NOT change d2 (caller's d2 already = target).
- sbtolong (8769): make string base into a long in D(regat). Emits clr.b (a3)+ (ceos). If sbgot>=2: d3=stackuse-thisstuse; ==6 -> emit movestst2 (first word |regat<<9, then rest via pokecode2); else emit movea7dn (word|reg<<9, then the long at movea7dn+2). If sbgot<2: emit movestdn|reg<<9, uselib(65235 = string lib), emit word 10(a1) (lib base a5-offset) as displacement.

### Misc helpers
- getvname (8709): in d0=first char (already read). If '\': usedfrom=a5-1, a5=usedpath (source redirected into the "UsedPath" buffer; getchar restores a5 from usedfrom on char 2 - line 7066), getchar3. flagmask=0; '*' -> flagmask=1, '@' -> 5, then makename (else makename2 = bakup+makename). Z=1 from makename -> syntaxerr. Then '(' -> flagmask|=2; '.' -> storeloc, read type name into namebuff2 via makename3 (d2 preserved), resloc, if next was '(' flagmask|=2; '$' -> flagmask|=8, storeloc, getchar3, then same '(' check via resloc. out: namebuff, namebuff2, flagmask, d0 = char after name (after resloc d0 = char saved by storeloc!), d2 = name length. Note: after '.'/'$' branch, resloc rewinds a5 to the '.'/'$' (so caller re-reads the suffix).
- dopusha3 / dopulla3 (8755/8761): uselib(65235); pokecode pusha3..pusha3f / pulla3..pulla3f (templates outside range, disps patched at 5973).
- arrevalchk (8809): if debugga==0 -> bra sharreval; else arreval then falls into arrszchk.
- arrszchk (8812): emit cmpi.l #$10000,Dr ; bcs.w +8 ; then tokejsr(arrerr) (must be a 6-byte jsr abs.l). Uses template somecode/somecode2.
- sharreval (8823): d2=2,d1=0,d3=0 -> maineval2. arreval (8828): d2=3,d1=0,d3=0 -> maineval2.

### Evaluator entry points (all tail into maineval; in d2 = wanted type)
| entry | pre | d1 (push) | d3 (unknown) |
|---|---|---|---|
| bakpevalu (8833) | bakup | falls into pevalu | |
| pevalu (8835) | | -1 | -1 but then `bra peval` which resets d3=0 -> **pevalu == peval** (quirk) |
| bakpeval (8839) | bakup | falls peval | |
| peval (8841) | | -1 | 0 |
| bakevalu (8845) | bakup | falls evalu | |
| evalu (8847) | | 0 | -1 |
| evalu2 (8851) | | 1 | -1 |
| bakeval (8855) | bakup | falls eval | |
| eval (8857, label `eval:`) | | 0 | 0, falls into maineval |

- maineval (8860): in d1 push flag (0 none, <0 push result, >0 push type word when known), d2 type wanted (0 = any), d3: 0 = convert result to d2, !=0 = d2 is hint only ("unknown"). If regat>=6: d1=-1 and regat=6 (**regat not restored**). Falls into maineval2.
- maineval2 (8879): saves thisstuse, pushflag, sbgot, unknown, d2 on a7; thisstuse=stackuse; pushflag=d1; sbgot=0; unknown=d3. eval3. If wanted!=0 && unknown==0: convtype(got->wanted). pushflag>0: [if string: emit getstlen|r<<9 (move.l (a7)+,Dr)], stackuse+=2, emit moveq #type,D(r+1) ($7000|(r+1)<<9|d2), move.w D(r+1),-(a7) (pushut|r+1), [string: putstlen|r]. If d2==7: stackuse+=4, sbtolong. Else if sbgot>=2 emit ststfix (addq #4,a7). Restore sbgot. pushflag<0: stackuse+=2 (word) or +=4 (d2>=3), emit move.w/.l Dr,-(a7). Restore pushflag, thisstuse. out: d2 = result type, d0 = terminator char (char after expression, from getchar3/reget), Z meaningless. Strings: length long on stack, a3 = end of string, D(regat) = start.
- eval3 (8968): eval4 ; forthsp -= 18 (drop final forth entry).
- eval4 (8972): pushprec(a0=garbage, d1=1 sentinel) ; eval5 ; d1!=0 -> syntaxerr ; precsp -= 6. Used by bracket/notit for sub-expressions.
- eval5 (8983): recursive precedence climbing. eval2 -> (d1 prec, a0 op). Loop: if d1 > prec on top of precstack (unsigned bhi): pushprec; push d2 (left type); regat++; eval5 (right; returns next op in d1/a0); regat--; d3=left type; push d1/a0; if d3!=0 convtypef(left d3 -> right type d2); popprec (a0=op); doop. Then **constant folding** (see G). Then forthsp -= 18, pop d1/a0, reget, loop. Else rts with d1/a0 = pending operator (0 if none).
- eval2 (9085): getelement; scan `operators` table (32-byte entries, word 0 terminated) for d0. '<' '>' '=' peek next char (getchar3) for 2-char ops: `<=`->ople, `<>`->opne, `>=`->opge, `><`->opne, `=>`->opge, **`=<`->opne (quirk/bug, not ople)**; non-matching peek -> bakup. out: d1 = 30(a0) prec (0 if no operator), a0 = entry, d2 type, d0 op char.
- doop (9142): in a0 op entry, d2 type; d2-1<0 -> illoperr; handler = 2+(d2-1)*4 (a0) ; 0 -> illoperr; `jmp (a0)` tail.
- pushprec (9153): in a0,d1; stores {a0.l,d1.w} at precsp, precsp+=6. **no overflow check**. clobbers a1.
- popprec (9161): if precsp >= precstackf -> pserr (check at pop time); out d1 prec, a0 op; precsp-=6. a1 clobbered.

### Operator code generators (jumped to via doop; in regat (left Dr), regat+1 (right); out d2 = result type; all tail-call pokewd/etc.)
- Compares: do{eq,ne,lt,le,gt,ge}{b,w,l} (9189-9226): d1 = cmp0/cmp1/cmp2 template (cmp.b/.w/.l), bra doXXcmp2.
- doXXcmp2 (9300..9320) = bsr cmpit then fall into doXXcmp. cmpit (9292): emit d1|(regat+1)|regat<<9 = `cmp.x D(r+1),Dr`. doXXcmp (9301..9321): d1 = Scc template, fall to cpoke.
- cpoke (9322): emit Scc Dr (template|regat), ext.w Dr ($4880|r); d2=2. (true=-1 word.)
- docflib (9229): d1=FFP LVO -> libjsr+2; savereg; pokemovem d4; tokejsr(getffpbase); ritetod0; pokel libjsr; cfregat=regat; regat=0. (so subsequent Scc/ext go to d0, uses CCR from SPCmp.)
- docflib2 (9242): regat=cfregat; ritefromd0; pokemovem d5.
- do{eq..ge}f (9248-9271): docflib(-42 SPCmp), doXXcmp, bra docflib2.
- do{eq..ge}s (9273-9290): tokejsr(strcomp), bra doXXcmp (Scc on regat, after lib sets CCR).
- doplusb (9334): d1=$d040, d2=2 -> doplus2. doplusw: $d040, d2=3, doplus2 then bra wordtolong. doplusl (9341): $d080, falls doplus2.
- doplus2 (9343): emit d1|(regat+1)|regat<<9 (op.x D(r+1),Dr). Shared by minus/times/div/and/or. Clobbers d3.
- doplusf (9351): d1=-66 (SPAdd), falls doflib.
- doflib (9355): same sequence as fdo2 (patch libjsr+2, savereg, movem, getffpbase, ritetod0, jsr, ritefromd0, movem).
- addstrings (9368): emit addcode word0|r<<9 and word1|r<<9 (`move.l (a7)+,d0 / add.l d0,(a7)` with dest/src reg-field patched by OR) - concatenation length add.
- dominusb/w (9380/9383): $9040 -> doplus2 (type not changed; byte minus stays type 1). dominusl $9080. dominusf -72 (SPSub) -> doflib.
- dotimesb (9402): $c1c0 (muls.w), d2=2. dotimesw: $c1c0, d2=3. -> doplus2.
- dotimesq (9410): d1=quickmult -> domylib. dotimesl (9413): d1=longmult, falls domylib.
- domylib (9414): in d1 lib token: savereg; pokemovem d4; ritetod0; tokejsr(d1); ritefromd0; pokemovem d5.
- dotimesf (9425): -78 (SPMul) -> doflib.
- dopow (9428): convert left (regat) to float (convtypef d3=d2->5); regat++ ; convert right d3=orig->5 ; regat--; nocando; d1 = mathq*128+1 -> domylib. (d2 ends 5.)
- nocando (9441): forthsp top entry constgot = -1 (word at -2(forthsp)): disables folding.
- domodb/w (9449/9452): modbyte/modword -> domylib. domodl: nocando, modlong. domodq: modquick, d2=2. domodf: nocando, modfloat, d2=3 (sic).
- dodivb (9472): emit ext.l Dr, ext.l D(r+1) (via addq #1,d1) then falls dodivw. dodivw (9478): emit ext.l Dr, then $81c0 (divs.w) -> doplus2. (no ext of r+1 in word case; quotient in low word.)
- dodivl/dodivq: longdiv/quickdiv -> domylib. dodivf: -84 (SPDiv) -> doflib.
- doand{b,w,l}: $c000/$c040/$c080 -> doplus2. door{b,w,l}: $8000/$8040/$8080 -> doplus2.
- doeorb (9510): $b100, falls doeor2 (9512): emit d1|regat|(regat+1)<<9 = `eor.x D(r+1),Dr` (EOR encodes source Dn in bits 9-11, dest in ea field - hence different from doplus2). doeorw $b140, doeorl $b180.
- dolslb/dolslw/dolsll (9526-9528, 3 labels same address): $e1a8 (lsl.l Dx,Dy) -> shpoke. dolsr{b,w,l}: $e0a8. doasr{b,w,l}: $e0a0.
- shpoke (9547): convert right (regat+1) to word (convtype, d3=d2,d2=2); if left type!=4 convert left to long (convtypef ->3); emit d1|regat|(regat+1)<<9 -> `lsx.l D(r+1),Dr`. d2 = original type (unchanged even if left became long - type left as is).
- bittst (9572): dothebit($0100 btst); emit sne Dr ($56c0|r), ext.w Dr; d2=2.
- bitset (9586): d1=$1c0 (bset), falls dobitmip (9588): dothebit; d2=3. bitclr $180, bitchg $140 -> dobitmip.
- dothebit (9599): left -> long (convtypef), right -> byte (regat+1 convtype); emit d1|regat|(regat+1)<<9 (`bxxx D(r+1),Dr`).

### Register move helpers
- ritetod0 (9623): if regat!=0 emit move.l Dr,d0 ($2000|r) and move.l D(r+1),d1 ($2201|r computed as +$201). Nothing when regat==0.
- ritefromd0 (9633): if regat!=0 emit move.l d0,Dr.
- savereg (10283): out d4 = $48e7<<16 | mask (movem.l -(a7)), d5 = $4cdf<<16 | mask. regat!=0: save d0; regat>=2: also d1. regat==0 && fuckpos!=0: d1 only. clobbers d1.
- pokemovem (10279): if d1.w (mask) != 0 -> pokel(d1).
- savem (10502): if regat!=0 emit movem.l d0-d(regat-1),-(a7) ($48e7,mask). out d3 = predecrement mask (bits 15..16-regat) or 0. Caller later emits $4cdf + bit-reversed d3 (loop lsl/roxr 16x) - inline in libfunction and afunction.
- makefjsr (10515): in d1 address; emit jsr abs.l ($4eb9), addoff (reloc), pokel(d1).

### Elements (getelement / getelement2 family; out d2 type, d0 next char; mostly tail jumps)
- getelement (9643): if regat>=7 spill: emit move.w/.l d6,-(a7) ($3f06/$2f06 by incoming d2<3), regat--, getelement2, regat++, emit move.w/.l d6,d7 ($3e06/$2e06 by new d2), emit (saved & $f000)|$0c1f = move.x (a7)+,d6. else bra getelement2.
- negate (9667): getelement2; neg.w ($4440|r) for d2<3, neg.l ($4480) for 3,4, float -> d1=-60 (SPNeg) bra fdo2, >=6 illoperr.
- bracket (9681): eval4; require ')' or '}' (else syntaxerr); bra getchar3.
- notit (9688): eval4 (NOT binds whole rest of expression); d2>=5 illoperr; emit not.w ($4640|r) or not.l ($4680) for d2>=3. No getchar (d0 already terminator).
- fetchpi (9700): d2=5, emit move.l #$c90fda42,Dr (FFP pi), getchar3.
- getelement2 (9718): getchar3; '('/'{' -> bracket; '-' -> negate; NOT token ((opnot-opabcd)/$1c+$8000+fnum) -> notit. Push forth entry (overflow -> conmemerr). Constant-capable: tstnum -> fetchnum, '.' fetchfrac, '#' fetchconst, '$' fetchhex, '%' fetchbin, $8000+47 fetchsize (SizeOf), $8007+tnum fetchpi, $8000+tnum+18 fetchon, +19 fetchoff. If constmode!=0 (assembler operand): alpha/'_' -> fetchasm, '"' -> fetchqasm, else syntaxerr. Else if cfetchmode!=0 -> badconerr. Mark entry non-const (`not -(a1)` -> constgot=$ffff). alpha/'*'/'\' -> variable, '"' litstring, bit15 token -> function, '&' ampersand, '?' qmark, else syntaxerr.
- fetchsize (9801): getchar3; '.' -> `.sizeobj`: makename, findtype(alltypes) (else notypeerr); optional '\' field: makename, findvar in field list 4(type) (else noofferr), d3=4(field).w; else d3=8(type).w (size). Else (var): bakup, makename, findvar(firstvar) (else notypeerr), d3=4(var).w ext.l. d2 restored, bra fetchnum2.
- fetchon / fetchoff (9839/9843): d3=-1/0, getchar3, fetchnum2.
- chkhex (9847): Z=1 if d0 hex digit. **Side effect: d0 &= $ffdf for anything > '9'** (uppercased even if not hex).
- chkbin (9861): Z=1 if '0'/'1'.
- fetchhex (9866): first must be hex (else syntaxerr); accumulate d3 (if d3>=$10000000 before shift -> overerr); fetchnum2. fetchbin (9883): carry out -> overerr; fetchnum2.
- fetchconst (9896): makename, findadd (firstlabel list), not found or 4(a2)!=1 -> cnferr; d3=8(a2).l; fetchnum2.
- ampersand (9906): ampersand2; d2=3; rts (returns to getelement2's caller).
- ampersand2 (9910): getchar3, getvname, fetchvar, calcvar. String var (d2.b==7): bit15 set -> emit move.l (a2),Dr (stamp|r<<9), rts; else patch stamp2+2=d3, pokela5s(stamp2 = move.l d3(a5),a2), then move.l a2,Dr. Non-string: bit15 clear -> patch leaa5d3... (leaamp+2=d3), pokela5s(lea d3(a5),a2); then emit leaamp2|r<<9 (move.l a2,Dr). Also called from 5703.
- doern (9941): tokejsr(ern); d2=3; rts. **Does not getchar** - d0 still the Err token (works since token isn't an operator -> expression ends; reget in caller?).
- fetchit (9952): getchar3; must be token (bit15) else syntaxerr; findtoke(d0&$7fff) -> a2 lib, a3 sub; d0 = 18(a2).l; word -2(a2,d0) == 0 -> freeerr. d2 preserved. out a2, d0 (offset to lib's object info).
- doused (9989): Used Obj: d3 = shift -2(a2,d0). shift<9 -> template usedcode (patch ucodediv lsr count = shift&7 in bits 9-11) else usedcode2 (second lsr = shift-8). Patch sub.l disp (ucodeskip+2) = -6(a2,d0) (object var a5 offset), first move.l disp (+2) = that +4. If regat!=0 wrap: move.l d0,-(a7) / template / move.l d0,Dr / move.l (a7)+,d0. d2=2, bra getchar3. **Self-modifying template writes.**
- domaximum (10042): fetchit; d3 = -4(a2,d0).w (unsigned); getchar3; fetchnum2.
- doaddr (10053): Addr Obj(n): getchar3 token (saved), getchar3 must be '(' ; getmaxel (outside, a2=lib object info) ; reget; ')' required. Patch addrcode: disp=-6(a2), `adda.w Dr,a0`, `move.l a0,Dr`; pokecode; d2=3; getchar3.
- function (10087): d0 <= $8000+tnum -> syntaxerr (asm/basic keyword token). $8003+tnum doern, $8005+tnum doaddr, $8011+tnum domaximum, $801e+tnum doused. Push d2 (old type); findtoke; (a3).w bit1 must be set (function) else syntaxerr; bit2 -> amigalib; else falls into libfunction.
- libfunction (10112): d1 = flags>>8 (return type); if bit3 of 1(a3): sizespec2 (type suffix -> d1). push ret type; string -> makesbase (a2 kept). savem; push mask. getchar3: '(' -> countpees (d1=nparams, 0 -> syntaxerr) else bakup,d1=0. push regat, regat=0, push nparams. sizespec: emit moveq #type,d0 ($7000|6(a7)), regat=1. dolibtoke(d1=nparams) (outside). nparams!=0 -> require ')', getchar3. pop regat; !=0 emit move.l d0,Dr. pop mask -> emit movem (a7)+ reversed. pop ret type -> d2; string -> putstlen|r. bra varcont (which pops the old d2 pushed in `function`).
- amigalib (10183): OS lib call. uselib(6(a3)); savereg; push d5; pokemovem d4; push 10(a1) (lib base var a5 offset); a3+=8; push (a3)+ (LVO); push a3 (reg list); getchar3; push d0 (first bracket). For each reg byte (until negative): d2=3, peval (push long), stackuse-=4. ',' -> next; else next reg byte must be terminator (else syntaxerr). tstbras. Walk reg bytes backwards emitting move.l (a7)+,An ($205f|(r&7)<<9, bit4 set) or move.l (a7)+,Dr ($201f|r<<9). (bset into d4/d5 for r<regat is dead - d4 already emitted, d5 already pushed.) patch libjsr+2=LVO, libbase+2=base offset, emit libbase (move.l off(a5),a6), libjsr (jsr lvo(a6)); regat!=0 -> move.l d0,Dr; pokemovem(saved d5); d2=3; reget; bra varcont.
- tstbras (10260): in d1 first bracket char, d0 last char, d3 nparams. d3!=0: require '(' and ')' then bra getchar3. d3==0: if d1=='(' require d0 after getchar3 == ')' then getchar3; else rts.
- makesbase (10305): sbasegot!=0: if sbgot==0 {sbgot=2; emit move.l a3,-(a7)}. sbasegot==0: sbasegot=-1, sbgot=1, tokejsr(workstart).
- litstring (10327): makesbase; litdata1=data1; pokedata1 x2 (placeholder long); chars -> pokedata1b until '"' (0 -> notqerr). If !nomemleft write {0.w,len.w} at *litdata1 (direct store into data buffer). data1 rounded even, the pad byte not written (BUG(orig) #969; the port writes it as 0). makelit; d2=7; bra getchar3.
- makealab (10352): makename2 then falls into makealab2 (10353): findadd (Z=1 found, a2 node). Not found: addhere (outside), 4/8/12(a2) cleared, 16(a2).w=linenumat. Not found or 8(a2)==0: allocate 12-byte ref node via doallocmem(12, MEMF_PUBLIC) and link to 4(a2) (see E); rts. Found & defined: 4(a2) bit0 set -> illlaberr (it's a constant); procnum != 16(a2) -> referr. out a2 = label node. Called from 12358/13405/13441 (13404 does `subq.l #2,pc` before - ref records pc+2).
- makelit (10391): emit move.l #litdata1,a0 (numtoa0 + addoff + long), move.l (a0)+,-(a7) (pushlen), tokejsr(copstring). d0 preserved.
- qmark (10406): ?label -> getchar3; makealab; emit move.l #x,Dr ($203c|r<<9), addoff, pokel 8(a2); d2=3; bra reget.
- fetchqasm (10449): 'abcd' (in double quotes) packed into d3 (lsl.l #8 per char), Z from getchar3 -> syntaxerr; getchar3; fetchnum2.
- fetchasm (10460): makename2, findadd (not found -> cnferr); d3=8(a2); if 4(a2)!=1 (real label, not constant) `not asmtype` (toggle, sic). fetchnum2.
- variable (10471): push d2; getvname; '{' -> afunction; fetchvar; calcvar; variable2; falls into varcont.
- varcont (10479): if d2==1 emit ext.w Dr (d2 stays 1). pop old type d3; 0 -> reget. old==3 && new==4 -> convert quick->long. else if old>new: exg, convtype (bump new up to old, d2=old). bra reget (d0 = lastchar). Jump target from libfunction, amigalib, afunction.
- afunction (10522): local function call `name{...}`: findproc (else noprocerr); 5(a2).b ret type (neg -> illprocerr); push it; stackuse -= (4(a2).b>>4)*4; string -> makesbase; push sbasegot, !=0 -> dopusha3; savem, push mask; d1=14(a2).l (function pc). 4(a2).b==0 -> makefjsr, getchar3. else push regat, push d1, d0=4(a2).b, a2+=22 (param var desc), regat=0, fetchpees2 (outside), jsr chkstak, pop d1, makefjsr, pop regat. Require '}'. regat!=0 -> move.l d0,Dr; movem restore reversed; pop sbasegot !=0: ret string -> emit subq #1,a3 (deca3) else pokecode geta3; dopulla3. pop ret type -> d2; string -> putstlen|r. getchar3; bra varcont.
- stvar (10617): string variable value. makesbase; bit15 clear -> patch leaa5d3a2+2=d3, pokela5s. Patch template stvarget2 word0 = $7000|r<<9 (moveq #0,Dr), word1 src field = regat; pokecode stvarget2..stvarget2f; tokejsr(copstring) (**must emit exactly 6 bytes: template's beq.w skips it**); d2=7; bra reget.
- variable2 (10641): in d2 (from calcvar: low byte type, bit15 = address already in a2, bit14 = pointer), d3 = a5 offset. type 7 -> stvar. pointer -> d2.b=3. d2.b==0 (struct) -> d2=3, emit move.l a2,Dr ($200a|r<<9), rts. bit15 -> emit movea2dn|r<<9 sized; else moved3a5dn|r<<9 sized via pokewda5s, then d3 word. d2 &= 255. `.size`: long if bit14 or type not 1/2; byte: eor $3000; word: or $1000.

## B. Register conventions
- a5 = source text pointer (getchar), may be temporarily redirected to usedpath.
- d0 = current char/token (getchar3: Z=1 on 0, ':' or Else-token; tokens have bit15 set).
- d2 = expression type (low byte; calcvar adds bit15/bit14 flags), d3 = secondary type / immediate value for fetchnum2 / offset.
- d1 = opcode word/long being built for pokewd/pokel; also precedence in eval2/eval5; push flag on maineval entry; lib token for tokejsr.
- a0 = operator table entry (eval2/popprec/doop), template start for pokecode; a1 = template end.
- a2 = var/label/lib node from find* routines; a3 = token/sub entry from findtoke.
- a4 = clobbered by every pokewd (new pc).
- d4/d5 = movem.l save/restore longs from savereg (survive across emitted call).
- Generated code: D(regat) = result, D(regat+1) = right operand, a3 = string work pointer, a5 = globals base (a4 in procedures when varmode/procmode), a6 = lib base, d0/d1 lib args.

## C. Globals defined in range
| name | size | init | purpose |
|---|---|---|---|
| bigpc (8512) | l | 0 | highest pc emitted (pc-relative data in code) |
| usedfrom (8707) | l | 0 | saved a5 while reading UsedPath |
| unknown (8801) | w | 0 | "don't convert" flag of current maineval |
| thisstuse (8962) | w | 0 | stackuse at maineval entry |
| cfregat (9228) | w | 0 | regat saved across FFP compare |
| precsp (9171) | l | precstack | precedence stack ptr (reset in prepstack 2824) |
| precstack (9173) / precstackf | 6*32 words = 384 bytes (64 entries x 6) | | operator stack |
| forthstack (9715) | 18*32 bytes | | constant-folding element stack (32 entries) |
| forthsp (9716) | l | forthstack | must directly follow forthstack (used as end bound) |
| fuckpos (10303) | w | 0 | nesting count: right operand live in d1 |
Code-pointer tables used (defined outside): convtable (14693, 49 x l), operators (14704, entries {op.w, 7 x l handlers for types 1..7, prec.w}, 32 bytes, 0-terminated). Precedences: 1000 And/Or/Eor tokens, 2000 compares, 3000 +-, 4000 */, 5000 & | Mod, 6000 shifts, 7000 ^, 8000 bit ops.
External globals read/written: pc, libat, nomemleft, cfetchmode, varmode, procmode, regat, debugga.b, optreq2ga4+13, stackuse, pushflag, sbgot, sbasegot, lasta6 (+quoteflag via long), firstoff, numoffs, maths, ffplib, data1, litdata1, constmode, asmtype, linenumat, procnum, flagmask, namebuff, namebuff2, usedpath, libjsr/libbase (templates).

## D. Machine-code templates
In range (word values verified against the `blitz2` binary):
- .code1/.code2/.code3 (8600): $2f00 move.l d0,-(a7) ; $201f move.l (a7)+,d0 ; $2000 move.l d0,d0 (src field |= regat).
- ceos (8767): $421b clr.b (a3)+.
- somecode/somecode2/codeisok (8803): $0c80 (|regat) cmpi.l #imm,Dn ; imm $00010000 poked separately ; somecode2 long = $6500,$0008 (bcs.w over a 6-byte jsr) ; `jsr 0` = $4eb9 0 (not emitted, tokejsr provides it).
- ststfix (8964): $584f addq.w #4,a7. pushut (8966): $3f00 move.w d0,-(a7) (|reg).
- mseq..msge (9178): seq $57c0, sne $56c0, slt $5dc0, sle $5fc0, sgt $5ec0, sge $5cc0 (|regat).
- cmp0/cmp1/cmp2 (9185): $b000/$b040/$b080.
- addrcode..addrcodef (9947): $206d,disp(+2) move.l d(a5),a0 ; $d0c0 (+4, low3 = Dregat) adda.w d0,a0 ; $2008 (+6, bits 9-11 = regat) move.l a0,d0. Patched in place.
- usedcode..usedcodef (9966): 202d dddd(+2 = off+4) 6604 70ff 6006 90ad dddd(+10 = off) e048(+12 ucodediv, count bits 9-11). 16 bytes.
- usedcode2..usedcodef2 (9974): same but 6008 and e048 e048 (second at +14 = ucodediv2). 18 bytes.
- usedprep $2f00, useddone $201f, usedfix $2000 (|regat<<9).
- deca3 (10615): $534b subq #1,a3.
Templates used here but defined in another chunk (1060-1220): stol $584f (addq.w #4,a7), movestst2..movestst2f (move.l 6(a7),d0 ; move 4(a7),8(a7) ; move.l (a7)+,(a7)), movea7dn (move.l 4(a7),d0 ; move.l (a7)+,(a7)), movestdn (move.l 0(a5),d0), getstlen/putstlen, pushd0wd/pushd0l, addcode (2 words), pusha3/pusha3f, pulla3/pulla3f, geta3/geta3f (disps patched at 5973-5976 from lib 65235's 10(a1)), stamp, stamp2(+2), leaamp(+2), leaamp2, leaa5d3a2(+2), stvarget2..stvarget2f (7000 2f00 4a92 6700 000e 2052 5948 2e98 ; word0 and src field of word1 patched), movea2dn, moved3a5dn, movea3a7, numtoa0, pushlen, libbase(+2 = a5 offset), libjsr(+2 = LVO).
In C: all these need to be writable `uint16_t` arrays (self-modified at compile time) shared across chunks.

## E. Structures
- forth entry (18 bytes, forthstack): +0 pc.l (code start of element), +4 firstoff.l, +8 long read of lasta6 (= lasta6.w + quoteflag.w, because lasta6 is a word followed by quoteflag), +12 lasta6.w, +14 0.w, +16 constgot.w (0 = constant, $ffff = not). Comment in source: "pc.l,firstoff.l,optat.l,lasta6.w,0,constgot.w".
- prec entry (6 bytes): op entry ptr.l, prec.w.
- offset/reloc node (firstoff, numoffs; alloc 8 via addoff2/doallocmem, freed in eval5 with exec FreeMem 8): +0 next.l, +4 offset.l (pc - pcat).
- label node (firstlabel via findadd/addhere): +4 refs list ptr.l (==1 => it's a constant, bit0 set => illegal as label), +8 pc/value.l (0 = not yet defined), +12 data label.l, +16 context word (procnum; init'd with linenumat on create - quirk).
- label ref node (alloc 12, MEMF_PUBLIC, doallocmem): +0 next.l, +4 pc+2 (address of long to patch), +8 procnum.w, +10 linenumat.w.
- proc node (firstproc via findproc): +4.b param count (high nibble = longs on stack), +5.b return type (neg = statement), +14.l function code pc, +22 param var descriptors.
- type node (alltypes/findtype): +4 field list head, +8 size.w. field/var node: +4.w offset/size.
- lib (findtoke a2): +14/+16 token range, +18.l offset to object info; info -6.w object var a5-offset, -4.w maximum, -2.w used-shift (0 = not an object lib). uselib a1: +10.w a5 offset of lib base var (-1 none), +12 flags (bit7 used, bit6 allowed in constants).
- token/sub entry (a3): +0.w flags (bit1 function, bit2 Amiga OS lib, byte1 bit3 size spec, high byte return type); amigalib: +6.w lib number, +8.w LVO, +10 reg bytes (bit4=An, low3 reg) terminated by negative byte.

## F. External calls
- exec FreeMem (`jsr freemem(a6)`, a6=4.w) in eval5 fold cleanup, 8 bytes each (nodes were allocated by doallocmem = editor callback comdata+88 - mismatch, faithful).
- doallocmem (comdata->88) in makealab2: d0=12, d1=1.
- flushc (exec CacheClearU if available) before executing generated code.
- FFP (mathffp) LVOs emitted as data: -30 SPFix, -36 SPFlt, -42 SPCmp, -60 SPNeg, -66 SPAdd, -72 SPSub, -78 SPMul, -84 SPDiv (plus getffpbase lib token $c900).
- Blitz lib tokens via tokejsr: wtobover..ftolover ($c316-$c31c), qutofl $d300, fltoqu $d301, quickmult $ca00, longmult $ca01, quickdiv $cb00, longdiv $cb01, modbyte..modfloat $d400-$d404, mathq*128+1 (pow), strcomp $ce00, arrerr $c309, workstart $c601, copstring $c600, ern $d200. Lib 65235 = string base lib.
- Called foreign routines: getchar3, reget, bakup, storeloc, resloc, tstnum, tstalpha, makename/2/3, findadd, findvar, findtype, findproc, findtoke, fetchvar, calcvar, fetchnum, fetchnum2, fetchfrac, uselib, tokejsr, addoff, addhere, pokedata1, pokedata1b, countpees, dolibtoke, sizespec2, fetchpees2, chkstak, getmaxel, and *err routines (never return): syntaxerr, illoperr, pserr, conmemerr, badconerr, notypeerr, noofferr, overerr, cnferr, freeerr, noprocerr, illprocerr, notqerr, illlaberr, referr, noconerr.

## G. Special handling for C
- **Constant folding executes generated code** (eval5 9007-9083): when both operand forth entries have constgot==0 and !nomemleft: emit `move.l Dr,d0 ; rts`, restore lasta6 (and quoteflag via long) from left entry, pc = left entry pc (rewind), a6=maths (FFP base), a5 = &maths - 10(ffplib) (fake globals base so `move.l off(a5),a6` loads maths), flushc, `jsr (a0)` into the object buffer; d0 = value; then emit move.l #d0,Dr ($203c|r<<9) or move.w ($303c) if d2<3; free reloc nodes back to the left entry's firstoff. Needs inline asm thunk (set a5/a6, call buffer, preserve d2/a5) and only works on 68k host. forth entry pop: forthsp-=18 always.
- Computed jumps: convtype (convtable), doop (operators table) -> C function pointer tables; all handlers are tail calls sharing registers (regat, d2 in/out) -> give them a common signature using globals/out-params.
- Self-modifying templates (D): writes into code labels (usedcode, addrcode, stvarget2, stamp2, leaamp, leaa5d3a2, libjsr, libbase, pusha3...). Make them data arrays.
- Byte-exact assumptions: tokejsr must emit a 6-byte jsr abs.l where templates branch over it (arrszchk bcs.w +8, stvarget2 beq.w).
- Host-stack bookkeeping: maineval2, function/libfunction/amigalib/afunction push/pop values across calls and varcont pops the `move d2,-(a7)` pushed by `variable`/`function` - in C pass old type explicitly (varcont(oldtype)).
- Recursion: eval5 <-> getelement2 <-> bracket/notit/eval4/maineval (via peval in amigalib) - deep recursion fine in C.
- Quirks to keep: pevalu==peval; `=<` parsed as `<>`; maineval with regat>=6 leaves regat=6; domodf sets d2=3; fetchasm toggles asmtype; chkhex uppercases d0; doern does not advance; amigalib dead bset on d4/d5; makealab2 stores linenumat in context but compares procnum; lasta6 saved/restored as long (also clobbers quoteflag); pushprec has no overflow check (popprec checks); bytetoword no-op; pokela5d tests procmode.
- Flag-returning routines: chkhex/chkbin (Z), makename (Z = empty), findXXX (Z=1 found) -> return bool in C.
