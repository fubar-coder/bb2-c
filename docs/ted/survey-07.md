# Survey 07 – ted.s:6286–8072 (Intuition-Daten, Menüs, Requester, incbins, `flink`)

Bereich: fast nur Daten. Einziger Code: `flink` (7953) plus die Einsprünge in die
incbin-Blöcke `rawed` und `filerequest`. Adressen aus `build/ref/ted.labels`.

| Label | Adresse | Zeile |
|---|---|---|
| newscreen | $6316 | 6286 |
| newwindow | $63EE | 6316 |
| newgads | $648E | 6346 |
| menu1 | $653A | 6383 |
| findreq | $6D4A | 6906 |
| undobuffer | $7320 | 7105 |
| rawed (incbin, 660 B) | $73E0 | 7109 |
| repall | $7674 | 7111 |
| errorreq | $78C6 | 7195 |
| erasereq | $7A02 | 7239 |
| defaultreq | $7BFA | 7310 |
| alertstuff | $8C76 | 7948 |
| flink | $8CBE | 7953 |
| filerequest (incbin, 15916 B = $3E2C) | $8CE0 | 7967 |
| prtreq | $CB0C | 7971 |
| font6 (768 B) / font8 (1024 B) | $CDFA / $D0FA | 8070/8071 |

---

## A. Routinen

| Routine | ein | aus | zerstört | Notizen |
|---|---|---|---|---|
| `flink` (7953) | – | – | d0–d2, a0, a1 | Einmalig beim Start (`jsr flink`, ted.s:76, vor der Verzweigung extern/intern). Reloziert den incbin-Block `filerequest` zur Laufzeit (s. D). Nicht idempotent. |
| `rawed+32` (= Code+0) | – | – | d0–d2, a0–a3, a6 | Init: console.device öffnen, Tabellen bauen (ted.s:39). Kein Rückgabewert, kein Fehlertest. |
| `rawed+32+4` | d0.w = Rawkey (nur Key-down, Aufrufer filtert Bit 7), d1.w = Qualifier (ted.s:5926 `move 26(a1),d1`) | d0.w = Zeichen (0 = keins), Z nicht zuverlässig (Aufrufer macht `move.b d0,d7 / beq`) | d1; sichert a0–a2/a6 | Rawkey→ASCII (ted.s:5928). |
| `rawed+32+8` | – | – | a1, a6, d0/d1 | CloseDevice (ted.s:128 `fail5`). Tail-`jmp` in exec. |
| `filerequest+36` (= Code+4) | a0 = `reqstruct` {path.l, name.l, title.l, screen.l} | d0 = Zeiger auf Ergebnis (voller Pfad), **Z=1 ⇔ d0=0** (Abbruch) | nur d0 (d1–a6 per MOVEM gesichert) | Einsprung aus `ezfreq` (ted.s:3507) als **Tail-Call** (`bra`), Rückkehr direkt zu dessen Aufrufer. |
| `filerequest+32` (= Code+0) | – | – | – | Alternativer Einsprung mit internem Default-Struct (Code+$B3E); von TED **nicht** benutzt. |

Aufrufer-Kontext (außerhalb des Bereichs, nur zur Schnittstelle):
* `ezfreq` (ted.s:3502): `a0`=Pfadpuffer, `a1`=Namenpuffer, `a2`=Titel → schreibt
  `reqstruct+0/4/8`, `reqstruct+12` = Screen (gesetzt in `screenopen` ted.s:4675),
  `lea reqstruct,a0`, `bra filerequest+36`. Zerstört a3 (→ reqstruct+12) und a0.
  Exportiert als `comdata+24` an blitz2.
* `getfname3` (ted.s:3493): `beq .skip` → `cmp #1,d0 / beq outmem` sonst
  `addq #4,a7` (Aufrufer verwerfen!). Siehe I (toter Zweig).

## B. Registerkonventionen

* Keine persistenten Register im Bereich.
* `filerequest` sichert d1–a6 selbst (`movem.l d1-a6,-(sp)` bei Code+4, Rückkehr
  über gespeicherten SP `moveal %pc@(0x628),sp`), liefert Ergebnis + Flags über
  `move.l (sp)+,d0` (MOVEA/MOVEM/RTS lassen CCR stehen).
* `rawed+4` sichert a0–a2/a6, sonst nur d0/d1.

---

## C. Globals / Datenstrukturen

### C.1 Screen/Window

| Label | Typ | Init | Laufzeit-Änderungen |
|---|---|---|---|
| `newscreen` (6286) | `struct NewScreen` (36 B) | 0,0,640,200, Depth 2, Pens 1/2, Type `$100f` = CUSTOMSCREEN\|NS_EXTENDED, Font `screenfont`, Title `newsctit`, Extension `extnewscreen` | `+4` Width ← `scdata+12` (WB-Breite & $fff0), `+6` Height ← berechnet (ted.s:4635/4658); `+6` gelesen von `openreq`, `gotolined0`-Umfeld (2554, 3596) |
| `viewmode` (6288) | = `newscreen+12` ViewModes, Init `$8004` | ← `scdata+44+32` = vp_Modes des WB-Screens (4632) – Initwert also wirkungslos |
| `screenfont` (6293) | `struct TextAttr` | "topaz.font", 8, 0, 0 | – |
| `extnewscreen` (6301) | TagItem-Liste | `SA_Pens`($8000003A)→`pens`, `SA_LikeWorkbench`($80000047)=1, `SA_Depth`($80000025)=2, TAG_END | – |
| `pens` (6310) | UWORD[10] DrawInfo-Pens | 1,2,2,1,2,3,1,0,3,~0 | – |
| `newsctit` (6312) | char[32+80] | 'Blitz Basic 2 - Ted Version 1.1' | Screen-Titel; `showfname` (732, 1598, 2730) schreibt `"File: "`+filename (≤63) hinein |
| `newwindow` (6316) | `struct NewWindow` (48 B) | 0,0,640,200, Pens 0/1, IDCMP `$56C` = RAWKEY\|GADGETUP\|GADGETDOWN\|MENUPICK\|MOUSEBUTTONS, Flags `$1900` = BACKDROP\|BORDERLESS\|ACTIVATE, FirstGadget `newgads`, Type 15 | `+2` TopEdge ← `topskip` (=12), `+4` Width, `+6` Height (4630–4656) |
| `idcmp` (6318) | = `newwindow+10` | | nur Label, keine Referenz im Code gefunden |
| `winscreen` (6321) | = `newwindow+30` Screen | 0 | ← myscreen (4674) |

### C.2 Scroller (Window-Gadgets) und ihre Rahmen

| Label | Typ | Init / Zweck |
|---|---|---|
| `newgads` (6346) | `struct Gadget`, Flags 4 (GADGIMAGE), Act 3 (GADGIMMEDIATE\|RELVERIFY), Type 3 PROP, ID **0** | vertikaler Scroller, Next → `newgads2` |
| `proprend` (6354) | `struct Image` 20 B, 0 | AUTOKNOB-Puffer |
| `prop1sinfo` (6359) | `struct PropInfo` | Flags 13 = AUTOKNOB\|FREEVERT\|PROPBORDERLESS, HPot -1, VPot 0, HBody -1, VBody -1 |
| `newgads2` (6362) | `struct Gadget`, Act 1 RELVERIFY, PROP, ID **1** | horizontaler Scroller |
| `proprend2` (6370) | Image 20 B | |
| `prop2sinfo` (6375) | PropInfo, Flags 11 = AUTOKNOB\|FREEHORIZ\|PROPBORDERLESS, HBody $8000 | |
| `newgadbord`/`nextngad` (6324/6329) | 2× `struct Border` (Pens 1 bzw. 2, JAM1, 3 Punkte) + `ngc1`/`ngc2` (je 3 Paare) | 3D-Rahmen um vertikalen Scroller, `DrawBorder` in `screenopen` (4779) |
| `newgadbord2`/`nextngad2` (6335/6340) | dito + `ngc3`/`ngc4` | Rahmen um horizontalen Scroller (4784) |

Laufzeit (`screenopen`, ted.s:4721–4765) – **jedes Öffnen setzt neu**, weil danach
relativ verschoben wird (`addq #3,newgads+4` usw.):
* `newgads+4/+6/+8/+10` (Left/Top/Width/Height): `clr.l +4`, `+8 = $000A0000`, dann
  Left = edcols*8 (+3), Top +2, Height = pixrows (−4).
* `newgads2+4/+6/+8/+10`: Width = edcols*8 (−6), Top = pixrows (+2), Height 4, Left +3.
* `prop2sinfo+6` HorizBody ← edcols·65536/chcols.
* `ngc1+10`, `ngc2+6/+10`, `ngc3+0/+10`, `ngc4+0/+4/+6/+10`, `newgadbord+0`,
  `nextngad+0`, `newgadbord2+2`, `nextngad2+2`.
* Gelesen: `prop1sinfo+4` VertPot (ted.s:1470/1485, Ziehen → Zeile = VPot·numlines>>16),
  `prop2sinfo+2` HorizPot (5722), `prop2sinfo+6` (5328). Gesetzt per
  `NewModifyProp(newgads/newgads2, mywindow, 0, flags 13/11, …, 1)` (5299, 5331).

### C.3 Menüleiste

Equates: `mp1=2` (FrontPen), `mp2=1` (BackPen), `mj=1` (JAM2), `mwidth=160`.
Typen: `struct Menu` (30 B = `menu2name-menu2`), `struct MenuItem` (34 B) direkt gefolgt
von seiner `struct IntuiText` (20 B, `mXiYname`: Pens 2/1, JAM2, Pos 2,2, Font 0) und
dem Text. Items: Width 160, Height 12, TopEdge 0,12,24,…; Flags `2+16+64[+4]` =
ITEMTEXT\|ITEMENABLED\|HIGHCOMP[\|COMMSEQ]; Command-Byte + KludgeFill im `dc.b 'X '`.

**Kette der Menüs:** `menu1` → `menu4` → `menu3` → `menu2` → 0 (NextMenu); daraus
MENUNUM: 0 PROJECT, 1 EDIT, 2 SOURCE, 3 SEARCH, 4 COMPILER (zur Laufzeit angehängt).
Die Item-Nummer ist die **Position in der Kette, nicht die Label-Nummer**
(`menu1item4` → `menu1item6`, `menu4item2` → `menu4item4` → `menu4item5` →
`menu4item3`). `menu` (ted.s:1565) bildet Byte `item<<4 | menu` (nur Items 0–15) und
sucht in `menutable` (ted.s:6199).

| Menü (Left/Width) | Index: Text [Taste] (Label) |
|---|---|
| PROJECT `menu1` (0/64) | 0 NEW (item1) · 1 LOAD [L] · 2 SAVE [S] · 3 DEFAULTS [?] (item4) · 4 ABOUT (item6) · 5 PRINT (item7) · 6 CLI [Z] · 7 CLOSE WORKBENCH · 8 OPEN WORKBENCH · 9 QUIT [Q] (item11) |
| EDIT `menu4` (72/40) | 0 COPY [C] (item1) · 1 KILL [K] (item2) · 2 BLOCK TO DISK (item4) · 3 INSERT FROM DISK (item5) · 4 FORGET [W] (item3) · 5 INSERT LINE [I] · 6 DELETE LINE [D] · 7 DELETE RIGHT [Y] · 8 JOIN [J] · 9 BLOCK TAB []] (itema) · 10 BLOCK UNTAB [[] (itemb) · 11 MARK INDENT [A] (itemc) |
| SOURCE `menu3` (120/56) | 0 TOP [T] · 1 BOTTOM [B] · 2 GOTO LINE [G] |
| SEARCH `menu2` (184/56) | 0 FIND [F] · 1 NEXT [N] (itema) · 2 PREVIOUS [P] · 3 REPLACE [R] · 4 REPLACE ALL |

Initial deaktiviert (ohne ITEMENABLED): COPY, KILL (`2+64+4`), BLOCK TO DISK (`2+64`).

**Laufzeit-Patches:**
* `bloken` (ted.s:3890) `or #16,…+12` / `blokdis` (3903) `and #~16,…+12` auf
  `menu4item1`, `menu4item2`, `menu4item4`, `menu4itema`, `menu4itemb` (Offset 12 =
  MenuItem.Flags, Bit ITEMENABLED). Direkt in der angehängten Leiste, ohne
  ClearMenuStrip/OnMenu. BLOCK TAB/UNTAB sind initial aktiv, bis zum ersten `blokdis`
  (Aufrufe ted.s:690, 1655, 1805).
* `menu2+0` (NextMenu, Init 0) ← das COMPILER-Menü (s. u.).

**COMPILER-Menü aus `menitems` (= edstruct+4)** – `getstuff` (ted.s:2782), nur wenn
`menitems ≠ 0`:
1. `ClearMenuStrip(mywindow)`.
2. `AllocMem(30, MEMF_PUBLIC)` (unbeprüft!), Kopie von `menu2` (Wortschleife bis
   `menu2name`), `menu2.NextMenu` ← neu; neu: NextMenu 0, MenuName (+14) = `menitems`
   (Zeiger in blitz2-Speicher, keine Kopie), LeftEdge (+4) = 248, Width (+8) =
   (strlen+1)·8.
3. Format `menitems`: `title\0` dann je Eintrag `text\0` + 1 Byte Tastenkürzel (0 = keins),
   Ende = 0-Byte an Stelle eines Textes (blitz2.s:724 `menus`: "COMPILER", 10 Einträge
   à 23 Zeichen + Taste; nicht-final zusätzlich `'PRINT TOKENS',0,0`).
4. Je Eintrag `AllocMem(54, MEMF_PUBLIC)` (unbeprüft), Kopie von `menu2item1`+`m2i1name`
   (34+20 B), dann: NextItem 0, Width (+8) = 200, TopEdge (+6) = 12·n, Vorgänger.NextItem
   bzw. Menu.FirstItem (+18) ← neu, ItemFill (+18) ← +34 (eingebettete IntuiText),
   IText (+46) ← Text in `menitems`, Command (+26) ← Kürzel, Flags (+12) COMMSEQ (4)
   gesetzt/gelöscht.
5. `SetMenuStrip(mywindow, menu1)` als Tail-`jmp`.
Freigabe in `escape` (ted.s:1018): Menu (30) + alle Items (54) per FreeMem.
MENUNUM 4 → `extmenu` (ted.s:1542) → blitz2 `menu` (architektur.md §3.3).
`screenopen` hängt bei jedem Öffnen wieder `menu1` an (4774), das COMPILER-Menü bleibt
über `menu2.NextMenu` erhalten.

### C.4 Requester (gemeinsames Muster)

Typen exakt wie NDK: `struct Requester` (112 B: Older, Left/Top/W/H, RelLeft/Top,
ReqGadget, ReqBorder, ReqText, Flags, BackFill, Kludge, ReqLayer, Pad1[32], ImageBMap,
RWindow, ReqImage, Pad2[32]), `struct Border` (16 B, Pens 1/2 bzw. 2/1, JAM2(1), Count 3),
XY-Arrays (6 Worte), `struct IntuiText` (20 B, Pens 1/0, JAM2), `struct Gadget` (44 B),
`struct StringInfo` (36 B: hier `dc.w 3 + dc.w 7 + dc.l 2` – die letzten 2 Worte der
7 überdecken `Extension`, `dc.l 2` = LongInt, AltKeyMap), `struct PropInfo` (11 Worte),
`struct Image` als 5×`dc.l 0` (AUTOKNOB-Puffer). Jede Border-Paar = 2 Borders + 2 XY.

**Wichtig: TED benutzt `Request()` nicht.** `openreq` (außerhalb, `comdata+0`) liest vom
Requester nur `+8` W/H, `+16` ReqGadget, `+20` ReqBorder, `+24` ReqText; öffnet ein
eigenes Fenster `reqwindow` (W+12, H+15, zentriert, Screen = myscreen), Gadgets =
Fenster-Gadgets; ReqText/ReqBorder werden per `PrintIText`/`DrawBorder` bei (6,12)
gezeichnet. **Einmalige Verschiebung** aller Gadgets um (+6,+12) (`+4`,`+6`), Merker
= `UserData`-Hochwort (`+40`) des **ersten** Gadgets ← −1. Deshalb fehlt
GTYP_REQGADGET zu Recht. Rückgabe von `getinput`: d7 = −1 bei Gadget, d6 = GadgetID.
Alle String-Gadgets teilen `undobuffer` (7105, `ds.b 192`; auch `gotoreqsg1` ted.s:2391).

| Requester | Größe | ReqBorder-Kette | ReqText-Kette | Gadgets (ID: Art, Text) |
|---|---|---|---|---|
| `findreq` (6906) | 80,6 376×92 | bo1→la2→bo2→la5→bo3→la8→bo4→la11→0 (8 Borders) | in1→in2→in3 ('REPLACE:', 'FIND:', 'Find/Replace') | 1 STR find (`findreqsg1`, Puffer `findreqla16` 64 B, MaxChars 64, Flags $200 TABCYCLE, Act RELVERIFY) · 2 STR replace (`findreqsg2`, `findreqla17` 64 B, Act RELVERIFY\|ENDGADGET) · 3 NEXT · 4 PREVIOUS · 5 REPLACE · 6 REPLACE ALL · 7 CANCEL (Bool, Act 5) · 8 CASE SENSITIVE (Act $101 TOGGLESELECT\|RELVERIFY) |
| `repall` (7111) | 132,46 240×40 | bo1 (2) | in1 'Replace all.....' | 1 FROM HERE · 2 FROM TOP · 3 CANCEL |
| `errorreq` (7195) | 128,58 312×40 | bo1 (2) | in1, IText = **`errorpnt`** (Laufzeit) | 1 ' OKAY ' |
| `erasereq` (7239) | 128,50 256×48 | bo1 (2) | in1→in2 ('This will erase altered file!', 'Continue?') | 1 ' OK ' · 2 ' CANCEL ' |
| `defaultreq` (7310) | 60,1 400×172 | bo1→…→bo10 (20 Borders; bo8–10 LeftEdge 4 = Slider-Rahmen) | in1→…→in13 (Titel, 'Use Small Font', 'Tokenise', 'Create Icons', 3× Margin/Tab, 'Text Colour:', **'' (ehem. 'Label Colour:')**, 'Token Colour:', 'Red:', 'Green:', 'Blue:') | s. u. |
| `prtreq` (7971) | 60,46 360×68 | bo1→la2→bo2→la5→bo3→la8→0 (6) | in1→in2 ('PRINTER DEVICE:', 'Print Text') | 1 STR (`prtreqsg1`, Puffer `prtname` 'PRT:'+59×0 = 64 B, MaxChars 64, Act RELVERIFY) · 2 ' PRINT ' · 3 ' CANCEL ' |

Buttons: Flags 0 (GADGHCOMP), Act 5 = RELVERIFY\|ENDGADGET, Type 1 BOOL, GadgetRender =
eigenes Border-Paar, GadgetText eigene IntuiText (4,2). ENDGADGET ist im Fenster wirkungslos.

**defaultreq-Gadgets (Kette ga1…ga27, aber `ga13 → ga18`: ga14–17 abgehängt, 23 aktiv):**

| ID | Gadget | Art | Daten |
|---|---|---|---|
| 1–3 | ga1–3 (75,120/132/144, 306×4) | PROP, Act 3, Image `defaultreqla35/39/43`, `defaultreqhp1–3` Flags 11, HPot (6<<16)/15, HBody 4096 (16 Stufen) | R/G/B der Palette `propcol` |
| 4–6 | ga4–6 (20,54/66/78, 24×8) | STR, Act $801 = LONGINT\|RELVERIFY, MaxChars 3, Puffer `defaultreqla47/48/49` (3 B: '12','6','2') | LongInt = **`int1`/`int2`/`int3`** (Init 12, 6, **4**) = hscmarg, vscmarg, tabstop |
| 7–9 | ga7–9 (16,16/28/40, 32×12) | BOOL Toggle, Act 257 = TOGGLESELECT\|RELVERIFY, Text '  ' | Small Font, Tokenise, Create Icons (ga9 Flags 128 = SELECTED initial) |
| 10–13 | ga10–13 (280..352, 36, 16×12) | BOOL, Flags 2 GADGHIMAGE, Act 2 GADGIMMEDIATE, Render=SelectRender=`bo14–17`, Text ' ' mit BackPen 0..3 | Farbe „Text“ (Gruppe 0 → `normcol`) |
| 14–17 | ga14–17 (y 52) | dito, `bo18–21` | Gruppe 1 `labcol` – **nicht in der Kette** |
| 18–21 | ga18–21 (y 68) | dito, `bo22–25` | Gruppe 2 `tokcol` |
| 22–25 | ga22–25 (16..312, 100, 72×12) | dito, `bo26–29`, Text 8 Leerzeichen, BackPen 0..3 | Gruppe 3 `propcol` (Palettenregister für die Slider) |
| 26 | ga26 ' OK ' | Button | |
| 27 | ga27 ' CANCEL ' | Button | |

Auswertung in `defaults` (ted.s:401ff.): ID<4 → Slider-Schleife `propadj`; 10–25 →
Gruppe = ((ID−10)>>1)&6, nur Gruppe 3 löst `setprops`/`dispprops` aus; 4–9 →
Validierung + Übernahme; ≥26 → Ende (26 OK, 27 Cancel → `setdefs` mit alten Werten).

### C.5 Vom Code geänderte/gelesene Felder der Requester (Offset!)

| Feld | Zugriff | Stelle |
|---|---|---|
| `findreqsg1/2 +8` BufferPos, `+12` DispPos | `clr` | find 853–856 |
| `findreqla16/17` (Find/Replace-Text) | Sicherung nach charbuff/charbuff2 und Rückkopie bei CANCEL; gelesen von Suche/Ersetzen | 857–896, 902, 932, 965, 984, 991, 3540 |
| `findreqga8+13` Bit 7 (= Flags & SELECTED, Low-Byte) | `btst` → case-sensitiv | 3543, 3550 |
| `errorpnt` (= errorin1.IText) | ← a0 (Fehlertext) | error 3512 |
| `defaultreqga7/8/9 +12` Flags SELECTED ($80) | and/or | setdefs 3097–3108, getstuff 2784 |
| `defaultreqga8 +12` Flags DISABLED ($100) | or/and (ohne Token-Liste deaktiviert) | 406, 409 |
| `defaultreqga7/8/9 +13` Bit 7, `+12`&128 | gelesen: fonthite, tokenise, makeicons | 506, 514, 518, 805 |
| `defaultreqga9+13` | **Adresse** wird an blitz2 exportiert: `comdata+84` ← `#defaultreqga9+13` | ted.s:82 (in blitz2.s keine Verwendung von `84(aX)` gefunden) |
| `defaultreqga4/5/6 +12` SELECTED | gelöscht (Schleife, Schrittweite `ga5-ga4` = **84 B**) + RefreshGList(3) | 495–503 |
| `defaultreqla47/48/49` | `move #$3000` ("0\0"), `numtoascii2` | 480–490, 3122–3128 |
| `int1/2/3` (LongInt) | `cmp.l`/`clr.l`; `+2` (Low-Word) ↔ hscmarg/vscmarg/tabstop | 477–489, 522–524, 3134–3136 |
| `defaultreqhp1/2/3 +2` HorizPot | schreiben (setprops, Schrittweite `hp2-hp1` = **86 B**), lesen >>12 (propadj) | 3174–3185, 3200–3208 |
| `defaultreqbo14…bo29` +4/+5 (FrontPen/BackPen, beide Borders je Paar) | getauscht: `swapbords` (alle 32 Borders, Schritt `la63-bo14` = **28 B**), `swapd0bord` (Index·**56 B**) | 3238–3255 |
| `defaultreqga10…ga25` | `dispgadd0`: Gadget = ga10 + n·**44 B**, RefreshGList(1) | 3229 |
| alle Requester-Gadgets `+4/+6/+40` | einmalig verschoben/markiert durch `openreq` | s. C.4 |
| `prtname` | als DOS-Name an `Open(…,1006)` | 585 |

### C.6 Sonstige Daten

| Label | Größe | Inhalt/Zweck |
|---|---|---|
| `undobuffer` (7105) | 192 B | gemeinsamer UndoBuffer aller StringInfos |
| `rawed` (7109) | 660 B incbin | Hunk-Datei (s. D/F) |
| `;rawmaker incbin rad:rawmaker.obj` (7107) | – | auskommentierter Vorgänger von `rawed`, nicht gebaut |
| `alertstuff` (7948) | Alert-String | x=56 (Wort), y=22 (Byte), 'Not enough memory for operation! - Click Mouse Button to Continue.', 0, Fortsetzungsbyte 0. `doalert` (3058): `DisplayAlert(RECOVERY_ALERT=0, alertstuff, 40)` |
| `filerequest` (7967) | 15916 B incbin | Hunk-Datei, durch `flink` reloziert; `;include rad:filerequester` = Quelle nicht vorhanden |
| `font6` (8070) | 768 B | Zeichensatz Höhe **6** (nicht Breite!): 8 Zeilen à 16 Zeichen, je Zeichen 1 Byte breit (8 px), Zeilenabstand 16 B. Zeichen c: `font6 + (c&15) + (c>>4)·96` (`fontcalc` 5214) → 128 Zeichen |
| `font8` (8071) | 1024 B | Höhe 8: `font8 + (c&15) + (c>>4)·128` → 128 Zeichen; liegt am Hunk-Ende |

---

## D. Selbstmodifizierender Code / Daten im Code / incbin-Einsprünge

### `flink` – Laufzeit-Relokation von `filereq` (exakt)

Aufbau `filereq` (geprüft): `$3F3, 0, 1, 0, 0, $E46` (HUNK_HEADER, 1 Hunk, 3654 Langworte),
`$3E9, $E46` (HUNK_CODE), Code (14616 B) ab Dateioffset 32, `$3EC` (RELOC32) mit **einem**
Block `n=312, hunk=0, Offsets…`, `0`, `$3F2` (HUNK_END).

```
flink	lea	filerequest+28(pc),a0  ; a0 → Größenwort des HUNK_CODE
	move.l	(a0)+,d0               ; d0 = 3654 (Langworte), a0 = Code-Start (+32)
	move.l	a0,a1                  ; a1 = Basis für Offsets
	move.l	a0,d2                  ; d2 = Laufzeitadresse des Codes = Relokationswert
	lsl.l	#2,d0                  ; Bytes
	lea	4(a0,d0.l),a0          ; hinter Code + $3EC-Kennung → Zähler n
	move.l	(a0),d0                ; d0 = n (312)
	addq	#8,a0                  ; n und Hunknummer überspringen
.loop	move.l	(a0)+,d1               ; Offset
	add.l	d2,0(a1,d1.l)          ; *(ULONG*)(code+off) += code
	subq.l	#1,d0
	bne	.loop
	rts
```
Annahmen (hart codiert): genau 1 Hunk, Header 7 Langworte, genau ein RELOC32-Block,
Zielhunk 0 wird ignoriert, n≠0 (n=0 → 2³²-Schleife). Kein CacheClearU danach. Nicht
idempotent (zweiter Aufruf = doppelte Relokation). Die Daten des Requesters (Fenster,
Gadgets, Puffer) liegen ebenfalls im Block → `filerequest` schreibt zur Laufzeit in TEDs
Code-Hunk.

### `rawed` (keine Relokation nötig)
`rawed.obj`: HUNK_HEADER + HUNK_CODE 156 Langworte, **kein RELOC32** (nur PC-relativ und
`$4.w`). Code ab +32 (Disassembly `rawed.obj.dsm`):
* +0 `bra init`, +4 `bra convert` (Code+$10A), +8 `bra cleanup` (Code+$20A).
* Daten im Code: 'console.device', Device-Basis (Code+28), 2-Byte-Puffer (+32),
  `struct InputEvent` (+34, ie_Class=RAWKEY), Tabellen Code+56/+161 (je 105 B:
  Rawkey 0..$68 ohne/mit Qualifier 1), IOStdReq (48 B, Code+$218), MsgPort (34 B).
* `init`: IOStdReq/MsgPort initialisieren (FindTask(0), AllocSignal(-1)),
  `OpenDevice("console.device", -1, io, 0)`, Device-Basis merken, beide Tabellen per
  `convert` füllen. Tabellen werden von TED **nicht** benutzt (keine Referenz auf
  `rawed+32+56`).
* `convert`: Sonderfälle vor RawKeyConvert, unabhängig vom Qualifier:
  $50–$59 (F1–F10) → $81–$8A; $4C–$4F (Cursor) → $1C–$1F; $5F (Help) → $8B;
  sonst `RawKeyConvert(&ie{Code=d0,Qualifier=d1}, buf, 1, NULL)`; Ergebnis −1 → 0, sonst
  buf[0] (Puffer wird vorher gelöscht → 0 bei keinem Zeichen).
* `cleanup`: `CloseDevice(io)` (Tail-`jmp`); Signal wird nie freigegeben.

### `filerequest+36` vs. freqlib `dofreq`
* Code+0: `lea Code+$B3E,a0` (internes Default-Struct mit eigenen Puffern) – fällt in
  Code+4 durch. Code+4: `movem.l d1-a6,-(sp)`, `a5 = a0`; Struct: `+0` Pfadpuffer,
  `+4` Namenpuffer, `+8` Titel, `+12` Screen (0 → WBENCHSCREEN, sonst CUSTOMSCREEN mit
  Screen; außerdem `setheight` liest `screen+14` und an Code+$88C `screen+76`
  **ohne Nulltest**).
* freqlib.c: `freqst[4] = {path, file, title, screen}` und `freqcode()` = derselbe
  Code (Offsets dort „im Binary“, Datenblock `fq_data`). TEDs `reqstruct` ist
  layoutgleich zu `freqst`; `dofreq_c` ist nur der Blitz-String-Wrapper darum.
  → In C: `freqcode()` aus libs-c/libs/freqlib.c als gemeinsame Quelle verwenden, mit
  Parameter `ULONG st[4]` statt statischem `freqst`; Rückgabe Zeiger (0 = Abbruch), Z-Flag
  für asm-Aufrufer (`comdata+24` = `ezfreq`) im Glue erzeugen.
* Ergebnis = Zeiger in den Requester-Datenblock (`RESULT`), wird von TED per d7 weiter
  benutzt; OK kopiert außerdem Pfad/Name zurück in `filepath` (192 B) / `filename` (64 B).

---

## E. Heap-/Listenstrukturen

| Struktur | Größe | Zuständig | Felder |
|---|---|---|---|
| COMPILER-Menu | 30 B, MEMF_PUBLIC | getstuff / escape | Kopie `menu2`: +0 NextMenu=0, +4 Left=248, +8 Width, +14 MenuName→menitems, +18 FirstItem |
| COMPILER-Item | 54 B, MEMF_PUBLIC | getstuff / escape | +0 NextItem, +6 Top=12n, +8 Width=200, +12 Flags (COMMSEQ), +18 ItemFill→+34, +26 Command, +34 IntuiText (Kopie `m2i1name`), +46 IText→menitems |
| AUTOKNOB-Images | 20 B statisch | Intuition | `proprend`, `proprend2`, `defaultreqla35/39/43` |

Keine weiteren Allokationen im Bereich (filereq allokiert intern 48 B Chip + 40-B-Knoten).

## F. Externe Aufrufe (aus Bereich bzw. für diese Daten)

* intuition: `OpenScreen(newscreen)` (Tags V39: SA_LikeWorkbench), `OpenWindow(newwindow)`,
  `SetMenuStrip`/`ClearMenuStrip`, `DrawBorder(newgadbord[2])`, `NewModifyProp`,
  `RefreshGList` (defaultreq), `PrintIText`/`DrawBorder` (openreq), `DisplayAlert`.
* graphics: `SetRGB4(&screen->ViewPort, propcol, r,g,b)` (propadj).
* exec: AllocMem/FreeMem (Menüs), in rawed: FindTask, AllocSignal, OpenDevice,
  CloseDevice; console: `RawKeyConvert` (−48).
* filereq intern: dos/intuition/graphics per OldOpenLibrary (nie geschlossen),
  OpenWindow, Lock/Examine/ExNext, ActivateGadget …
* blitz2: `menitems` (edstruct+4) nur gelesen (Zeiger bleiben in blitz2-Speicher);
  `comdata+84` ← Adresse des „Create Icons“-Flagbytes.
* Kein Hardware-Zugriff im Bereich.

---

## G. Was in C Sonderbehandlung braucht

1. **Feste Schrittweiten über Structs hinweg** (dürfen in C nicht über getrennte Statics
   laufen): `defaultreqga4→5` 84 B, `defaultreqhp1→2` 86 B, `defaultreqga10→11` 44 B,
   `defaultreqbo14→15` 56 B, `defaultreqbo14→la63` 28 B (32 Borders), `menu2item1→m2i1text`
   = MenuItem+IntuiText 54 B. Lösung: Zeigertabellen (`colgad[16]`, `colbord[32]`,
   `hp[3]`, `intgad[3]`) oder ein gepacktes Struct je Gruppe; Menü-Vorlage als
   `struct { struct MenuItem mi; struct IntuiText it; }`.
2. **Alias-Labels mitten in Structs:** `viewmode` = newscreen.ViewModes,
   `winscreen` = newwindow.Screen, `idcmp` = newwindow.IDCMPFlags, `errorpnt` =
   errorin1.IText, `int1..3` = defaultreqsg1..3.LongInt (Code greift auf `int1+2` =
   Low-Word zu), `defaultreqhp1+2` = HorizPot, `nextngad` = zweite Border. → `#define`.
3. **Byte-Aliasing auf Flags:** `btst #7,xxx+13` = `Flags & GFLG_SELECTED`;
   `comdata+84` muss `(UBYTE*)&defaultreqga9.Flags + 1` sein (big-endian, Bit 7).
   `move #$3000,defaultreqla47` = `"0"`.
4. **UserData als Merker** (openreq): gadget[0].UserData Hochwort = −1 → in C
   `(ULONG)UserData` ≠ 0 o. ä., aber Layout für blitz2-Requester gleich halten (blitz2
   ruft `openreq` über comdata mit eigenen Requestern).
5. **`flink`/`filerequest`**: entfällt in C, wenn freqlib.c’s `freqcode` mitgelinkt wird
   (nur eine Kopie im Projekt). Glue für `ezfreq` als comdata-Einsprung: a0/a1/a2 →
   C, Rückgabe d0 + Z-Flag (`tst.l d0`), a3 = `reqstruct+12` (Original lässt a3 so).
   Alternativ asm-incbin + flink in `glue*.c` (dann CacheClearU ergänzen? Nein –
   Verhalten beibehalten, nur dokumentieren).
6. **`rawed`**: in C neu (console.device RawKeyConvert) – Sonderfall-Mapping als reine
   Funktion; init/cleanup als UI. Register-Interface nur intern (Aufrufer in TED).
7. **Menü-Flags** werden in der angehängten Leiste direkt verändert – in C ebenso
   (keine OnMenu-Aufrufe einführen, sonst Verhaltensänderung bei Neuzeichnen).
8. Die NewScreen-Tags (`SA_LikeWorkbench`, `SA_Pens`) brauchen NS_EXTENDED-Struct
   (`struct ExtNewScreen`).

### Eignung von `blitz2-c/tools/req2c.py`
Geeignet für die sechs Requester (gleiche Direktiven-Signaturen wie blitz2s
`optreq`/`typereq`), mit kleinen Erweiterungen:
* Parts-Liste: `ted.s` 6906–7104, 7111–7308, 7310–7946, 7971–8068; `EXPORT_RE`/`req`-Erkennung
  um `findreq|repall|errorreq|erasereq|defaultreq|prtreq` erweitern.
* `CONT` um `int1, int2, int3` (sonst hat `defaultreqsgN` nur 3 Statements →
  IndexError) und `errorpnt` (sonst `errorin1`/`errorpnt` „unknown“) ergänzen;
  danach `#define int1 defaultreqsg1.LongInt`, `#define errorpnt errorin1.IText`.
* `ds.b` im Requester wird schon ignoriert (nur st[1], st[2] gelesen) – ok.
* `(6<<16)/15` in PropInfo: wird unverändert durchgereicht – gültiger C-Ausdruck (26214).
* `dc.b '',0` (defaultreqla30) → leerer String, ok; Kommentare `;dc.l …` ok.
* Gadget-UserData wird NULL, GadgetRender auf `image`/`border` korrekt.
* **Nicht** abgedeckt: Strides (G.1) – Generator erzeugt getrennte Statics; Code muss
  über Zeigertabellen gehen (oder Generator gibt Gruppen als Arrays aus).
* Menüs (`Menu`/`MenuItem` mit Equates `mp1,mp2,mj,mwidth` und Ausdrücken `2+16+64+4`,
  `96+12`), NewScreen/NewWindow/Window-Gadgets: neue Kinds nötig oder besser von Hand
  (4 Menüs, 31 Items, je 1 Screen/Window, 2 Props, 4 Borders) – überschaubar.

---

## H. Einstufung

| Routine/Block | Klasse | Bemerkung |
|---|---|---|
| `flink` | GLUE (entfällt bei C-Filerequester) | |
| `filerequest` (Code) | UI | = freqlib.c `freqcode`, wiederverwenden |
| `rawed` init/cleanup | UI | console.device |
| `rawed` convert: Sondertasten-Mapping | KERN | reine Funktion (Rawcode→Code), RawKeyConvert-Rest UI |
| Daten Screen/Window/Menü/Requester | UI | |
| `font6`/`font8` | KERN-Daten | als `const UBYTE[]` (z. B. per `xxd -i`) |
| `undobuffer`, Such-/Ersetz-Puffer `findreqla16/17` | KERN-Daten | vom Suchkern (isithere/replace2) gelesen → für Host-Tests als normale Puffer bereitstellen; case-Flag als bool statt `findreqga8.Flags` abstrahieren |
| `int1..3`, `defaultreq*`-Flags | UI-Daten | Optionen → in C über `hscmarg/vscmarg/tabstop/tokenise/makeicons` entkoppeln |

---

## I. Auffälligkeiten / mögliche BUG(orig)

1. **`int3 = 4`, Puffer zeigt '2'** (7837/7838), `tabstop` Default 2: Ohne geladene
   Defaults-Datei übernimmt `defaults` bei jedem Klick auf Gadget 4–25 `tabstop = int3`
   → Tabstopp springt still von 2 auf 4.
2. **Slider-Initialwert** `(6<<16)/15` passt nicht zu `palit` (8,8,8); `defaults` ruft vor
   der ersten Anzeige kein `setprops` (nur `setdefs` beim Laden / Gruppe-3-Klick).
3. **`getfname3`: `cmp #1,d0 / beq outmem` ist toter Code** – Z=1 nur bei d0=0. Schlägt im
   Filerequester die 48-B-Chip-Allokation fehl, liefert er den ersten Langwort des
   uninitialisierten FIB-Puffers (Z i. d. R. 0) → TED nimmt Müll als Dateinamenzeiger.
4. `flink`: kein Cache-Flush (68040/060 copyback: relozierte Langworte evtl. noch im
   Datencache, wenn der Code ausgeführt wird); nicht idempotent; nur ein RELOC-Block.
5. COMPILER-Menü: `AllocMem` unbeprüft (2 Stellen); `escape` gibt Items frei, ohne
   FirstItem auf 0 zu prüfen (Menü ohne Einträge → FreeMem(0)); Freigabe bei noch
   angehängter Leiste (vor `freeall`).
6. COMPILER-Items Width 200 bei 23 Zeichen (184 px) + COMMSEQ-Feld → Kürzel überlappt
   evtl. den Text (`;!*!`-Kommentar 2805).
7. Menüauswertung `and #$f0` nach `lsr #1`: nur Items 0–15 (EDIT hat 12 – ok), Untermenüs
   nicht vorgesehen; MENUNUM via `and #15`.
8. `defaultreqga13 → defaultreqga18`: ga14–17 + `bo18–21` + Texte bleiben als tote Daten,
   werden von `swapbords`/`setdefs` aber weiter umgefärbt (harmlos).
9. `rawed`: OpenDevice-Ergebnis ungeprüft (Device-Basis 0 → Absturz in convert);
   AllocSignal nie freigegeben; Tabellen Code+56/+161 werden gebaut, aber nie benutzt.
10. `filerequest` liest `screen+14`/`screen+76` ohne Nulltest – bei TED unkritisch, da
    `reqstruct+12` nach `screenopen` immer gesetzt ist (vorher aufgerufen → Lesezugriff auf
    Adresse 14/76).
11. `font6`/`font8` decken nur Zeichen 0–127 ab; Zeichen ≥128 lesen hinter die Tabelle
    (font8 liegt am Hunk-Ende) – prüfen, ob Aufrufer das ausschließen.
12. `viewmode`-Initwert `$8004` (HIRES|LACE) wird immer von WB-Modes überschrieben;
    `idcmp`-Label unbenutzt.
13. Menü-Flags (ITEMENABLED) werden bei angehängter Leiste direkt geändert (OS-Regel
    verlangt ClearMenuStrip/ResetMenuStrip) – funktioniert, beibehalten.
