# Survey chunk 02 - blitz2.s lines 1912-3290

Range: `funcs` .. just before `subs` (line 3291). Main compiler driver (compile, dopass),
cleanup (qfree/freeemup), per-line advance + "racing stripe" progress display
(nextline), macro expansion (expandmax), STOP requester + exec exception handler,
end-of-pass checks, debugger trap records (debugstuff/undodebug), statement dispatch
(dostatement/handle*), library statement call (handlelib/dolibtoke/sizespec).

Conventions used below: "Z" = condition code Z after return. `err*` routines
(`bsr err` + inline string, defined ~7570..7700) never return: they longjmp to
`errcont` with `a7=errstack` (see G).

## A. Routines

### Driver / cleanup
- `funcs (1913)` : marker label only, no code.
- `compile (1917)` : in: comdata, numlines, option gadgets `optreqga10+13` bit7 (debugger), `optreqga15+13` bit7 (two-pass "small code"), nomemleft (set by passes) ; out: nothing (anyerrs=-1 on error/cancel) ; called from menu0/menu1 (1556..1601, outside). Sets `errstack=a7` (a7 at entry = pointer to own return address), `errcont=freeemup`, `concomsp=concomstack`. Computes stripe params: `riteend = (*(UWORD*)(comdata[80]+8) >> 3) - 7`; `titleadd = divu((riteend<<16), numlines)` low word, if 0 (also the divu-overflow case, where d0 stays riteend<<16 so low word 0) -> $FFFF; `bmapmul = *(UWORD*)(comdata+92)`; `botline = bmapmul*9`; `debugga` = sne on optreqga10 bit7 (byte store!). Clears dirmode, ezerr, thisproc.l, connest, varmode, procmode, numincs, intstring, smallpass. Loop: `.retry: dopass`; if small-option: `.makesmall: alloxadj (Z=fail -> .anerr); smallpass++; dopass`. `.nosmall`: if nomemleft: open `nomemreq` via comdata[0], loop comdata[8] (getinput) until d7==-1, d6: 1 -> `alloxinc` (Z -> bra freeemup = return) then .retry ; 2 -> .makesmall ; 3 -> .anerr (anyerrs=-1, tail `qfree`). else tail `qfree`. Returns via qfree's tail chain. On error longjmp lands in `freeemup` with compile's stack -> freeemup's rts returns from compile.
- `qfree (1998)` : no in/out. jsr freereps, freesels, freetlist, freepends, freeifs, freeincs, freexincs, freefors; tail `jmp freeasms`.
- `freeemup (2010)` : bsr qfree; jsr killoffs, frallvars, fralltypes, freelabels, freemacs, freeprocs; tail `jmp freeprocvs`. Also `errcont` target (entered with a7=errstack, so its rts returns from compile). Used from 1456, 4399.
- `fralltypes (2019)` : a0=lastrestype (ptr to last built-in type node); a2=(a0); clr.l (a0); tail `bra freetypes` (a2=first to free).
- `frallvars (2024)` : a3=firstvar; firstvar=0; tail `bra freevars`.
- `freeifs (2028)` : a2=&firstif, d2=14 ; tail `jmp freeslist` (item size 14).

### Per-line advance
- `nextline (2038)` : in: numincs, thisitem, linesleft, titleat/titleadd ; out: Z=1 => no more lines (end of program); NE => next line, a5 = line text. Clears macmask, quoteflag; `freembuff`. If numincs!=0: `jsr readinc` - if NE (include line read, readinc sets a5): include-stripe update then `moveq #-1,d0` / rts (NE). Include stripe (only numincs<4): d2=titleadd[numincs] (byte column), if d2<riteend: titleadd[numincs]++, row=numincs*bmapmul, plane=comdata[36]: `plane[row+d2] ^= 0xFF; plane[botline-row+d2] ^= 0xFF`. Main stripe (`.skip3`): `titleat` is 16.16 fixed point: new = titleat + titleadd[0] (zero-extended); if high word changed (`cmp titleat,d0` compares old high word): `plane[new_hi-1]^=0xFF; plane[botline+new_hi-1]^=0xFF`; titleat=new. Then linenumat++, linedone=0, `--linesleft==0` -> rts Z=1 ; else `thisitem=*(thisitem)` (next), `a5=thisitem+9`, rts with NE from the `move.l a0,thisitem`. Callers: passloop, domacro (12868: `jsr nextline / beq macenderr`). Clobbers d0-d2,a0,a1,a6 (freembuff).
- `hexascii (2105)` : in d1.w, a0 ; writes 4 uppercase hex ASCII chars at a0[0..3] (no NUL) ; preserves d0 ; clobbers d1,d2. Used at 7489/7495.

### Macro expansion
- `expandmax (2121)` : in: a5=statement text, comflag (0 => return immediately) ; out: a5 = (possibly new) statement text (expanded into `myline` buffer, size `macrolen`) ; sets macmask=-1 when something expanded. Scans until 0, ':' , ';' ; tokens (byte with bit7) skip 2 bytes; skips "..." strings (0 inside -> `notqerr`). On '!': a3=a5 (after '!'), `makename` (Z -> syntaxerr), `bakup`, `findmac` (NE -> nomacerr; a2=macro). Params: if next char '{': collect into `macrobuff` as 6-byte records `{char *start; UWORD len}` using `findit`, count in d4 -> `numarg`. Alloc new buffer `doallocmem(macrolen, MEMF_PUBLIC)`; `myoline=myline; myline=new`; a6 = buffer end (a6 NOT execbase here!). Copy line start..a3, back over '!'; then macro text (a2->4 ptr, a2->8 len); `macnum = a2->10; a2->10++`. Backtick escapes in macro text: `` `@ `` -> macnum as decimal (jsr makelong, a0 = dest, d2 = value); `` `{expr} `` -> evalconst2 on macro text (a5 temporarily = macro text; must end with '}' else syntaxerr; value d3 must be >=0 else ilconsterr; quoteflag cleared) gives param number; `` `0-9 `` / `` `a-y `` (case-insens., 'z' and above -> ilconsterr, '9'<c<'a' -> ilconsterr) -> param number 0..34. Param number 0 -> two decimal digits of numarg; n>=1 -> text of param n-1 if n-1<numarg else nothing. Token bytes in macro text copied as 2 bytes. Then copy rest of source line (a5..NUL incl.). a5=myline; free myoline buffer (exec FreeMem, macrolen) if non-null; loop again from start (re-scan, allows nested). Buffer overflow anywhere -> `mbovererr`. Stack: pushes a5 at each scan (.loop0), pops at .loop2 (expansion) or .done (finish) -> balanced.
- `freembuff (2342)` : if myline: myline=0, FreeMem(myline, macrolen) (tail jmp freemem(a6), a6=execbase). Clobbers d0,a1,a6.
- `findit (2351)` : in a4 = param text ; out: a4 past terminator, d0 = ',' or '}' found at nesting 0 ; tokens skip 2 bytes ; toggles global quoteflag on '"' and ignores chars while quoteflag ; nesting via {,( / },) (d2, clobbered) ; NUL -> syntaxerr (no return).
- `isitopen (2384)` : Z=1 if d0.b is '{' or '('.
- `isitclose (2389)` : Z=1 if d0.b is '}' or ')'.

### STOP requester / exception
- `stopcode (2453)` : **Exec task exception handler** (installed in us->tc_ExceptCode). in: d0 = signals; if `stopit==0 && (d0 & (1<<stopbit))`: stopit=-1, `Signal(us, panicmask)` (exec LVO signal). Must return d0 unchanged (preserves d0/a6 around call). Clobbers d1,a1.
- `endstop (2469)` : if clrreq: comdata[4]() (close requester), clrreq = ~clrreq, `us->tc_SigExcept (30) = 0`, `us->tc_ExceptCode (42) = 0`. Called from 1657, 7664 (err), here.
- `setcvars (2482)` : per-pass init. blitzoff=-1; clear intsused, anyerrs, blitzmode, nomemleft, constmode, cmake, cfetchmode, intdata1.l, fuckpos, procnum, procnum2, dfetch, titleat.l, usedpath.b, inerr, intlevel, cont_pc.l, cont_pc2.l, lastcontlink.l; pc=bigpc=pcat; lib=libat; data1=data1at; data2=data2at; comflag=-1; linemode=-1; oldvcodelen=0; lastbiglab[0]=0. Also called at 15783.
- `openstopreq (2526)` : in a0 = two consecutive NUL-terminated strings (`compiling` or `executing`); clrreq=~clrreq; copies 1st string over `reqmessage` (then replaces its NUL with ' ' so the old " *****" tail remains - string must be 9 chars), 2nd into `stopreqla11` (7 chars); comdata[0](a0=&stopreq) open requester; stopit=0; `us->tc_ExceptCode=stopcode`; win=*(comdata+94); port=win->UserPort (86); `stopbit = port->mp_SigBit (15)` (byte stored to stopbit+1, i.e. low byte of word); `us->tc_SigExcept = 1<<sigbit`. Also called at 1615.
- `chkrealstop (2828)` : polls comdata[32]() repeatedly until d7==0; if any message has d7==-1 && d6==1 -> stop. out: stopit=d4 (0 / -1) and Z=1 if no stop. Clobbers d4,d6,d7,a0 (+ callback). Called from 1646.
- `chkstop (2847)` : if stopit: chkrealstop, if NE fall into gostop ; else rts. Called also at 5300.
- `gostop (2853)` : (fallthrough from chkstop) endstop; pointeron; `a7=errstack; anyerrs=-1; jmp (errcont)` - never returns (same longjmp as err).

### Pass
- `dopass (2556)` : openstopreq(compiling); sleepon; setcvars; varoff=$8000; defaulttype=&quicktype; **passstack=a7**; thisitem=&firstitem (pseudo node: first long = first line); linesleft=numlines+1; linenumat=-1; resetlibs; freeemup; titleadd[1..4]=0; pokecode2(nops4..nopsf) (4 NOPs); if debugga(byte): uselib(runerrlib=65500) and (makeexec ? uselib(rundebuglib=64050) : uselib(debuglib=64060)+uselib(dhandlerlib=1)). nextline; then loop.
- `passloop (2612)` / `passnxt (2613)` : `passloop: doline; passnxt: nextline; bne passloop`. **`passnxt` is a foreign jump target**: include code (12843) does `move.l passstack,a7 / jmp passnxt` (longjmp back into the pass loop). After loop: dorunerrson; pointchk; linenumat=0; errchx2; if debugga: debugstuff, tokejsr(finalscheck=$c300), debugstuff, pokel(*(long*)eopcode), if lastcontlink: `((UWORD*)lastcontlink)[3]=0`. endop=pc. blitzmode -> doamiga. intsused -> tokejsr2($c105). calcstatic -> numstatic=d4.w, staticdata=d3.l. calcmaxs. If tokeslist: pokewd(gotocode word = $4EF9 jmp abs.l), endjmpat=pc, addoff, pokel(d1 = garbage placeholder), fixjsrs, if !nomemleft `*(long*)endjmpat = pc`. makefinits; pokel($70004E75 moveq #0,d0/rts); makeinits; asmfixer; endstop; tail `bra pointeron`.
- `eopcode (2683)` : template, see D.
- `pointchk (2688)` : walk firsttype; type with `4(a2).l == $FF` (forward-declared, never defined) -> linenumat=8(a2).w, notypeerr.
- `errchx2 (2700)` : walk firstlabel: label with refs bit0 clear (not a constant) and pc(8)==0 -> linenumat=16(a2).w (here = line of first reference), nolaberr. Then concomsp!=&concomstack or connest!=0 -> `chxerr4`. Falls through into errchx.
- `errchx (2719)` : preserves d1. Errors (set linenumat from xxxlineat, then jmp err): firstsel -> sellineat/selerrz ; firstrep -> replineat/unterr3 ; intstring -> intlineat/interr2 ; firstif -> iflineat/noenderr ; procmode -> prolineat/nopenderr ; firstfor -> forlineat/nonexterr. Called at 3730, 3852, 13506.
- `chxerr4 (2747)` : linenumat=ciflineat; jmp nocenderr.
- `calcmaxs (2750)` : maxsat=data1; for each lib in firstlib with `12(a2).w <0` : o=18(a2).l; if `*(WORD*)(a2+o-2)` : count++, pokedata1(*(UWORD*)(a2+o-6)), pokedata1l((1 << *(UWORD*)(a2+o-2)) * *(UWORD*)(a2+o-4)). maxsused=count; if count: tail `bra uselib` d1=65530 (memlib).
- `doline (2780)` : loop: dostatement; reget; NE -> illeolerr; if d0==$8000+34 (Else): jsr doelse2, loop; if d0!=0 (':' etc.) loop. At EOL "un-then": while firstif && `firstif->12.w <0` (bit15 = single-line If..Then): d1=val&$7FFF, jsr doendif2 (a0=firstif, d1), jsr freetheifz. Called also from 3849 (direct mode).
- `createlab (2807)` : local label name: lastbiglab[0]==0 -> nolocerr; namebuff = lastbiglab + loclabch(39 = '\''); d2 = len incl. that char; a1 = after it; `makename4` appends rest; if nothing appended -> syntaxerr. out: namebuff, d2=len, d0=next char (from makename4).
- `prepstack (2824)` : forthsp=&forthstack, precsp=&precstack. Also called at 5298.

### Debug trap records
- `maketrap (2868)` : d1 |= $4E40 ; tail `bra pokewd` (emits TRAP #d1). Used at 3768, 13538, 13926, 14022.
- `debugstuff (2871)` : if debugga.b && !macmask && !dirmode: d_pc=pc, d_bigpc=bigpc, d_nomemleft=nomemleft; saves d0-d1; maketrap(1); `cont_pc2=cont_pc; cont_pc=pc` (pc after TRAP word); if old cont_pc && !nomemleft: `((UWORD*)old)[3] = pc - old`, lastcontlink=old. Emits pokel(thisitem), pokewd(a5-thisitem), pokewd(0). Record layout see D.
- `undodebug (2915)` : if debugga.b && !macmask: nomemleft=d_nomemleft, bigpc=d_bigpc, pc=d_pc, cont_pc=cont_pc2 (removes the trap record just emitted). Preserves all regs/flags? -> sets CC (tst), callers don't rely.
- `setsvars (2927)` : per-statement init: prepstack; regat=0; nonew=0; sbasegot=0; oldvcodelen=varcodelen; varcodelen=0; lasta6=-1. Also at 14366, 15784.
- `smode (2939)` : if linemode>=0 (special line mode, e.g. inside NewType): undodebug; **`addq #4,a7` drops return into handlechar**; a0=modetable[linemode]; reget; `jmp (a0)` -> mode handler returns directly to dostatement's caller. else rts.

### Statement dispatch
- `dostatement (2953)` : chkstop; expandmax; debugstuff; **falls into handlechar**.
- `handlechar (2961)` : getchar3 (Z -> handleeol); setsvars; smode; d0=='.' -> handlemouse; '#' -> handleconst; loclabch -> handlelocal; d0<0 (token) -> handletoken; else fall into handleascii. All are tail branches: out = whatever handler returns (d0/Z = current char via reget/getchar3, required by doline's reget).
- `handleascii (2977)` : letstart=a5; getvname (name -> namebuff, d2 len); reget: Z (end of statement) -> handlelabel (bare name = label definition); '{' -> handleproc; else tail `jmp dolet2`.
- `handleeol (2986)` : if d0==$8000+34 (Else, which getchar3 reports as Z) -> handletoken; else tail undodebug (empty statement: drop trap record).
- `handlelocal (2991)` : undodebug; createlab; bra handlelab2.
- `handlemouse (2996)` : '.' prefix label. getchar3 (Z -> handleeol); loclabch -> handlelocal; makename2 (bakup+makename); falls into handlelabel.
- `handlelabel (3002)` : undodebug; strcpy(lastbiglab, namebuff); falls into handlelab2.
- `handlelab2 (3011)` : in namebuff/d2. findadd: not found -> addhere (new label, a2) ; found: refs.l bit0 set (constant) -> illlaberr; pc!=0 -> duplaberr; procnum stored; walk refs list: ref->procnum(8) != label procnum -> linenumat=ref->line(10), referr; if !nomemleft: ref addr bit0 set (Restore ref) -> `*(long*)(addr-1)=data2` else `*(long*)addr=pc`; FreeMem(ref,12), refs=next. `.done`: refs=0, pc(8)=pc, data(12)=data2, procnum(16)=procnum; rts (d0 from reget in findlab/addhere).
- `handleconst (3065)` : `#name[=expr]`. prepstack; makename; findadd: not found -> addhere, `4(a2).l=1` (marks constant) ; found -> must have 4(a2)==1 else illconerr. If d0=='=': evalconst2 (saves a2), `8(a2).l = d3`. rts with d0/Z of last char.
- `handleproc (3083)` : statement call `name{...}`. findproc (NE -> noprocerr); `5(a2).b` bit7 clear (it's a function) -> illprocerr; push 14(a2).l (proc pc); if 4(a2).b (nparams): a2+=22 (param var-desc list), fetchpees2 ; else getchar3 ; d0!='}' -> syntaxerr; jsr chkstak; makefjsr(d1=proc pc); tail `bra getchar3`.
- `handletoken (3105)` : in d0 = token word ($8000|n). `$8000+fnum <= d0 < $8000+tnum` (asm mnemonic): if !(optreq2ga3+13 bit7) undodebug; tail `bra asmit`. Else bclr #15; if d0&$7F00 -> handlelib. If d0>=tnum: d0 -= tnum-fnum (remap post-asm tokens to contiguous index); only fnum+10, fnum+15 get undodebug. If d0<tnum: undodebug for 13,18,19,26,35..42,56,57 (no-debug statements). Index=(d0-1)*4; >= atokensend-atokens -> `flash` (hardware colour flash) and rts. Else **push atokens[idx] and `bra getchar3`** -> getchar3's rts enters the handler with d0/Z = next char (computed jump). Handler returns to dostatement's caller.
- `handlelib (3169)` : d1=d0 (lib token); findtoke -> a2=lib, a3=sub (marks lib used); sub flags byte `1(a3)`: bit2 -> tail `bra stamigalib`; bit0 clear (not a statement) -> syntaxerr; sizespec; countpees (d1 = #params); falls into dolibtoke.
- `dolibtoke (3181)` : in d1=#params, a2=lib, a3=sub, usertype ; out: d0/Z via tail `bra reget`. Recursive-safe: saves numreps, userp, stackuse, a2 on stack; userp=usertype. Searches forms at a3+6 (see E) for one matching d1 (exact, or repeatable: rep=flags&7, base=nparams-rep, (d1-base)%rep==0 with d1>=nparams); end marker (negative word) -> syntaxerr. Found: d1 |= flagsbyte<<8; stackuse=0; if d1: fetchlibps(a2=form+2, d1) else getchar3. If stackuse: emit preplibst with disp=stackuse (`lea stackuse(a7),a2`), if numreps emit movenumrep with imm=numreps (`move #numreps,d7`). userjsr(a2=lib, a1=form). If stackuse: <=8 -> emit `addq #stackuse,a7` = `$504F | ((stackuse&7)<<9)` ; else emit fixlibst with disp (`lea stackuse(a7),a7`). Restore saved vars. Also called from 10149 (function path, with explicit sizespec word poke).
- `sizespec2 (3260)` : saves a2/a3; prevloc=a5; getchar: '.' -> makename (Z -> syntaxerr), findtype in alltypes (NE -> notypeerr), then bakup ; '$' -> d1=7 (string, char consumed, no bakup) ; else a2=defaulttype, bakup. For type path: `4(a2).l >= 256` (not primitive) -> illtypeerr; d1=6(a2).w. out: d1=type code, usertype=d1. Used at 10119, 12633, 13501.
- `sizespec (3283)` : if `1(a3)` bit3 (sub needs size): sizespec2; pokewd($7000|d1) (= `moveq #type,d0`); regat++. in a3=sub.

## B. Register conventions observed
- a5 = source text pointer (tokenized line) throughout parser; `prevloc` = a5 before last getchar (bakup restores a5=prevloc, quoteflag=oldqflag).
- d0.w = current char/token; tokens are words `$8000|n` (2 bytes in text, first byte bit7 set); library tokens have bits 8-14 nonzero. getchar3/reget: Z=1 at end of statement (0, ':' , Else token $8000+34) except inside quotes.
- makename/getvname: name into `namebuff` (NUL-term.), d2.w = length, Z=1 if length 0.
- findXXX (findlab family): in namebuff+d2; Z=1 found, a2=node; NE: a2=last node (link for addhere), d1=-1; always ends with reget (d0=current char).
- addhere: a2 = prev node, d7 = name offset; out a2 = new node; d0 via reget.
- a6: execbase loaded ad hoc (`move.l 4.w,a6`) right before exec calls; NOT preserved; in expandmax a6 = buffer end pointer.
- a4 = scan pointer in findit; a3/a2 = lib sub / lib base in lib-call code; a1 = form ptr.
- Globals as "registers": `pc` (long, object code write pointer = real memory address), `bigpc`, `data1`/`data2` (data area pointers), `nomemleft` (nonzero = object buffer overflow, patches suppressed), `linenumat` (current line, used by err), `regat`, `stackuse`, `numreps`, `usertype`, `userp`.
- Editor callbacks get/return in d6/d7 (d7=-1 gadget event, d6=gadget ID) - see F.
- `debugga` written with `sne` (byte), read as `.b` and `.w` -> nonzero test only; in C treat as bool (value $FF00 if word-read).

## C. Globals defined in range
| name | line | size | init | purpose |
|---|---|---|---|---|
| smallpass | 1915 | w | 0 | 2nd-pass counter for small-code mode (read 5292) |
| bmapmul | 2032 | w | 0 | stripe bitplane bytes/row |
| botline | 2033 | w | 0 | offset to bottom stripe (bmapmul*9) |
| riteend | 2034 | w | 0 | right end column of include stripe |
| macmask | 2036 | w | 0 | -1 while current line is macro-expanded (no debug traps); read 5006 |
| stopreq | 2394 | struct Requester (Intuition, 112 bytes) | Left 172,Top 38,W 208,H 44, ReqGadget=stopreqga1, ReqBorder=stopreqbo1, ReqText=stopreqin1 | STOP requester |
| stopreqbo1..bo3, stopreqla1..la10 | 2404-2427 | struct Border chains + XY arrays (3 pairs each) | pens 1/2, DrawMode 1, Count 3 | bevel borders (bo1->la2 chain, bo2, bo3 = gadget render). Note labels `stopreqla2`,`stopreqla5`,`stopreqla9` are Borders, others are WORD XY[6] |
| stopreqin1 | 2429 | struct IntuiText | pens 1,0 mode 1, Left 20 Top 10, IText=stopreqla7 | message text |
| stopreqla7 / reqmessage | 2433/2434 | bytes | '***** ' + 'COMPILING *****\0' | message; reqmessage overwritten by openstopreq (9 chars) |
| stopreqin2 | 2436 | IntuiText | Left 4 Top 2, IText=stopreqla11 | gadget text |
| stopreqla11 | 2440 | bytes | ' STOP! \0' | button label (overwritten, 7 chars) |
| stopreqga1 | 2443 | struct Gadget | L72 T28 W64 H12 Flags 0 Activation 1 (RELVERIFY) Type 1 (BOOLGADGET) Render=stopreqbo3 Text=stopreqin2 GadgetID 1 | STOP button |
| stopbit | 2449 | w | 0 | sig bit of editor window port (low byte written) |
| stopit | 2450 | w | 0 | 0 / -1 stop requested (read 1642, 1726) |
| clrreq | 2451 | w | 0 | requester open flag (toggled with `not`) |
| intsused | 2480 | w | 0 | interrupts used -> emit ints-off at end (set 3737) |
| varsizeat | 2517 | l | 0 | (used 11392) |
| debugga | 2518 | w | 0 | debugger enabled (byte-written!) |
| compiling | 2520 | 2 strings | 'COMPILING\0',' STOP! \0' | openstopreq arg |
| executing | 2523 | 2 strings | 'EXECUTING\0','!PANIC!\0' | openstopreq arg (1614) |
| endjmpat | 2686 | l | 0 | address of jmp target placeholder in object |
| d_pc, d_bigpc | 2861 | l (ds) | - | saved pc/bigpc for undodebug |
| d_nomemleft | 2863 | w | - | saved nomemleft |
| cont_pc, cont_pc2 | 2864 | l | - | current / previous debug record address |
| lastcontlink | 2866 | l | - | last record whose link was patched |

Externally-defined globals used heavily (for header): comdata, numlines, linesleft, linenumat, linedone, thisitem, firstitem, titleat (l), titleadd (ds.w 5: [0]=main add, [1..4]=include columns), numincs, quoteflag, oldqflag, prevloc, myline, myoline, macrolen (l), macrobuff, macnum, numarg, comflag, linemode, modetable, atokens/atokensend, errstack, errcont, passstack, concomstack (ds.w 32)/concomsp, connest, panicmask, us (our Task*), pc, bigpc, data1, data2, pcat, libat, lib, data1at, data2at, endop, nomemleft, anyerrs, blitzmode, makeexec, tokeslist, gotocode, firstlabel, firstproc, firsttype, lastrestype, firstvar, firstif/firstsel/firstrep/firstfor, intstring, procmode, xxxlineat (sellineat, replineat, intlineat, iflineat, prolineat, forlineat, ciflineat), firstlib, numreps, userp, usertype, stackuse, regat, nonew, sbasegot, varcodelen, oldvcodelen, lasta6, forthsp/forthstack, precsp/precstack, lastbiglab (128), namebuff (128), letstart, defaulttype, quicktype, alltypes, procnum, maxsat, maxsused, numstatic, staticdata, optreqga10, optreqga15, optreq2ga3 (Gadget structs; `+13` = low byte of Flags, bit7 = GFLG_SELECTED), nomemreq (Requester, 753), dirmode, ezerr, thisproc, varmode, varoff, cmake, cfetchmode, intdata1, fuckpos, procnum2, dfetch, usedpath, inerr, intlevel, blitzoff, constmode.

Constants: loclabch=39, fnum=60, tnum (set by asmmac, end of asm mnemonic tokens), runerrlib=65500, debuglib=64060, dhandlerlib=1, rundebuglib=64050, memlib=65530, finalscheck=$c300, Else token = $8000+34, ints-off token $c105.

## D. Machine-code templates / emitted code
- `nops4..nopsf` (defined 940-944, outside): 4 x NOP ($4E71) copied by pokecode2 at start of each pass (dummy for init JSRs, later patched by makeinits presumably).
- `eopcode (2683)`: `moveq #3,d0 / trap #0` = long $70034E40, poked with pokel (copies the 4 bytes as data) at program end when debugging.
- `$70004E75` literal: `moveq #0,d0 / rts` program end.
- `gotocode` (1151, outside): `jmp abs.l` -> its first word $4EF9 poked, then placeholder long (addoff reloc), later `*(long*)endjmpat = pc` (if !nomemleft).
- `maketrap`: emits $4E40|n (TRAP #n).
- Debug record (debugstuff), emitted per statement when debugga: `TRAP #1` ($4E41) then record at cont_pc: `+0 l thisitem (editor line ptr) ; +4 w a5-thisitem (offset of statement in line) ; +6 w link = (next cont_pc - this cont_pc), 0 for last`. Link patched when next record is emitted; final one cleared at end of pass.
- dolibtoke patched templates (defined 1086-1090, outside): `movenumrep` = `move #imm,d7` ($3E3C imm) imm at +2 = numreps ; `preplibst` = `lea d16(a7),a2` ($45EF d16) d16 at +2 = stackuse ; `fixlibst` = `lea d16(a7),a7` ($4FEF d16) d16 at +2 = stackuse. The asm writes the immediates INTO the template in memory (`move d1,preplibst+2`) then pokel's the template longword -> in C just build `0x45EF0000|stackuse` etc.
- `addq #n,a7` built: `$504F | ((n&7)<<9)` (n=8 -> 0).
- sizespec: `$7000|type` = `moveq #type,d0`.
- makefjsr (outside): `$4EB9` jsr abs.l + addoff + long.

## E. Heap / linked structures
- **editor line list** (`firstitem`, `thisitem`): node `+0 l next ; +9 text (tokenized, NUL-terminated)`. `firstitem` is a long used as pseudo head node.
- **macro** (`firstmacro`, findmac d7=13): `+0 l next ; +4 l text ptr (AllocMem len) ; +8 w text len ; +10 w use counter (for `@) ; +12 b struct-len byte ; +13 name\0`. Created by domacro (12849).
- **macrobuff** param records (expandmax): array of `{l start; w len}` (6 bytes), len excludes terminator.
- **myline/myoline**: expansion buffers, size `macrolen`, alloc'd with doallocmem(…,1), freed with exec FreeMem.
- **label** (`firstlabel`, findadd d7=19): `+0 l next ; +4 l refs (ref list head; value 1 = it is a #constant) ; +8 l pc (0 = undefined; for constants: value) ; +12 l data2 at definition (for Restore) ; +16 w procnum (before definition: line of first reference) ; +18 b len ; +19 name\0`. Alloc size by addhere = d7+len+1 (`MEMF_PUBLIC` via doallocmem).
- **label ref** (12 bytes, created ~10370): `+0 l next ; +4 l patch address (pc+2 of the emitted instruction; bit0 set => Restore/data ref, real address = value-1, patched with data2) ; +8 w procnum ; +10 w line`. Freed in handlelab2 (FreeMem 12).
- **proc** (`firstproc`, findproc d7=33): `+4 b nparams ; +5 b flags (bit7 = statement, clear = function) ; +14 l pc ; +22 param var-desc list (for fetchpees2) ; +32 b len ; +33 name`.
- **type** (`firsttype`; lastrestype = last builtin): `+0 l next ; +4 l size ($FF = undefined forward type; <256 = primitive) ; +6 w (low word = type code for primitives) ; +8 w line (for undefined)`. findtype d7=11.
- **if** (`firstif`, 14 bytes): `+12 w flags/kind (bit15 = single-line If..Then)`.
- **lib** (`firstlib`): `+0 l next ; +4 w lib number (sign selects search mode in findtoke) ; +12 w flags (<0 = has maxs header) ; +14 w first token ; +16 w end token ; +18 l offset o of header end: maxs info at o-6 (w default count), o-4 (w multiplier), o-2 (w shift, also "used" test) ; +48 first sub`.
- **lib sub (token)** (a3 from findtoke): `+0 b ? ; +1 b flags (bit0 statement, bit2 amigalib -> stamigalib, bit3 needs size spec) ; +2 l next sub ; +6 forms[]`.
- **lib form**: `+0 b flags (bits0-2 repeat group size, bits3-6 position of first repeat) ; +1 b nparams ; +2 b types[nparams] ; align even ; reg list of 4-byte entries terminated by 0.w (skiplibreg) ; 12 bytes more` ; list ends with a negative word.
- Freed lists: if (14), others via freeXXX outside.

## F. External calls
- exec: `FreeMem` (freemem(a6), expandmax/freembuff/handlelab2), `Signal` (stopcode). AllocMem always via `doallocmem` = comdata[88] (d0 size, d1 flags, out d0).
- Exec Task struct poked directly: `us+30` tc_SigExcept, `us+42` tc_ExceptCode.
- Intuition structs read: `comdata[94]` -> Window*, `+86` UserPort, port `+15` mp_SigBit.
- comdata (editor interface, long ptr table at `comdata`):
  - `+0` open requester (a0 = struct Requester*)
  - `+4` close requester
  - `+8` getinput, blocking (out d7 = class, -1 = gadget; d6 = gadget ID)
  - `+12` report error (a0 = message string; called by err; saves d7/a6)
  - `+32` poll input non-blocking (out d7=0 none, else as +8)
  - `+36` (long data) stripe bitplane pointer
  - `+72` pointeron (normal pointer), `+76` sleepon (busy pointer)
  - `+80` (long data) ptr to struct with UWORD width at +8 (screen/window width pixels)
  - `+88` doallocmem
  - `+92` (word data) bitplane bytes per row
  - `+94` (long data) editor Window*
- Hardware: `flash` (1751, outside) writes $DFF180 (COLOR00) in a loop - called on invalid token.
- Compiler-internal calls (outside range) used: pokewd, pokel, pokecode2, addoff, pokedata1, pokedata1l, uselib, uselib3, resetlibs, findtoke, tokejsr, tokejsr2, makefjsr, userjsr, chkstak, countpees, fetchlibps, fetchpees2, skiplibreg, aligna1, stamigalib, asmit, asmfixer, fixjsrs, makeinits, makefinits, calcstatic, doamiga, dorunerrson, alloxinc, alloxadj, readinc, getchar, getchar3, reget, bakup, makename, makename2, makename4, getvname, findadd, findmac, findproc, findtype, addhere, evalconst2, makelong, doelse2, doendif2, freetheifz, dolet2, freeXXX list freers, killoffs, err* routines.

## G. Special handling for C
1. **Error longjmp**: `err` does `a7=errstack; jmp (errcont)`; compile sets errcont=freeemup with errstack=compile's a7 -> C: `setjmp` in compile; err/gostop -> `longjmp`; landing does freeemup() then returns from compile. errcont/errstack are also changed elsewhere (e.g. direct mode) -> design as a jmp_buf pointer + continuation function.
2. **passstack/passnxt**: include handling (12843) resets a7 to dopass frame and jumps into the pass loop -> second longjmp target (`setjmp` inside dopass before loop; longjmp resumes at `passnxt`).
3. **smode**: pops caller's return address and jumps to modetable handler -> in C: `if (linemode>=0) { undodebug(); reget(); modetable[linemode](); return; }` from handlechar.
4. **handletoken**: push handler + `bra getchar3` = call getchar3 then tail-call handler with d0/Z as returned -> handler signature must accept current char (and Z = end-of-statement status) as input. atokens is a table of code pointers.
5. Pervasive **condition-code returns** (Z from getchar3/reget/findlab/makename/nextline/chkrealstop) -> return ints/bools; d0 "current char" must be a global (e.g. `lastchar` already exists for reget).
6. **stopcode** is an exec exception handler running in exception context (d0 in/out, a1 except data, a6 execbase) -> needs asm stub or `__saveds __asm` register-args function returning d0.
7. Byte-store into word globals: `sne debugga` (high byte), `move.b d1,stopbit+1` (low byte) -> treat as plain ints in C.
8. Template patching (`move d1,preplibst+2`) is self-modifying data; replace by constructing opcodes.
9. `divu` in compile: guard numlines==0 (trap) and overflow (result>=65536 => $FFFF).
10. Raw opcode constants emitted ($4E40|n, $504F, $7000, $70034E40, $70004E75, $4EF9) -> keep as numeric constants.
11. `pc` is a real pointer into object memory, written through directly (`*(long*)endjmpat = pc`, label ref patching `*(long*)addr = pc`) - keep as `char*`/ULONG on the 32-bit target.
12. dolibtoke recursion saves globals on the 68k stack -> C locals.
