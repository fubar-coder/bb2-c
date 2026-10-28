# Survey 01 - blitz2.s lines 1-1911 (+ includes wbheader, options2.src.new, asmmac)

Range: header equates, startup (`mainbit` -> wbheader -> `actualbegin`/`begin`), edstruct + token
table, asm token table (asmmac), requester data, object-code templates (`objects`..`tstname`),
misc globals, `atokens` dispatch table, main init/cleanup, menu handlers, run-program machinery
(`menu10`, `procstart`/`runtheprog`), `calcvbr`, `flushc`, `flushlibs`, `makelibs`/`frlibslist`.

Whole program is ONE code section: code, data and templates intermixed; templates are patched in
place (self-modifying data) -> in C they must be writable (non-const) data.

Verified: the template block `objects`..`addrega2` (0x224 bytes) was assembled with vasm
(-devpac -no-opt) and compared with the shipped binary `blitz2` (file offset 10478): identical
except `exg.l a4,a5` which Devpac encodes `$CB4C` (vasm `$C94D`). Use `$CB4C` in C.

---------------------------------------------------------------------------------------------
## A. Routines

### wbheader (include at line 254; first code of the program)
- `wb_header` (wb:4) : entry from DOS. in: CLI: d0=arglen, a0=argptr, 4(a7)=stack size; WB: none.
  Sets `us`=ThisTask (ExecBase+276). If pr_CLI ($ac) != 0 -> `fromcli`. WB: WaitPort/GetMsg on
  pr_MsgPort ($5c) -> `wbmess`; `dosopen` (fail -> `wb_exit`); CurrentDir(sm_ArgList[0].wa_Lock)
  (sm_ArgList=$24); builds "<toolname>.info" in `progfile`, Open(MODE_OLDFILE=1005), Read $4e
  bytes INTO `progfile`, d2 = progfile+$4a (do_StackSize); on any failure d2=4000; `clr.b progfile`.
  If sm_NumArgs (word at $1e) >= 2: CurrentDir(arg[1].wa_Lock), copy arg[1].wa_Name -> progfile.
  Falls through to `wb_allgo` (= next code in blitz2.s, line 255). out: d2=stack size,
  `progfile`=file to edit ("" if none), `dos`=dos.library base.
  Notes: old CurrentDir lock never restored; `.allgo` label unused; .info file handle not closed
  on short read (`.openfailed` skips Close).
- `fromcli` (wb:73) : d2 = 4(a7); `wbmess`=0; if d0-1 != 0 copy args to `progfile` until ' ' or
  LF (terminator replaced by 0). Then `dosopen` (fail -> `wb_exit`), `bra wb_allgo`.
- `wb_exit` (wb:88) : if `wbmess` Forbid/ReplyMsg/Permit; `rts` = program exit (must be at the
  original entry stack level). Returns d0 = wbmess(0 from CLI)/garbage -> `returncode` set in
  `fail0` is LOST (d0 overwritten). Called by bra from `fail0` and dosopen failures.
- `dosopen` (wb:97) : OldOpenLibrary("dos.library") -> `dos`; out: d0, Z=1 if failed. a6=exec.
- `wb_allgo` (wb:112) : label only, falls into line 255.

### blitz2.s
- `mainbit` (253) : label = program start (wbheader code follows).
- line 255-261 : (non-final only: d2=4000) `stackfuck`=d2; `bra actualbegin`.
- `actualbegin`/`begin` (1353/1357) : main init. Counts tokens list (count unused), `lasttoken` =
  last node of `tokens` list (toktnum33). bsr `makeasmtab`; jsr `optreqprep`; `returncode`=-1;
  OldOpenLibrary("mathffp.library") -> `maths` (fail->`fail0`); AllocMem(`constmaxpc`,PUBLIC) ->
  `constpcat`, `constlibat`=constpcat+constmaxpc (fail->`failme`); AllocMem(`macrolen`,PUBLIC) ->
  `macrobuff`, `macrobufff`=end (fail->`fail2`); LoadSeg("blitz2:ted") -> `segadd`
  (fail->`fail2a`); call editor init: `jsr 8(BADDR(segadd))` with a1=`edstruct` -> out Z=1 ok
  (Z=0 -> `fail3`), a0=editor main entry (pushed on stack), a1=editor `comdata` table ->
  `comdata`. `clr returncode`; `int`=comdata[44]; openreq(verreq) via comdata[0]; `sleepon`;
  `linenumat`=-1; `makelibs`; `loadlibs`; `makeiallox`; `pointeron`; loop getinput (comdata[8])
  until d7==-1 && d6==1 (OK gadget). AllocSignal(-1) -> `panicbit` (unchecked), `panicmask`=1<<bit.
  LoadSeg("blitz2:defaultdbug") -> `dseg` (0 ok). Pop a1; a0=`progfile`; `jsr (a1)` = editor main
  loop (returns when user quits). Then `.skip`: FreeSignal, UnLoadSeg(dseg) if any, `freeemup`,
  `freeres`, `freelibs`, falls into fail chain.
- fail chain (fallthrough, each a bra target): `fail3` (1460) UnLoadSeg(segadd) -> `fail2a`
  (1462) `freeallox` -> `fail2b` (1463) FreeMem(macrobuff,macrolen) -> `fail2` (1467) closedos
  (CloseLibrary(dos)) -> `fail1a`/`fail1` (1468/1469) FreeMem(constpcat,constmaxpc) -> `failme`
  (1474) CloseLibrary(maths) -> `fail0` (1477) jsr `optreqdone`; `flushlibs`;
  d0=ext.l returncode; `bra wb_exit`. (failme/fail0 paths never close dos.library; fail2a
  calls freeallox before makeiallox ran - safe because objlen==0.)
- `t` (1482) : stray empty label. `handler` (1488), `funcs` (1913): empty labels.
- `pointeron` (1490) : tail-jump comdata[72] (editor normal pointer). no regs. preserves all
  (editor saves a0-a1/d0-d3/a6).
- `sleepon` (1494) : tail-jump comdata[76] (editor busy pointer). same.
- `menu` (1498) : EDITOR CALLBACK (edstruct+16). in: d6=menu item index (0..), a5=editor
  `firstitem` (first source line), d7=number of lines. Stores `firstitem`=a5, `numlines`=d7;
  `linenumat`=-1; `menuret`=-1; calls `menuadds[d6]`; out: d0.w=`menuret` (-1 = stay, -2 =
  editor shows all, else line number to jump to; set by `err`). Computed call through table.
- `menu_d` (1518) : menu "LOAD DEBUG MODULE". ezfreq (comdata[24]) a2=`dtext`, a0=`dpath`,
  a1=`dname`; Z=1 cancel -> `nodload`; else falls into `loaddmod`.
- `loaddmod` (1528) : in d0=filename. `sleepon`; sets `errstack`=a7, `errcont`=`nodload`
  (error recovery: err longjmps to nodload). LoadSeg(d0) (fail -> `dloaderr`); if old `dseg`:
  UnLoadSeg it + `pointeron` (pointeron NOT called if there was no old dseg - busy pointer
  stays); `dseg`=new. Falls into `nodload`.
- `nodload` (1544) : rts. (errcont target)
- `dloaderr` (1546) : `jsr err` + inline string "Can't LoadSeg debug module!" (never returns;
  err pops the string address from the stack, longjmps to errstack/errcont).
- `menu1` (1556) : "CREATE EXECUTABLE". ezfreq(a2=`xtext`,a0=`xpath`,a1=`xname`); Z -> rts.
  `execname`=d0; `makeexec`=-1; pushes `optreqga15`+12 (Gadget Flags word), sets bit7 of byte
  +13 (GFLG_SELECTED = "make smallest" forced on); bsr `compile`; if `anyerrs`==0 bsr
  `savefile`; restores flags word. Note: compile errors longjmp to errcont(`freeemup`) whose rts
  returns here (errstack = a7 at compile entry), so the stacked word survives.
- `docloseed` (1578) : statement handler (CloseEd token): `closeed`=-1; rts.
- `donocli` (1581) : statement handler (NoCli token): `nocli`=-1; rts.
- `flushc` (1587) : if ExecBase->lib_NegSize (16(a6)) >= 636 call CacheClearU (-636). Preserves
  d0-d1/a0-a1/a6 (all regs effectively). Used from other chunks (directtrap etc.).
- `menu0` (1595) : "COMPILE AND RUN". clr `makeexec`,`closeed`,`nocli`; bsr `compile`; FALLS
  THROUGH into `menu10`.
- `menu10` (1605) : "RUN". if `anyerrs` -> `dontrun`. If `closeed`: call comdata[-8]
  (screenclose = close editor screen) else `openstopreq` with a0=`executing` ("EXECUTING",
  "!PANIC!"). `tasklock` = CurrentDir(0) then CurrentDir(back) (lock NOT duplicated).
  `panictask`=0; CreateProc(d1=`procname`, d2=0 pri, d3=BPTR(`procstart`), d4=`stackfuck`)
  (dos -138); d0==0 -> `.fatal`. `.nostop`: Wait(`panicmask`); `stopit`==0 -> loop; >0 (1 =
  program ended) -> `.notpanic`; <0 (-1 set by `stopcode` exception) -> `chkrealstop` (Z=1 not
  really -> loop) else RemTask(`panictask`) (kills child process, no cleanup).
  `.notpanic`: if !closeed `endstop`. `.fatal`: clr.l `firstlocal`,`firstglob`; if closeed call
  comdata[-4] (screenopen2); TAIL-JUMP intuition ActivateWindow(comdata[80]) with a6=`int`.
  (CreateProc failure leaves stop requester open: endstop skipped.)
- `dontrun` (1673) : rts.
- `procstart` (1686) : fake segment list (see D/G). `cnop 0,4; dc.l 0 (len); procstart: dc.l 0
  (next BPTR)`, code begins at procstart+4 = `runtheprog`.
- `runtheprog` (1688) : RUNS IN THE CHILD PROCESS. `panictask`=ThisTask; CurrentDir(`tasklock`);
  if !`nocli` call comdata[60] (openio: open editor's console/"Hendrix" IO window); `calcvbr`;
  writes vector table: vbr+132 (trap#1 vector) = &`tokens`, vbr+136 (trap#2 vector) =
  `firsttype`, vbr+188 (trap#15 vector) = &`directtrap`; clr `dirmode`; clr.l `firstglob`,
  `firstlocal`; `dirpcat`=`pc` (code-gen pointer global, not the CPU PC); `flushc`;
  d2=`stackfuck`, d7='bLtZ' (magic: force CLI-type run), a0=`argreqla8` (CLI argument string, no
  d0 length!), a1=`pcat` (start of generated code) -> `runthing`.
- `runthing` (1718) : `jsr (a1)` into generated program (clobbers anything). Afterwards if
  !`nocli` call comdata[64] (closeio). `stopit`=1; Signal(`us`, `panicmask`); rts (= child
  process exits).
- `calcvbr` (1734) : d0=AttnFlags (ExecBase+296) & 15; d1 = position of highest set bit (count of
  shifts until 0); if d1>=2 (68020+; a 68010 is treated as no VBR): SuperState (-150),
  `movec vbr,d2` (`dc $4e7a,$2801`), UserState(d0) (-156). `vbr`=d2 (0 otherwise). Clobbers
  d0-d2/a6 (+ scratch of SuperState).
- `flash` (1751) : debug: writes d0 counter to COLOR00 ($dff180) 65536x. preserves d0. Called
  from handletoken on bad token.
- `flash2` (1758) : same with colour 15 (only referenced in a comment).
- `flushlibs` (1765) : TAIL-JUMP AllocMem(256MB, MEMF_PUBLIC) - must fail, forces expunge of
  unused libraries; result ignored (leaks if it ever succeeds).
- `menu8` (1778) : "RELOAD ALL LIBS": `sleepon`, `freeblitzlibs`, `makeblitzlibs`,
  `loadblitzlibs`, tail `bra pointeron`.
- `checklib` (1784) : in: d1=filename, a6=dos. Lock(d1, ACCESS_READ -2); if ok Examine(lock,
  `namebuff`) (result unchecked), UnLock; out d0 = fib_Size (`namebuff`+124), or 0 if no lock
  (Z set accordingly only in the fail case). clobbers d2,d7,d1.
- `makelibs` (1801) : a6=dos; `deflibs` = checklib("blitz2:deflibs") size; `acidlibs` =
  checklib("blitz2:acidlibs") size; FALLS THROUGH into `makeblitzlibs`.
- `makeblitzlibs` (1816) : second entry point. Lock(`libsdir` "blitz2:userlibs/"), Examine
  (fail or not a dir -> unlock, d0=0); ExNext loop over entries: skip files (fib_DirEntryType<0),
  skip names ending ".info" (`endininfo`, Z=1 if .info); allocate 38-byte node with
  `doallocmem` (comdata[88] extalloc; on failure the editor closes the requester and jumps to
  its out-of-memory handler - never returns) (a6=exec, d0=38, d1=MEMF_PUBLIC); insert sorted
  case-insensitive ascending into `libslist` (uppercasing letters via `tstalpha` Z=1 if alpha);
  node.used=-1; strcpy name (unbounded - FIB name up to 107 chars into 32 bytes = overflow).
  out: d0=-1 (Z=0) ok, d0=0 (Z=1) error. a6=dos on exit. clobbers d0-d1,d5-d7,a0-a2,a6.
- `frlibslist` (1898) : FreeMem(38) every node of `libslist`, `libslist`=0. a6=exec. Called from
  `freeblitzlibs` (6641).

### asmmac (include at line 577) - asm-checker subroutines (code inside the token include)
Called by `asmit` (5170: `jsr (a3)` with a3 = token data +10) as "do-it-yourself" opmode checks.
in: d1=opcode word (may be modified), d2=src reg, d3=src mode, d4=dest reg, d5=dest mode,
d6.w=asmsize (0=.b 4=.w 8=.l; default 4), d7 = whatever asmit left (unused by checkers),
d0 = scratch (aslchk family loads an alt opcode into it). Mode numbers: 0 Dn, 1 An, 2 (An),
3 (An)+, 4 -(An), 5 d(An), 6 d(An,Xi), 7 abs.w, 8 abs.l, 9 d(PC), 10 d(PC,Xi), 11 #imm, 12 CCR,
13 SR, 14 USP, 15 reglist/none. out: d0 + CC from moveq: 0 ok (Z), -1 ea error (N), -2 size
error (N), 1 = swap src/dest (asmit tests `bpl`, `beq`, `cmp #-2`). They may modify d1-d5.
- `noerror` d0=0 ; `eaerror` d0=-1 ; `szerror` d0=-2 ; `swapem` d0=1 (all rts; others tail-bra
  into them).
- `movemchk`, `movepchk`, `movechk` (handles move ccr/sr/usp forms, sets size bits in d1),
  `exgchk`, `bitchk`, `roxlchk`/`rolchk`/`lslchk`/`aslchk` (load alt memory-shift opcode in d0,
  all -> `aslchk2`), `abcdchk`, `addchk`, `andichk`. One-line semantics are only needed by the
  asmit owner; translate 1:1 as `int xxxchk(AsmRegs *r)` returning d0.

---------------------------------------------------------------------------------------------
## B. Register conventions seen in this range
- a6 = library base set immediately before each LVO call (exec via `4.w`, `dos`, `int`,
  `maths`). `calldos x` macro = `move.l dos(pc),a6; jsr x(a6)`. `closedos` = CloseLibrary(dos).
- Editor callbacks via `comdata`: `move.l comdata,aX; move.l off(aX),aX; jsr (aX)`.
- Editor -> blitz callback `menu`: d6=item, a5=first line, d7=lines, returns d0.w.
- getinput (comdata[8]/[32]): out d7=-1 gadget event with d6=GadgetID; d7=0 no event (getinput2).
- ezfreq (comdata[24]): in a0=path buf, a1=name buf, a2=title; out Z=1 cancelled, else d0=full
  path string.
- Error recovery: `errstack` (saved a7) + `errcont` (resume address). `err` (7641) pops the
  inline string pointer `move.l (a7)+,a0` after `jsr err`, shows it via comdata[12], then
  `a7=errstack; jmp errcont`. => C: setjmp/longjmp; err(const char*) noreturn.
- Statement handlers from `atokens` are entered by `move.l handler,-(a7); bra getchar3`, i.e.
  getchar3 "returns" into the handler with d0 = next char/token (and its flags) (see 3157-3164).
- asm checkers: d1-d7 as above.
- Generated program entry: d2=stack size, d7='bLtZ', a0=arg string, (a1=entry).

---------------------------------------------------------------------------------------------
## C. Globals defined in range

### equates (lines 13-251, 572-575) - put in header as #defines
demo=0, release=-1, cripple=0, final=-1, version=195.
Internal-lib token numbers ("toke" ids passed in d1 to tokejsr): wbstart $bf00; allocvars 65534;
localloc $c000 locfree $c001 globalloc $c002 globfree $c003 newmem $c004 freelast $c005;
setint $c100 clrint $c101 newint $c102 oldint $c103 intvwait $c104 intcleanup $c105
intgoblitz $c106 intgoamiga $c107; goblitz $c200 goamiga $c201 goqamiga $c202 ownblit $c203
disownblit $c204; runerrlib 65500; finalscheck $c300 remline $c301 contjmp $c304 chkret $c305
snxtchk $c306 lnxtchk $c307 stchk $c308 arrerr $c309 maxerr $c30a gosup $c30b gouse $c30c
eschk $c30d sachk $c30e arrchk $c30f datachk $c310 inblitz $c311 inamiga $c312 casechkw $c313
casechkl $c314 currentchk $c315 ltobover $c316 wtobover $c317 ltowover $c318 qtobover $c319
ftobover $c31a ftowover $c31b ftolover $c31c; newarr $c400 newlocarr $c401 arrmult $c402;
alstat $c500 localstat $c501; copstring $c600 workstart $c601; lastring $c700 astring $c701;
endjmp $c800; getffpbase $c900; quickmult $ca00 longmult $ca01; quickdiv $cb00 longdiv $cb01;
clrloc $cc00; getdstart $cd00; strcomp $ce00 casestrcomp $ce01; setmaxs $cf00 allocst $cf01
freest $cf02; maxlen $d000 lmaxlen $d001; qutofl $d300 fltoqu $d301; modbyte $d400 modword
$d401 modlong $d402 modquick $d403 modfloat $d404; ern $d200 seterr $d201 clrerr $d202
errfail $d205 (getsp/putsp commented out); debuglib 64060; dhandlerlib 1; rundebuglib 64050.
Misc: loclabch 39 (char "'" local-label prefix), inclen 1024, maxmacros 8, ifmacmax 8,
vcodelen 256, mostres 8, userand $7f80, mathq 247-55=192.
Token numbering: lnum=60, fnum=60 (first asm token), tnum = fnum+119 = 179 after asmmac (118
mnemonics + EndAsm). Source tokens are words $8000+num; library tokens have (num & $7f00)!=0.

### data
| name | line | type/size | init | purpose |
|---|---|---|---|---|
| dos | wb:107 | l | 0 | dos.library base |
| us | wb:108 | l | 0 | main Blitz2 Task* (Signal target) |
| wbmess | wb:109 | l | 0 | WBStartup msg or 0 |
| progfile | wb:110 | 192 b | 0 | file to edit; also .info read buffer |
| wb_infoname/wb_dosname | wb:103/105 | str | ".info"/"dos.library" | |
| (options2.src.new) optreq2 | 263 | Requester(112) + borders/ITexts/gadgets | | "Runtime Error Options" window template; gadgets optreq2ga1..9 (Gadget 44 b each). ga1 AUTO RUN (SELECTED), ga2 INTERRUPT CHECKING (SELECTED), ga3 ASSEMBLER CHECKING, ga4 OVERFLOW CHECKING, ga5-7 hidden/not linked (ga4->next=ga8) but still used as option storage, ga8 OK (id 8), ga9 CANCEL (id 9). Toggle gadgets: Flags (+12) bit7 (+13) = SELECTED = option on; Activation 257 = TOGGLESELECT|RELVERIFY. |
| tsthead..tstdone | 265 | longs | see D | hunk-file header templates |
| edstruct | 274 | 6 l + 4 b | tokens,menus,loadxtra,savextra,menu,0,':".;' | passed to editor init (editor copies 28 bytes): token list, menu text, .xtra load/save callbacks, menu callback, cleanup cb (0), chars colon/quote/label/remark |
| tokens | 283 | linked list | 59 static + asm + 33 | struct Token {Token *next; UWORD num; char name[]\0; char help[]\0; even}; tok1..tok59 (num 1..59), tokasm (asmmac, num 60..178, extended data, see E), toktnum..toktnum33 (num tnum..tnum+32 = 179..211, tnum+3 "Ern" removed), last next=0 (`lasttoken` -> toktnum33; libs' tokens get appended there). Names: 1 NEWTYPE 2 End 3 Let 4 Dim 5 Goto 6 Gosub 7 Return 8 Statement 9 Function 10 MouseWait 11 If 12 While 13 Macro 14 Select 15 Case 16 Default 17 AsmExit 18 XINCLUDE 19 INCLUDE 20 Jimi 21 SHARED 22 For 23 Next 24 To 25 Step 26 Data 27 Read 28 CNIF 29 CSIF 30 CELSE 31 CEND 32 CERR 33 Then 34 Else 35 Even4 36 Even8 37 Dc 38 Ds 39 Even 40 Dcb 41 GetReg 42 IncBin 43 Free 44 Use 45 Stop 46 Cont 47 SizeOf 48 SetInt 49 ClrInt 50 MaxLen 51 DEFTYPE 52 BLITZ 53 AMIGA 54 QAMIGA 55 VWait 56 ALibJsr 57 TokeJsr 58 BLibJsr 59 Jimi; 178 EndAsm; tnum+0 List +1 SetErr +2 ClrErr +4 ErrFail +5 Addr +6 MOD +7 Pi +8 Repeat +9 Until +10 PutReg +11 Pop +12 INCDIR +13 EndIf +14 Wend +15 SysJsr +16 WBStartup +17 Maximum +18 On +19 Off +20 Forever +21 Restore +22 Exchange +23 USEPATH +24 CloseEd +25 NoCli +26 BitTst +27 BitSet +28 BitClr +29 BitChg +30 Used +31 Runerrson +32 Runerrsoff. Must stay byte-exact (editor and debugger walk it; trap#1 vector exports it). |
| menus | 724 | strings | | editor menu: title "COMPILER\0", then items "TEXT(23 chars)\0" + shortcut char, ends 0. 10 items in final (+ "PRINT TOKENS" if !final) |
| menuadds | 741 | 10 code ptrs | menu0,menu10,menu1,menu4,menu5,menu6,menu9,menu7,menu8,menu_d (+menu3 !final) | menu dispatch (items: compile&run, run, create exe, options, create resident, view newtypes, cli argument, calculator, reload libs, load debug module) |
| undobuffer | 748 | 128 b, long aligned | 0 | lib path build buffer (loadblitzlibs) + string gadget undo buffer (argreq) |
| libsdir | 750 | str | "blitz2:userlibs/" | user libs dir |
| nomemreq (+bo1-4, la1-16, in1-4, ga1-3) | 753 | Requester + Borders + IntuiTexts + 3 Gadgets | | "Not Enough Object Memory Available!" ; gadget ids 1 INCREASE 150% - RECOMPILE, 2 MAKE SMALLEST - RECOMPILE, 3 CANCEL COMPILE. Activation 5 (RELVERIFY|ENDGADGET), type 1 BOOL |
| asmpnts | 837 | (tnum-fnum)=119 l | 0 | ptr to asm data of each asm token (filled by makeasmtab) |
| verreq (+...) | 839 | Requester etc | | version box: "Blitz Basic 2 Version 1.95" (digits computed from `version`), "(C)opyright 1995 - Acid Software", "Release Version"; gadget id 1 " OKEE DOKEE " |
| tstname | 1256 | str | "ram:test" | unused |
| memat | 1261 | l | 0 | unused |
| memlen | 1262 | w | 0 | unused; `memstackf` label (1264) unused |
| maths | 1265 | l | 0 | mathffp.library base |
| regat | 1266 | w | 0 | current register in expression eval |
| typelen | 1267 | w | 0 | NEWTYPE size being built |
| prevtype | 1268 | l | 0 | NEWTYPE builder |
| prevloc | 1269 | l | 0 | parser |
| lastchar | 1270 | w | 0 | parser |
| thistype | 1271 | l | 0 | current NEWTYPE |
| errstack | 1272 | l | 0 | saved a7 for err longjmp |
| errcont | 1273 | l | 0 | resume code ptr for err |
| linemode | 1274 | w | 0 | 0 normal, 1 inside NEWTYPE |
| linesleft | 1275 | w | 0 | lines left in pass |
| thisitem | 1276 | l | 0 | current editor line ptr (init to &firstitem in dopass) |
| firstvar | 1277 | l | 0 | var list head |
| alllocals | 1278 | l | 0 | |
| firstlocal | 1279 | l | 0 | local var list head (proc) |
| firstglob | 1280 | l | 0 | saved global list while in proc |
| varoff | 1281 | w | 0 | global var offset counter (a5-rel) |
| locvaroff | 1282 | w | 0 | local var offset (a4-rel) |
| firstitem | 1283 | l | 0 | editor's first line (set by `menu`); used as pseudo line node (its offset 0 = next) |
| numlines | 1284 | w | 0 | line count from editor |
| returncode | 1285 | w | 0 | exit code (-1 during init) |
| firstif | 1286 | l | 0 | if/structure stack |
| segadd | 1288 | l(BPTR) | 0 | editor seglist |
| segname | 1289 | str | "blitz2:ted" | |
| mathsname | 1291 | str | "mathffp.library" | |
| cifbras | 1294 | 12 code ptrs | ciflt,cifeq,cifgt,0, cifle,cifne,cifle,0, cifge,cifne,cifge,0 | CNIF/CSIF result branch per comparison code (index d1*4; entries 3,7,11 NULL) |
| atokens | 1298 | 92 code ptrs (atokensend) | | statement handler per token (index num-1; for num>=tnum index = num-(tnum-fnum)-1): 1 donewtype 2 doend 3 dolet 4 dodim 5 dogoto 6 dogosub 7 doreturn 8 dostate 9 dofunc 10 domwait 11 doif 12 dowhile 13 domacro 14 doselect 15 docase 16 dodefault 17 eos 18 doxinclude 19 doinclude 20 syntaxerr 21 doshared 22 dofor 23 donext 24/25 syntaxerr 26 dodata 27 doread 28-31 syntaxerr 32 docerr 33 dothen 34 doelse 35 doeven4 36 doeven8 37 dodc 38 dods 39 doeven 40 dodcb 41 dosetreg 42 doincbin 43 dofree 44 douse 45 dostop 46 docont 47 syntaxerr 48 dosetint 49 doclrint 50 domaxlen 51 dodeftype 52 doblitz 53 doamiga 54 doqamiga 55 dovwait 56 dolibjsr 57 dotokejsr 58 doblibjsr 59 syntaxerr; then (tnum+k): 0 syntaxerr 1 doseterr 2 doclrerr 3 syntaxerr 4 doerrfail 5-7 syntaxerr 8 dorepeat 9 dountil 10 doputreg 11 dopop 12 doincdir 13 mydoendif2 14 dowend 15 dosysjsr 16 dowbstartup 17 syntaxerr 18 doongo 19 syntaxerr 20 doforever 21 dorestore 22 doexchange 23 dousepath 24 docloseed 25 donocli 26-30 syntaxerr 31 dorunerrson 32 dorunerrsoff |
| debugname | 1483 | str | "blitz2:defaultdbug" | default debugger seg |
| dseg | 1485 | l(BPTR) | 0 | loaded debugger seglist (used at 11196) |
| dtext/dpath/dname | 1509 | str / "Blitz2:"+192 b / 192 b | | debug-module file requester buffers |
| dname2 | 1516 | l | 0 | name being loaded |
| xtext/xpath/xname | 1550 | str/192/192 | | executable file requester |
| execname | 1554 | l | 0 | ptr to exe name (savefile) |
| closeed | 1584 | w | 0 | CloseEd: close editor screen while running |
| nocli | 1585 | w | 0 | NoCli: no IO window |
| tasklock | 1675 | l | 0 | current dir lock for child (borrowed) |
| panictask | 1676 | l | 0 | child Process* (RemTask target) |
| panicbit | 1677 | l | 0 | signal bit no. |
| panicmask | 1678 | l | 0 | 1<<panicbit |
| procname | 1680 | str | "Blitz ][ Program Proc" | |
| defname / deflibs | 1770/1772 | str / l | "blitz2:deflibs" / 0 | file / its size |
| acidname / acidlibs | 1774/1776 | str / l | "blitz2:acidlibs" / 0 | file / its size |
Externally defined but heavily used here: comdata(14689) int(14866) stackfuck(14675)
vbr(14660) pc/pcat(14899/14900) dirpcat dirmode libslist(14948) namebuff(15013, long aligned,
128 b followed by namebuff2 128 + 8 = FIB) menuret linenumat anyerrs makeexec firsttype
constmaxpc(=1024) constpcat constlibat macrolen(options.src, 8192) macrobuff macrobufff
lasttoken argreqla8(128 b) optreqga15/16 (options.src) stopit(2450) executing(2523).

---------------------------------------------------------------------------------------------
## D. Machine-code templates (`objects` 914 .. `addrega2` 1254)

Copied to output by `lea X,a0; lea Xf,a1; bsr pokecode` (or pokecode2), or the opcode word /
long is fetched (`move X,d1` / `move.l X,d1`) then ORed with register fields and poked with
pokewd/pokel. Several are patched IN PLACE before copying (marked P). Hex verified vs binary.

| label (end) | code | hex | patch / use |
|---|---|---|---|
| lslimm | lsl #8,d0 | E148 | word ORed with regat (repeated for big shifts) |
| use (usef) | move.l 0(a5),a3; add d0,a3; move.l a3,0(a5) | 266D0000 D6C0 2B4B0000 | P: +2 and usef-2 = var offset (-6(a2)), usef-2 += 4 |
| beginarr | move.l #0,d0 | 203C 00000000 | opcode word only |
| mularr | mulu d1,d0 | C0C1 | unused |
| putarr | move.l d0,0(a5) | 2B400000 | P +2 = varoff/locvaroff; whole long poked |
| doarr (doarrf) | move #0,d2; move #0,d3 | 343C0000 363C0000 | P +2 = 4(a3), +6 = putarr+2 |
| tstword/tstlong | tst.w d0 / tst.l d0 | 4A40 / 4A80 | unused |
| nops8/nops4/nops3 (nopsf) | 8/4/3 x nop | 4E71.. | pokecode2 (debug padding) |
| dataget / dataput | move.l 0(a5),a3 / move.l a3,0(a5) | 266D0000 / 2B4B0000 | P +2 = 10(a1) (data ptr var offset) |
| stlenget | move.l (a3)+,-(a7) | 2F1B | word |
| stalign (stalignf) | addq #1,a3; exg d0,a3; bclr #0,d0; exg d0,a3 | 524B C18B 08800000 C18B | copy |
| dataletb (dataletbf) | addq #1,a3; move.b (a3)+,0(a5) | 524B 1B5B0000 | copy (caller pokes disp after?) |
| dataletbl (dataletblf) | addq #1,a3; move.b (a3)+,0(a4) | 524B 195B0000 | copy |
| dataletb2 (dataletb2f) | addq #1,a3; move.b (a3)+,(a2) | 524B 149B | copy |
| dataletw | move (a3)+,0(a5) / move (a3)+,(a2) / move (a3)+,0(a4) | 3B5B0000 349B 395B0000 | three unlabeled variants at +0,+4,+6 |
| dataletl | same .l | 2B5B0000 249B 295B0000 | +0,+4,+6 |
| nextfixw / nextfixw2 / nextfixl / nextfixl2 | addq #8,a7 / lea 10/12/14(a7),a7 | 504F / 4FEF000A / 4FEF000C / 4FEF000E | For-frame pop |
| nextb/nextw/nextl/nextf (..f) | step add for Next (nextf: jsr -66(a6) SPAdd) | see lst | copy |
| forcompb/w/l/f (..f), forcompf2 | For compare (forcompf2: jsr -42(a6) SPCmp) | contain `bpl/beq .skip` = 6A00 0004 / 6700 0004 (relative, inside template) | copy |
| swapd0 | swap d0 | 4840 | |
| pushindex | move.l a2,-(a7) | 2F0A | |
| leaamp / leaamp2 / leaampl | lea 0(a5),a2 / move.l a2,d0 / lea 0(a4),a2 | 45ED0000 / 200A / 45EC0000 | P +2 = d3 (offset) |
| stamp / stamp2 | move.l (a2),d0 / move.l 0(a5),a2 | 2012 / 246D0000 | stamp ORed; stamp2 P +2 |
| stol | addq #4,a7 | 584F | |
| pushd0wd / pushd0l / pushdn | move d0,-(a7) / move.l d0,-(a7) / same | 3F00 / 2F00 / 2F00 | ORed reg |
| movenumrep | move #0,d7 | 3E3C0000 | P +2 = numreps |
| preplibst / fixlibst | lea 0(a7),a2 / lea 0(a7),a7 | 45EF0000 / 4FEF0000 | P +2 |
| movea3a7 | move.l a3,-(a7) | 2F0B | |
| movestst2 (movestst2f) | move.l 6(a7),d0; move 4(a7),8(a7); move.l (a7)+,(a7) | 202F0006 3F6F00040008 2E9F | first word ORed, rest copied from +2 |
| movea7dn (movea7dnf) | move.l 4(a7),d0; move.l (a7)+,(a7) | 202F0004 2E9F | word ORed, then long at +2 |
| moved0a3 / movedna3 | move.l d0,a3 | 2640 | ORed |
| movestdn | move.l 0(a5),d0 | 202D0000 | P +2 = 10(a1) |
| movea3dn | move.l a3,d0 | 200B | unused |
| putstlen / getstlen | move.l d0,-(a7) / move.l (a7)+,d0 | 2F00 / 201F | |
| moverts | move.l (a7),0(a7) | 2F570000 | P +2 |
| fixstack / fixq / fix1 | lea 0(a7),a7 / addq #8,a7 / move.l (a7)+,(a7) | 4FEF0000 / 504F / 2E9F | fixstack P +2; fixq ORed with count<<9 |
| least / funcst | lea 0(a4),a2 / move.l 0(a4),-(a7) | 45EC0000 / 2F2C0000 | P +2 |
| addq4a2 | addq #4,a2 | 584A | |
| savesbase(2)(f) / ressbase(2)(f) | move.l a3,-(a7); move.l a3,0(a5) / move.l (a7)+,a3; move.l a3,0(a5) | 2F0B 2B4B0000 / 265F 2B4B0000 | unused |
| exga4a5 | exg a4,a5 | CB4C (Devpac) | unused |
| linka4 / unlinka4 | link a4,#0 / unlk a4 | 4E540000 / 4E5C | linka4 P +2 = frame size |
| a2toa7 / a7toa2 | | 2F0A / 245F | unused |
| gotocode / gosubcode | jmp abs.l / jsr abs.l | 4EF9 / 4EB9 + 00000000 | opcode word only, address poked separately (reloc via addoff) |
| mwait (mwaitf) | btst #6,$bfe001; bne mwait | 0839 0006 00BFE001 6600FFF6 | copy (rel. branch inside) |
| libreg | move.l d0,d0 | 2000 | unused |
| libbase / libjsr | move.l 0(a5),a6 / jsr 0(a6) | 2C6D0000 / 4EAE0000 | P +2 (base var offset / LVO) |
| regtransd / regtransa | move.l 0(a5),d0 / move.l 0(a5),a0 | 202D0000 / 206D0000 | used via a3 ptr, ORed |
| litget / litget2 | move.l #0,a3 / move (a3)+,-(a7) | 267C00000000 / 3F1B | unused |
| movea2dn / moved3a5dn | move.l (a2),d0 / move.l 0(a5),d0 | 2012 / 202D0000 | ORed |
| stvarget2 (stvarget2f) + `jsr 0` + varget2skip | moveq #0,d0; move.l d0,-(a7); tst.l (a2); beq varget2skip; move.l (a2),a0; subq #4,a0; move.l (a0)+,(a7) | 7000 2F00 4A92 6700000E 2052 5948 2E98 | P word0 = moveq with reg<<9, word1 low 3 bits = regat; only up to stvarget2f copied; the beq disp $0E ASSUMES the following tokejsr(copstring) emits a 6-byte `jsr abs.l` |
| addcode (addcodef) | move.l (a7)+,d0; add.l d0,(a7) | 201F D197 | both words ORed separately |
| pushlen | move.l (a0)+,-(a7) | 2F18 | |
| leaa5d3a2 | lea 0(a5),a2 | 45ED0000 | P +2 = d3 |
| pusha2 / geta2 / pulla2 | move.l a2,-(a7) / move.l (a7),a2 / move.l (a7)+,a2 | 2F0A / 2457 / 245F | geta2 unused |
| pulla2st (pulla2stf) | move.l 4(a7),a2; move.l (a7)+,(a7) | 246F0004 2E9F | copy |
| pusha3/pusha32 (pusha3f) | move.l 0(a5),-(a7); move.l a3,0(a5) | 2F2D0000 2B4B0000 | P +2 and +6 = 10(a1) |
| pulla3 (pulla3f) / geta3 (geta3f) | move.l (a7)+,0(a5) / move.l 0(a5),a3 | 2B5F0000 / 266D0000 | P +2 = 10(a1) |
| numtoa0 / numtoa2 | move.l #0,a0 / a2 | 207C / 247C + 00000000 | opcode only, then addoff + long |
| endarray | move #0,d4 | 383C0000 | P +2 = count-1 |
| preparray, dimbegin, dimb4, dimaf, dimfin (+f) | | 343C0000 363C0000, 203C00000004, 2B400000, C0C1, 2B400000 | unused |
| movea0 / adda0a0 | move.l (a2),a2 / add.l a2,a2 x2 | 2452 / D5CAD5CA | |
| getebase | move.l 4.w,a6 | 2C780004 | unused |
| movea5a0 / movea0a0 / leaa0a0 | move.l 0(a5),a2 / move.l 0(a2),a2 / lea 0(a2),a2 | 246D0000 / 246A0000 / 45EA0000 | P +2 |
| addqa0 | addq #1,a2 | 524A | P: bits 9-11 replaced (and #$f1ff / or d3) |
| addrega2 | add d0,a2 | D4C0 | ORed |

C suggestion: keep each as `UWORD name[]` (writable), end labels as `name##f` = name+len, i.e.
`#define usef (use+5)`; or one big `UWORD objects[0x112]` with label offsets (from vasm listing
in scratch; offsets relative to `objects`: lslimm 0, use 2, beginarr $c, ..., addrega2 $222).

Hunk templates (line 265-272), must be contiguous as written with write(32)/write(12)/write(4):
- `tsthead` dc.l $3f3,0,1,0,0 ; `tstsize1` dc.l 256,$3e9 ; `tstsize2` dc.l 256 -> 32 bytes
  HUNK_HEADER(no names,1 hunk,first 0,last 0,size) + HUNK_CODE + size. P: tstsize1, tstsize2 =
  code size in longs (savefile 6836, menu5 4140).
- `tstend` dc.l $3ec ; `tstsize3` dc.l 0,0 -> 12 bytes HUNK_RELOC32, count (P = numoffs), hunk 0.
  tstsize3 also reused as a 4-byte zero terminator.
- `tstdone` dc.l $3f2 HUNK_END.

Fake segment for CreateProc (1683-1687): longword aligned `{ULONG len=0; BPTR next=0;}` then
code; BPTR(&next) passed as seglist.

---------------------------------------------------------------------------------------------
## E. Heap / linked structures
- `libslist` node (makeblitzlibs, alloc 38 bytes MEMF_PUBLIC via editor extalloc, freed by
  `frlibslist`): +0 next.l, +4 used.w (-1 = load it; tested by loadblitzlibs `tst 4(a2)`),
  +6 name[32] (dir name under blitz2:userlibs/). Sorted case-insensitive ascending. Head is
  global `libslist` (link insertion treats &libslist as a node: next at offset 0).
- `tokens` list (static, see C). Token: +0 next.l, +4 num.w, +6 name\0, help\0, even.
  Asm token extra data (after even, pointed to by `asmpnts[i]`, filled by makeasmtab 4609):
  +0 opcode.w, +2 src EA mask.w, +4 dest EA mask.w (bits: dn 1, an 2, (an) 4, (an)+ 8, -(an) 16,
  d(an) 32, d(an,xi) 64, abs.w 128, abs.l 256, d(pc) 512, d(pc,xi) 1024, imm 2048, ccr 4096,
  sr 8192, usp 16384, mem(reglist) 32768), +6 byte (srcreg bitpos<<4 | srcmode bitpos), +7 byte
  (destreg bitpos<<4 | destmode bitpos) (15 = none), +8 word (size-field bitpos<<12 |
  .l code<<8 | .w code<<4 | .b code; code 15 = size illegal), +10 chk routine ptr .l (0 = none),
  +14 special.w (bit15 set: bit0 -> if dest An use token+2 (ADDA/MOVEA/...), bit1 -> if src imm
  use token+1 (ADDI/...); low values 1 quick, 2 branch, 3 moveq, 4 trap, 5 dbcc).
  The `c` macro entry "EndAsm" (num 178) links to `toktnum`.
- Editor line items (ted.s rsreset): +0 nextitem.l, +4 lastitem.l, +8 numchars.b, +9 chars[]
  (tokenised). `firstitem` global acts as a dummy node (its offset 0 = first line) and
  `thisitem` starts at &firstitem.
- WBStartup/WBArg, Process, FileInfoBlock (namebuff: +4 DirEntryType, +8 FileName, +124 Size),
  DiskObject (+$4a do_StackSize) accessed by raw offsets -> use NDK structs in C.
- Intuition Requester(112)/Gadget(44)/Border(16)/IntuiText(20) static templates (nomemreq,
  verreq, optreq2) - NOT real Intuition requesters: ted's openreq uses Width/Height (8(a0)) and
  ReqGadget/ReqBorder/ReqText to build a window. Gadget Flags word at +12 doubles as option
  storage (bit 7 of byte +13 = SELECTED).

---------------------------------------------------------------------------------------------
## F. External calls
exec (a6=4.w): OldOpenLibrary -408, CloseLibrary -414, AllocMem -198, FreeMem -210,
AllocSignal -330, FreeSignal -336, Wait -318, Signal -324, RemTask -288, Forbid -132,
Permit -138, GetMsg -372, ReplyMsg -378, WaitPort -384, SuperState -150, UserState -156,
CacheClearU -636 (guarded by lib_NegSize>=636).
dos: Open -30, Close -36, Read -42, Lock -84, UnLock -90, Examine -102, ExNext -108,
CurrentDir -126, CreateProc -138, LoadSeg -150, UnLoadSeg -156.
intuition (`int` = editor's base comdata[44]): ActivateWindow -450.
mathffp (in templates, executed by generated code): SPAdd -66, SPCmp -42.

Editor (blitz2:ted) interface:
- init: `jsr BADDR(seg)+8` (ted `outside`), in a1=edstruct; out Z=1 ok, a0=main entry
  (`.tomain`), a1=&comdata. Main entry: in a0=filename, returns when editor quits.
- `comdata` table (ted.s 6030), offsets used here (and elsewhere):
  -8 screenclose, -4 screenopen2, 0 openreq(a0=requester template; opens window),
  4 closereq, 8 getinput (wait; out d7=-1/d6=gadget id, or menu code), 12 error(a0=text; modal
  error box, waits OK), 16 detokallz, 20 tokall, 24 ezfreq(a0 path,a1 name,a2 title -> Z/d0),
  28 refgads(a0 gadget,d0 n), 32 getinput2 (poll, d7=0 none), 36 combitmap.l, 40 myscreen.l,
  44 extint.l (intuition base), 48 colors, 52 halfcolors, 56 tokone(a0 line), 60 openio,
  64 closeio, 68 actgad(a0), 72 pointeron, 76 sleepon, 80 mywindow2.l (editor window),
  84 (set by ted to defaultreqga9+13), 88 extalloc(d0,d1,a6=exec; AllocMem or abort to editor
  outmem), 92 bmapmul.w, 94 req_window.l (requester window; 86(win)=UserPort, used by
  openstopreq).
- Editor -> Blitz callbacks (in edstruct): `menu` (d6,a5,d7 -> d0), `loadxtra`, `savextra`
  (defined at 15345/15412, other chunk), cleanup = 0.
- Loaded seg "blitz2:defaultdbug" (`dseg`) - used elsewhere (11196).
Hardware: $dff180 (COLOR00) in flash/flash2; CIA-A $bfe001 in mwait template (generated code).
CPU: movec vbr,d2 in supervisor (calcvbr); writes to exception vector table (runtheprog).
Child process: CreateProc of `procstart` fake seglist; program code at `pcat` executed.

---------------------------------------------------------------------------------------------
## G. Special handling needed in C
1. Startup (wbheader): needs stack size from 4(a7) at CLI entry and the raw arg string/len;
   with a C startup, get stack via pr_CLI->cli_DefaultStack*4 (or tc_SPUpper-tc_SPLower) and
   argv / WBenchMsg; replicate WB .info stack read (or GetDiskObject). Original exit code is
   always 0 (bug) - decide whether to keep.
2. Editor init/main entry are register-called code in a LoadSeg'ed hunk: asm stubs
   `ted_init(a1) -> {ok(Z), a0, a1}` and `ted_main(a0)`. All comdata callbacks take/return
   registers (d6/d7 results, Z flag for ezfreq) -> one asm glue per offset (or GCC
   `register ... __asm("a0")` + reading d6/d7 via asm). Editor's register-saving is per routine.
3. Callbacks called BY the editor with register args: `menu` (d6,a5,d7 in; d0 out), loadxtra,
   savextra -> asm entry thunks that save d2-d7/a2-a6 and call C.
4. `procstart`/`runtheprog`: CreateProc needs a longword-aligned fake seglist whose code at +4
   jumps to C `runtheprog` (static aligned struct {0, 0, 0x4EF9, &runtheprog}; flush cache), or
   use CreateNewProc(NP_Entry) on V36+. Child shares all globals with the parent: `stopit`,
   `panictask`, `firstglob`, `firstlocal`, `dirmode`, `dirpcat`, `vbr` -> volatile.
   RemTask kills the child abruptly (C runtime of child must not hold resources).
5. `runthing`: `jsr (pcat)` with d2=stack, d7='bLtZ', a0=args; generated code may clobber
   every register (incl. a4/a5/a6) -> asm wrapper saving d2-d7/a2-a6.
6. `calcvbr`: SuperState/UserState + `movec vbr,d2` (or Supervisor() with an asm routine).
7. Vector table writes (vbr+132, +136, +188 = trap #1/#2 used as mailboxes for &tokens and
   firsttype, trap #15 = `directtrap` handler which must be asm: it edits the exception frame
   and `rte`s).
8. `flushc` must be called after code generation / before jumping into generated code.
9. `err`/errstack/errcont: non-local exits across many levels (setjmp/longjmp); `loaddmod`,
   `compile` etc. set their own recovery points; err's `move.l (a7)+,a0` inline-string trick ->
   `err("text")`.
10. Templates are mutable data with in-place patching at fixed byte offsets (section D);
    stvarget2 relies on a following 6-byte jsr.
11. `atokens` (92 code pointers) and `menuadds`, `cifbras` are computed-jump tables; atokens
    handlers are entered via push+`bra getchar3` (d0 = next char + CCR as input) - the handler
    signature must be agreed with the getchar/handletoken owner.
12. asmmac checkers return results in d0 + condition codes and modify d1-d5 in place -> C
    functions with a struct of the "registers".
13. Token list / menus / edstruct are shared byte-exact data with the editor and debugger
    (editor copies edstruct 28 bytes; walks token list) -> must be laid out exactly (packed,
    big-endian, word-aligned, names inline) - use hand-written initialisers or assembler data.
14. `flash` touches custom chip register; `flushlibs` deliberately requests 256MB.
15. `stopcode` (other chunk, 2453) is installed as tc_ExceptCode (Task+42) with tc_SigExcept
    (Task+30) - exec exception handler called with d0=signals -> asm thunk.
