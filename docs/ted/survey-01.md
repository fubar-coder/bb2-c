# Survey 01 – ted.s Zeilen 1–1015 (+ wbheader, aboutreq, specialreq, incbins)

Bereich: Equates/rs-Offsets, Init-Einsprung `outside` (blitz2 und Standalone), `edstart`
(= wbheader), Fail-Kette, `mainedit` (Hauptschleife, Sprungtabelle), Mauszeiger/Sleep-Zeiger
mit VBlank-Server, Menüs an/aus, `blocktab`/`blockuntab`, `defaults`, `prtfile`,
`.xtra`/`.info`, `fileopen`/`filesave`/`realsave`, `doabout`, Suchen/Ersetzen.

Adressen (Hunk-Offset, aus `ted.labels`): `outside` $0004, `edstart`=`wb_header` $0204,
`mainedit` $0428, `mouseinit` $04AA, `pointeron` $0504, `sleepon` $0534, `myroutine` $05FC,
`myint` $0632, `sleep` $0656, `pointer` $0916, `dumsleep` $0956, `finish` $09AE,
`fileopen` $0E72, `fileopen2` $0E7A, `filesave` $0F7A, `realsave` $0FB4, `srcinfo` $1074,
`aboutreq` $1200, `specialreq` $150C, `doabout` $194A, `find` $198E, `replaceall` $1A48,
`next` $1AA0, `dosearch` $1AAC, `replace2` $1B2C, `comdata` $5DC2, `rawed` $73E0, `flink` $8CBE.

Programm = EIN Code-Hunk; Daten (Variablen, Intuition-Strukturen, Sprite-Daten, Icon)
liegen mitten im Code und werden beschrieben → in C alles beschreibbare Daten.

---------------------------------------------------------------------------------------
## Equates / rs-Offsets (ted.s:2–20)

| Name | Wert | Bedeutung |
|---|---|---|
| `ngerms` | 6 | 2 Verwendungen außerhalb des Bereichs |
| `cripple` | 0 | `ifeq cripple` um `filesave` (746) und bei 1678: 0 = volle Version |
| `doslen` | 2048 | Größe des DOS-Lesepuffers `dosbuff` |
| `nextitem` | 0 (.l) | nächste Zeile |
| `lastitem` | 4 (.l) | vorige Zeile |
| `numchars` | 8 (.b) | Länge inkl. abschließender 0 (s. `realsave`: schreibt `numchars` Bytes, 0 bzw. LF an `numchars-1`) |
| `chars` | 9 | Zeichen (tokenisiert oder ASCII), 0-terminiert |
| `lenitem` | 9 | Kopfgröße; Allokation = `lenitem + d5` (d5 = Länge inkl. 0), MEMF_PUBLIC |
| `sleepwait` | 1 (279) | Animationsteiler Sleep-Zeiger (Frame alle 2 VBlanks) |

`numchars` ist ein Byte → Zeile max. 255 Bytes; Allokationsgröße nicht gespeichert, wird aus
`numchars` rekonstruiert (`freeall`, außerhalb: `moveq #lenitem,d0 / add.b numchars` → Byte-
Überlauf ab numchars ≥ 247, siehe I).

---------------------------------------------------------------------------------------
## A. Routinen

### Start / Init / Ende

- Zeile 22 `bsr edstart` : erster Befehl des Hunks (Offset 0). Nur Standalone (DOS springt an
  Offset 0). blitz2 springt direkt auf `outside` = Hunk+4 (`jsr 8(BADDR(segadd))`).
- `edstart` (136) = `include wbheader` → `wb_header` (wb:4). Ein (Standalone): CLI d0/a0 =
  Argumente; WB: keine. Setzt `us`, `wbmess`, `dos` (via `dosopen`), `progfile` (Datei aus
  CLI-Arg bzw. WB-Arg[1]; vorher missbraucht als Puffer für `<tool>.info`), d2 = Stack (in TED
  unbenutzt). Endet in `wb_allgo` (wb:112) → Durchfall in Zeile 137 `sub.l a1,a1 / rts` →
  zurück nach `outside` mit **a1 = 0** (= „nicht extern“).
  Stack: wegen `bsr edstart` liest `fromcli` mit `4(a7)` die DOS-Rücksprungadresse statt der
  Stackgröße (d2 egal). `dosopen`-Fehler → `bra wb_exit` → dessen `rts` kehrt zu Zeile 24
  (`outside`) zurück statt das Programm zu beenden (s. I).
  - `fromcli` (wb:73): kopiert Argument bis ' ' oder LF nach `progfile` (keine Anführungszeichen-
    Behandlung hier; `a0toname` entfernt sie später).
  - `wb_exit` (wb:88): ReplyMsg(wbmess) falls ≠0 (Forbid/Permit), `rts`. Aus: d0 = wbmess,
    **Z=1 wenn wbmess=0** (wird vom externen Aufrufer als Erfolg gelesen!).
  - `dosopen` (wb:97): OldOpenLibrary("dos.library") → `dos`; aus d0, Z=1 bei Fehler; a6=Exec.
- `outside` (24) : **Init-Einsprung**, mehrere Einsprünge (Standalone per Durchfall nach
  `edstart`, extern per `jsr` von blitz2). Ein: a1 = `&edstruct` (blitz2) oder 0.
  `returncode`=-1; `external`=a1; falls a1≠0: kopiert 14 Wörter (28 Bytes) nach `tokens`…
  `remark` (ted.s:6269ff.: tokens, menitems, loadxtra, savextra, menurout, cleanup, colon,
  quote, label, remark) und `dosopen` (Fehler → `wb_exit`). Dann `opengr` (→`fail2`),
  `openint` (→`fail3`; IntuitionBase auch nach `extint` = comdata+44), `bsr rawed+32`
  (console-Init im incbin), AllocMem(doslen+3*chcols+6, MEMF_PUBLIC) → `dosbuff` (→`fail5`),
  verteilt `charbuff`=dosbuff+doslen, `charbuff2/3/4` je +chcols+2. `loaddefs`, `clr defalt`,
  `screenopen` (N=1 → `openfailed`), `donew` (Z=1 → `allocfailed`), `printmem`, `mouseinit`,
  `pointeron`, `jsr flink` (reloziert filereq). Extern: `getstuff` (Menüs/Tokenize-Flag aus
  edstruct), **aus: a0 = `.tomain`, a1 = `comdata`, comdata+84 := `&defaultreqga9+13`,
  d0=0 → Z=1**, `rts` zu blitz2 (Bildschirm bleibt offen). Standalone: `.edmain`: a0=`progfile`,
  Durchfall in `.tomain`.
- `outside.tomain` (87) : **zweiter Einsprung von blitz2** (`jsr (a1)` mit a0 = Dateiname,
  blitz2s `progfile`). `a0toname` (→ `filepath`/`filename`); wenn `filename`≠"": baut
  `filepath` + ('/' falls letztes Zeichen ≠ ':') + `filename` in `charbuff`, d7 = charbuff,
  `fileopen2`, `showvprop`, `showmice`, `printmem`. `.domain`: `bsr mainedit` (kehrt bei
  Programmende zurück), `mousefinit`, `clr returncode`, falls `defalt`≠0 `savedefs`, Durchfall
  in die Fail-Kette. Ende über `wb_exit` → `rts` = Programmende (Standalone) bzw. Rückkehr
  zu blitz2.
  Trick: leerer `filepath` → `-2(a0)` liest das Byte VOR `filepath` = ':' aus `dc ' :'`
  (ted.s:6139) → kein '/' eingefügt. In C explizit behandeln.
- Fail-Kette (Durchfall, jedes Label ist auch `beq`-Ziel): `allocfailed` (116) `screenclose`
  → `openfailed` (118) FreeMem(dosbuff, gleiche Größe) → `fail5` (128) `bsr rawed+32+8`
  (console-Cleanup) + `closeint` → `fail3` (130) `closegr` → `fail2` (131) `closedos` →
  `fail1` (132) d0 = ext.l `returncode` → `bra wb_exit` (d0 dort überschrieben, s. I).
- `mainedit` (142) : Hauptschleife, kehrt nur über `escape` (1016, außerhalb) zurück. Je
  Durchlauf: `clr mousealt/memalt`, `getline` (aus: d7 = Tasten-/Menücode, `anyalt`), a4 =
  `thisitem`; wenn `anyalt`: `allalt`=anyalt, `charstobuff`, `makeline` (neue Zeile ersetzt
  a4), bei `tokenise` `showthisitem`. Dann **berechneter Sprung** über `jumptable` (6171):
  Einträge 6 Bytes `{dc.b code, dc.b vpropflag, dc.l routine}`, Ende = Code-Byte 0;
  Vergleich `cmp.b d0,d7`; nicht gefunden → `mainedit`. Gefunden: Flag-Byte per
  `move.b (a0)+,-(a7)` auf den Stack (2 Bytes!), `memstack` := a7, `jsr (a0)` mit a4 =
  thisitem, d7 = Code. Danach `memcont`.
- `memcont` (167) : Einsprung auch von `outmem` (3046: a7 := memstack, `bra memcont` =
  longjmp nach Speicherfehler). Wenn `memalt` `printmem`, wenn `mousealt` `showmice`, Flag
  vom Stack; ≠0 → `showvprop`. `bra mainedit`. `rts` in 179 unerreichbar.
- `funcs` (182) : leeres Label.

### Mauszeiger, Sleep-Zeiger, Menüs

- `mouseinit` (184) : AllocMem(finish-pointer = 152, MEMF_CHIP) → `pointerat` (kein
  Fehlertest!), `dumat` = pointerat+64; kopiert `pointer`+`dumsleep` (38 Longs) ins Chip-RAM;
  `sleepcurr`=0.
- `mousefinit` (200) : ClearPointer(mywindow), Tail-`jmp` FreeMem(pointerat,152). Entfernt
  den Interrupt-Server nicht (setzt voraus, dass zuvor `pointeron` lief).
- `pointeron` (209) : **sichert a0-a1/d0-d3/a6** (comdata+72, von blitz2 gerufen). Falls
  `sleepcurr`≠0: RemIntServer(INTB_VERTB=5, `myint`), `sleepcurr`=0. SetPointer(pointerat,
  Höhe 14) über `setpoint` (→ auch Menüs neu, s. u.).
- `sleepon` (223) : **sichert a0-a1/d0-d3/a6** (comdata+76). Schon aktiv → `bra makemenus`
  (nach movem, d. h. a6 bleibt erhalten, makemenus ändert nur a0/d0 – a0/d0 also NICHT
  gesichert auf diesem Pfad!). Sonst: `makedum(sleep)`, `sleepcurr`=sleep,
  `sleepdelay`=1, `setpoint`(dumat, Höhe 20), AddIntServer(5, myint).
- `setpoint` (242) : ein: a1 = Sprite-Daten (Chip), d0 = Höhe. SetPointer auf `req_window`
  (falls ≠0) und `mywindow`, Breite 16, XOffset −1, YOffset 0. a6 := `int`. Durchfall in
  `makemenus`.
- `makemenus` (258) : `req_window`≠0 oder `sleepcurr`≠0 → `menusoff`, sonst `menuson`.
  Auch von 3668 gerufen.
- `menuson` (263) / `menusoff` (271) : `bclr`/`bset #0,25(window)` auf mywindow und ggf.
  req_window = **Window.Flags Bit 16 = WFLG_RMBTRAP** direkt gepatcht (rechte Maustaste
  öffnet keine Menüs). Kein OS-Aufruf.
- `makedum` (281) : ein a0 = Sleep-Frame. Kopiert 20 Longs ab Frame+4 nach dumat+4 (Kontroll-
  long vorn und Endlong hinten bleiben 0 aus `dumsleep`).
- `myroutine` (291) : **VBlank-Interrupt-Code** (`is_Code` von `myint`). `sleepdelay`−1; <0 →
  =1, `sleepcurr`+=88, ≥ `sleepf` → `sleep`; `makedum`. Aus: **d0=0, Z=1** (moveq) – Server-
  Kette läuft weiter. Benutzt d0/a0/a1 (erlaubt). Absolute Adressen (`subq #1,sleepdelay`).

### Blockoperationen

- `blocktab` (327) : ein a4/d7 aus mainedit. `bboty`=−1 (kein Block) → `beepem`.
  `bloksetup2` (aus: a5 = erste Blockzeile, d6 = deren Nummer). Je Zeile: `charbuff2` =
  `tabstop` Leerzeichen + Zeile (detok wenn `tokenise`, sonst Kopie bis 0), Rest bis
  `charbuff3` mit Leerzeichen (kein 0-Terminator → „Bildschirmzeile“), `thisitem`=a4,
  `makefirst` (charstobuff(a2=charbuff2) + makeline; a4 := neue Zeile), weiter bis
  d6 > `bboty`. `calcitem`, Tail `bra showall`.
- `blockuntab` (361) : analog; nach dem Kopieren werden die ersten `tabstop` Zeichen
  **beliebigen Inhalts** entfernt (`tst.b (a0)+` nur auf 0). Tail `bra showall`.

### Optionen

- `defaults` (401) : Optionsrequester (`defaultreq`). Sichert `defstart..defend` (50 Bytes,
  wortweise) nach `charbuff` (Backup für Abbruch; Offsets dort: 0 fonthite, 6 tokenise,
  8 makeicons, 10 normcol …). Tokenize-Gadget `defaultreqga8` disabled (Flags+12 |= $100),
  wenn `tokens`=0. Bildschirm neu öffnen mit `not copok` drumherum, `openreq`, `swapbords`.
  Schleife `getinput` (d7=−1, d6=Gadget-ID):
  - d6 0…3: Farbregler (Prop): a5 = `palit` + `propcol`*6; Schleife `propadj` +
    `getinput2` bis d7≠0 (Live-SetRGB4).
  - d6 10…25: Farbwahl, 4 Gruppen à 4 (d6−10: Gruppe = ((d6−10)>>1)&6 → Wort in
    `normcol/labcol/tokcol/propcol`), Rahmen umschalten (`dispgadd0`, `swapd0bord`);
    Gruppe propcol (d6−10 ≥ 12) → `setprops`, `dispprops`.
  - d6 4…9: Integer-/Check-Gadgets: prüft `int1`<32, `int2`<10, `int3`<80 (unsigned,
    sonst 0 + Puffer `defaultreqla47..49` := "0" ($3000) + beep + SELECTED in ga4..ga6
    löschen + RefreshGList(ga4, req_window, defaultreq, 3)). Übernimmt sofort: Font
    (`defaultreqga7+13` Bit 7 → fonthite 6/fonthite2 5, sonst 8/7; `fontmul` =
    fonthite*bmapmul), `tokenise` ← ga8 SELECTED, `makeicons` ← ga9 SELECTED, `hscmarg`,
    `vscmarg`, `tabstop` ← Low-Word von int1..3.
  - d6 26 (OK): `color`=normcol, `defalt`=26 (→ savedefs beim Ende), `swapbords`; falls
    Fonthöhe geändert: `cursy` auf < `edrows` begrenzen (läuft `lastitem` zurück),
    `calcpos`; `screenclose`/`screenopen2`; wenn tokenise geändert Tail `detokall2` bzw.
    `tokall2`, sonst `rts`.
  - d6 ≥ 27 (Abbruch): `setdefs`(a5=charbuff-Backup), `colors`, `swapbords`,
    `screenclose`, Tail `bra screenopen2`.

### Drucken

- `prtfile` (570) : Requester `prtreq`, `actgad(prtreqga1)` (String-Gadget mit `prtname`
  „PRT:“). Gadget 2 = Drucken, 3 = Abbruch. Open(prtname, 1006); je Zeile (`numlines`, ab
  `firstitem`): Kopie nach `charbuff`, Bytes mit Bit 7 = Token (2 Bytes) → `ESC[1m` +
  Name (`findtoke`) + `ESC[22m`; Zeilenende 0 → LF; Write(d7, charbuff, len); Close. a6 =
  DOS über die ganze Schleife (findtoke ändert a6 nicht). Fehler bei Write ignoriert.

### .xtra / .info

- `openmore` (664) : ein a1 = Endung. Hängt sie an den String bei `xtraat` an, merkt das
  alte Ende in `xtranull`; aus: d1 = xtraat (Name für Open/Lock). a6 unverändert.
- `openxtra` (658) : ein d2 = Modus, **a6 = DOSBase (vom Aufrufer, aus `doclose`)**.
  Open(name+".xtra", d2); aus d7 = Handle, **Z=1 wenn fehlgeschlagen**.
- `openinfo` (641) : Lock(name+".info", ACCESS_READ=−2); existiert → UnLock, d7=0 (Z=1);
  sonst Open(…,1006) → d7, Z=1 bei Fehler. Icon wird also nie überschrieben.
- `closextra` (674) : `clr.b (xtranull)` – entfernt die Endung wieder.
- `xtraat`, `xtranull`, `xtratxt`, `infotxt`: Daten (636–639).

### Laden / Speichern

- `fileopen` (678) : `erase` (Sicherheitsabfrage; bei Abbruch `addq #4,a7` → zurück nach
  mainedit), `getfname` (Dateirequester; Abbruch → `addq #4,a7`, ebenfalls Abbruch des
  Aufrufers; d0=1 → `outmem`), d7 = Name. Durchfall in `fileopen2`.
- `fileopen2` (680) : ein d7 = Dateiname (Pfad). `xtraat`=d7; `doopen` (Fehler: `addq #4,a7`
  + `bra error` → kehrt zum Aufrufer von fileopen2 zurück); `clr allalt`; `doclean`
  (Rückruf `cleanup` aus edstruct, sichert d1-d7/a0-a6); `fillbuff` – 0 Bytes → `doclose`,
  Tail `bra donew`. Sonst `sleepon`, `blokdis`, `freeall`, `numlines`=1, erste Zeile
  `readline`+`allocline` → `firstitem`=`thisitem`=`enditem`, `lastitem`=0; Schleife
  `readline` (N=1 = Ende) / `allocline` / verketten / `numlines`++. `doclose`. Rückruf
  `loadxtra` (falls ≠0): a5 = Routine, d2=1005, `openxtra`; Erfolg → **`jsr (a5)` mit
  d7 = Handle, a6 = DOSBase**; danach Close(d7) über **a6 nach dem Rückruf** (Rückruf muss
  d7/a6 erhalten). `closextra`. Cursor/Scroll auf 0, `showhprop`, `showall`, `pointeron`,
  Durchfall in `showfname`.
- `showfname` (731) : schreibt "File: " + `filename` in `newsctit` (112-Byte-Puffer, ist
  der Screen-Titel), Durchfall in `showme`. Auch von 821, 2727.
- `showme` (737) : ShowTitle(myscreen, TRUE) als Tail-`jmp` (Titel-String wurde in-place
  geändert; ShowTitle erzwingt das Neuzeichnen). Auch von 1556, 1601.
- `filesave` (745) : `getfname2` (a2 = "Name to save file as"; Abbruch-Trick wie oben), d7 =
  Name, `xtraat`=d7. In `dosbuff`: Name+".bak"; DeleteFile(.bak); Rename(Name → Name.bak).
  Durchfall in `realsave`. (Mit `cripple`≠0 nur `rts`.)
- `realsave` (769) : nur Durchfall-Label (keine weiteren Aufrufer). `doopen` d2=1006,
  `sleepon` (erhält a6=DOS), für jede Zeile: ASCII-Modus (`tokenise`=0): 0 an `numchars-1`
  temporär durch LF ersetzt; schreibt `numchars` Bytes (tokenisiert: inkl. 0 als
  Zeilentrenner), stellt 0 wieder her. Write<0 → `error(error3)`, Abbruch, `allalt` bleibt
  gesetzt; sonst `clr allalt`. `doclose`. Rückruf `savextra` analog loadxtra (d2=1006,
  d7 = Handle, a6 = DOS). Icon: falls `defaultreqga9+13` Bit 7 (Gadget „Icons“ SELECTED –
  nicht die Variable `makeicons`!) `openinfo` → Write(`srcinfo`, 396 Bytes), Close,
  `closextra`. `pointeron`, Tail `bra showfname`. `rts` 825 nur bei cripple.

### Über-Requester

- `doabout` (834) : `openreq(aboutreq)`; Gadget 1 (OKAY) → `rts`; Gadget 2 (unsichtbarer
  Bereich mit Text „Thanks go to:“) → `openreq(specialreq)`, warten auf dessen Gadget 1
  (ENOUGH!), `rts`. Kein closereq (geschieht in getinput/openreq, außerhalb).

### Suchen / Ersetzen

Globals außerhalb: `findreqla16` = Suchstring-Puffer, `findreqla17` = Ersetzstring,
`findreqsg1/2` = StringInfo (Offset +8 BufferPos, +12 DispPos), `findreqga8+13` Bit 7 =
Groß/Klein beachten.

- `find` (853) : BufferPos/DispPos beider StringInfos := 0; Sicherung von la16 → `charbuff`,
  la17 → `charbuff2`; `openreq(findreq)`, `actgad(findreqga1)`. Gadget-IDs: 1 = Return im
  Such-Gadget → Ersetz-Gadget aktivieren; 2 (Return im Ersetz-Gadget) und 3 → `bra next`;
  4 → `bra previous`; 5 → `bra replace`; 6 → `bra replaceall`; 8 (Case-Schalter) →
  Such-Gadget wieder aktivieren; sonst (Abbruch) Strings aus charbuff/charbuff2
  zurückkopieren, `rts`. Alle Aktionen als Tail-Call.
- `next` (926) : a4 := nextitem(a4) (0 → `beepem`), d6=1, Durchfall in `dosearch`.
- `previous` (959) : a4 := lastitem(a4) (0 → beep), d6=−1, `bra dosearch`.
- `dosearch` (931) : ein d6 = Richtung ±1, a4 = Startzeile (bereits die Nachbarzeile).
  Leerer Suchstring → beep. `sleepon`; d7 = curstop+cursy+d6 (Zeilennummer). Schleife
  `isithere` (ab Spalte 0; aus d0 = Spalte, N=1 nicht gefunden). Nicht gefunden: d7+=d6,
  <0 oder ≥ numlines → `beepem` (macht pointeron). Gefunden: `pointeron`, `gotolined0`
  (d0 = Zeile, a4 = thisitem), `horizpos` (d1 = Spalte), Tail `bra showhprop`. Sucht nie
  in der aktuellen Zeile hinter dem Cursor.
- `replace` (965) : leerer Suchstring → beep; `replace2` auf a4; Z=0 (nichts ersetzt) →
  beep, sonst Tail `bra showall`. Ersetzt ALLE Vorkommen der aktuellen Zeile.
- `replaceall` (902) : leerer Suchstring → beep; Requester `repall` (1 = ab hier (a4),
  2 = ab Anfang (`firstitem`), 3 = Abbruch). `sleepon`; jede Zeile: `thisitem`=a4,
  `replace2`, nächste. `calcitem`, `showall`, Tail `bra pointeron`.
- `replace2` (971) : ein a4 = Zeile. Aus: **Z=1 (d0=0) wenn ≥1 Ersetzung**, sonst d0=−1/Z=0;
  a4 = ggf. neue Zeile (makeline), `thisitem` = a4. Ablauf: `isithere` (aus d0 = Position,
  a1 = Zeilentext (detok in `charbuff` oder `chars(a4)`)); `allalt`=−1; baut in `charbuff2`:
  Präfix (d0 Zeichen) + Ersetzstring + Rest nach len(Suchstring), Leerzeichen bis
  `charbuff3`; d1 = Position hinter der Ersetzung → Stack; `charstobuff`(a2=charbuff2),
  `makeline`; d5 := Position, `isithere2` ab d5 (verhindert Endlosschleife bei
  Ersetzstring ⊇ Suchstring), weiter solange gefunden.

---------------------------------------------------------------------------------------
## B. Registerkonventionen

- **Hauptschleife**: beim Aufruf eines Handlers aus `jumptable`: a4 = `thisitem`,
  d7 = Code (Byte; ASCII/Steuerzeichen, Menücodes `$IM`: High-Nibble = Item, Low-Nibble =
  Menü, z. B. $10 fileopen, $20 filesave, $30 defaults, $50 prtfile, $40 doabout (Menü 0);
  $03/$13/$23/$33/$43 find/next/previous/replace/replaceall; $91/$a1 blocktab/untab;
  $90 escape). Handler dürfen alle Register zerstören.
- Dateiroutinen (`doopen`/`fillbuff`/`doread`/`readline`/`doclose`): **d7 = Name vor
  doopen, danach Dateihandle**; a5 = Lesezeiger in `dosbuff`; d6 = Restbytes; d2 = Modus
  (1005/1006) für doopen/openxtra; d5 = Zeilenlänge inkl. 0 (readline → allocline);
  a6 = DOSBase nach `calldos`/`doclose` und wird von openxtra/openinfo stillschweigend
  vorausgesetzt.
- Suche: d6 = Richtung, d7 = Zeilennummer, a4 = Zeile, d5 = Startspalte (`isithere2`).
- `defaults`/Requester: `getinput` → d7 = −1 bei Gadget, d6 = GadgetID; `getinput2` → d7 = 0
  wenn nichts.
- a6: keine globale Konvention; `int(pc)`/`dos(pc)`/Exec (4) jeweils neu geladen.
- Von blitz2 über comdata gerufen und deshalb registererhaltend: `pointeron` (comdata+72),
  `sleepon` (+76) – movem a0-a1/d0-d3/a6 (d4-d7/a2-a5 werden ohnehin nicht berührt).
  Ausnahme: sleepon-Pfad „schon aktiv“ (`.done`) springt nach dem movem zu `makemenus`,
  das a0/d0 zerstört.
- Rückrufe in blitz2 (aus edstruct): `loadxtra`/`savextra`: ein d7 = Handle, a6 = DOSBase;
  müssen d7/a6 erhalten. `cleanup` (via `doclean`): TED sichert d1-d7/a0-a6.
- `outside` extern: ein a1 = edstruct; aus a0 = `.tomain`, a1 = `comdata`, d0 = 0, Z=1.
  `.tomain`: ein a0 = Dateiname (C-String, ggf. in "…").

---------------------------------------------------------------------------------------
## C. Globals in diesem Bereich

| Name (Zeile) | Typ | Init | Zweck |
|---|---|---|---|
| `int` (35, im Makro `openint`) | .l | 0 | IntuitionBase – **liegt im Code** hinter `bra int+4` |
| `intname` (35) | String | "intuition.library" | im Code |
| `gr` / `grname` (33, Makro `opengr`) | .l / String | 0 / "graphics.library" | GfxBase, im Code |
| `myint` (306) | struct Interrupt (22 B) | ln_Type=2 (NT_INTERRUPT), Pri 0, Name 0, is_Data 0, is_Code=`myroutine` | VBlank-Server |
| `pointerat` (311) | .l | 0 | Chip-Kopie von `pointer` |
| `dumat` (312) | .l | 0 | Chip-Puffer für aktuellen Sleep-Frame (= pointerat+64) |
| `sleepcurr` (313) | .l | 0 | aktueller Frame in `sleep`; ≠0 = Sleep aktiv (auch Menü-Sperre) |
| `sleepdelay` (314) | .w | 0 | VBlank-Zähler |
| `sleep` (316) | incbin sleep.bin, 704 B | – | 8 Frames à 88 B (Sprite: Ctrl-Long, 20 Zeilen×2 Wörter, End-Long) |
| `pointer` (319) | incbin pointer.bin, 64 B | – | normaler Zeiger, Höhe 14 |
| `dumsleep` (322) | ds.b 88 | 0 | Vorlage für Chip-Sleep-Frame (wird mitkopiert) |
| `finish` (325) | Label | – | Ende des Chip-Blocks (pointer..finish = 152 B) |
| `xtraat` (636) | .l | 0 | Zeiger auf aktuellen Dateinamen (für .xtra/.info) |
| `xtranull` (637) | .l | 0 | Position der angehängten Endung |
| `xtratxt`/`infotxt` (638/639) | String | ".xtra"/".info" | je 6 Bytes, Code danach gerade |
| `bakname` (742) | String | ".bak" | |
| `srcinfo`..`srcinfof` (827) | incbin src.info, 396 B | – | Icon für Quelltexte, direkt aus dem Code-Hunk geschrieben |
| wbheader: `wb_infoname`, `wb_dosname`, `dos` (.l), `us` (.l, eigener Task), `wbmess` (.l), `progfile` (192 B) | | | Startcode-Variablen; `dos` ist DOSBase für ganz TED |

Intuition-Strukturen:
- `aboutreq` (aboutreq:1): struct Requester (Left 108, Top 2, 312×164), ReqGadget →
  `aboutreqga1` → `aboutreqga2` (verkettet über NextGadget), ReqBorder → 3 Border-Ketten
  (bo1→la2→bo2→la5; bo3 = Rahmen des OK-Gadgets), ReqText → IntuiText-Kette in1…in9
  (Titel „BLITZ BASIC 2“, „Release 1.0“, Copyright, Namen). Gadgets: BOOL, Activation 5
  (RELVERIFY|ENDGADGET); ga1 ID 1 „ OKAY! “ (128,148,64×12), ga2 ID 2 ohne Rahmen, Text
  „Thanks go to:“ (108,86,104×8). Keine Laufzeitänderung.
- `specialreq` (specialreq:1): Requester (4,2, 512×172), ein Gadget `specialreqga1` ID 1
  „ ENOUGH! “ (208,156,80×12), 3 Border-Ketten, 16 IntuiTexts. Keine Laufzeitänderung.
- Requester dienen nur als Vorlage für `openreq` (baut daraus ein Fenster, außerhalb).

Zur Laufzeit geänderte Felder fremder Strukturen (definiert außerhalb):
- `defaultreqga8+12` (Gadget.Flags.w): |= $100 GFLG_DISABLED / &= ~$100; Bit $80 SELECTED
  (tokenise) – wortweise. `defaultreqga7+13`, `defaultreqga9+13` (Low-Byte von Flags) Bit 7 =
  SELECTED (6-Pixel-Font, Icons) – byteweise. `defaultreqga4..6+12`: SELECTED gelöscht.
- `defaultreqla47..49`: String-Puffer := "0".
- `findreqsg1/2 +8/+12`: StringInfo.BufferPos / DispPos := 0.
- `comdata+84` (Long, Init 0): := `&defaultreqga9+13` (blitz2 liest darüber „Icons erzeugen“).
- `newsctit`: Screen-Titelpuffer, in-place überschrieben.
- Window.Flags (+24, Byte +25 Bit 0) von `mywindow`/`req_window`.

---------------------------------------------------------------------------------------
## D. Selbstmodifizierender Code / Daten im Code

- Makros `opengr`/`openint` (33/35) legen `gr`/`int` + Namen in den Codefluss (`bra x+4`
  überspringt). Kein Problem in C (normale Globals).
- `bsr rawed+32` (39) / `bsr rawed+32+8` (128): Einsprünge in den incbin-Hunk `rawed.obj`:
  +32 = erster Code-Befehl nach Hunk-Header (HUNK_HEADER…HUNK_CODE+Länge = 8 Longs).
  Sprungtabelle: +0 Init (console.device öffnen), +4 Rawkey→ASCII (bei 5928), +8 Cleanup.
  **rawed wird NICHT reloziert** → muss positionsunabhängig sein.
- `jsr flink` (76): reloziert den incbin `filerequest` (7953): liest Code-Länge bei +28,
  addiert die Basis (Hunk+32) zu jedem Offset der ersten HUNK_RELOC32-Gruppe. Nur EINE Gruppe,
  Anzahl 0 würde 2^32 Iterationen laufen. Wird bei jedem `outside` ausgeführt (nur einmal je
  Ladevorgang – Doppel-Relozierung wäre fatal). Kein CacheClearU danach (68040-Copyback!).
- `filerequest+36` (`ezfreq`, außerhalb) = Einsprung in den relozierten Code.
- Kein Patchen von Befehlen in diesem Bereich; Sprite-Daten werden im Interrupt in den
  Chip-Puffer kopiert (Intuition zeigt direkt auf `dumat`, d. h. die Animation ändert
  laufend die von SetPointer referenzierten Daten).

---------------------------------------------------------------------------------------
## E. Heap-/Listenstrukturen

| Struktur | Allokation | Felder | Routinen |
|---|---|---|---|
| Zeile (`item`) | `lenitem`+d5 (d5 = Länge inkl. 0), MEMF_PUBLIC via `alloc2` (Fehler → `outmem`) | +0 nextitem.l, +4 lastitem.l, +8 numchars.b, +9 chars[] 0-term. | allocline, makeline, freeall; Liste: `firstitem`, `enditem`, `thisitem`, `numlines` |
| Arbeitspuffer | AllocMem(doslen+3*chcols+6 = 2438 bei chcols=128, PUBLIC) | +0 `dosbuff` (2048), `charbuff` (chcols+2), `charbuff2` (chcols+2), `charbuff3` (chcols+2), `charbuff4` = **Blockende (0 Bytes)** | Init in outside, frei in `openfailed`; charbuff3 dient als Endmarke für charbuff2 |
| Chip-Zeigerblock | 152 B MEMF_CHIP | +0 pointer (64), +64 Sleep-Frame (88) | mouseinit/mousefinit/makedum |
| `.xtra`-Name | kein Heap: Endung wird in den vom Aufrufer gelieferten Namenspuffer (`xtraat` = d7) angehängt und von `closextra` wieder entfernt | | openmore/closextra |
| `.bak`-Name | in `dosbuff` | | filesave |

Zeilenformat in Datei: ASCII-Modus Zeilen mit LF; tokenisiert: Rohbytes je Zeile inkl. 0.
Laden (`readline`, außerhalb) erkennt LF → ASCII (+ Tabs zu `tabstop` expandiert),
Token-Bytes ≥ $80 sind 2 Byte lang.

---------------------------------------------------------------------------------------
## F. Externe Aufrufe

- exec: OldOpenLibrary (dos/graphics/intuition), CloseLibrary, AllocMem (dosbuff PUBLIC;
  Zeigerblock CHIP), FreeMem, AddIntServer/RemIntServer (INTB_VERTB=5), Forbid/Permit/
  ReplyMsg/WaitPort/GetMsg (wbheader). `move.l 4,a6` absolut.
- dos: Open (1005/1006), Close, Read (wbheader, loaddefs/fillbuff), Write (prtfile, realsave,
  .info), DeleteFile, Rename (.bak), Lock/UnLock (.info-Test), CurrentDir (wbheader).
- intuition: SetPointer (mywindow, req_window), ClearPointer, ShowTitle, RefreshGList
  (`callint`); über Hilfsroutinen openreq/getinput/actgad/screenopen.
- graphics: nur über Hilfsroutinen (propadj → SetRGB4).
- Interrupt: `myint`/`myroutine` VBlank-Server (läuft nur zwischen sleepon und pointeron).
- Chip-Mem: Zeiger-Sprite-Daten.
- Direkte Struktur-Pokes: Window.Flags RMBTRAP.
- blitz2-Rückrufe: `loadxtra` (fileopen2), `savextra` (realsave), `cleanup` (doclean;
  in blitz2 = 0). `menurout`/`menitems` nur über `getstuff` (außerhalb).
- Kein `$dff000`-Zugriff, kein Blitter in diesem Bereich.

---------------------------------------------------------------------------------------
## G. Sonderbehandlung in C

- **Init-Einsprung `outside`** (Hunk+4, kein Startup-Code davor!): blitz2 erwartet
  aus a0 = `.tomain`, a1 = `comdata`, Z-Flag. Glue-Stub in Asm: Offset 0 `bsr edstart` (4 Bytes),
  Offset 4 Einsprung; `.tomain` als zweiter Stub (a0 = Name). Standalone-Start über libnix-
  Startup statt wbheader möglich, aber dann muss das Hunk-Layout (blitz2: `jsr 8(seg)`)
  trotzdem stimmen → eigener Entry-Stub als erstes Objekt.
- **comdata-Einträge** sind Asm-Konvention: pointeron/sleepon müssen alle Register erhalten
  → Glue-Wrapper mit movem.
- **Rückrufe loadxtra/savextra/cleanup**: TED → blitz2 mit d7/a6 → Glue-Stub
  (`call_xtra(fn, fh, DOSBase)`), cleanup mit movem-Sicherung.
- **VBlank-Server `myroutine`**: is_Code-Konvention (a1 = is_Data, a5 = Code), Rückgabe Z=1;
  gcc garantiert Z nicht → Asm-Stub `moveq #0,d0; rts` um C-Funktion, oder ganz in Asm.
  Gemeinsame Variablen (`sleepcurr`, `sleepdelay`) `volatile`; Zugriff über A4 (baserel)
  im Interrupt nicht verfügbar → Stub muss a4 laden (oder Variablen absolut).
- **`jumptable`**: Tabelle {code, flag, fnptr} → C-Array von Structs; `mainedit` Flag vom
  Stack → lokale Variable.
- **`memstack`/`outmem`**: longjmp zurück nach `memcont` → `setjmp` in mainedit; outmem kann
  aus blitz2 heraus (comdata `extalloc`) kommen → Sprung über fremde Frames (Asm-Glue mit
  gespeichertem a7, oder setjmp-Puffer + Glue, der a7 setzt). Vor der ersten Iteration
  ist memstack = 0 (s. I).
- **Stack-Tricks `addq #4,a7`** in getfname3/doopen/erase: bewirken Abbruch des *Aufrufers*
  (fileopen, fileopen2, filesave, realsave). In C: Rückgabewert prüfen und return.
  Achtung realsave: doopen-Fehler bricht nach dem Rename ab (Datei liegt dann nur als .bak).
- **Condition Codes als Rückgabe**: outside (Z), screenopen (N), donew (Z), openxtra/openinfo
  (Z), fillbuff (Z über d6), readline (N), isithere (N, d0), replace2 (Z), wb_exit/dosopen (Z).
- **Byte/Wort-Aliasing**: Gadget.Flags wortweise (`+12`) und byteweise (`+13`, Bit 7 =
  $0080) – in C einheitlich `Flags & GFLG_SELECTED`; comdata+84 muss aber weiterhin auf das
  Low-Byte (big-endian +13) zeigen. `int1+2` = Low-Word eines Longs. `move #$3000,la47` =
  String "0". `move.b (a0)+,-(a7)` = 2 Bytes Stack.
- **Layout-Abhängigkeit**: `dc ' :'` vor `filepath` (Leerer-Pfad-Trick in `.tomain`).
  `defstart..defend` wird als Block gelesen/geschrieben (`l:blitzeditor.opts`, 50 Bytes)
  und mit festen Offsets (0, 6, 8, 10) aus dem Backup gelesen → als gepackte Struktur
  big-endian halten.
- `newsctit` wird als Screen-Titel in-place geändert (ShowTitle als Refresh).
- Datenstrukturen mit Code-Zeigern: `myint.is_Code`, `jumptable`, `comdata`.

---------------------------------------------------------------------------------------
## H. Einstufung

| Routine | Klasse | Bemerkung / Schnitt |
|---|---|---|
| `edstart`/wbheader | GLUE | durch Startup ersetzen; Argument-Parsing (`fromcli`) als KERN-Hilfsfunktion möglich |
| `outside`, `.tomain`, Fail-Kette | GLUE+UI | Entry-Stub Asm; Rest C (Init/Teardown) |
| `mainedit`, `memcont` | UI | Dispatch-Kern (Tabelle) host-testbar, aber getline/show* UI |
| `mouseinit`, `mousefinit`, `pointeron`, `sleepon`, `setpoint`, `makedum` | UI | pointeron/sleepon mit Glue-Wrapper für comdata |
| `makemenus`, `menuson`, `menusoff` | UI | |
| `myroutine`, `myint` | GLUE | Interrupt |
| `blocktab`, `blockuntab` | KERN | Zeilenumbau per detok/makefirst; `showall` am Ende → Hook `redraw_all()`; `bloksetup2`/`calcitem` sind Logik |
| `defaults` | UI | Werteprüfung (int1..3) und setdefs-Backup trennbar |
| `prtfile` | UI + KERN | Formatierung „Zeile → Text mit ESC[1m/ESC[22m“ als KERN-Funktion herauslösen; Requester/Open UI |
| `openmore`, `closextra`, `openxtra`, `openinfo` | KERN | DOS Open/Lock abstrahierbar |
| `fileopen` | UI | erase/getfname |
| `fileopen2` | KERN + Hooks | Ladeschleife (doopen/readline/allocline) KERN; Hooks: `sleepon` vorher, `pointeron`/`showall`/`showhprop`/`showfname` nachher, Rückrufe loadxtra/cleanup (GLUE) |
| `filesave` | UI + KERN | Requester UI; .bak-Rename KERN |
| `realsave` | KERN + Hooks | Schreibschleife KERN; error()/sleepon/pointeron/showfname Hooks; savextra GLUE; .info schreiben KERN (srcinfo als Byte-Array) |
| `showfname`, `showme` | UI | |
| `doabout` | UI | |
| `find`, `replaceall` | UI | Requesterlogik; Schleife über Zeilen in replaceall KERN-fähig |
| `next`, `previous`, `dosearch` | KERN + UI | Suche (Zeilenzähler, isithere) KERN; Ende `gotolined0`/`horizpos`/`showhprop`/`beepem` als Hook „cursor_to(line,col)“ bzw. „beep()“ |
| `replace` | KERN + UI | replace2 KERN, `showall`/`beepem` Hook |
| `replace2` | KERN | nutzt charstobuff/makeline (KERN, falls makeline nicht zeichnet) |

---------------------------------------------------------------------------------------
## I. Auffälligkeiten / mögliche Bugs des Originals

1. **Fehlschlag von `outside` bei externem Aufruf meldet Erfolg** (24–34, 132–134): Fail-Kette
   endet in `wb_exit`; dort `move.l wbmess,d0` (=0, da wbheader nicht lief) → Z=1 → blitz2
   hält Init für erfolgreich und benutzt a0/a1 (Müll). Außerdem wird `returncode` nie als
   Exitcode geliefert (d0 überschrieben). BUG(orig).
2. Standalone, `dosopen` in wbheader schlägt fehl: `wb_exit`-`rts` kehrt zu `outside` zurück
   (wegen `bsr edstart`), a1 ≠ 0 möglich → Init läuft weiter, ggf. doppeltes ReplyMsg.
   Theoretisch (dos.library fehlt nie). BUG(orig).
3. `fromcli` liest „Stackgröße“ aus 4(a7) = DOS-Rücksprungadresse (wegen bsr). Folgenlos (d2
   ungenutzt).
4. wbheader: CurrentDir wird nie zurückgesetzt, .info-Handle bei kurzem Read nicht
   geschlossen (wie in blitz2-Survey).
5. `mouseinit`: AllocMem(CHIP) ohne Fehlertest → Kopie nach Adresse 0. BUG(orig).
6. `mousefinit` entfernt einen evtl. noch aktiven VBlank-Server nicht (Server zeigt dann auf
   freigegebenen Code/Chip-Speicher, falls TED während Sleep endet).
7. `sleepon`, Pfad „schon aktiv“: a0/d0 werden durch makemenus nach dem movem-Restore
   zerstört – comdata+76 erhält dann nicht alle Register.
8. `outmem` vor dem ersten mainedit-Dispatch (z. B. Speichermangel beim Laden der
   Startdatei in `.tomain` → `fileopen2`): `memstack` = 0 → a7 := 0. BUG(orig).
9. `charbuff4` zeigt genau auf das Ende des Puffers (0 Bytes). Unbenutzt, aber Falle.
10. Pufferüberläufe ohne Prüfung (in charbuff3 bzw. hinter das Allokationsende):
    `replace2` (lange Ersetzungen), `blocktab` (tabstop + Zeile > chcols+2),
    `prtfile` (expandierte Tokens + 8 Bytes ESC je Token in `charbuff`, nur 130 Bytes, nur
    `charbuff2/3` dahinter), `.tomain` (filepath bis 191 + filename bis 63 in charbuff).
    Bei blocktab/replace2 schneidet `charstobuff` zusätzlich still auf chcols ab
    (Datenverlust am Zeilenende).
11. `blockuntab` entfernt die ersten `tabstop` Zeichen auch wenn sie keine Leerzeichen sind;
    bei `tabstop`=0 wird jede Blockzeile komplett durch Leerzeichen ersetzt (a1 steht noch
    am Pufferanfang, `beq .loop3` füllt ab dort). BUG(orig).
12. `defaults` erlaubt `tabstop`=0 (nur Obergrenze geprüft) → `readline` (`divu tabstop`)
    löst beim Laden einer Datei mit TAB eine Division-durch-Null-Exception aus. BUG(orig).
13. `defaults` OK: `cursy` wird mit dem alten `edrows` begrenzt, bevor der Bildschirm mit der
    neuen Fonthöhe geöffnet wird (bei 6→8 Pixel evtl. Cursor unterhalb des Fensters) –
    unsicher, prüfen in screenopen/calcpos.
14. `prtfile` behandelt Bytes ≥ $80 immer als Token, auch im ASCII-Modus/Standalone; dort ist
    `tokens` = 0 → `findtoke` liest Liste ab Adresse 0. Write-Fehler werden ignoriert;
    `numlines`=0 würde 65536 Iterationen laufen (praktisch ≥1).
15. `realsave`: icon-Schalter wird am Gadget (`defaultreqga9+13`) statt an `makeicons`
    abgefragt (gleichwertig, solange beide synchron). Bei Schreibfehler wird nach `error`
    trotzdem `.xtra`/`.info` geschrieben. doopen-Fehler nach Rename → nur noch `.bak` vorhanden.
16. `openxtra`/`openinfo` und die Close-Aufrufe nach den Rückrufen verlassen sich auf a6 =
    DOSBase aus `doclose` bzw. aus dem Rückruf (implizite Konvention).
17. `flink`: keine Cache-Leerung nach Relozierung (68040/060 Copyback); nur eine Reloc-Gruppe.
18. `dosearch` durchsucht nie den Rest der aktuellen Zeile; `next` auf der letzten Zeile
    piept, auch wenn der Treffer in der aktuellen Zeile hinter dem Cursor läge.
19. `a0toname` (2990, außerhalb, von `.tomain` genutzt) vergleicht `d0` mit ':'/'/' ohne d0
    zu laden → Pfad/Name-Trennung zufällig; Ergebnis bleibt durch die Wiederzusammensetzung
    in `.tomain` meist korrekt, `filepath`/`filename` (Requester-Vorgabe) aber falsch.
20. Offene Frage für den escape-Chunk: `escape` (1016) poppt `lea 12(a7)`, die Stack-Bilanz
    aus mainedit ist 10 Bytes (bsr mainedit 4 + Flag 2 + jsr 4) – Rücksprung von `freeall`
    läge dann 2 Bytes daneben. Prüfen, ob irgendwo (getline?) 2 Bytes zusätzlich liegen.
21. `freeall` (außerhalb): Größe per `moveq #lenitem,d0 / add.b numchars` → Byte-Überlauf bei
    numchars ≥ 247 → FreeMem mit falscher Größe.
