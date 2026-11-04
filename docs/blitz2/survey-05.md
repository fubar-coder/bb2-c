# Survey 05 - blitz2.s 6740-8450

Range: `calcstatic` (6740) .. `brchkcode` (8449). `pokedata1*` (6714-6738) belongs to chunk 04,
`pokewda5s` (8453..) to chunk 06. Checked against the assembled binary `blitz2` in the same dir
(template byte patterns below are confirmed from it).

## A. Routines

### Static structs / data / object file
- `calcstatic (6740)` : a2 = firstvar, falls through into `calcstatic2`.
- `calcstatic2 (6746)` : in a2 = first var node of a var list (callers: firstvar, firstlocal@14025);
  out d3.l = data1 at entry (start of table), d4.w = count. For each var with `6(a2).w==0`
  (no pointer/array flags) and type `10(a2)` with `4(type).l >= 256` (NewType): `pokedata1`
  var offset `4(a2).w`, then type size `8(type).w`. If d4!=0: `uselib #65335` (staticslib, see equates blitz2.s 145-148)
  then **tail `bra uselib #65530`**
  (memlib). Clobbers d1,a2,a3 + whatever uselib clobbers (d3/d4 must survive uselib - they
  do in asm; caller 14025 relies on d3/d4 after return).
- `datastart (6770)` : in d3.l (data1 table addr), d4.w (count). Emits `move.l #d3,a2`
  (pokewd numtoa2 opcode, `addoff` = reloc, pokel d3), then patches `endarray+2 = d4-1` and
  pokel `endarray` (`move #d4-1,d4`). Tail `bra pokel`. Clobbers d1,d4.
- `nextopt (6782)` : **macro** (not code): `addq #8,\1; if \1==optend then \1=optbuff` - ring
  buffer advance for optimizer buffer (8-byte entries). Used elsewhere.
- `mywrite (6790)` : in d1=fh, d2=buf, d3=len, a6=dos, d7=fh (for close). `Write`; if
  d0!=d3: `Close(d7)` then falls into `execerr` -> never returns. Out on success: nothing.
- `execerr (6797)` : `jmp makeexecerr` (never returns). Dead `rts` after it.
- `savefile (6800)` : whole body inside `ifeq cripple` (cripple=0 -> compiled). Sets
  `errstack=a7, errcont=.rts` (error anywhere inside -> savefile simply returns to caller;
  NOT restored afterwards). sleepon; anyerrs=-1 (never cleared here). If !nomemleft:
  for each firstoff node: `if (*(WORD*)(pcat+off) >= 0) *(LONG*)(pcat+off) -= pcat+noinits`
  (absolute->hunk-relative). Open(execname, 1006=MODE_NEWFILE) fail->execerr. Writes AmigaDOS
  hunk file:
  `tsthead` (HUNK_HEADER $3f3,0,1,0,0,size) + `tstsize1`/$3e9/`tstsize2` (32 bytes total;
  sizes = (objlen-noinits+3)>>2 longs), code bytes from `pcat+noinits`, len rounded to 4;
  if numoffs: `tstend` ($3ec, tstsize3=numoffs, 0) 12 bytes, then each offset `-noinits`
  (temporarily subtracted in node and restored), then a 0 long (via tstsize3 cleared).
  If debug option (`btst #7,optreqga16+13`): $3f0 (`sysdebug`) then for each label on
  firstlabel with `4(l)` even or ==1: longcount, name (`19(l)`, (len+1+3)&~3 bytes - reads up to
  3 bytes past the alloc; in C pad with zeros), value `8(l)-pcat-noinits`; terminating 0 long.
  Then $3f2 (`tstdone`). Close. If icon option (`btst #7,optreqga9+13`): append ".info" to
  execname in place, if Lock(name,-2) fails -> Open new + Write `objicon..objiconf` (383 bytes,
  incbin obj.info) + Close; then restore NUL. pointeron. Out: nothing.
- `addoff (6997)` : add current object pc to reloc list: d2 = pc-pcat, bsr addoff2. Preserves d2
  (and all via addoff2).
- `addoff2 (7006)` : in d2.l = offset. alloc 8 bytes (doallocmem d0=8,d1=1), push front on
  `firstoff`, numoffs++. Preserves d0-d1/a0-a1/a6 (everything).
- `killoffs (7022)` : free whole firstoff list (FreeMem 8), firstoff=0, numoffs=0.
  Clobbers a1,a2,a6,d0,d1.

### Character fetch (central parser primitives)
- `getchar (7042)` : in a5 = source ptr; out d0.w = char/token (0 = end of line), lastchar=d0,
  prevloc = a5 before the last raw fetch, oldqflag = quoteflag before that fetch.
  **Condition codes NOT meaningful** (callers never test them; use getchar3/reget for Z).
  Loop (`bra getchar` re-sets prevloc/oldqflag every iteration):
  1. `getcharb`; lastchar=d0.
  2. d0==0 -> return 0 (always, even when not compiling).
  3. d0<0 (token): $801c CNIF -> `.cnif`, $801d CSIF -> `.csif`, $801e CELSE, $801f CEND
     (handled regardless of comflag, see below); other tokens -> step 4.
  4. comflag==0 (inside false conditional block) -> loop (discard char).
  5. comflag!=0: '~' -> `.cmake`; 2 -> a5=usedfrom, loop (end of spliced UsePath text);
     1 -> a5=cmakea5, loop (end of cmake text); else return d0.
  - `.cnif/.csif`: ciflineat=linenumat; if comflag==0: connest++ & loop; else `getchar3`,
    `docnif`/`docsif` (chunk ~12299: evaluates, pushes old comflag on concomstack via concomsp,
    sets comflag, requires end of statement, ends with `bakup`), loop. Recursive into getchar.
  - `.celse`: if connest==0 then comflag = ~comflag (`not`); loop.
  - `.cend`: connest--; if >=0 loop; else connest++ (back to 0), if concomsp==concomstack ->
    `nociferr`, else pop comflag from concomstack (word stack, ptr pre-decrement), loop.
  - `.cmake` ('~' while compiling): if cmake!=0 return '~' (terminator for the inner eval).
    Else cmake=-1, save d0-d7/a0-a6, copy namebuff(128) -> cmakebak, `evalconst2` (const expr
    -> d3, reading via getchar from a5), require d0=='~' else syntaxerr, cmakea5=a5,
    `makelong` d3 as unsigned decimal ASCII into cmakebuff(12) and turn its NUL into byte 1,
    cmake=0, restore namebuff and regs, a5=cmakebuff, loop. (So `~expr~` is textually replaced
    by its decimal value; the byte 1 returns a5 to after the closing '~'.) On error inside,
    cmake stays -1 (cleared at compile init line 2489).
  - '~', 1, 2 are tested even inside quotes: a '~' inside a string literal starts a constant
    maker (BUG(orig) #966; the port takes it as an ordinary character, `docs/libs/deviations.md`).
- `getcharb (7147)` : raw fetch. d0 = 0-extended byte from (a5)+; 0 -> return 0; byte<0 ->
  token: d0 = (b<<8)|next byte (2 bytes consumed, $80xx..$ffxx); '"' (34) -> tail `swapquote`
  (toggle quoteflag, return 34); ';' with quoteflag==0 -> comment: skip to and past the NUL,
  d0=0, lastchar=0. After returning 0, a5 points past the NUL in all cases. Used directly at
  12863. Label `.loop0` unused.
- `swapquote (7169)` : quoteflag = ~quoteflag; rts. Only reached from getcharb.
- `getchar2 (7172)` : getchar, then repeat while quoteflag==0 && d0==' '. Out d0.
- `getchar3 (7184)` : getchar2 then falls into `gchar2`. Out d0, **Z=1 if end of statement**:
  d0==0, or (quoteflag==0 and (d0==':' or d0==$8022 [Else token])). Inside quotes:
  if `(UBYTE)d0 < 32` (byte compare!) d0 = gs[d0] (`move.b`, flags of that byte) - gs = 26
  zero bytes then 228,246,252,196,214,220 (ae oe ue Ae Oe Ue Latin-1): chars 1..25 in a string
  read as 0 (= end), 26..31 map to umlauts. Otherwise Z from `tst d0`.
- `gchar2 (7188)` : entry point = classify d0 as above (flags from caller's `tst d0` on entry -
  must be preceded by tst/move of d0). Used by reget.
- `reget (7202)` : d0 = lastchar, `bra gchar2` -> same out/Z as getchar3, no fetch. Note
  reget uses current quoteflag. Many routines end with `bra reget` to return d0+Z.
- `bakup (7205)` : quoteflag=oldqflag, a5=prevloc (undo the last raw fetch, one level only;
  d0/lastchar unchanged).
- `storeloc (7209)` : locchar=d0, locloc=a5.
- `resloc (7215)` : a5=locloc, d0=lastchar=locchar (quoteflag not restored). Flags: from move.

### Errors
- `errs (7221)` : label only, marks start of stub table (unreferenced).
- 127 stubs `xxxerr: bsr err / dc.b 'msg',0 / even` (7222-7634) - all **never return**.
  `err` pops the return address = pointer to the inline message. C: `void xxxerr(void)
  { err("msg"); }` noreturn. Names/messages are a 1:1 list (see section C for the specials).
  Special stubs:
  - `interr2 (7317)` : linenumat=intline first. `macenderr (7523)` : linenumat=macline first.
  - `noliberr (7488)` : in d1.w = lib number; `hexascii` into `liberrnum` (4 hex digits,
    self-modified message "Library not Found : $XXXX."), then err.
  - `notokerr (7494)` : in d1.w = token; same into `tokerrnum` ("Token Not Found : $XXXX.").
    Missing `even` after it (harmless in C).
  - `opterr (7294)` : message contains patchable `opchars`/`opchars2` ("ffff" fields); no
    writer found in blitz2.s (dead / patched elsewhere).
  - `asmerr2 (7360)` : a2 = asmbuff, falls into `asmerr (7361)`: linenumat = 12(a2).w; **rts**
    (not an error; used by err in constmode).
  - `execerr` -> `makeexecerr`. Other error entries outside the range: `dloaderr` (1546,
    `jsr err`), `docerr` (12293: pushes a0=namebuff as message, `bra err`).
- `ezerr (7636)` : data word (see C).
- `comperror (7639)` = `err (7641)` : **never returns.** In: message pointer = longword on top
  of stack ((a7) at entry, i.e. caller's `bsr err` return address or a pushed ptr).
  - dirmode!=0 (`.quiet`): d0 = msg ptr (not popped), a7=errstack, `jmp (errcont)`
    (dircont 3871 takes d0 = error text).
  - else: pointeron; if ezerr==0 (real compile error): if cfetchmode: libat=oldlibat;
    if constmode: `asmerr2` (linenumat from asm line); if firstlocal|firstglob: `droplocals`;
    anyerrs=-1; menuret=linenumat; `jsr endstop`.
    Then (also for ezerr!=0): a0 = popped msg ptr, call editor `comdata->12` with a0=msg
    (d7/a6 saved around), a7=errstack, `jmp (errcont)`.
  - C: `__attribute__((noreturn)) void err(const char *msg)`; errstack/errcont ==
    setjmp/longjmp context (see G).

### Char classes
- `tstalpha (7681)` : Z=1 if d0 in 'A'..'Z' or 'a'..'z' (word compare; tokens never match).
  No clobbers.
- `tstnum (7694)` = `tstnum2 (7697)` : Z=1 if '0'..'9'. No clobbers.
- `tstlab (7704)` : Z=1 if alpha, digit, '_' or loclabch (=39, apostrophe). No clobbers.

### Names / lists
- `makename2 (7715)` : bakup, falls into makename.
- `makename (7716)` : a1=namebuff -> `makename3 (7722)`: d2=0 -> `makename4 (7723)`: (entries
  with caller a1 / a1+d2 to append). getchar3; if d0 != '_' and !alpha -> done. Else store
  byte, d2++, `getchar` (raw, no space skip) while tstlab. If then d0==' ' -> getchar3.
  done: `clr.b (a1)` (a1 left pointing at NUL), `tst d2` -> **Z=1 if no name**. Out d0 = next
  char, d2.w = len, namebuff. No length check (namebuff 128). Clobbers a1,d0,d2.
- `findlast (7746)` : in a2 = addr of a next-field; out a2 = last node (or unchanged if list
  empty); `bra reget` (d0=lastchar, Z).
- `addhere2 (7754)` : like addhere but inserts after a2 (keeps old (a2) as new->next).
- `addhere (7761)` : dirmode -> illdirerr. In a2 = addr of link to store into, d7 = name offset
  in struct, d2 = name len, namebuff. Alloc `d7+d2+1` (doallocmem, d1=1 MEMF_PUBLIC; not known
  to be cleared - callers set fields), `(a2)=new`, a2=new, `0(new).l=0`, `byte d7-1 = total size`
  (d2 also = total size on return), name+NUL copied to d7. `bra reget`. Out a2=new node,
  d2=size, d0=lastchar, Z. Clobbers d0,d1,a0,a1,a6.
- `findlab (7822)` : in a2 = address of a next field (list head or a node to continue after),
  d7 = name offset, d2 = name len, namebuff. Node matches if `size_byte(d7-1) - d7 - 1 == d2`
  and bytes equal. Found: a2=node, `bsr reget`, then **Z=1** forced. Not found: a2 = last node
  (or the head address if empty, so addhere can append), d1=-1, **Z=0**, d0=lastchar.
  Clobbers a0,a1,d0,d1. Edge: d2==0 compares until mismatch (dbf with -1).
  Wrappers (load d7 [+ list head] then `bra findlab`):
  | entry | d7 | a2 |
  |---|---|---|
  | findinc 7789 | 19 | &firstinc |
  | findxinc 7793 | 5 | &firstxinc |
  | findconst 7797 | 9 | &firstconst |
  | findmac 7801 | 13 | &firstmacro |
  | findadd 7805 | 19 | &firstlabel |
  | findtype 7809 | 11 | caller (alltypes) |
  | findproc 7812 | 33 | &firstproc |
  | findfor 7816 | 17 | &firstfor |
  | findvar 7820 | 15 | caller (falls through into findlab) |

### Freeing
- `freeprocvs (7850)` : walk alllocals (12-byte recs), freevars on `4(r)` and `8(r)`, FreeMem 12;
  alllocals=0. Clobbers a1,a3,a4,a6,d0,d1.
- `freevars (7866)` = `freevars2 (7867)` : in a3 = first var/offset node; FreeMem each with size
  byte 14. Out a3=0.
- `freelabels (7878)` : a0=lastrescon (addr of link after last resident label, init &firstlabel);
  list after it is freed, `*(lastrescon)=0`. For each label: if `4(l)` even, free its ref list
  (12-byte nodes); FreeMem label with size byte 18.
- `freepends (7906)` : firstpend list, 8-byte nodes; firstpend=0. `fpnd2 (7909)` inner loop
  entry with a3 (unused elsewhere).
- `freeprocs (7918)` : firstproc list, size byte 32; firstproc=0.
- `freetypes (7933)` : in a2 = first type node; for each: if `4(t)` != $ff (`cmpa.w #$ff`) free
  its offset list via freevars2, then FreeMem size byte 10. (Only called on NewTypes after
  lastrestype; primitive types have 4(t)=1..7 and must never be passed.)

### Variables / array address code
- `fetchvar (7953)` : find/create var named in namebuff (d2 len). In: d0 = char after name,
  flagmask (low byte: bit0 pointer `*`, bit1 array; word copied to var flags), procmode,
  nonew, defaulttype, varoff/locvaroff. `jsr usevars` (uselib allocvars). added=0, varmode=0.
  procmode!=0 -> `fvarlocal`. Else a2=&firstvar -> `fvarback`.
  Out: a3 = var node, a2 = type node, d0 = next char (after optional `.type` / `$`),
  added = -1 if created, varmode = 0 (global, a5 based) or -1 (local, a4 based).
- `fvarback (7965)` : findvar; on name hit but `7(v).b != flagmask.lo` continue findlab from that
  node; exact hit -> fvarfound; miss -> fvaradd. Re-entered by fvarlocal for locals.
- `fvaradd (7973)` : nonew -> nonewerr; addhere; added=~added; a3=new; type: defaulttype,
  `.name` -> makename+findtype(alltypes) else notypeerr, `$` -> stringtype + getchar3.
  Alignment: non-byte types: global `varoff=(varoff+1)&~1`, local `varoff&=~1`.
  `10(v)=type`, `6(v)=flagmask`, size = 4 if pointer/array/NewType (NewType var holds ptr
  to static struct) else `8(type)`. Local: varoff-=size, `4(v)=varoff`; global: `4(v)=varoff`,
  varoff+=size. rts (a2=type).
- `fvarlocal (8016)` : first search firstglob (SHAREd globals) with same flag rule -> fvarfound;
  else varmode=-1, save varoff, varoff=locvaroff, a2=&firstlocal, `bsr fvarback`,
  locvaroff=varoff, restore varoff.
- `fvarfound (8034)` : a3=a2; if `.type` given must equal `10(v)` else rongtypeerr (notypeerr
  if unknown); if `$` var type must be stringtype (rongtypeerr) + getchar3; if flagmask bit0
  and var not pointer -> pointerr. Out a2=`10(v)`.
- `addd0a0 (8061)` : in d3.w = offset; emit code adding it to a2: 0 none; 1..8 (unsigned)
  -> `addq #n,a2` (patches `addqa0` template in place: `&$f1ff | (n&7)<<9`); else
  `lea d3(a2),a2` (patches `leaa0a0+2`). Clobbers d1, d3 (shifted in addq case!).
- `makeamul (8082)` = `makelmul (8084)` : emit 32-bit "d(r-1) += d(r).w * dimsize.l"
  (r=regat). **Reads caller's stack frame**: `4(a7)` at entry = varoff word at top of
  calcvar2's frame (a5/a4 offset of the dimension-size long). Uses pokewda5s (varmode).
  Tail `bra pokewd`. Clobbers d1,d2,d3.
- `calcvar (8170)` = `calcvar2 (8172)` : in a3 = var, a2 = type, flagmask, d0 = char after name
  ('(' for arrays), varmode, regat, debugga. Generates code to address the variable/field.
  Out: **d2**: bit15 = address of thing generated into a2; else d3.w = offset from a5/a4
  (simple var); bit14 = result is a pointer; bit13 = pointer field without flag bit2;
  low byte = type code (0 = struct address/NewType, 1..7 primitive: b w l q f a s).
  **d3** = offset (only valid if bit15 clear; garbage after addd0a0 path). a3 = var or last
  offset node, a2 = type, lastoffset = last offset node (0 if none), d0 = next char.
  regat net unchanged. Errors: noarrerr, nolisterr, syntaxerr, rongtypeerr, noofferr.
  Flow:
  - array (`7(v)` bit1): if added -> noarrerr. getchar3; `()` -> list current item (needs `6(v)`
    bit0 list flag): emit listcode1 (off patched=4(v), pokela5s), listcode2, [debug tokejsr
    currentchk], listarfix. Else: listoff=8 if list; push frame {a2,a3 (movem), varmode.w,
    ndims=8(v).w, varoff=4(v).w, type a2.l, flagmask.w}; bakup; arreval (subscript -> d(regat));
    flagmask/varmode restored; [debug arrszchk]; muld1 by (4 if pointer else typesize)+listoff;
    regat++. Loop per dim: frame varoff += 4 (global) / -4 (local); [debug: emit
    `cmp.l off(a5),d(r-1)` (zarrchk|((r-1)<<9), pokewda5s, off) + arrchk2 (bcs.w +8) + tokejsr
    arrerr]; ndims--; if more: need ',', arrevalchk, makeamul. End: need ')', regat--, pop,
    emit `move.l 4(v)(a5),a2` (movea5a0, pokela5s), `add.l d(regat),a2` ($d5c0|regat),
    list -> listarfix.
    `.normalar`: getchar3; pointer elem + '\' -> emit `.pfix` (move.l (a2),a2), d2=$8000;
    pointer elem without '\' -> d2=$c000; non-pointer -> $8000; -> `.loopm1`.
  - non-array: '(' -> noarrerr. primitive type -> `.simpvar`: d2=6(type).w (=type code),
    d3=4(v); pointer -> bit14; rts (no code). NewType pointer not followed by '\' -> d2=0,
    d3=4(v), bit14 (pointer value). Else d2=$8000, emit `move.l 4(v)(a5),a2`.
  - `.loopm1`/`.loop0` field chain: while '\': need NewType; makename (syntaxerr if empty);
    findvar in `4(type)` list (noofferr); d3 += 4(off); type = 10(off); lastoffset=off.
    `[`: field must be array (rongtypeerr); flush d3 (addd0a0); push a2,a3,varmode,lastoffset;
    emit pusha2; `eval` d2=2 (word); need ']'; emit pulla2; restore; [debug && 8(off)!=0:
    emit brchkcode with reg=regat and bound=8(off)]; muld1 by 4 (pointer) or 8(type);
    emit `adda.w d(regat),a2` (addrega2|regat); getchar3; d3=0, d2=$8000.
    pointer field + '\': emit `move.l (a2),a2` (d3==0) or `move.l d3(a2),a2` (movea0a0
    patched); flag bit2 -> emit adda0a0 (2x add.l a2,a2); d3=0, continue. pointer field
    otherwise -> bit14 (+bit13 if !bit2).
  - `.done`: primitive -> d2.lo = 7(type).b; `bra addd0a0` (flush d3).

## B. Register / global conventions
- a5 = source char pointer throughout the parser (getchar advances it). It may temporarily
  point into cmakebuff (`~expr~`) or usedpath (`\var` UsePath splice) - return markers are
  bytes 1 and 2, handled transparently by getchar.
- d0.w = current char/token (token = $8000|n, 2 bytes in source), mirrored in `lastchar`.
  Z=1 "end of statement" only from getchar3/reget/gchar2 and routines ending with `bra reget`
  (addhere, findlab, findlast).
- prevloc/oldqflag = 1-level undo for `bakup`; locloc/locchar = explicit mark (`storeloc/resloc`).
- quoteflag toggled by every '"' fetched; affects space skip, ';' comments, ':' / Else ends.
- comflag (0 = skipping, !=0 compiling), connest (nested CNIF inside skipped region),
  concomstack/concomsp (saved comflags) - all driven from getchar.
- namebuff + d2.w = current identifier for all find*/addhere; d7 = name offset in node.
- List search convention: a2 = address of a "next" field (head var address works because
  next is at offset 0).
- Code gen: regat = current data register number (d0..d7) for expression results; a2 = address
  register of generated code; a5 = globals base, a4 = locals base in generated code
  (varmode: 0 global / -1 local; pokewda5s/pokela5s flip a5->a4 by bclr #0 of EA reg).
- a6 = exec (4.w) for AllocMem/FreeMem, dos in savefile.
- doallocmem: d0=size, d1=flags -> d0=mem (no NULL test by callers: editor allocator handles
  failure).

## C. Globals defined in range
| name | size | init | purpose |
|---|---|---|---|
| objicon..objiconf | 383 bytes | incbin obj.info | default project icon written for executables |
| infoname | str | ".info" | icon suffix |
| sysdebug | l | $3f0 | HUNK_SYMBOL id |
| cmakebuff | 12 bytes | 0 | decimal text of `~expr~`, ends with byte 1 |
| cmakea5 | l | 0 | a5 to resume after `~expr~` |
| cmake | w | 0 | inside `~` evaluation |
| cmakebak | 128 bytes | - | namebuff backup during cmake |
| ciflineat | w | 0 | line of last CNIF/CSIF (for nocenderr at 2747) |
| gs | 32 bytes | 26x0, 228,246,252,196,214,220 | control->umlaut map in strings |
| opchars/opchars2 | in msg | "ffff" | patchable hex in opterr message |
| liberrnum/tokerrnum | in msg | "ffff." | patched by hexascii |
| ezerr | w | 0 | !=0: simple error (no compile cleanup); set -1 by 4397/6607/15379, cleared 1950 |
| `calcvar2.listoff` | w (in code) | 0 | element size extra (8 for List arrays) - static local |
| templates | see D | | |
Error stubs (127): message strings 1:1, names as labels; C: table or individual functions.
Globals used but defined elsewhere: prevloc, lastchar, errstack, errcont (1269-1273), quoteflag,
oldqflag, comflag, connest, concomstack(ds 32 words? `ds 32` = 32 words = 32 deep), concomsp,
locloc, locchar, usedfrom(8707), namebuff(128), flagmask, added, nonew, varoff, locvaroff,
varmode, firstvar/firstlocal/firstglob/alllocals, firstoff/numoffs, data1, pc, pcat, noinits,
objlen, execname, tsthead/tstsize1-3/tstend/tstdone (265-272), debugga (word, set with `sne`
= high byte only; tested both `tst` and `tst.b` -> treat as "nonzero"), lastoffset, regat.

## D. Machine-code templates (copied into generated code)
All verified in assembled binary.
| label | bytes | emitted by | patch |
|---|---|---|---|
| zarrchk `cmp.l 0(a5),d0` | B0AD 0000 | calcvar2 debug | opcode word only: `|(regat-1)<<9`, pokewda5s; disp poked separately from frame |
| arrchk2 `bcs.w arrchk3` | 6500 0008 | pokel | skips the 6-byte jsr emitted by `tokejsr arrerr` |
| arrchkf `jsr 0` / arrchk3 | 4EB9 0000 0000 | (not used here) | |
| arrmul1 `move.l d0,-(a7)` | 2F00 | makelmul | `|(r-1)` |
| arrmul2 `move.w d0,d0` | 3000 | | `|r |(r-1)<<9` -> move.w dr,d(r-1) |
| arrmul3 `mulu 0(a5),d0` | C0ED 0000 | | `|r<<9`, pokewda5s, disp = frame varoff+2 |
| arrmul4 `mulu 0(a5),d0` | C0ED 0000 | | `|(r-1)<<9`, disp = frame varoff |
| arrmul5 `swap d0` | 4840 | | `|(r-1)` |
| arrmul6 `clr.w d0` | 4240 | | `|(r-1)` |
| arrmul7 `add.l d0,d0` | D080 | | `|(r-1) |r<<9` -> add.l d(r-1),dr |
| arrmul8 `move.l (a7)+,d0` | 201F | | `|(r-1)<<9` |
| (immediate) | D080 | makelmul end | `|(r-1)<<9 |r` -> add.l dr,d(r-1) |
| listcode1 `move.l 0(a5),a2` | 246D xxxx | calcvar2 | `listcode1+2` = 4(v) (self-modified in place), pokela5s |
| listcode2 `move.l -32(a2),a2` | 246A FFE0 | | list "current" pointer at -32 of array data |
| listarfix `addq #8,a2` | 504A | | skip list node header |
| `.pfix` `move.l (a2),a2` | 2452 | calcvar2 | |
| (immediate) $d5c0 | D5C0 | | `|regat` -> adda.l dR,a2 |
| brchkcode0 `cmp.w #0,d0` | B07C 0000 | | opcode base |
| brchkcode | B07C bbbb 6504 7002 4E40 | pokel,pokel,pokewd (10 bytes) | word0 = brchkcode0 `|regat<<9`, +2 = bound 8(off); runtime `trap #0` d0=2 = bounds error |
Templates defined elsewhere but patched here: `addqa0` (524A, data field), `leaa0a0+2`,
`movea5a0+2`, `movea0a0+2`, `endarray+2` (383C imm), `numtoa2` (247C) - all self-modified in
place; in C use local copies (`opcode | field`) rather than writing to the template.

## E. Heap / linked structures (all: `0.l next`, size byte at d7-1, name+NUL at d7)
- **firstoff** reloc list (addoff2): 8 bytes {next.l, offset.l (from pcat)}; numoffs = count.
- **var / NewType offset** node (d7=15; firstvar, firstlocal, firstglob, type->4 list):
  `0 next.l, 4 offset.w (a5/a4 or field offset), 6 flags.w (=flagmask; byte6 bit0 = List
  array; byte7 bit0 pointer, bit1 array, bit2 ?(pointer->adda0a0 x4)), 8 ndims.w (array var) /
  max index for `[n]` field (0 = no bounds check), 10 type.l, 14 size.b, 15 name`.
- **type** node (d7=11; alltypes -> bytetype..stringtype then NewTypes from firsttype):
  `0 next.l, 4.l = primitive code 1..7 (b w l q f a s) or ptr to offset list, $ff = no
  offsets yet (forward/pointer-only); 6.w = low word = type code; 7.b = type code byte;
  8 size.w, 10 size.b, 11 name`. Test "NewType" = `4.l >= 256`.
- **label** (d7=19; firstlabel; created makealab 10352): `4.l` ref list of 12-byte
  {next, pc+2 of reference, procnum.w@8, line.w@10}, or odd value = used as constant;
  `7 bit0` same flag; `8.l` address (0 = undefined); `12.l` data label; `16.w` procnum
  context; `18 size.b`; `19 name`.
- **alllocals** recs (droplocals 14104): 12 bytes {next, firstlocal list, firstglob list}.
- const (d7=9, size@8), macro (13, @12), inc (19, @18), xinc (5, @4), for (17, @16),
  proc (33, @32), firstpend 8-byte nodes. Inner fields of those owned by other chunks.
- Array header in generated program: var slot holds pointer to array data; dim sizes are
  longs at successive var-frame offsets (+4 global / -4 local) after the slot; List arrays:
  `-32(data)` = current item, element = 8-byte node + payload.

## F. External calls
- dos: `Open` (1006), `Write`, `Close`, `Lock` (-2 ACCESS_READ), `Unlock` (savefile/mywrite,
  a6=dos via `dos` global, `jsr lvo(a6)`).
- exec: `FreeMem` (killoffs, free*), via `move.l 4.w,a6`.
- editor via `comdata` (other chunk wrappers): `+88` doallocmem (d0 size,d1 flags -> d0),
  `+72` pointeron, `+76` sleepon, `+12` **error display callback: a0 = message string**
  (called from err; d7/a6 preserved by err around it).
- Compiler routines called from here (other chunks): pokedata1, uselib/usevars, pokewd,
  pokel, pokela5s, pokewda5s, makelong, hexascii (d1.w -> 4 hex at a0, clobbers d1,d2),
  evalconst2 (-> d3), docnif/docsif, droplocals, endstop, makeexecerr, tokejsr (d1 token),
  muld1 (d1 = constant multiplier of d(regat)), arreval, arrevalchk, arrszchk, eval (d2 type).
- Generated code: `trap #0` with d0=2 (bounds) - runtime/debugger side.

## G. Special handling for C
- **err / errstack / errcont**: `bsr err` + inline string -> `err(msg)` noreturn. The
  errstack/errcont pair is a hand-made setjmp: setters at 1532, 1919 (compile -> freeemup),
  3824 (dircont, direct mode, receives msg in d0), 4395, 4429, 6605, 6805 (savefile ->
  return), 15377. C: one global `jmp_buf *errjmp` (+ `const char *errmsg` for dircont);
  each setter = `setjmp`. savefile overwrites the context and does not restore it.
  The `.quiet` path (dirmode) skips pointeron and the editor callback.
- `makelmul` reads `4(a7)` of its caller (calcvar2 frame) -> pass the varoff as a parameter.
  calcvar2 keeps a manual stack frame across arreval/makeamul -> C locals.
- getchar is a loop with tail jumps, recursive through docsif/docnif/evalconst2; saves all
  regs in cmake path (in C: just save namebuff/a5). Watch that getchar may switch a5 to
  cmakebuff/usedpath and back.
- gchar2 entry depends on flags of the caller's `tst d0` -> in C compute from d0.
  getchar3 in-quote path uses byte compare + `gs(pc,d0.w)` index; in C apply only when
  `(d0 & 0xff00)==0 && d0<32` (tokens inside quotes don't occur).
- Self-modifying templates (addqa0, leaa0a0, movea5a0, movea0a0, listcode1, brchkcode,
  endarray, liberrnum/tokerrnum/opchars message text) -> build words in C; error text with
  hex -> snprintf into a static buffer.
- `calcvar2.listoff` is data inside code -> static variable.
- savefile reads up to 3 bytes past a label alloc when writing symbol names; pad instead.
- `findlab` with d2==0 runs past the name (dbf -1); keep or guard.
- `cmpa.w #$ff,a3` in freetypes == compare pointer with 0xff.
- 65335 = staticslib (not a typo, equates 145-148).
- Big-endian assumption: `tst 0(a1,d0.l)` tests high word of a reloc long; `move.b
  flagmask+1` / `7(a2)` low byte of a word; `bclr #0,varoff+1`; `varoff+1`, `optreqga16+13`.
  Use explicit masks in C.
