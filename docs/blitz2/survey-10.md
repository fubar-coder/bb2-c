# Survey chunk 10 - blitz2.s 14656-16015 (+ typereq, options2.src.new, options.src, calcreq, asmmac)

Range content: `mydata` data section (compiler globals), the operator and conversion tables, the builtin
type list, the Type-Info browser (menu6, typereq), the .xtra load/save (loadxtra/savextra = editor
callbacks), the Options requester (menu4, optreq/optreq2), the Calculator (menu7, calcreq), and the
CLI-argument requester (menu9, argreq). Included files are almost entirely Intuition data. The exception is
asmmac, which is the asm-mnemonic token table plus the EA-check routines (it is included at line 577,
in chunk 1's token list, but I describe it here as I was asked to).

`typereq.src` is **not** asm. It is a requester-editor save file (title, then gadget coordinates and
texts as plain numbers). The `typereq` file (asm) was generated from it. Ignore typereq.src for the port.

Intuition offsets used throughout (standard 1.3 structs):
- Requester: +16 ReqGadget, +20 ReqBorder, +24 ReqText. Our reqs: `dc.l 0 ; dc.w L,T,W,H,0,0 ; dc.l gad,border,itext ; ...` (size 112).
- Gadget: +0 Next, +4 L, +6 T, +8 W, +10 H, **+12 Flags.w** ($80 GFLG_SELECTED, $100 GFLG_DISABLED, $4 GADGIMAGE), +14 Activation.w (1 RELVERIFY, 2 IMMEDIATE, **4 ENDGADGET**, $100 TOGGLESELECT, $800 LONGINT), +16 GadgetType.w (1 bool, 3 prop, 4 string, $1000 REQGADGET), +18 Render, +22 SelRender, +26 Text, +30 MutualExclude, +34 SpecialInfo, **+38 GadgetID.w**, +40 UserData.
  - "`xxxga+13` bit 7" = low byte of Flags = GFLG_SELECTED, i.e. "toggle is on". "`+12` word" = whole Flags word, and a value of 128 means only SELECTED is set.
- StringInfo (36 bytes): +0 Buffer, +4 UndoBuffer, **+8 BufferPos**, +10 MaxChars, **+12 DispPos**, ..., **+28 LongInt.l**, +32 AltKeyMap. In these files the text buffer comes right after the StringInfo, so `bufferlabel-8` = LongInt and `bufferlabel-6` = low word of LongInt.
- PropInfo: +0 Flags, +2 HorizPot, **+4 VertPot**, +6 HorizBody, **+8 VertBody**.
- IntuiText 20 bytes, then its string.

Editor callback vector `comdata` (pointer to the ted.s table; offsets as used here; meanings come from ted.s 6030):
| off | ted name | in | out |
|---|---|---|---|
| 0 | openreq | a0=Requester | d0=0 fail (never tested here) |
| 4 | closereq | - | - |
| 8 | getinput | - (waits) | d7=-1 gadget (d6=GadgetID), -2 menu, -3/-4 mouse ...; **auto-closes the requester on GADGETUP of an ENDGADGET gadget** |
| 12 | error | a0=message | (used by `err`) |
| 28 | refgads | a0=first gadget, d0=count | RefreshGList(a0,req_window,0,d0); a2 ignored |
| 32 | getinput2 | - (poll) | d7=0 if no message, else like getinput |
| 40 | myscreen (data) | - | Screen* (for DisplayBeep) |
| 56 | tokone | a0=line text | tokenises the line in place |
| 68 | actgad | a0=gadget (a2=req, ignored) | Delay(8) + ActivateGadget(a0,req_window,0), d0=BOOL |

---------------------------------------------------------------------------------------------------
## A. Routines

| name (line) | in | out / notes |
|---|---|---|
| zerotypes (15018) | - | Fills the 12 type-list line strings typereqla30..la41 (stride `typereqla31-typereqla30` = 48 bytes, 27 chars each, NUL left) with spaces. Clobbers d0,d1,a0,a1. |
| makebody (15031) | maxtypes | typereqvp1+8 (VertBody) = $FFFF if maxtypes<=12, else (12<<16)/maxtypes (divu, low word). typereqvp1+4 (VertPot)=0. Clobbers d0,d1. |
| countlist (15043) | a2=first node (linked via +0) | maxtypes=count; typeat=0. Clobbers d0,a1. |
| showlist (15064) | a2=first item, d2=0 for type list / nonzero for field list, d3=name offset in node (11 type, 15 field) | Saves a2,d2,d3 to f1,f2,f3. **Falls through** into showlist2. |
| showlist3 (15059) | f1,f2,f3 | Reloads a2,d2,d3, then `bra showlist2`. |
| showlist2 (15067) | a2,d2,d3, typeat, maxtypes | zerotypes. Skips `typeat` nodes, then renders up to 12 lines into la30..: optional prefix '*' (field flags.w@6 bit0) or '@' (bit0+bit2), then the name. For a field it adds '.'+typename (field@10 -> type, name at +11) and "[n]" if bit1 (n=word@8, via makelong into `fuckoff`). Each line is clipped at 27 chars. Clobbers d0-d4,a0-a5. |
| typeshow (15140) | typereqvp1+4, maxtypes, typeat | If maxtypes>=11: typeat=((maxtypes-11)*VertPot)>>16. If typeat is unchanged it returns; else showlist3, then **falls into reflist**. |
| reflist (15152) | - | refgads(typereqga5, 12) as a **tail jump** through comdata+28. |
| allofem (15158) | firsttype | a2=firsttype (first user type); countlist; makebody; d2=0, d3=11; `bra showlist` (tail). |
| menu6 (15165) | menu entry (menuadds) | "View Type Information" browser. See gadget table. Returns rts after FAREWELL (id 4, ENDGADGET). Calls findtype (chunk ~7809: a2=list head, namebuff/d2=len; Z=1 found, a2=item). Uses DisplayBeep(comdata->myscreen) through `callint` (a6=int). Clobbers everything. |
| typename (15281) | a1=src name | Copies the name into typereqla21 (TYPE: string buffer), at most 31 chars, NUL included if it fits. |
| loadxtra (15345) | **called by the editor** (edstruct+8): d7=DOS file handle, a6=DOSBase | freeres2 (UnLoadSeg of the residents, needs a6=dos). Reads every optspnts item with Read(d7,addr,size). On a short read it goes to .done2. If vers==0 (old file) it migrates optreq2 flags. Else it reads the 128-byte CLI arg into argreqla8. Then it loops reading 4-byte records {libnum.w,max.w}: findlibnoerr(d1) -> if found, sets the lib's max word; stops at EOF or unknown lib. Sets errstack/errcont=.done2, ezerr=-1 around that loop. Then clears resbuff[i].seg (8x), makeiallox, loadres (d7/a6 saved). rts. |
| savextra (15412) | **editor callback** (edstruct+12): d7=fh, a6=DOSBase | vers=1 (byte). Writes all optspnts items, then argreqla8 (128 bytes), then for every lib in firstlib with a max (flag word at lib+[lib+18]-2 != 0) it writes libnum.w (lib+4) and max.w (lib+[18]-4). Write results are not checked. |
| menu4 (15456) | menu entry | Options requester. Snapshots every optspnts item into optssave. Converts the 6 buffer-size LongInts to text (makelong into the buffers). **Falls into menu4c**. |
| menu4c (15483) | - | reschg=allchg=maxat=0; makemax; makeres; openreq(optreq); event loop (see table); ends in optsdone or menu4canc. Only referenced as fallthrough. |
| optsdone (15592) | d6=17/18/19 | If allchg: makeiallox. If reschg: loadres. d6=18 -> **jmp menu0** (compile/run, tail call); 19 -> **jmp menu1** (create executable, tail call); 17 -> rts. |
| menu4canc (15612) | - | Restores the optssave snapshot back into all optspnts targets. rts (the requester was already closed by ENDGADGET). Reached by `beq` from menu4c. |
| newres (15627) | - | makeres; refgads(optreqga23,2); then refgads(optreqga27,2) as a tail `jmp (a5)`. |
| makeres (15637) | resat | Clears BufferPos/DispPos of optreqsgz/sgy. Sets sgz.Buffer=resat, sgy.Buffer=resat+68. Up arrow ga23 is DISABLED if resat==resbuff; down arrow ga24 is DISABLED if resat+68==resbuff+7*68. |
| makemax (15662) | maxat, nummaxs, firstlib | Blanks optreqla54/55 (12 bytes) and la108/109 (14 bytes). Shows max entry #maxat in row 1 (ga25 label, ga7 int) and #maxat+1 in row 2 (ga26, ga8). Enables/disables these and the arrows ga21 (up, disabled if maxat==0) and ga22 (down, disabled if fewer than 2 remain). Calls maxit. |
| maxit (15713) | d1=index among libs that have a max, a1=label dest, a0=number dest | Finds that lib. Copies the object name (lib + long@[lib+18]-26 = offset to the token name) to a1 and **replaces the NUL with 's'** (plural, text is not terminated, buffer is pre-blanked). Then `jmp makelong` (tail) with d2=max word. |
| menu7 (15739) | menu entry | Calculator. calclibat=libat; errstack=a7, errcont=.calcerr (so a compile error in the expression comes back to .calcerr with a7 reset, and the err requester is shown first). Loop: actgad(calcreqga1) until d0!=0, getinput. id1 (Return in string): tokone(calcreqla15), setcvars, setsvars, connest=0, a5=calcreqla15, **evalconst2** -> d3=value. Formats the result over calcreqla15 as dec (makelong), %bin or $hex (no leading zeros, '-' sign if negative, magnitude). Resets BufferPos/DispPos, refgads(ga1,1). id2..4: radio-button logic (clears the other SELECTED, sets the clicked one, refgads(ga2,3)). id5: rts. |
| menu7.calcerr (15891) | via err/errcont | libat=calclibat; clears the buffer; bra .pc (refresh, loop). |
| menu9 (15979) | menu entry | CLI argument. Backs up argreqla8 (128 bytes) to namebuff. openreq(argreq); actgad(argreqga1). Waits for id 1/2 (keep the text) or 3 (CANCEL: restore from namebuff). All three are ENDGADGET. rts. |

asmmac code (EA/opcode check routines, called by `asmit` (5150) via the table's checker pointer, `jsr (a3)`):
- Register interface (in **and out**, everything modified is used afterwards): d1=opcode word, d2=src reg, d3=src mode, d4=dst reg, d5=dst mode (mode numbering below), d6=asmsize (0=.b, 4=.w, 8=.l; the header comment "d6=src longint" is stale), d7=dst?, extraword(s).
- Result is in d0 **and the condition codes**: asmit tests `bpl/beq` right after the jsr. Codes: noerror d0=0 (Z=1), swapem d0=1 (swap src/dst operands, also swaps extraword, buff1/2 and asmlen), eaerror d0=-1 (N), szerror d0=-2 (N). abcdchk/addchk `.ok` return with Z=1 from a tst/cmp and **without setting d0**. Treat that as "ok".
- noerror(334) eaerror(337) szerror(340) swapem(343); movemchk(346: may bit-reverse the reg mask in d2 via d6 for -(an)); movepchk(374); movechk(381: picks MOVE size bits, or rewrites d1 into move ccr/sr/usp forms and zeroes the ea regs); exgchk(451); bitchk(464); roxlchk/rolchk/lslchk/aslchk -> aslchk2(485: register or memory shift forms); abcdchk(519); addchk(527: dn,<ea> form, bset #8 + swapem); andichk(535: to-ccr/sr size check).

## B. Register conventions

- a6: dos base on entry to loadxtra/savextra (`read(a6)`/`write(a6)` = LVO Read/Write). `callint` loads a6=int itself.
- Requester event loops: d7/d6 come from getinput (d7=-1 means gadget, d6=GadgetID). menu4 `.ugw` **calls getinput again and then re-dispatches the new event at `.igot`** (the int gadget value is committed only after the next event).
- showlist family: a2=list node, d2=field-mode flag, d3=name offset; kept in f1/f2/f3 for showlist3.
- evalconst2: a5=tokenised text in, d3=result out (used in menu7).
- makelong: d2=unsigned value, a0=dest. It writes decimal, NUL-terminates and returns a0 at the NUL. It clobbers d1-d4.

## C. Globals defined (blitz2.s 14658-15015 and the other labels in range)

Scalars (dc = word):
| label | type | init | purpose |
|---|---|---|---|
| mydata | label | - | start of the data section |
| vbr | .l | 0 | saved VBR (68010+, set ~1748) |
| numclibs | equ 3 | | number of constlibs |
| cfetchmode | w | 0 | 1 = evaluating a constant (uselib intercepted) |
| constlibs | 3 w | 64935,64735,64835 | lib numbers allowed in constant evaluation |
| constpcat, constlibat | .l | 0 | PC buffer for constant evaluation |
| constmaxpc | .l | 1024 | size of that buffer |
| stackfuck | .l | 0 | stack size (from wbheader d2) |
| lastrescon/lastresmac/lastrestype | .l | &firstlabel/&firstmacro/&firsttype | end of the resident part of each list |
| rescnt, reschg | w | 0 | resident count / residents changed |
| resat | .l | resbuff | top visible resident slot in optreq |
| linedone | w | 0 | |
| dirpcat | .l | 0 | direct-mode pc |
| diropt | w | 0 | |
| dira4,dira5,dira6,dirstack | .l | 0 | direct-mode saved regs/stack |
| comdata | .l | 0 | editor callback table pointer |
| incdir | 96 bytes | 0 | INCDIR path |
| dirmode | w | 0 | direct-mode flag (err is quiet) |
| int | .l | 0 | IntuitionBase |
| maxat | w | 0 | top row index in the Object Maximums list (optreq) |
| maxsat | .l | 0 | data1 pos of the max info |
| nummaxs | w | 0 | number of libs with a max |
| maxsused | w | 0 | |
| dark, qlab | w | 0 | saved-with-source defaults |
| defaulttype | .l | quicktype | default var type |
| anyerrs | w | -1 | errors in compile |
| makeexec | w | 0 | building an executable |
| noinits | .l | 0 | 0 = there are init routines? (6 = none) |
| letstart | .l | 0 | |
| nomemleft | w | 0 | |
| lastbiglab | 128 bytes | | last non-local label name |
| locdatast, firstpend | .l | 0 | |
| data2at,data2,data1at,data1,pcat,pc,libat,lib,allat | .l | 0 | code/data buffers: `*at` = base, plain = current pointer; allat = highest allocated |
| dummyasm | 5w+.l+w (16 bytes) | 0 | dummy asm table entry (asmpnts block format) |
| buff1,buff2 | .l | 0 | asm src/dst text buffers |
| constmode | w | 0 | !=0 in assembler |
| asmtype | w | 0 | |
| extraword, extraword2 | w | 0 | asm extension words (read as one .l at extraword) |
| asmsize | w | 0 | 0/4/8 = b/w/l |
| asmbuff | .l | 0 | |
| asmlen, asmlen2 | w | 0 | |
| firstasm | .l | 0 | list of pending asm fixups (one pass) |
| macnum | w | 0 | |
| nextinc0 | .l | 0 | next NUL in the include buffer |
| titleat | .l | 0 | |
| titleadd | 5 w | | compile progress |
| connest | w | 0 | conditional nesting level |
| firstconst | .l | 0 | constant list (#a) |
| numarg | w | 0 | macro arg count |
| myoline, myline, mylinef | .l | 0 | macro line buffer |
| concomstack | 32 w | | CNIF stack |
| concomsp | .l | concomstack | |
| oldqflag | w | | |
| lastconloc .l, lastconop w | | | |
| firstfor | .l | | FOR list |
| pushflag, sbgot, usertype, userp | w | | |
| libslist | .l | 0 | |
| stackuse, numreps | w | | lib parameter gathering |
| locloc .l, locchar w | | | |
| stbuff | 8 .l | | string var offsets |
| commode | w | | |
| lastoffset | .l | | last offset in calcvar |
| linenumat | w | -1 | current line (copied to menuret on error) |
| menuret | w | -1 | menu return code to the editor |
| optresoff | w | | |
| varcode, varcode2 | vcodelen (256) bytes each | | captured var access code |
| varcodelen, varcodelen2, oldvcodelen | w | | |
| procjmppc | .l | | |
| memlib | .l | | memory library address |
| memlibstat, tempvmode, tempvmode2, varmode | w | | varmode 0 = a5 points to globals, else locals |
| thisproc, firstproc | .l | 0 | proc list |
| procmode | w | 0 | 0 normal, 1 statement, 2 function |
| comflag | w | | 0 = no compile |
| passstack | .l | | |
| zero | .l | 0 | constant zero |
| numincs | w | | include count |
| firstxinc, firstinc | .l | | xinclude/include lists |
| lasta6 | w | -1 | |
| quoteflag | w | | |
| macrobuff, macrobufff | .l | | macro buffer start/end |
| firstmacro | .l | | macro list |
| endop | .l | | pc before the finish code |
| firstlabel | .l | | label list |
| lasttoken | .l | | |
| numstatic w, staticdata .l | | | statics |
| firstlib | .l | | loaded Blitz lib list |
| sbasegot | w | | |
| litdata1 | .l | | |
| objlen, numoffs, firstoff | .l | | reloc offset list (hunk reloc) |
| flagmask, added, nonew | w | | var search |
| temp1, temp2 | .l | | scratch (temp1 = 4-byte read buffer in loadxtra) |
| namebuff, namebuff2 (+8 pad) | 128+128+8, **cnop 0,4 (4-aligned)** | | name buffers. **The whole 264 bytes is also used as a FileInfoBlock** (Examine/ExNext at 1794..1835, 6437), so it must stay one contiguous, long-aligned block in C |
| optstuff | 64 bytes | | temp opt buffer |
| typeat, maxtypes | w | 0 | typereq list position / count |
| f1 .l, f2 w, f3 w | | | saved showlist args |
| fuckoff | 8 bytes | | number text for "[n]" |
| allchg | w | | buffer-size gadget changed (1..6) -> makeiallox |
| vers | w, **accessed as a byte** (high byte) | 0 | .xtra version (1 = new) |
| calclibat | .l | | libat saved during the calculator |

Tables:
- **convtable** (14693): 7x7 code pointers, index [(from-1)*7+(to-1)], type order b,w,l,q,f,a(address),s. A 0 entry means same type / no conversion. noconerr = error. Address behaves like long; string converts only to l/a (stringtolong).
- **operators** (14704): list of 32-byte entries {op.w, handler.l x7 (b,w,l,q,f,a,s; 0 = invalid for that type), priority.w}, terminated by op.w=0. op is an ASCII char or pair ('<>','<=','>=' as a word), a token ($8006+tnum=Mod, $8000+26..29+tnum = BitTst/BitSet/BitClr/BitChg), or an asm-mnemonic token used as a word operator (AND/OR/EOR/LSL/ASL/LSR/ASR = `(opX-opabcd)/$1c+$8000+fnum`, see D/asmmac; the real indices are AND=6, ASL=8, ASR=9, EOR=55, LSL=64, LSR=65, OR=78, NOT=77 (opnot used at 9728), so token $8000+60+idx). Priorities: comparisons 2000, + - 3000, * / 4000, & | Mod 5000, shifts 6000, ^ 7000, bit ops 8000, AND/OR/EOR keywords 1000. Labels opeq,opne,oplt,ople,opgt,opge mark individual entries (referenced elsewhere).
- **modetable** (14831): dc.l typemode (one code pointer, used by 2947).
- **alltypes** (14833): .l -> bytetype. The builtin type nodes are bytetype->wordtype->longtype->quicktype->floattype->addresstype->stringtype. Each node is {next.l, kind.l (=1..7, <256 means primitive), size.w (1,2,4,4,4,4,4), nlen.b=13 (=11+namelen+1), name 'b','w',..,'s',0, pad}. **`firsttype` is a label placed on stringtype**, so `firsttype` (read as .l) = stringtype.next = first user type, and `lea firsttype,a2` is the list head for user types. `lea alltypes,a2` walks builtin+user types. In C: one shared node array and a head pointer aliasing stringtype.next.
- dummytype (14858): {0,0,size 0, nlen 1}.

Options globals defined in options.src (used across the compiler; they are the LongInt fields of the int gadgets):
`maxpc`=16384 (optreqga1 Object Buffer), `maxlib`=32768 (ga2 Libs Buffer; text says 16384), `maxdata1`=4096 (ga3 System Buffer), `maxdata2`=4096 (ga4 Data Buffer), `macrolen`=8192 (ga5 Macro Buffer), `stringwork`=10240 (ga6 String Buffer). Each is followed by AltKeyMap .l 0 and then its 12-byte text buffer optreqla48..53. `resbuff` = 8 x 68 bytes {name[64], seglist BPTR @64}. `uimageloc/dimageloc` = chip copies of the arrow images. Also `undobuffer` (748, 128 bytes, outside range) is shared by all string gadgets.

optspnts (15290) - descriptor list of everything saved in .xtra / snapshotted by menu4:
```
{ size.w ; addr.l ... ; 0.l }*  terminated by size.w = 0
size 1 : vers(byte), optreqga9+13           ; icon flag byte
size 2 : jimi0: optreqga10..16 +12, optreq2ga1..7 +12  (14 Flags words) jimi1: 0
size 4 : jimi2: optreqla48-8 .. la53-8 (6 LongInts = maxpc..stringwork) jimi3: 0
size 544 : resbuff
size 384 : xpath (xpath[192]+xname[192], defined at 1552)
0
```
optssave = 982 bytes (sum of the above). File layout of .xtra = optspnts items in order, then argreqla8[128], then {libnum.w,max.w}* until EOF. Version 0 files have no CLI arg block, and on load the optreq2 flags get migrated (ga1=128, ga2=ga6, ga3=0, ga4=ga7).

## Requesters (data from includes)

**typereq** (typereq, 272x172, "View Type Information"):
| id | gadget | kind | code use |
|---|---|---|---|
| 1 | typereqga1 | prop, vertical (PropInfo typereqvp1, Flags 13=AUTOKNOB/FREEVERT/BORDERLESS), RELVERIFY+IMMEDIATE | VertPot -> typeat (typeshow). makebody writes VertBody/VertPot. Refreshed with ga2. |
| 2 | typereqga2 | string, buffer typereqla21[32], MaxChars 32 | type name typed in; activated every loop |
| 3 | typereqga3 | bool "TYPE:" | clear name, show all types |
| 4 | typereqga4 | bool " FAREWELL " ENDGADGET | exit |
| 5..16 | typereqga5..16 | bool, text-only (IntuiText typereqin4..15 -> la30..la41, 27 chars) | list lines; click -> descend into type / field type |

**optreq** (options.src, 424x172, "BLITZ BASIC 2 COMPILER OPTIONS"). Gadget chain: ga1..ga10 -> **ga15** (ga11..14 are not linked: hidden, but their Flags are still saved) -> ga16..ga28.
| id | gadget | kind | meaning / code use |
|---|---|---|---|
| 1..6 | optreqga1..6 | int string (LONGINT, RELVERIFY), LongInt = maxpc,maxlib,maxdata1,maxdata2,macrolen,stringwork | click -> allchg=id -> makeiallox on OK |
| 7,8 | optreqga7/8 | int string, buffers optreqla54/55 | max value for row 1/2. Value goes to lib max (-4) via the low word at la54-6/la55-6 |
| 9 | optreqga9 | toggle | **+13 bit7 = Create icons for executables** (6946) |
| 10 | optreqga10 | toggle | **+13 bit7 = Runtime error debugger on** (1945,3296). On click while on -> runerropts (optreq2) |
| 11 | optreqga11 (hidden) | toggle | "Debugger" (old) |
| 12 | optreqga12 (hidden) | toggle | "Long Arrays" (old) |
| 13 | optreqga13 (hidden) | toggle | "Expand Macros" (old) |
| 14 | optreqga14 (hidden) | toggle | "Quoted Macros" (old) |
| 15 | optreqga15 | toggle | **+13 bit7 = Make smallest code** (1570 forces it on temporarily, 1961, 5290) |
| 16 | optreqga16 | toggle | **+13 bit7 = Create debug info for executables** (6879) |
| 17 | optreqga17 | " OK " ENDGADGET | optsdone, rts |
| 18 | optreqga18 | " COMPILE/RUN " ENDGADGET | optsdone -> jmp menu0 |
| 19 | optreqga19 | " CREATE EXECUTABLE " ENDGADGET | optsdone -> jmp menu1 |
| 20 | optreqga20 | " CANCEL " ENDGADGET | menu4canc |
| 21,22 | optreqga21/22 | image up/down arrows (GADGIMAGE) | maxat -/+, makemax, refresh ga7(2), ga21(2), ga25(2). DISABLED bit toggled |
| 23,24 | optreqga23/24 | image arrows | resat -/+68, newres |
| 25,26 | optreqga25/26 | text-only labels optreqla108/109 (14 chars) | "Screens" etc. Toggled DISABLED with ga7/ga8 |
| 27,28 | optreqga27/28 | string (64 chars), StringInfo optreqsgz/sgy, Buffer = resat / resat+68 | resident file name. Click -> reschg=-1 -> loadres on OK |

**optreq2** (options2.src.new, 240x132, "Runtime Error Options", used by runerropts at 4577). Chain: ga1..ga4 -> **ga8** (ga5..7 hidden, still saved):
| id | gadget | meaning |
|---|---|---|
| 1 | optreq2ga1 | **AUTO RUN** (+13 bit7, 11163) |
| 2 | optreq2ga2 | INTERRUPT CHECKING (no direct btst found) |
| 3 | optreq2ga3 | **ASSEMBLER CHECKING** (3112) |
| 4 | optreq2ga4 | **OVERFLOW CHECKING** (8576) |
| 5..7 | optreq2ga5..7 (hidden) | old layout. ga6 = old interrupt, ga7 = old overflow (migration) |
| 8 | optreq2ga8 " OK " ENDGADGET | keep |
| 9 | optreq2ga9 " CANCEL " ENDGADGET | runerropts restores the 7 Flags words |

**calcreq** (calcreq, 376x84, "Calculator"):
| id | gadget | use |
|---|---|---|
| 1 | calcreqga1 | string "EXPRESSION:", buffer calcreqla15[128], StringInfo calcreqsg1, RELVERIFY |
| 2 | calcreqga2 " DEC " | toggle, initially SELECTED. Tested `cmp #128,+12` |
| 3 | calcreqga3 " BIN " | toggle. Tested `cmp #128,+12` (else hex) |
| 4 | calcreqga4 " HEX " | toggle |
| 5 | calcreqga5 " BYE BYE " ENDGADGET | exit |
Radio stride = `calcreqga3-calcreqga2` (gadget struct + nothing between, i.e. 44 bytes; computed, keep it as an array in C).

**argreq** (blitz2.s 15895, 408x56, "CLI Arguement:"): id1 argreqga1 string (buffer **argreqla8[128]**, the CLI arg used at 1715 and saved in .xtra, MaxChars 128, ENDGADGET), id2 " OK ", id3 " CANCEL " (both ENDGADGET).

Borders (`xxxbo*`) and the coordinate lists (`xxxla*` dc.w lines) are just bevel rendering. Port them as static const data, or replace them with generated bevel boxes.

## D. Machine-code templates

None in this range. (The asm mnemonic table is data for the inline assembler, not a code template.)

asmmac token/table format (built by `tokasm include asmmac`, tnum starts at fnum=60):
- Macro `a` = one token-list node, linked into the main `tokens` list (tok59 -> tokasm ... `c` -> toktnum):
  ```
  +0 next.l (n\@ = next node)   +4 tnum.w (60..177)   +6 name,0,0 (name\0 + empty help\0), even
  then the "asm block" (asmpnts[i] points here):
  +0 opcode.w  +2 srcEAmask.w  +4 dstEAmask.w
  +6 byte (srcRegBitPos<<4 | srcModeBitPos)   +7 byte (dstRegBitPos<<4 | dstModeBitPos)   (15 = none)
  +8 word (sizeFieldBitPos<<12 | sizecodes: nibble0=.b, nibble1=.w, nibble2=.l, value 15 = size illegal)
  +10 checker.l (0 or a routine above)   +14 special.w
  ```
  Node size = 28 ($1c) for names of 4 chars or less, and 30/32 for MOVEP/MOVEA/MOVEQ/MOVEM/RESET/TRAPV/ILLEGAL, and 26 for OR/ST/SF. The operator table's `(opX-opabcd)/$1c` only gives the right index because of integer truncation. **In C use explicit enum indices.**
- Macro `c` = final node 'EndAsm' (tnum 178), next = toktnum. No asm block (asmpnts[118] points into the next node: garbage, never used for assembly).
- 118 mnemonics in order: ABCD ADD ADDI ADDA ADDQ ADDX AND ANDI ASL ASR Bcc(14: CC CS EQ GE GT HI LE LS LT MI NE PL VC VS) BCHG BCLR BSET BTST BRA BSR CHK CLR CMP CMPI CMPA CMPM DBcc(14) DBF DBRA DBT DIVS DIVU EOR EORI EXG EXT ILLEGAL JMP JSR LEA LINK LSL LSR MOVE MOVEP MOVEA MOVEQ MOVEM MULS MULU NBCD NEG NEGX NOP NOT OR ORI PEA RESET ROL ROR ROXL ROXR RTE RTR RTS SBCD Scc(14) ST SF STOP SUB SUBI SUBA SUBQ SUBX SWAP TAS TRAP TRAPV TST UNLK.
- EA mode numbers (bit n in the masks): 0 Dn, 1 An, 2 (An), 3 (An)+, 4 -(An), 5 d(An), 6 d(An,Xi), 7 abs.w, 8 abs.l, 9 d(PC), 10 d(PC,Xi), 11 #imm, 12 CCR, 13 SR, 14 USP, 15 register list (mem). Masks: all=$fff, $1fd = data-alterable, $7e4 = control, etc. (equates in asmmac).
- special.w: negative ($800x) = auto-select a variant: bit0 -> if dst is An use entry idx+2 (ADD->ADDA, SUB->SUBA, CMP->CMPA, MOVE->MOVEA); bit1 -> if src is #imm use idx+1 (ADDI/SUBI/CMPI/EORI/ORI). Positive: 1 quick/shift (imm goes into the opcode), 2 Bcc/BRA/BSR (src d(PC) -> mode 9), 3 MOVEQ, 4 TRAP, 5 DBcc.
- **makeasmtab** (4609, chunk elsewhere): walks `tokens` lnum-1 nodes to reach tokasm. Then for 119 (tnum-fnum) nodes it skips name\0 and help\0, aligns to even, and stores the pointer in **asmpnts[119]** (.l array at 837). asmit (5150): index = token-$8000-fnum, a0=asmpnts[idx].

## E. Heap / linked structures seen

- Type node (firsttype/alltypes list, built by donewtype 13719): {+0 next.l, +4 fields.l (head of the field list; <256 = primitive kind; $ff = forward-declared pointer type), +8 size.w (or line number for forward ptr), +10 nlen.b, +11 name\0}.
- Field node (type+4 list): {+0 next.l, +4 offset.w, +6 flags.w (bit0 '*' pointer, bit2 '@' (with bit0), bit1 array), +8 arraycount.w, +10 type.l (-> type node), +14 nlen.b, +15 name\0}.
- Lib struct (firstlib list): +0 next.l, +4 libnum.w, +18 offset.l (lib-relative) to the end of the max-info block. Relative to `p = lib + [lib+18]`: p-2 = has-max flag word (nonzero = lib defines an object maximum), p-4 = max value word, p-26 = .l lib-relative offset to the object's token name. findlibnoerr (6253): d1=libnum -> a1=lib, Z=1 if **not** found.
- resbuff[8] x 68: {name[64], seglist.l @64}. freeres2 unloads them, loadres loads them.

## F. External calls

- DOS (a6=dos from the editor): Read/Write (`read(a6)`, `write(a6)`) in loadxtra/savextra. UnLoadSeg via freeres2.
- Intuition: DisplayBeep(screen=comdata[40]) in menu6. Everything else goes through the comdata vectors (openreq, getinput, getinput2, refgads, actgad, tokone) listed above. Gadget state is changed **directly in the static Gadget/PropInfo/StringInfo structs** (Flags SELECTED/DISABLED, Buffer, BufferPos, DispPos, VertPot/Body, LongInt) while the requester is open, then refreshed with RefreshGList.
- Internal compiler calls: makelong, findtype/findlab, findlibnoerr, evalconst2, setcvars, setsvars, makeiallox, loadres, freeres2, runerropts, menu0, menu1, err (via errstack/errcont).

## G. Special handling for C

- **loadxtra/savextra are called by the editor** through edstruct (+8/+12) with d7=fh and a6=DOSBase in registers. They need asm-callable wrappers (or `__asm("d7")` register params).
- The menu entry points (menu4, menu6, menu7, menu9) are reached through `menuadds` from the editor menu dispatch. This uses the same calling convention as the other menus.
- The comdata callbacks must be called with register args (a0, d0, a2) and results in d7/d6 (getinput) or d0 (actgad). Needs a register-call shim.
- Error recovery: menu7 and loadxtra set `errstack=a7` and `errcont=label`, and `err` does `move.l errstack,a7 ; jmp (errcont)`. In C this is setjmp/longjmp (menu7 -> .calcerr resumes the loop; loadxtra -> .done2). ezerr=-1 makes err quiet in loadxtra and is **not reset** afterwards.
- Tail jumps across routines: optsdone -> menu0/menu1; reflist/newres/maxit tail through refgads/makelong. typeshow falls into reflist, showlist falls into showlist2, menu4 falls into menu4c.
- Gadgets are linked by `next` pointers that skip the hidden gadgets (optreq ga10->ga15, optreq2 ga4->ga8). Hidden gadget structs must still exist, because their Flags are saved and migrated.
- Byte/word aliasing: `vers` is a word stored and loaded as a byte (high byte). Options are tested as `+13 bit7` and saved as a `+12` word. In C use a UWORD Flags with `& GFLG_SELECTED`, and use big-endian byte order for .xtra compatibility (the file layout is the in-memory big-endian image, which is fine on m68k).
- namebuff/namebuff2/+8 must be one 4-aligned 264-byte block (it doubles as a FileInfoBlock).
- The alltypes/firsttype aliasing trick (firsttype label = stringtype node) needs an explicit pointer in C.
- operators entries that use `(opX-opabcd)/$1c` must be replaced by explicit token constants (AND=$8000+66, OR=$8000+138, EOR=$8000+115, LSL=$8000+124, ASL=$8000+68, LSR=$8000+125, ASR=$8000+69; NOT=$8000+137 at line 9728).
- asmmac checker routines use a multi-register in/out convention (d1-d6 modified and used, flags and d0 as the result, some paths leave d0 unchanged with Z=1). In C: `int chk(struct asmregs *r)`, returning 0 ok / 1 swap / -1 ea / -2 size.
- menu6's prop drag is a busy-poll loop on getinput2. Keep it as it is.
