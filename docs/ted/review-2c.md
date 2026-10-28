# Review Phase 2c – Stichprobe auf Befehlsebene

Gegenstand: sechs Routinengruppen, Zeile für Zeile gegen `ted.s` (Zeilen =
Listing-Zeilen). Gesucht: vorzeichenbehaftete/vorzeichenlose Vergleiche
(`bcs/bhi` vs. `blt/bgt`), `.w`/`.l`-Breiten, `dbf`-Zählungen (n+1),
Rückgaben über Flags, Registerwerte, die in späteren Code weiterleben,
Reihenfolge der Seiteneffekte. Quellen nicht geändert.

Ergebnis vorweg: **keine echte Fehlübersetzung gefunden.** Drei kleine
Punkte (C-UB in einem Hazard-Pfad, eine `addq.w`-Breite, eine
Konsistenzfrage bei `add.w chloc`) sind unten als Befund „gering“
aufgeführt; keiner ist im normalen Betrieb sichtbar.

## 1 getline-Dispatch (`src/input.c` getline, `src/core/lineedit.c` gl_*, Dispatch in `src/main.c` mainedit / `src/edit.c` menu)

Geprüft:

* Einstieg `getline`/`getline2`/`getline3` (5416–5431): `clr anyalt`,
  `cursor`, Reihenfolge `cursor`/`showhprop`/`getinput`/`cursor`; `a0/a1 =
  bitmaps+scloc` (`add.l`), `a2 = chmap + chloc` (`add.w`, C `(WORD)chloc`),
  `d0 = cursx`.
* Klassifikation (5435–5442): `tst.l d7 / bmi`, `bclr #30 / bne .isasc`
  (Bit wird gelöscht), `cmp #32 / bcs` und `cmp #127 / bcc` vorzeichenlos
  auf dem Low-Word – C `(UWORD)d7` korrekt.
* `.isasc` (5444–5482): `anyalt = d7.w`; Blockanpassung
  `inbloks`→`btopx+1`, `inblokf`→`bbotx+1`, `blokfix`; Bitmap-Schieben
  (`edcols`-Schleife, `bcc`, innere `dbf fonthite2` = fonthite2+1
  Durchläufe, `add.w bmapmul`, `sub.w fontmul`); chmap-Schieben ab
  `cursx(Register)+cursoff(frisch)`; `print`, `cursrite` mit `d0 = cursx`.
  In C liegt das chmap-Schieben vor dem Bitmap-Schieben (gl_isasc vor
  shiftright) – unabhängige Speicher, `d0`/`a0`/`a1` sind vor der
  Blockanpassung gesichert (asm: `inbloks`/`inblokf`/`blokfix` ändern nur
  d2/d3, `blokdis` sichert d0/a0–a2). OK.
* `charlow` 31 (5484–5519): Shift-Test `and #3`, `.tababove` (Zeile 0 →
  getline2, `lastitem` = 4(a0), Zählung ab −1, Leerzeile → getline2). Der
  C-Wächter „−1 = nichts“ kollidiert nicht mit gültigen Spalten. OK.
* `charnot31` 30 (5521–5541): `bpl`-Schleife, `bcs curspos`, `subq`. OK.
* `charnot30` 8 (5543–5553): `or cursoff / beq`, `cursleft`, `d0 = cursx`
  neu, `d7 = 8`, nach `char127`. OK.
* `charnot8` 9 (5555–5657): Einfügen (`addq`, `bcc getline2`, `divu`-Rest
  über `moveq #0/move d0,d1` = nullerweitert), `anyalt` erst nach
  gefundenem Tabstopp; Löschen: `anyalt` **vor** der Schleife,
  `subq/bmi`, Blockanpassung ohne `blokfix`; `.zap`-Schleife (`bmi` auf
  d3 vorzeichenbehaftet, `bcs` d3<d2 vorzeichenlos), `.done`-Schleife
  (`bmi`, `bcc`); `.loop5/.loop6` mit `bcs`. `reline`: `add.w cursoff,a2`,
  `dbf edcols-1`, dann `curspos` mit der gesicherten Zielspalte. OK.
* `char127` (5662–5716): `cmp btopx,d2` nutzt d2 = Spalte aus `inbloks`
  (C: `abscol() != (UWORD)btopx`), `bbotx+1 / bcc .skipmo`, `blokfixb` nur
  nach `inblokf`; Schieben nach links; letzte sichtbare Zelle aus
  `a2[edcols+cursoff-1]` mit `cursx` gerettet. OK.
* `charnot127`/`charnotgad` (5718–5737): `cmp #-1,d7`/`cmp #-2,d7` als
  Wortvergleich (C `0xFFFF`/`0xFFFE`), `cmp #1,d6`, `HorizPot*chcols` →
  `swap`; `$f8e1` → Rest der Zeile löschen, `anyalt = d6`. OK.
* `glinedone`: Rückgabe d7 (und d6) unverändert. `horizpos` (5742,
  `bcc` vorzeichenlos, `bgt` vorzeichenbehaftet, `btst #0`), `cursleft`
  (5775), `cursrite` (5789): alle Vergleiche richtig gecastet.
  `refresh` (5337) zerstört d6/d7 – folgt aber immer `getline2` →
  `getinput`, daher folgenlos (C entsprechend).
* `inbloks`/`inblokf`/`blokfix`/`blokfixb` (5360–5414): `bhi`, `bcs`,
  `bls`, `bpl`, `blt` (bbotx/btopx `WORD`) korrekt. „eq“-Rückgabe über
  `moveq #0,d3`: d3 wird von keinem Aufrufer danach gelesen.
* Dispatch `mainedit` (142–177): `cmp.b d0,d7` – C vergleicht
  `(UBYTE)d7` (also auch Latin-1 `$FF` → gadget, survey-06 I.6, wie
  Original). Tabelleninhalt `jumptable` (6171) und `menutable` (6199)
  Eintrag für Eintrag gleich (Code, Flag, Routine, Reihenfolge,
  Endmarken 0 bzw. 255). d6/d7 überleben `charstobuff`/`makeline`/
  `showthisitem` auch im asm. `menu` (1565): `and #15`, `lsr #1/and #$f0`
  (Wort), `cmp.b #4` → `extmenu` mit dem noch nicht ge-or-ten d6,
  `cmp.b d6,d0`. OK.

Urteil: **OK.**

Befund gering (Konsistenz, nicht erreichbar):
`src/core/lineedit.c:29` `chrow()` rechnet `chmap + chloc` (vorzeichenlos),
das asm `add chloc(pc),a2` (5430) erweitert das Wort mit Vorzeichen; so tut
es auch `src/input.c` (`(WORD)chloc`). Unterschied erst ab chloc ≥ 32768
(chmap ist dafür zu klein). Vorschlag: `return chmap + (WORD)chloc;`.

## 2 handled0 (`src/input.c`)

Geprüft (5866–6001):

* Klassen als `cmp.l` (`$20`, `$40`, `$2000`, `$100`, `$8`, `$400`, `$200`).
* `.yesgad2`: `d6 = GadgetID` (38), `d7 = -1`; `cmp.b #4,17(a0)` =
  Low-Byte von GadgetType (C `(UBYTE)`), `cmp #9,24(a1)` Code; `btst
  #2,15(a0)` = Low-Byte Activation & ENDGADGET; `cmp #$20,d1` als Wort (im
  `.khere`-Pfad ist nur das Low-Word von d1 = `$40` gesetzt – C setzt d1
  ganz, gleiches Ergebnis). Reihenfolge `ReplyMsg` → `closereq` → `d0=-1`.
* MENUPICK: `cmp #-1,d0` (MENUNULL) → `.again`, sonst d6 = Code, d7 = −2.
* MOUSEBUTTONS: `$68` → −3, alles andere → −4.
* RAWKEY: `btst #7` → `.again`; `qualifier` vor `rawed`; `moveq #0,d7 /
  move.b d0,d7 / beq .again`; im Requester `cmp #13,d0` / `cmp #27,d0`
  als Wort auf dem Rückgabewert von rawed (C: `c`, UWORD – passt, rawed
  liefert ein ganzes Wort). `.retloop` (erstes) und `.escloop` (letztes)
  BOOL-ENDGADGET ab `62(a0)` = FirstGadget. `germs`: `dbf ngerms-1` =
  ngerms Paare, Treffer → Byte | Bit 30 (C `germs_map`).
* CLOSEWINDOW → `.doesc` (Hazard bei `req_window = 0` markiert).
* `.done`/`.again`: „ne“ ⇔ d7 ≠ 0 in allen `.done`-Pfaden (d7 ist dort nie
  0), daher ist die C-Rückgabe „d7“ statt Flag gleichwertig; `.again`
  setzt d7 = 0. `getinput`/`getinput2` (5833–5864) entsprechend.
* `refme` (6003): `or #$80,12(a0)`, RefreshGList(…,1), `Delay(4)`,
  `and #$ff7f`, nochmals RefreshGList. OK.

Urteil: **OK.**

## 3 tokit / tokit2 / putchar_ / alpha (`src/core/tokit.c`, tokit2 in `src/core/fileio.c`)

Geprüft (4195–4343):

* `tokit4`: Rohbyte-Vergleich mit 32 (`$A0` ≠ Leerzeichen), `subq/bpl`
  (Wort); Leerzeile: `addq #4,a7` (Start verworfen), `clr.b (a1)` ohne
  Weiterzählen, d5 = 1. `tokit2` → `tokit3` (erst `subq`). OK.
* Hauptschleife: `bclr #7` bei jedem Lesen; Quote-Test vor `invflag`-Test
  (quote = 255 trifft nach `bclr` nie – wie Original); Reihenfolge `_` `.`
  `\` → remark → `$`; Hex-Schleife mit `bcs/bhi/bls/bcs/bcc/bls`
  (alle vorzeichenlos, Werte < $80). `.nothex` gibt das aktuelle Zeichen
  ungeprüft aus.
* `.tsttoke`: `subq #1,a2`, Knotenlayout next(0)/num(4)/name(6), `or.b
  #32` beidseitig, `.found` prüft das Folgezeichen (`alpha` oder `_` →
  nächster Knoten), `.notoke` liest das Zeichen neu.
* `.putina3`: erst die zwei Bytes schreiben (High-Byte | $80, Low-Byte),
  dann `add.w d4,d0` (d4 = a4−a2), `cmp d1,d0 / bhi pchover2`.
* `putchar`: `addq.w`, `cmp d1,d0 / bhi pchover` (vorzeichenlos) –
  Makro `putchar_()` mit `goto enddo` entspricht `addq #4,a7` + `pchover2`.
* `enddo`: `clr.b (a1)+`, d5 = a1 − Start.
* `alpha`: `bcs 65`, `bhi 122`, `bcs 91` → ja, `bcs 97` → nein.

Urteil: **OK.**

## 4 readline (`src/core/fileio.c`)

Geprüft (3325–3453):

* `doread`: `subq.w d6 / bmi`, `move.b` + `cmp d0,d0` (N=0), `fillbuff`
  mit Z-Rückgabe (`move.l d0,d6` bzw. `moveq #0,d6`).
* Flags nach `move.l (a7)+,a0`: Ziel ist ein Adressregister → Flags
  bleiben die von `doread`; `bmi .oops` / `bpl .loop` testen also doread.
  C entsprechend.
* Token-Paar (`tst.b / bpl`): zweites Byte ungeprüft, EOF dazwischen →
  `.oops`, dann direkt `.nottab` (kein LF/TAB-Test für das 2. Byte).
* LF → 0 und d4 = −1; `move.b d0,(a0)+ / beq .skip3`.
* TAB: `subq #1,a0`; Spalte `sub.l`, Zähler `addq.w` (C: nur Low-Word
  erhöht), `divu tabstop` auf das ganze Langwort, Rest-Test; Leerzeichen
  werden **vor** dem Überlauftest geschrieben (BUG(orig) dokumentiert).
* `.nottab`: `cmp.l charbuff3,a0 / bcs .ok` sonst `a0 = charbuff3−3`,
  `.oops`.
* `.oops`: `clr.b (a0)+`, d5 = Länge inkl. 0, d0 = 0; `.skip3`: d0 = 1,
  kein zusätzliches 0-Byte. `.sit`: `tst d4`, `tokenise`, `subq.w #1,d5 /
  bpl`, `mytoke` (d1 = d5.w, in place), d0 über den Stapel gerettet
  (mytoke sichert d0 nicht).

Urteil: **OK.**

## 5 Blitter-Scrolls (`src/screen.c`) und chmap_* (`src/core/tokit.c`)

Geprüft:

* `getblitter` (4964): OwnBlitter, WaitBlit, AFWM/ALWM = $FFFF,
  BLTCON1 = 0. `putblitter`: WaitBlit, DisownBlitter.
* `scrollup` (4346–4431): d6 = edrows−d7−1 vor `getblitter`; BLTCON0
  $9F0 vor dem chmap-Blit; AMOD/DMOD = blitmod; d4 = `mulu`(d6,fonthite)
  `lsl.w #6` | edcols/2; d2 = fonthite<<6 | edcols/2; d3 = `mulu`
  (edrows−1, fontmul). `scup2`: `cmp #64,d4 / bcs` (vorzeichenlos),
  `add.l d5`, `add.w fontmul` (C `(WORD)fontmul`), DPT → APT → SIZE →
  WaitBlit; Füllzeile: DPT, ADAT = 0, Blockbereich (`bcs`/`bhi`) →
  $FFFF, dann BLTCON0 $1F0, SIZE, WaitBlit. Zweite Ebene mit erneutem
  $9F0. Schreibreihenfolge identisch.
* `scrolldown` (4434–4528): `getblitter` **vor** der d6-Rechnung (wie
  C), `beq eraseline`; $9F0 / BLTCON1 = 2 (absteigend); `scdown2`: `add.l
  d5`, `sub.w fontmul`, DPT/APT/SIZE/WaitBlit.
* `eraseline` (4479): AMOD, DMOD, dann `move.l #$1f00000,$040` (BLTCON0
  vor BLTCON1, wie C), ADAT 0/$FFFF (`cursy`-Zeile = d7+curstop), beide
  Ebenen mit WaitBlit; nur aus scrolldown erreicht (Blitter schon
  belegt).
* `cls` (4986): BLTCON0 $1F0, chmap-Füllung, ADAT 0, `cls2`
  (DPT, DMOD = blitmod, SIZE ohne Warten), WaitBlit, `cls2`, putblitter.
* chmap-Gegenstücke: `chmap_scrollup` – D = chmap + `mulu`(row,chcols),
  A = D + chcols, Größe `d6<<6 | (chcols/2 & 63)`, Modulo 0; letzte Zeile
  über `add.w` (C `(WORD)(UWORD)…`) mit `chcols/2 | 64` (ohne `and #63`).
  `chmap_scrolldown` – letzter Wort-Zeiger, A = D − chcols, Größe ohne
  `and #63` (eine Zeile mehr bei geradem d6, Lesen vor chmap
  abgefangen), dann `chmap_eraseline` (`and #63` | 64). `chmap_cls` –
  `edrows<<6 | (chcols/2 & 63)`. Höhe 0 = 1024, Breite 0 = 64 Wörter
  (blt_rows/blt_bytes). Absteigende Wortkopie ≡ absteigende Bytekopie bei
  geradem Abstand chcols.

Urteil: **OK.**

Befund gering 5a (C-UB nur im Hazard-Fall):
`src/core/tokit.c:379` und `src/core/tokit.c:407`: `(UWORD)(d6 << 6)` mit
`WORD d6`; für d6 < 0 (row ≥ edrows, ORIG_HAZARD-Fall, survey-05 I.13) ist
das Linksschieben einer negativen Zahl in C undefiniert. asm (4364/4453):
`lsl #6,d0` auf dem Wort. Vorschlag (beide Stellen):
`(UWORD)((UWORD)d6 << 6)`.

Befund gering 5b (Breite, praktisch nicht erreichbar):
`src/screen.c:132` `d5 = pixrows22 - (ULONG)(blitmod + 2);` – asm
4469–4472: `moveq #0,d0 / move blitmod,d0 / addq #2,d0 / sub.l d0,d5`;
`addq` ist `.w`, d.h. (blitmod+2) wird auf 16 Bit gekappt. Unterschied nur
für blitmod ≥ $FFFE. Vorschlag: `d5 = pixrows22 - (ULONG)(UWORD)(blitmod +
2);` (dieselbe Stelle in `scrollrite`, `src/screen.c:524`, hat dasselbe
Muster, asm 5023–5027 – gleiche Korrektur; nicht Teil der Stichprobe).

## 6 openreq / closereq (`src/reqs.c`)

Geprüft (3580–3670):

* `req = a0`; Breite/Höhe per `move.l 8(a0)` + `add #12` / `add #15`;
  Links = (edcols·8 − Breite) `lsr #1`, Oben = (newscreen.Height − Höhe)
  `lsr #1` – vorzeichenlos (C `(UWORD)`), richtig.
* `req_gads = ReqGadget (16)` vor dem Test; `tst 40(a1)` = High-Word von
  UserData des **ersten** Gadgets, `move #-1,40(a1)` setzt nur das
  High-Word (C `| 0xFFFF0000`), dann `addq #6,LeftEdge` / `add #12,TopEdge`
  für die ganze Kette (do-while, wie `bne.s .loop` nach dem ersten
  Durchlauf). Einmaligkeit nur über das erste Gadget – wie Original.
* `req_screen = myscreen`, OpenWindow, Fehler → `outmem`; Ringtausch
  `req_old2 ← req_old ← req_window ← neu`; PrintIText(ReqText = 24) und
  DrawBorder(ReqBorder = 20) bei (6,12) auf `RPort` (50); `bra pointeron`.
* `closereq`: nur mit `req_window`: Port (86) leeren, ClearPointer,
  CloseWindow, Rücktausch, `req_old2 = 0`, `flushmw`; `makemenus` immer.

Urteil: **OK.** (Hinweis ohne Änderungsbedarf: ein Requester ohne
Gadgets – `ReqGadget = 0` – liest im Original Adresse 40; C dereferenziert
dort ebenfalls NULL. In TED nicht erreichbar; der Header-Kommentar nennt es
bereits, ein `ORIG_HAZARD` wäre auf dem Amiga-Teil wirkungslos.)

## Zusammenfassung der Vorschläge

| # | Datei:Zeile | asm | Änderung |
|---|---|---|---|
| 1 | `src/core/lineedit.c:29` | 5430 `add chloc(pc),a2` | `return chmap + (WORD)chloc;` (Konsistenz) |
| 5a | `src/core/tokit.c:379`, `:407` | 4365, 4454 `lsl #6` | `(UWORD)((UWORD)d6 << 6)` (kein UB bei d6 < 0) |
| 5b | `src/screen.c:132` (und `:524`) | 4471 `addq #2,d0` (.w) | `(ULONG)(UWORD)(blitmod + 2)` |

## Umgesetzt (Koordinator, Phase 2c)

Alle drei Vorschläge sind übernommen: `chrow()` mit `(WORD)chloc`,
`(UWORD)((UWORD)d6 << 6)` in `chmap_scrollup`/`chmap_scrolldown`,
`(ULONG)(UWORD)(blitmod + 2)` in `scrolldown` und `scrollrite`.

Weitere Befunde des Integrations-Reviews (außerhalb dieser Stichprobe,
Einzelheiten in [comdata.md](comdata.md), [abweichungen.md](abweichungen.md),
[changes.md](changes.md)):

* **32-Bit-Division**: `showvprop`/`showhprop` (input.c) und der
  Dateirequester (`fq_divu_w`) benutzten C-`/`/`%` → libnix20
  `__udivsi3`/`__umodsi3` → `UtilityBase` (nie geöffnet) = Absturz bei jedem
  Aufruf. Jetzt `divu_w` = `divu.w` in src/glue.c, ctest `checklink`.
* **ezfreq** (comdata+24): a3 nach dem Aufruf ist im Original die Adresse
  `reqstruct+12`, nicht deren Inhalt – korrigiert.
* **refgads** (comdata+28): d0 als ganzes Register an `RefreshGList`.
* **openio** (comdata+60, `mycli`): d0/Z von `WBenchToFront` wie im
  Original (über `callregs`, `REGSTUB_CCR`).
* Schutzabfragen (`tokens == 0`, `freeall`, `freeupa5`,
  `chmap_scrolldown`) nur noch im Host-Build; `swapd0bord` ≥ 16 und
  `outmem` mit `memstack == 0` wie im Original.
