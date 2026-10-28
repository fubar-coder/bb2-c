# Survey 07 - blitz2.s lines 10684-12135

Range: `fetchdata` (number fetch) ... `strcom`, up to the `;-----built in tokens---` marker (`tokes` at 12137 NOT included).
Template bytes below were verified against the assembled binary `blitz2` in the source dir (it matches this source).

Type codes used throughout (d2): 0=unknown/any, 1=byte, 2=word, 3=long, 4=quick (16.16 fixed), 5=float (FFP), 7=string.
FFP = Motorola Fast Floating Point via `maths` (mathffp.library base): -30 SPFix, -36 SPFlt, -66 SPAdd, -72 SPSub, -78 SPMul, -84 SPDiv.

## A. Routines

### Number fetch (10684-10904)
- `fetchdata (10686)` : in: a5 src (first char not yet read), d2=wanted type ; sets `dfetch=-1`, reads char, handles leading `-` ; out: d0=value (int / packed quick / FFP), d2=final type, `dfetch` cleared ; neg: int -> `neg.l d0` if d2<5, FFP -> `bchg #7,d0` ; called from Data statement (12644). Uses local `.datado` (dispatch `$`->fetchhex, `%`->fetchbin, `.`->fetchfrac, else fetchnum) - `fetchhex`/`fetchbin` are foreign (9866/9883) and end in `bra fetchnum2`. With dfetch set, NO code is emitted.
- `fetchfrac (10709)` : entry for `.5` style: d3=0, bra fetchpoint2. Ext. caller: eval primary (9749).
- `fetchnum (10712)` : in: d0=first digit char, d2=type ; d3=0, fetchdec; `.`->fetchpoint; `e/E` (after `or #32,d0`!) -> int->FFP, dothee, reget, consttype2, fetchnum3 ; else -> fetchnum2. Ext. caller: eval (9747). Tail-calls into fetchnum3.
- `fetchpoint (10734)` : overflow check d3<$800000 else overerr; falls into fetchpoint2.
- `fetchpoint2 (10737)` : d6=-1 (float); FFP(d3) pushed on stack; next char must be digit (else syntaxerr - "1." is an error); fetchdec frac -> d3, d5=10^n; if frac!=0: d3 = whole + frac/d5 (FFP), optional `E` -> dothee; `.pskip` when frac==0: pops whole into d3 and does NOT check for `E` (so "1.0e5" ignores the e - faithful quirk). Ends reget, consttype2, bra fetchnum3. Requires a6=maths (set here).
- `dothee (10775)` : after `E`: optional `+`/`-`, getsign -> d4=10^n FFP; d3 = d3*d4 (SPMul) or d3/d4 (SPDiv). in: a6=maths, d3 FFP. clobbers d0,d1,d4.
- `getsign (10804)` : 1 or 2 exponent digits (first must be digit else syntaxerr); >18 -> overerr; out d4 = `facts[n]` (FFP 10^n), d0=next char.
- `fetchdec (10833)` : in: d0=digit char, d3=accum ; out: d3=d3*10+digits (overflow only detected on the shifts -> overerr, final adds unchecked), d5=10^ndigits (unchecked wrap), d0=next non-digit char (tstnum Z=0).
- `fetchnum3 (10856)` : if `dfetch`: clr dfetch, rts (d0=value). Else emit `move.w #d0,Dregat` (`$303c|regat<<9` + word) for d2=1,2 or `move.l #d0,Dregat` (`$203c|regat<<9` + long) for d2=3,4,5, d2 other -> mismatcherr; tail `bra reget` (d0=lastchar, Z per getchar3).
- `makeint (10892)` : d0=d3; if d6!=0: SPFix (a6=maths) ; on V set (overflow) -> `addq #4,a7` (drops own return addr = returns from the CALLER, consttype2) and `bra numdofloat` (d2=5,d0=d3). Relies on V flag from SPFix.
- `fetchnum2 (10902)` : d6=0 (int), consttype2, bra fetchnum3. Main external entry for integer constants: called from fetchhex/fetchbin/fetchconst/fetchsize/fetchon/fetchoff etc. (9817..10469). in: d3=int, d2=type.
- `constotype (10906)` : push d2, evalconst, pop d2, fall into consttype2. No external refs found (dead?).
- `consttype2 (10913)` : in: d3=value (int if d6==0, FFP if d6!=0), d2=type (0=decide), a6=maths if float ; out: d2=final type (may be promoted: byte->word->long, quick int out of +-32767 -> LONG (3), quick w/ bad frac -> float), d0=value (int, quick as `int<<16|frac`, or FFP). Mismatch type -> mismatcherr. d2=0: float -> type 4 via `.quick2`, int -> type 1 then promote by range.
  - QUIRK: `.quick2` assumes d0=int part, but on paths `d2=0 && d6!=0` and `d2=2 && d6!=0` d0 is NOT loaded (holds lastchar from reget). Result usually falls back to float (frac>=65536 or <=0) -> "works by accident"; frac==0 also goes float (`beq .knob`). Negative fractions -> float (unsigned cmp). Keep faithful, flag in header.
  - `.quick2` pushes a word (d0) and `.knob` pops it (`addq #2,a7`) - balanced.
  - Uses Z flag from SPFix (`beq .knob`).
- `numdofloat (11011)` : d2=5, d0=d3, rts. Jumped to from makeint (after stack pop) and consttype2.

### Code push/pull (code motion, 11022-11104) - non-reentrant (single static save area)
- `pushstart (11022)` : pushpc=pc, pushoff=firstoff. Callers 13183, 14341.
- `pushdo (11028)` : len=pc-pushpc; pc=pushpc; pushclen=len.w; if len && !nomemleft: pushat=doallocmem(len, MEMF_PUBLIC); copy words; pushdooff=firstoff. preserves d0-d1/a0-a1 (a6 clobbered = execbase). Callers 13186, 14346.
- `pushput (11057)` : re-emits saved block at current pc, pc+=len; if dest reaches libat -> nomemleft=-1 and SKIP free (leak); adjusts all reloc-offset nodes from pushdooff up to (excl.) pushoff by delta=(newpc-pushpc) unless node addr+pcat >= libat; FreeMem(pushat,len) via exec. preserves d0-d1/a0-a1. Callers 13192, 14379, 14416.

### Code generation helpers
- `muld1 (11106)` : emit `Dregat *= #d1.w`: if d1 power of 2: `lsl.l #n,Dregat` (`$e188|n<<9|regat`, split >=8 as lsl #(n-8) then lsl #8); n=0 emits nothing; else `mulu.w #d1,Dregat` (`$c0fc|regat<<9`, word). Clobbers d1-d4. BUG: n==8 (x256) emits lsl#8 twice (=x65536) - faithful? flag. Callers 8241, 8400.
- `makeinits (11140)` : emit lib init calls, highest lib# first (findhilib, d7 iterator). For each used lib (bit15 of 12(a2)) with init offset 22(a2)!=0: special pre-code by lib# 4(a2):
  allocvars(65534): makevsize; debuglib(64060): `moveq #0/#-1,d0` (`$7000`, low byte = sne of bit7 of `optreq2ga1+13` = gadget SELECTED, "auto run"); 65235(string): makessize; 65035: `move.l #endop,d0` + reloc; 64535(data): `move.l #data2at,d0` + reloc; dhandlerlib(1): `move.l #dseg,d0` (no reloc).
  Then lasta6=-1, makelibsub(a2,22(a2)); allocvars && intdata1: emit `putidata1` (`move.l a5,abs.l` + reloc + intdata1); if 26(a2)!=0: `move.l d0,x(a5)` ($2b40) or `move.w d0,x(a5)` ($3b40, 26<0) + word 10(a2); if lib 65530: statics (numstatic/staticdata -> datastart, tokejsr2 alstat) and maxs (maxsused/maxsat -> datastart, tokejsr2 setmaxs).
  End: start pc kept on stack; if no code emitted: `noinits+2 = 8`; else `noinits+2 = 0`, (if !nomemleft) write `jsr abs.l <startpc>` at pcat (+addoff2 d2=2) and emit `rts`. Called 2677.
- `makessize (11269)` : d1=stringwork, bra makesize.
- `makevsize (11272)` : d1=varoff-$8000 (word, zero-ext), 0 -> 8; falls into makesize.
- `makesize (11280)` : emit `move.l #d1,d0` ($203c + long).
- `findhilib (11288)` : in d7=upper bound (excl., unsigned word) ; out: d7=d1=highest lib# < d7, a2=that lib, Z=1 if none (lib# 0 not findable). clobbers a1,d1.
- `findlolib (11307)` : in d7 lower bound (excl.) ; out d7=lowest lib# > d7, a2=lib, Z=1 if none (d1==$ffff). clobbers a1,d1.
- `makefinits (11327)` : word-align varoff; ascending lib loop (findlolib). For used libs: if max lib (shift word at E-2!=0, E=lib+18(lib)) and free sub (offset at E-10) has a routine (4(a3) after skipping its reg-pair list): emit per-item free loop:
  `move.w #max,-(a7)` / `move.l varoff(a5),a3` / loop: <makelibsub free-sub> / `lea 1<<shift(a3),a3` / `subq #1,(a7)` / `bne.w loop` (disp poked = loopstart - pc) / `addq #2,a7`. Then if finit 28(a2)!=0: allocvars -> makevsize, 65235 -> makessize; lasta6=-1; makelibsub(a2,28(a2)). Called 2674.
- `chkstak (11416)` : if debugga.b && !makeexec: tail `tokejsr stchk`. Callers 3100,10564,13391,13432,14250.
- `tokejsr2 (11427/.skip)` : lasta6=-1, fall into tokejsr. (stchk/makeexec filter commented out.)
- `tokejsr (11434)` : in d1=token ; findtoke(d1) -> a2 lib, a3 sub; makelibsub(a2, a3-a2) ; preserves d2,a3 ; out a2=lib. Many callers (~40).
- `userjsr (11444)` : in a2=lib, a1=sub entry whose first word low byte = length of param-format bytes to skip (then word-align) ; pushes libisat, libisat=a2, a2=params-skipped ptr, bra makelsub2 (which pops libisat). Caller 3240.
- `cutejsr (11459)` : in a2=lib, a3=sub reg-list ptr ; like userjsr without skip. Caller 6098 (also `lea cutejsr,a0` at 6100 as callback).
- `makelibsub (11466)` : in a2=lib struct, d1=sub offset (entry = lib+6+d1) ; pushes/restores `libisat` ; out a2 = lib (libisat) ; clobbers d1,d2,a0,a1(uselib),a4(poke). Callers in range + 6135, 12264.
- `makelsub2 (11471)` : secondary entry: expects old libisat ON STACK (pushed by caller), a2=entry; bsr makelsub3; pop libisat; rts.
- `makelsub3 (11474)` : fetchregs (emit param register loads), then emit calls: debug && ((L0|L8)&~1): doajsr(L0: pre-check, bit0=blitz-only -> blitzerr if blitzmode>=0, makebtst inblitz), doajsrrout(L4 main), doajsr(L8 post-check); else: if bit0 of L0 -> blitz-only check; doajsrrout(L4). (L0/L4/L8 = 3 longs after reg-list.)
- `doajsrrout (11518)` : in d1=routine offset ; bit0 = amiga-only: blitzmode<0 -> amigaerr; debug -> makebtst inamiga; clear bit0; `tst.l d1` then doajsr.
- `doajsr (11535)` : in d1, **Z flag set by caller** (`tst.l d1`/`move.l`) ; Z=1 -> nothing; else doajsr2, preserves a1.
- `doajsr2 (11541)` : in d1=offset from lib struct start, a0<-libisat. cfetchmode: emit `jsr abs.l (lib+d1)` (absolute address of lib copy in compiler memory, no reloc). Else scan inline prefixes at lib+d1: `$a0,00|other,len.w` -> inline length (d1+=4); `$a0,01,blitzoff.l` -> blitz/amiga variant (blitzmode<0: debug makebtst inblitz, jump to lib+long; else debug makebtst inamiga, skip, d1+=6). No prefix: inline -> pokecode2 copy [a1-1, a1-1+inline); else emit `jsr abs.l` + addoff + (d1 - 18(lib) + 40(lib)). Uses global `inline` (cleared on entry - recursion via makebtst clears it: edge bug).
- `makebtst (11608)` : in d1=check token (inblitz/inamiga) ; emit `tst.w blitzoff(a5)` ($4a6d + getbbase) then tokejsr(d1) ; saves d0/d3-d7/a0-a2/a4 around tokejsr.
- `getbbase (11619)` : out d1 = `blitzoff`; allocates a word-aligned 2-byte var at varoff if blitzoff==-1. Callers 3645, 3666.
- `fetchregs (11639)` : in a2 = reg list: pairs (libnum.w, regspec.w), 0-terminated ; calls toreg for each ; out a2 past terminator (-> the 3 routine longs).
- `toreg (11647)` : in d1=lib#, d2=regspec ; spec>=$ff00 -> unconditional; bit15 -> only if blitzmode<0; bit14 -> only if blitzmode>=0 (bits cleared). uselib(d1) -> a1=lib. Then by spec low byte:
  - 0 `.norm`: hi byte reg (bit4=An, $16=a6 with `lasta6` cache = 10(lib)) or $ff=push; makeregop; word var (26(lib)<0) -> `|$1000` (move.w), push-word -> stackuse-=2; emit opcode + 10(lib).
  - 4: `move.w #(word[E-6]+4),reg`; 3: `move.w #word[E-4],reg` (`$303c`/`$307c` if bit7 of hi byte - NOTE uses bit7 here, bit4 in makeregop; `$16` a6 test unreachable here). E = lib+18(lib).
  - else `.notmax`: `move.l E-6var(a5),reg` (makeregop); low byte <0: Dn=low3 bits: debug -> `cmp.w #max,Dn` ($b07c), `bcs.w +8` (maxchk), jsr maxerr (tokejsr, a2 saved); `lsl.w #8,Dn` while shift>=9, then `lsl.w #shift,Dn`; `adda.w Dn,An` (`$d0c0|Dn|An<<9`, An from spec hi byte bits 8-10). Low byte 1 -> plain base; other >0 -> offset+4 ("used" field) and debug -> `.theretest` (tst.l x(a5)/bne/moveq #1,d0/trap #0).
  - Stack: pushes temporaries (`move #4,-(a7)` + `move.l #-6,-(a7)` consumed by `.sendmd`, spec/offset words) - all balanced.
- `makeregop (11841)` : in d2=reg byte ; $ff -> d1=stackop ($2f2d `move.l 0(a5),-(a7)`), stackuse+=4 ; else d1=regtransd ($202d) or regtransa ($206d, bit4) | (d2&7)<<9. preserves a3.
- `ifchar (11861)` : d0 in '<','=','>' -> Z=1, d0-=60 (0..2); else Z=0, d0 unchanged. Callers collif (12944/12948).
- `evalconst (11871)` : bakup (a5=prevloc), fall into evalconst2.
- `evalconst2 / evalconst3 (11880, same addr)` : compile-time constant eval by generating+RUNNING code. Saves regat,pc,libat,nomemleft,lasta6,cfetchmode on stack; regat=0; if not already cfetchmode: oldlibat=libat, pc=constpcat (else continue at current pc); libat=constlibat, nomemleft=0, lasta6=-1, cfetchmode=-1; `arreval` (compile expr, long, into D0); emit `move.l d0,d3` ($2600|regat) + `rts`; nomemleft -> conmemerr; flushc; **`jsr (a0)` into generated code**; restore all; bra reget. out: d3=value, d6 as left by eval (callers test d6!=0 = float -> error), d0=lastchar/Z. Errors inside unwind via errstack; error handler restores libat from oldlibat (7650). Callers: 2258,3078,3551..3733,5311,7088,12300,12504..,13811,15789.

### Includes
- `doincdir (11936)` : statement handler (in jump table at 1329). Copies chars d0.. into `incdir` skipping `"` until d0==0 (stored); d0<0 (token) -> syntaxerr. No length check (96 bytes). Note getchar3 returns ':' with Z=1 but d0=':' -> copied.
- `makeiname (11947)` : namebuff = incdir + name chars (bakup first, skip `"`), d0<0 -> syntaxerr ; out d2 = total length (0 -> syntaxerr), a1 clobbered. Callers 12763,12772,12817.
- `readinc (11972)` : next line from current include (firstinc). out: **Z=0** -> a5 = line start (0-terminated), a2=inc node; **Z=1** -> all includes closed (numincs==0). Refills buffer via readpend; at EOF -> closeinc. Caller nextline (2046: `jsr readinc / beq`).
- `readpend (11988)` : (fallthrough target) moves partial line to buffer start, dos Read(fh, buf+n, inclen-1-n), <0 -> readerr; total 0 -> closeinc; terminates buffer with 0; falls to imakeend.
- `imakeend (12014)` : 14(a2) = ptr after next 0 from a5 ; Z=0 from `move.l a0,14(a2)`.
- `closeinc (12020)` : dos Close(fh) (a6=dos assumed), FreeMem(buf,inclen), unlink, FreeMem(node, byte 18(node)), numincs--; !=0 -> bra readinc ; else rts Z=1.
- `freexincs (12037)` : free firstxinc list (size byte at 4(node)). Caller 2006.
- `freeincs (12050)` : free firstinc list: Close fh (if), FreeMem buf (if), FreeMem node. Caller 2005.

### Data / misc
- `pokedata2l (12074)` : poke long d0 (hi word first) via pokedata2 (swap trick) ; falls into pokedata2.
- `pokedata2 (12081)` : *(word*)data2 = d0 if data2<allat, data2+=2 ; else nomemleft=-1, data2+=2. clobbers a4.
- `pokedata2b (12094)` : same for byte.
- `getsp (12104)` : d0 must be `"` else syntaxerr ; `getsp2`: copy chars to (a0) until `"`, d0==0 -> syntaxerr; NUL-terminate. Callers 12296,12308,12312.
- `numcom (12118)` : `tst.l d3` -> CC (signed) ; used as compare callback (cnif).
- `strcom (12121)` : compare namebuff vs namebuff2 -> CC consumed by signed Bcc (cifeq..cifge): mismatch -> CC of `cmp.b (a1),d0` (signed byte compare c1 vs c2); s1 shorter -> "less"; equal -> Z=1. In C: return int sign, signed char compare.

## B. Register conventions
- a5 = source char pointer (getchar3/reget/bakup); d0 = current char/token (word, tokens $8000+), Z from getchar3 = end of statement (0 / ':' / Else).
- d2 = expression/constant type (codes above); d3 = constant value (long or FFP); d6 = 0 int / !=0 float; d5 = decimal divisor in fetchdec; d4 = exponent factor.
- a6 = maths base during float fetch/consttype2 (NOT reloaded on every path - dependency).
- `regat` (global word) = target data register number for generated code.
- Code gen: d1 = word/long to poke (pokewd/pokel clobber a4; pokel uses swap); a2 = lib struct during lib call generation; a1 = lib from uselib; `libisat` global = current lib for doajsr2.
- d7 = lib# iterator in makeinits/makefinits (findhilib/findlolib).
- Include reader: a2 = include node, a5 = line ptr.

## C. Globals defined in range
| name | size | init | purpose |
|---|---|---|---|
| facts (10822) | 19 x dc.l | FFP 10^0..10^18 ($80000041..$de0b6c7c) | exponent table |
| dfetch (10854) | w | 0 | 1=Data fetch, no code emit (cleared in setcvars 2495) |
| pushfrom (11015) | l | 0 | unused |
| pushpc | l | 0 | pc at pushstart |
| pushoff | l | 0 | firstoff at pushstart |
| pushdooff | l | 0 | firstoff at pushdo |
| pushclen | w | 0 | pushed block length (bytes) |
| pushat | l | 0 | AllocMem'd save buffer |
| libisat (11464) | l | 0 | current lib struct for jsr gen |
| inline (11635) | w | 0 | inline code length found by doajsr2 |
| blitzoff (11636) | w | -1 | var offset of blitzmode flag (set -1 in setcvars) |
| blitzmode (11637) | w | 0 | + amiga, - blitz mode (cleared in setcvars; set elsewhere) |
| stackopuse (11838) | w | 0 | unused |
| oldlibat (11934) | l | 0 | libat saved during const eval (used by error handler 7650) |

## D. Machine-code templates (bytes verified in binary)
| label | bytes | use / patch |
|---|---|---|
| prepd1 (11263) | 222d 0000 | `move.l 0(a5),d1` - no refs in range (unused?) |
| putidata1 (11265) | 23cd 0000 0000 | `move.l a5,abs.l`: only opcode word taken, long = intdata1 + reloc |
| prepint (11267) | 247c 0000 0000 | `move.l #0,a2` - unused in range |
| freemax (11407) | 3f3c 0000 | `move.w #max,-(a7)`: opcode taken, word = max |
| freemax2 (11409) | 5357 6600 fffc | `subq #1,(a7)` / `bne.w`: first long taken, disp word computed |
| freemax4 (11412) | 544f | `addq #2,a7` |
| freemax3 (11414) | 47eb 0000 | `lea x(a3),a3`: opcode taken, word = item size |
| free (12247, outside) | 266d 0000 | `move.l x(a5),a3` used by makefinits |
| tstmode (11633) | 4a6d 0000 | `tst.w x(a5)`: opcode, x = blitzoff |
| .theretest (11803) | 4aad 0000 6600 0006 7001 4e40 | `tst.l x(a5)/bne.w +6/moveq #1,d0/trap #0`; word0 + x then 2 longs poked |
| lsldn (11829) | e148 | `lsl.w #8,d0`: OR count<<9 and Dn |
| maxchk (11831) | 6500 0008 4eb9 0000 0000 | first long (`bcs.w +8`) poked, then real jsr via tokejsr maxerr (assumes 6-byte jsr; also used at 12224) |
| maxdo (11835) | d0c0 | `adda.w d0,a0`: OR Dn and An<<9 |
| stackop (11839) | 2f2d 0000 | `move.l x(a5),-(a7)` (opcode compared in toreg) |
| regtransd/regtransa (1165/1166, outside) | 202d / 206d | `move.l x(a5),d0/a0` |
| immediate opcodes | $303c move.w #,Dn; $307c move.w #,An; $203c move.l #,Dn; $2b40/$3b40 move.l/w d0,x(a5); $e188 lsl.l #n,d0; $c0fc mulu #,d0; $b07c cmp.w #,d0; $7000 moveq; $4eb9 jsr abs.l; $4e75 rts; $2600 move.l d0,d3 |

## E. Structures
- **lib struct (firstlib list)**, as used here: 0 next.l; 4 lib#.w; 6 len.l; 10 var offset of lib base (word, -1 = none yet); 12 flags.w (bit15 used, bit14(=bit6 of byte 12) allowed in const mode); 14 first token.w; 16 last token+1.w; 18 offset to end of max-info E (long); 22 init sub offset.l; 26 base var kind .w (0 none, >0 long, <0 word); 28 finit sub offset.l; 32 nrelocs.l; 36 relocs ptr; 40 address of lib code in output; 44 errchk cut offset; 48 subs start. Max-info (relative to E=lib+18(lib)): E-14.l (extra used ptr flag), E-10.l free-sub offset, E-6.w varoff of max struct array, E-4.w max count, E-2.w log2 item size (0 = not a max lib).
- **lib sub entry** (at lib+6+offset): reg-load pairs (libnum.w, regspec.w)... 0.w; then L0.l pre-check offset (bit0=blitz-only), L4.l routine offset (bit0=amiga-only), L8.l post-check offset. Routine code may start with `$a0` prefixes (see doajsr2). userjsr variant has a leading word whose low byte = bytes to skip.
- **reloc offset list (firstoff)**: 8-byte nodes {next.l, offset-from-pcat.l} (addoff2); pushput walks/patches them.
- **include node (firstinc)**: 0 next.l; 4 buffer ptr (inclen=1024 bytes); 8 dos fh; 12 chars in buffer.w; 14 current ptr.l; 18 node size.b; 19 name\0 (created by addhere2 in doinclude 12820). **xinclude node (firstxinc)**: 0 next, 4 size.b, 5 name. `numincs` = depth.
- **const eval save frame** (evalconst2, on stack): regat.w, pc.l, libat.l, nomemleft.w, lasta6.w, cfetchmode.w, codestart.l.

## F. External calls
- mathffp via `maths`: -30 SPFix (V=overflow, Z used), -36 SPFlt, -66 SPAdd, -72 SPSub, -78 SPMul, -84 SPDiv.
- exec: FreeMem (`freemem(a6)`, a6=4.w) in pushput, closeinc, freeincs, freexincs. Allocation goes through `doallocmem` = editor callback `comdata+88` (d0 size, d1 flags; out d0 ptr) but freeing is direct exec FreeMem.
- dos (`dos` base): Read (readpend), Close (closeinc, freeincs).
- `flushc` (cache flush, exec -636 if v>=36) before running const code.
- Intuition gadget flags read: `optreq2ga1+13` bit7 (SELECTED) of options requester gadget.
- Error exits (never return): overerr, syntaxerr, mismatcherr, blitzerr, amigaerr, conmemerr, readerr (and via callees notokerr, badconerr, dirliberr).

## G. Special handling for C
- **evalconst2 executes generated 68k code** (`jsr (a0)` into constpcat buffer, result in d3) - needs inline asm / function-pointer call with register result; cfetchmode makes doajsr2 emit absolute jsrs into lib code in compiler memory.
- CCR-returning routines: findhilib/findlolib (Z), ifchar (Z), readinc (Z), doajsr (Z input from caller!), numcom/strcom (signed CC consumed by Bcc table cifeq..), getchar3/reget results. Use bool/int returns.
- makeint aborts its caller (`addq #4,a7; bra numdofloat`) -> in C: return flag, consttype2 returns float result.
- makelsub2 expects caller-pushed libisat on stack (userjsr/cutejsr push it) -> in C: save/restore libisat in a wrapper.
- Lib FFP flag dependence: SPFix V and Z flags (makeint, consttype2 .quick2) - C must test results explicitly (V: SPFix saturates to $7fffffff/$80000000).
- Generated byte patterns depend on Devpac word-branch sizes (maxchk bcs.w, theretest bne.w, freemax2 bne.w) - use the verified constants above.
- Latent bugs to preserve or decide on: consttype2 `.quick2` uninitialised d0 (word/unknown + float); muld1 x256 emits 16-bit shift; fetchpoint2 ignores `E` after `.0`; doajsr2 global `inline` clobbered by recursive makebtst; pushput leaks buffer on overflow; toreg `.sendmd` uses bit7 for An while makeregop uses bit4.
- `noinits+2` word write into a long (big-endian low word).
- FFP arithmetic: in C keep using mathffp calls (or bit-exact emulation) since constants are emitted as FFP longs.
