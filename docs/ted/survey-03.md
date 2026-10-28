# Survey 03 – ted.s Zeilen 1981–3062

Bereich: Maus im Textbereich (`mousedown`, Ziehen/Scrollen, `gomouse` = Klick in die
Label-Spalte), Goto-Requester (`gotoreq`, `gotoline`), F1/F2-Blockmarken
(`blockstart`/`blockend`), Workbench öffnen/schließen, Konsolenfenster für laufende
Programme (`openio`/`closeio`, comdata+60/64), CLI (`mycli`), comdata-Dienste
(`refgads`, `tokone`, `tokall`, `detokallz`, `actgad`, `extalloc`), Hilfe-Taste (`help`),
Cleanup-Rückruf (`doclean`), COMPILER-Menü bauen (`getstuff`), Label-Spalte
(„mousables“: `makemice`/`freemice`/`remember*`/`mforget2`/`showmice`), `a0toname`,
`printmem`, Speicherfehlerbehandlung (`alloc2`/`outmem`/`extalloc`/`doalert`).

Alles liegt in der einen Code-Section; Daten im Code (`gotoreq*`, `ioname`, `helphex`,
`newctop` …) werden zur Laufzeit beschrieben → in C beschreibbare (nicht-`const`) Daten.

Lokale Labels: `oneloop` (2290) ist ein Top-Level-Label **mitten in `mousedown`**; die
danach folgenden `.skip`, `.skip2`, `.nben` gehören daher zum Scope `oneloop`, nicht
`mousedown` (bei der Übersetzung beachten, es gibt gleichnamige `.skip`/`.skip2` davor).

---------------------------------------------------------------------------------------------
## A. Routinen

Konvention „Handler“: Einsprung über `jumptable`/`menutable` aus `mainedit` (ted.s:156–171):
`jsr (a0)` mit a4 = `thisitem`, d7 = Tastencode/Ereignis, d6 = Menü-/Gadget-Nr.; auf dem
Stack liegt darüber das Flag-Wort (vprop neu zeichnen); `memstack` = a7 vor dem `jsr`.
Rückkehr per `rts` nach `memcont`.

### Maus

- `gomouse` (1981) : Klick in die Label-Spalte. **Kein Unterprogramm**: nur per `bcc gomouse`
  aus `mousedown` (2007) erreicht (Tail-Branch, `rts` geht an `mainedit`). ein: d2 = Spalte
  (Bildschirm, Zeichen), d3 = Zeile (Bildschirm) aus `getmousexy`. Ignoriert, wenn
  d2 < `edcols`+2 (2 Spalten Abstand) oder d3 ≥ `nummice` (Wort-Zugriff auf `dc.l`, s. G).
  Läuft d3+1 Knoten der `mice`-Liste ab (`lea mice,a0` / `move.l (a0),a0` / `dbf`), sucht
  den Zeilenzeiger `4(node)` in der Zeilenliste ab `firstitem` (Index in d0) und springt
  **`beq gotolined0`** (Tail-Call, d0 = Zeilenindex ab 0). Nicht gefunden → `rts`.
  Index der Label-Spalte = Bildschirmzeile, die Liste wird nicht gescrollt.
- `mousedown` (2001) : `jumptable`-Code 253 (Flag 1). Textauswahl mit der Maus.
  ein: a4 = `thisitem`. aus: nichts; Endet per Tail-Call `bra showall` / `bra cursor2`
  oder `rts`. Ablauf:
  1. `getmousexy` (4015: d0 = MouseX px, d1 = MouseY px, d2 = Spalte, d3 = Zeile; N=1
     wenn außerhalb des Textbereichs) – N → `rts`. d2 ≥ `edcols` → `gomouse`.
     curstop+d3 ≥ `numlines` → `rts`.
  2. **Stack-Frame**: push `btopx/btopy` (als `.l`), `bbotx/bboty` (als `.l`). Existiert ein
     Block (`bboty` ≠ −1): `blokdis` (setzt `bboty`=−1, `bstx`=−1) + `showall`.
  3. Anker: `bloktopx/y` = `blokbotx/y` = (cursoff+d2, curstop+d3); `showprops2`;
     d5/d6 = letzte Endposition (Spalte absolut, Zeile Bildschirm); `invflag`=0;
     push `curstop`, `cursoff`, a4.
  4. Schleife (`.loop`): `invertarea`, dann **`getinput2` (nicht blockierend) → Busy-Poll**.
     d7 = −4 (SELECTUP, gesetzt in `handled0`, ted.s:5915) → `.mouseup`.
     Sonst `getmousexy`: außerhalb und MouseY < `topskip` → `.mup` (eine Zeile hoch
     scrollen: `scrdownall`, a4 = lastitem, curstop−1, `showtopitem`, d6+1; Kommentar
     „currently FUCKED“); außerhalb unten → eine Zeile runter (`scrupall`, a4 = nextitem,
     curstop+1, `showbotitem`, d6−1), nur wenn curstop+edrows < numlines. Danach
     Korrektur der Invertierung, falls die Auswahl oberhalb des Ankers liegt.
     Innerhalb: d2 += cursoff (absolute Spalte), Zeile auf numlines−1 begrenzt;
     MouseX = 0 (d0) → horizontal links scrollen (cursoff −= 2, `scrollrite`), Spalte −
     cursoff ≥ edcols → rechts scrollen (cursoff += 2, `scrollleft`), jeweils mit
     temporärem `btop/bbot` = Auswahl, `sortblox`, danach `blokdis`.
  5. `.skip2`: Position unverändert → `.loop`. Sonst `blokbotx/y` setzen, Richtung zum
     Anker bestimmen (`.above`/`.below`); gleiche Richtung wie `invflag` → nur Differenz
     invertieren (`orderem` + `invertarea2`, ±1-Korrektur); Richtungswechsel (`.fix`) →
     `invflag` umdrehen, alte Fläche Anker…alt (auf Bildschirm geklemmt) und neue Fläche
     Anker…neu invertieren.
  6. `.mouseup`: `newctop/newcoff` = aktuelle curstop/cursoff; pop a4 → `thisitem`,
     `cursoff`, `curstop` (= **Scrollposition vor dem Ziehen wird wiederhergestellt**).
     `blokbotx` < 0 → Ende der Vorzeile (`blokboty`−1, x = chcols−1) bzw. x = 0, wenn
     schon auf der Ankerzeile; ≥ chcols → chcols−1.
     Auswahl > 1 Zeichen: `bloken`, gesicherte btop/bbot verwerfen (**`addq #8,a7`**),
     btop/bbot = Auswahl (`.l`-Kopie), `sortblox`, `showhprop`, `bra showall`.
     `.one` (Klick ohne Ziehen): btop/bbot vom Stack zurück (alter Block bleibt!),
     Cursor auf den Anker: cursx = bloktopx − cursoff; `oneloop` (2290) läuft von
     `thisitem` (Zeile curstop+cursy) per next/last zur Zeile `bloktopy`, setzt cursy,
     `thisitem`, `calcpos`. Alter Block vorhanden → `bloken` + `bra showall`; sonst
     `bra showall` wenn sich curstop/cursoff gegenüber `newctop/newcoff` geändert haben,
     sonst `bra cursor2`.
- `oneloop` (2290) : nur interne Schleife von `mousedown` (s. o.), kein eigener Einsprung.

### Goto / Blockmarken / Workbench

- `actgad` (2409) : comdata+68 und intern (574, 869, 899, 2428). ein: a0 = Gadget. `Delay(8)`
  (dos, a6 = dos), dann `ActivateGadget(a0, req_window, NULL)`. aus: d0 = BOOL von Intuition.
  zerstört: a1, a2 (=0), a6 (= int), d0/d1. a2 vom Aufrufer wird ignoriert (immer 0).
- `gotoline` (2420) : `menutable` $22. Setzt StringInfo `BufferPos` (sg1+8) und `DispPos`
  (sg1+12) = 0, `gotoint` (LongInt) = 0, Puffer = "", `openreq(gotoreq)`, `actgad(ga1)`.
  Schleife `getinput` bis d7 = −1; d6 = 1 (Return im String) / 2 (OK) → `.ok`, 3 (CANCEL)
  → `rts`; andere → weiter. Kein `closereq` hier: `getinput` schließt den Requester bei
  Gadgets mit ENDGADGET (Activation Bit 2) selbst (ted.s:5888–5894); alle drei haben es.
  `.ok`: d0 = `gotoint`; 0 oder > numlines (Wortvergleich!) → `beq/bhi beepem` (Tail);
  sonst d0−1 → `bra gotolined0` (Tail).
- `blockstart` (2458) : `jumptable` 129 (F1). `bstx` = cursoff+cursx, `bsty` = curstop+cursy.
- `blockend` (2468) : `jumptable` 130 (F2). `bstx` < 0 → `bmi beepem` (Tail). Schreibt
  zunächst **`move d0,btopx` mit undefiniertem d0** (Tastencode-Byte aus mainedit, s. I).
  Ende = Cursor (d0,d1). Gleich Start → `blokdis` + `bra showall`. Sonst: Ende x = 0 und
  y > 0 → Ende = (chcols, y−1); dann x−1 (exklusives Ende) → `bbotx/bboty`, `btopx/btopy`
  = Start; `sortblox`, `bloken`, `bra showall`.
- `closewb` (2501) : `menutable` $70. `CloseWorkBench()`. (auskommentiert: Screen-Typ
  patchen.)
- `openwb` (2507) : `menutable` $80. `OpenWorkBench()`, `ScreenToFront(myscreen)`,
  Tail `jmp ActivateWindow(mywindow)` (a6 = int aus `callint`).

### Konsolenfenster / CLI

- `findio` (2513) : ein: a0 = Name (ohne Länge), d0 = Länge, `dos` = DOSBase. Läuft
  `DOSBase->dl_Root(34)->rn_Info(24, BPTR)->di_DevInfo(4, BPTR)` und die
  DevInfo-Kette (`dvi_Next` @0, `dvi_Name` BSTR @40) ab, Vergleich Länge + Zeichen
  (case-sensitiv). aus: d0 = 0, **Z=1 wenn gefunden**; d0 = −1, Z=0 sonst.
  zerstört: d1, a1, a2, a3. Kein Forbid/LockDosList.
- `calciorows` (2541) : ein: `gr` = GfxBase. aus: d0 = 200 wenn `GfxBase->DisplayFlags`
  Bit 0 (NTSC, Byte @207) gesetzt, sonst 256. zerstört a1.
- `openio` (2548) : comdata+60 (blitz2.s:1697 vor RUN) und `mycli`. Patcht `hiteasc`
  (3 Leerzeichen), Höhe d0 = `newscreen+6` (Höhe des Editor-Screens) wenn
  `screensopen`, sonst `calciorows`; `numtoascii` schreibt die Ziffern in `hiteasc`.
  `FindTask(0)` → a3, sichert `pr_CIS` (156) → `oldih`, `pr_COS` (160) → `oldoh`.
  `findio("NEWCON",6)`: gefunden → `Open("NEWCON:0/0/640/<h>/Blitz 2 CLI Window", 1006)`,
  sonst `Open("CON:…")`. Handle → `myhandle`; 0 → `rts` mit **Z=1**. Sonst
  pr_CIS = pr_COS = Handle, `WBenchToFront()`, `rts` – **Z danach undefiniert** (Flags
  aus Intuition; `mycli` testet `beq`!). zerstört d0–d3, a0–a3, a6.
  Kein `movem` – blitz2 sichert selbst nichts Besonderes (Asm-Aufrufer rufen danach nur
  `calcvbr`).
- `closeio` (2597) : comdata+64 (blitz2.s:1722) und `mycli`. `FindTask(0)`, pr_CIS/pr_COS
  = `oldih/oldoh`, `Close(myhandle)` (auch wenn 0, s. I). Wenn `screensopen`:
  `ScreenToFront(myscreen)` + Tail `jmp ActivateWindow(mywindow)`; sonst `rts`.
- `mycli` (2616) : `menutable` $60. `openio`; Z=1 → `rts`. `Execute("", myhandle, 0)`
  (d1 = `nullcom` = Leerstring, d2 = Input, d3 = 0) = interaktive Shell im Fenster bis
  EndCLI; dann Tail `bra closeio`.

### comdata-Dienste: Gadgets / Tokenizer

- `refgads` (2629) : comdata+28. ein: a0 = Gadget, d0 = Anzahl. Tail
  `jmp RefreshGList(a0, req_window, NULL, d0)`.
- `tokone` (2636) : comdata+56. ein: a0 = 0-terminierte Zeile. d1 = strlen, a2 = a1 = a0
  (**in-place**), `bpl tokit2` (Tail; immer genommen bis 32767 Zeichen). aus (von
  `tokit2`): Zeile tokenisiert, d5 = Länge inkl. 0. zerstört praktisch alles außer a4.
- `tokall` (2648) : comdata+20. `tokenise` ≠ 0 → `bne beepem` (Tail). `not tokenise`
  (→ $FFFF). Fällt durch in
- `tokall2` (2653) : auch Einsprung aus 557 (Defaults-Requester). Für alle `numlines`
  Zeilen (Zähler als Wort auf dem Stack): d1 = numchars−1 (Länge ohne 0; < 0 → Zeile
  überspringen), `tokit2(chars → charbuff)`, `thisitem` = a4, `makeline` (ersetzt die Zeile,
  a4 = neue Zeile). Danach Tail `bra detokallc` (= `calcitem` + `showall`).
- `detokallz` (2670) : comdata+16. `tokenise` = 0 → `beq beepem` (Tail). `clr tokenise`.
  Fällt durch in
- `detokallz2` (2673) : auch aus `detokall2`. Für jede Zeile `detok(chars → charbuff)`,
  d5 = Länge+1, `makeline`. **Zeichnet nicht neu und lässt `thisitem` = letzte Zeile**
  (erst `detokallc` korrigiert). `rts`.
- `detokall` (2689) : ohne Referenz (toter Einsprung). Prüft/löscht `tokenise`, fällt in
- `detokall2` (2692) : Einsprung aus 556 (Defaults). `bsr detokallz2`, fällt in
- `detokallc` (2694) : `calcitem` + Tail `bra showall`. Auch Ziel von `tokall2`.

### Hilfe, Cleanup, Menü

- `help` (2697) : `jumptable` 139 (HELP). ein: a4 = `thisitem`. `tokenise` = 0 → `beepem`.
  Läuft die tokenisierte Zeile bis zur Cursorspalte (cursoff+cursx) ab; zählt
  Anzeigespalten (Token = Länge des Namens). Merkt den **letzten Token, der an oder vor
  der Cursorspalte beginnt** (nicht zwingend unter dem Cursor): `helptoke` = Token-Nr.,
  d1 = Name (`findtoke` → a3 = Knoten+6), a2 = Hilfetext (nach dem Namen).
  Kein Token → Tail `beq showfname`. Sonst `newsctit` = Name + (" " außer Hilfe beginnt
  mit "(") + Hilfe, `ShowTitle(myscreen, TRUE)`. Wenn `qualifier` & $80 (rechte
  Amiga-Taste): `helphex` = 4 Hex-Ziffern von `helptoke`, Tail
  `jmp Execute("blitz2:help XXXX", 0, 0)` (synchron).
- `doclean` (2771) : intern (684 fileopen, 1035, 1595 new). Ruft `cleanup`
  (edstruct+20, bei blitz2 = 0) mit `movem.l d1-d7/a0-a6` gesichert; d0 ebenfalls
  gesichert → erhält **alle** Register.
- `getstuff` (2781) : einmal aus `outside` (79), nur wenn `external`. `tokens` ≠ 0 →
  `tokenise` = 1, `defaultreqga8+12` |= $80 (Gadget-Flag SELECTED = Häkchen
  „Tokenise“). `menitems` ≠ 0 → `ClearMenuStrip(mywindow)`, baut aus `menitems`
  (edstruct+4) ein Menü „COMPILER“ als Kopie von `menu2` + je Eintrag eine Kopie von
  `menu2item1`+IntuiText (s. E), hängt es an **`menu2.NextMenu`** (statische Daten
  gepatcht), Tail `jmp SetMenuStrip(mywindow, menu1)`. Format `menitems`:
  `Titel\0 { Text\0 Kürzelbyte(0 = keins) }* \0`.

### Label-Spalte („mice“)

- `makemice` (2850) : aus `showmice`. Für jede Zeile: erstes Nicht-Leerzeichen =
  `label`-Zeichen (edstruct+26, „.“) → `remember2(a0 = Zeile)`; setzt dabei
  `mousealt` = d1 (Labelzeichen, ≠ 0). Sichert a1. zerstört d0, d1, a0, a6.
- `freemice` (2869) : aus `showmice` und 1036. Gibt `nummice` Knoten (8 Bytes) frei,
  `mousealt` = alte Anzahl, `nummice` = 0; `mice` bleibt hängend (nur über Zähler benutzt).
- `remember` (2883) : aus `makeline` (3822) und 3372, ein: a0 = Zeile (ignoriert). Nur
  `mousealt` = −1 → nach dem Handler baut `memcont` die Liste komplett neu (`showmice`).
- `remember2` (2887) : ein: a0 = Zeile. `AllocMem(8, MEMF_PUBLIC)` (nicht gelöscht);
  Fehlschlag → `beq outmem` (mit a0 noch auf dem Stack – egal, outmem setzt a7 zurück).
  Hängt den Knoten **ans Ende** (Liste mit `nummice`−1 Schritten abgelaufen), `4(node)` = a0,
  `nummice`+1. **`next` des neuen Knotens bleibt uninitialisiert.** Sichert d0/d1/a1/a2.
- `mforget2` (2912) : aus 3717, 4595. Nur `mousealt` = −1 (wie `remember`).
- `showmice` (2916) : aus `outside` (105), `.tomain` (172 = memcont, wenn `mousealt`).
  `label` = 255 (kein blitz2) → `rts`. `freemice`, `makemice`. Keine Labels und
  `mousecols` ≠ 0 → `mousecols` = 0, `screenclose`, Tail `bra screenopen2`. Labels und
  `mousecols` = 0 → `mousecols` = 12, `screenclose`, `screenopen2` (zeichnet alles neu).
  Dann für Zeile 0…edrows−1: Label-Text (bei `tokenise` erst `detok` nach **`charbuff`**)
  ohne führende Leerzeichen bis 0/`colon`/Leerzeichen, max. `mousecols` Zeichen, Rest mit
  Leerzeichen; Ausgabe per `qprint` in `bitmaps+4` (zweite Plane) ab Byte
  `edcols+2`, Zeilenabstand `fontmul`.

### Sonstiges

- `a0toname` (2990) : aus `.tomain` (87). ein: a0 = Pfad (evtl. in `"`). Soll in `filepath`
  (Verzeichnis) und `filename` (Name) zerlegen; vergleicht aber **d0 statt des kopierten
  Zeichens** (s. I). Effektiv: `filepath` = "", `filename` = ganzer String (schließendes
  `"` entfernt). Funktioniert, weil vor `filepath` das Wort `dc ' :'` (6139) steht: `.tomain`
  prüft `-2(a0)` = ':' und hängt dann kein '/' an.
- `printmem` (3019) : init (73, 106), `memcont` (169, wenn `memalt`). `AvailMem(MEMF_LARGEST|
  MEMF_PUBLIC)` / 1024, `numtext` = "     \0", `numtoascii` (nur unteres Wort!), Tail
  `bra pstats` an `statsloc+38`. zerstört d0–d2, a0, a1, a3, a6.
- `alloc2` (3033) : intern (1381, 1872, 1899, 3353, 3809 `makeline`). ein: d0 = Größe,
  d1 = Flags, **a6 = ExecBase**. `memalt` = −1, `AllocMem`. aus: d0 ≠ 0, Z=0. Fehlschlag →
  `beq outmem` (kehrt nicht zurück).
- `outmem` (3038) : **kehrt nie zurück**. `doalert`; `filehand` ≠ 0 → `Close` (ohne
  `filehand` zu löschen); `curstop` = `cursy` = 0, `thisitem` = `firstitem`, `showall`;
  **`move.l memstack,a7` + `bra memcont`** (longjmp in die Hauptschleife, verwirft alle
  Frames dazwischen, auch die von blitz2). Ziele: 1154, 1406, 1779, 2894, 3036, 3054,
  3498, 3620 (`openreq`).
- `extalloc` (3049) : comdata+88 (`doallocmem`, blitz2.s:3379, ~10 Aufrufer). ein: d0, d1,
  **a6 = ExecBase** (Aufrufer setzt `move.l 4.w,a6`). Erfolg → `rts` mit d0, Z=0.
  Fehlschlag → `closereq` (nur oberstes Requester-Fenster, harmlos wenn keins) +
  `bra outmem` – **verlässt den Compiler mitten im Lauf**.
- `doalert` (3057) : Tail `jmp DisplayAlert(0 = RECOVERY_ALERT, alertstuff, 40)`.

---------------------------------------------------------------------------------------------
## B. Persistente Registerkonventionen

- Handler aus `jumptable`/`menutable`: a4 = `thisitem` beim Einsprung (`help` liest
  `chars(a4)`, `mousedown` scrollt a4 mit und sichert/restauriert es). d7 = Ereigniscode.
- Innerhalb `mousedown`: d2 = aktuelle Spalte (**absolut**, cursoff schon addiert) bzw.
  Bildschirmspalte direkt nach `getmousexy`; d3 = Bildschirmzeile; d5/d6 = letzte
  Endposition (gleiche Koordinaten); `invflag` = Auswahl liegt vor dem Anker;
  Stack-Frame (von oben): a4.l, cursoff.w, curstop.w, bbotx/bboty.l, btopx/btopy.l.
  `invertarea` nimmt (d0,d1)–(d2,d3) mit x = Spalte in der Zeichen-Bitmap (chcols breit),
  y = Bildschirmzeile.
- a6 = ExecBase als Eingangsbedingung für `alloc2`/`extalloc`; `calldos`/`callint`-Makros
  laden a6 selbst (`dos`/`int`). Viele Routinen hinterlassen a6 = int/dos.
- `memstack` = a7 im Hauptschleifen-Dispatch; `outmem` setzt darauf zurück.
- comdata-Routinen dieses Bereichs (`openio`, `closeio`, `tokone`, `tokall`, `detokallz`,
  `refgads`, `actgad`, `extalloc`) sichern **keine** Register (kein `movem`); einziges
  `movem`-Sichern aller Register: `doclean` (Rückruf nach außen), `remember2`
  (d0/d1/a1/a2), `makemice` (a1). blitz2-Asm ruft `extalloc` mit a6 = Exec und erwartet
  danach nur d0.

---------------------------------------------------------------------------------------------
## C. Globals (in diesem Bereich definiert)

| Name | Zeile | Typ | Init | Zweck |
|---|---|---|---|---|
| `newctop` | 2318 | w | 0 | curstop beim Loslassen (mousedown) |
| `newcoff` | 2319 | w | 0 | cursoff beim Loslassen |
| `subs` | 2322 | Label | – | Marker, unbenutzt |
| `gotoreq` … `gotoreqga3` | 2324–2407 | Intuition-Daten | – | Goto-Requester (s. u.) |
| `gotoint` | 2394 | l,l | 0 | = StringInfo.LongInt (+28) / AltKeyMap (+32) von `gotoreqsg1` |
| `gotoreqla8` | 2395 | 6 B | 0 | Puffer des String-Gadgets |
| `ioname` | 2446 | 3 B | "NEW" | Präfix, **ohne 0**, läuft in `ioname2` weiter |
| `ioname2` | 2447 | 12 B | "CON:0/0/640/" | ohne 0, läuft in `hiteasc` weiter |
| `hiteasc` | 2448 | str | "   /Blitz 2 CLI Window" | Höhe (3 Zeichen) wird gepatcht |
| `oldih`/`oldoh` | 2451/2452 | l | 0 | gesicherte pr_CIS/pr_COS |
| `myhandle` | 2453 | l | 0 | Konsolen-FileHandle |
| `bstx` | 2455 | w | −1 | F1-Blockstart x (−1 = keiner; `blokdis` setzt −1) |
| `bsty` | 2456 | w | 0 | F1-Blockstart y |
| `nullcom` | 2614 | l | 0 | Leerstring für `Execute` |
| `helptoke` | 2767 | w | 0 | letzte Token-Nr. für Hilfe |
| `helpex` | 2768 | str | "blitz2:help " | **ohne 0**, läuft in `helphex` weiter |
| `helphex` | 2769 | 5 B | "1234",0 | Hex-Token-Nr., gepatcht |

Goto-Requester (Intuition-Strukturen, nur als Vorlage für `openreq`):
- `gotoreq`: `struct Requester` (112 B): Left/Top 140/58, 184×40; ReqGadget = `gotoreqga1`,
  ReqBorder = `gotoreqbo1`, ReqText = `gotoreqin1`.
- Borders (je 12 B + Koordinaten 3 Paare): `bo1`→`la2`→`bo2`→`la5` (Rahmen 184×40, 3D),
  `bo3`→`la10` (OK-Rahmen 40×12), `bo4`→`la14` (CANCEL 72×12). Die `gotoreqlaN` sind teils
  Koordinatenfelder, teils selbst Borders (`la2`, `la5`, `la10`, `la14`).
- IntuiTexts: `in1` „LINE TO GOTO:“ (12,6), `in2` „ OK “, `in3` „ CANCEL “.
- Gadgets (44 B): `ga1` String (Type 4, Activation $805 = LONGINT|ENDGADGET|RELVERIFY,
  124,6, 48×8, ID 1, SpecialInfo `gotoreqsg1`) → `ga2` Bool (ID 2, OK) → `ga3` Bool (ID 3).
- `gotoreqsg1`: `struct StringInfo` (36 B): Buffer `gotoreqla8`, UndoBuffer `undobuffer`
  (7105), MaxChars 6; LongInt = `gotoint`.
- Laufzeit-Schreibzugriffe: `gotoline` → sg1+8 (BufferPos), sg1+12 (DispPos), `gotoint`,
  `gotoreqla8[0]`. `openreq` (3605ff) → `UserData` (+40, Wort) des ersten Gadgets = −1
  als „schon verschoben“, und **einmalig** LeftEdge(+4) += 6, TopEdge(+6) += 12 aller
  Gadgets der Kette. Intuition schreibt Buffer/LongInt/Flags.

Von außerhalb benutzt und hier wichtig: `nummice` (6163, **`dc.l`, aber nur als Wort
benutzt** = oberes Wort), `mice` (6164, Kopfzeiger), `mousealt` (6165, w), `memalt`,
`memstack`, `filehand`, `tokenise`, `bloktop*/blokbot*/btop*/bbot*` (6085–6092,
benachbart, paarweise als `.l` gelesen), `invflag`, `numtext` (6 B), `newsctit` (32+80 B),
`filepath` (192 B, davor Sentinel `' :'`), `filename` (64 B).

---------------------------------------------------------------------------------------------
## D. Selbstmodifikation / Daten im Code

- Keine Patches von Befehlen. Keine Einsprünge in `incbin`-Blöcke in diesem Bereich.
- Geschriebene Daten in der Code-Section: `newctop/newcoff`, Goto-Requester (s. C, auch
  durch `openreq` und Intuition), `hiteasc` (Höhe), `oldih/oldoh/myhandle`, `bstx/bsty`,
  `helphex`, `helptoke`; `menu2.NextMenu` (6554, durch `getstuff`),
  `defaultreqga8+12` (Gadget-Flags, `getstuff`).
- String-Verkettung durch Adjazenz: `ioname`+`ioname2`+`hiteasc` ergibt
  „NEWCON:0/0/640/hhh/Blitz 2 CLI Window“, `ioname2` allein „CON:…“;
  `helpex`+`helphex` = „blitz2:help XXXX“. In C: `sprintf`/Konstanten.
- `findio` liest DOS-interne Strukturen direkt (DosInfo/DevInfo, BPTR/BSTR).

---------------------------------------------------------------------------------------------
## E. Heap-/Listenstrukturen

- **Mice-Knoten** (8 B, `AllocMem(8, MEMF_PUBLIC)`, `remember2`): +0 `next.l` (beim
  letzten **uninitialisiert**), +4 `line.l` (Zeiger auf Zeilen-Item). Kopf `mice`, Anzahl
  `nummice` (Wort im oberen Teil des `dc.l`). Alle Durchläufe zählen über `nummice`,
  außer `showmice` (liest einmal `next` des letzten Knotens, s. I). Wird bei jeder
  Änderung (`mousealt`) komplett neu aufgebaut.
- **Zeilen-Item** (Zeile 16–20): +0 `nextitem.l`, +4 `lastitem.l`, +8 `numchars.b`
  (Länge **inkl.** 0-Terminator), +9 `chars[]` (0-terminiert, Tokens = 2 Bytes mit
  Bit 7 im ersten). `lenitem` = 9. Neu angelegt durch `makeline` (3803) aus `charbuff`.
- **COMPILER-Menü** (`getstuff`, MEMF_PUBLIC, nie freigegeben, nicht auf NULL geprüft):
  `struct Menu` 30 B (Kopie `menu2`): NextMenu = 0, LeftEdge = 248, Width = (strlen+1)*8,
  MenuName = Zeiger in `menitems` (blitz2-Speicher), FirstItem-Kette ab +18.
  Je Eintrag 54 B (Kopie `menu2item1` 34 B + IntuiText 20 B): NextItem 0, TopEdge = 12*i,
  Width 200, Flags (+12) ITEMTEXT|ITEMENABLED|HIGHCOMP, COMMSEQ (4) nur mit Kürzel,
  ItemFill (+18) = eigene IntuiText (+34), Command (+26) = Kürzel, IText (+46) = Zeiger in
  `menitems`.
- `charbuff`: Arbeitspuffer der aktuellen Zeile; wird von `tokall`, `detokallz2` und
  **`showmice`** (bei `tokenise`) überschrieben.

---------------------------------------------------------------------------------------------
## F. Externe Aufrufe

| Bibliothek | LVO | Wo / wozu |
|---|---|---|
| exec | FindTask(0) | `openio`, `closeio` (pr_CIS @156, pr_COS @160) |
| exec | AllocMem | `remember2` (8, PUBLIC), `getstuff` (30/54, PUBLIC), `alloc2`, `extalloc` |
| exec | FreeMem | `freemice` |
| exec | AvailMem(LARGEST\|PUBLIC) | `printmem` |
| dos | Delay(8) | `actgad` (Fenster aktiv werden lassen) |
| dos | Open(…, 1006 = MODE_NEWFILE) / Close | `openio` / `closeio`, `outmem` (`filehand`) |
| dos | Execute | `mycli` ("", h, 0), `help` ("blitz2:help XXXX", 0, 0) |
| intuition | ActivateGadget, RefreshGList | `actgad`, `refgads` |
| intuition | CloseWorkBench, OpenWorkBench, WBenchToFront | `closewb`, `openwb`, `openio` |
| intuition | ScreenToFront, ActivateWindow | `openwb`, `closeio` |
| intuition | ShowTitle | `help` |
| intuition | ClearMenuStrip, SetMenuStrip | `getstuff` |
| intuition | DisplayAlert | `doalert` |
| intuition | DisplayBeep | indirekt über `beepem` (1054) |

- Direkte Strukturzugriffe: DOSBase `dl_Root` (34) → DosInfo (24) → DevInfo (4/40);
  GfxBase `DisplayFlags` (206/207); Screen MouseY/MouseX (16/18, in `getmousexy`).
- Rückruf nach blitz2: `cleanup` (edstruct+20) in `doclean` – bei blitz2 0.
- Von blitz2 gerufen: comdata+16 `detokallz`, +20 `tokall`, +28 `refgads`, +56 `tokone`,
  +60 `openio`, +64 `closeio`, +68 `actgad`, +88 `extalloc`.
- Timing: `mousedown` pollt ohne Warten (`getinput2`) – Scrollgeschwindigkeit beim Ziehen
  = CPU-Geschwindigkeit. `actgad`: 8 Ticks Delay.
- Kein Hardware-Zugriff, keine Interrupts. Ausgabe in die Bitplanes über `qprint`,
  `invertarea` (andere Chunks).

---------------------------------------------------------------------------------------------
## G. Sonderbehandlung in C

- **`outmem` = longjmp**: `memstack`/`memcont` als `setjmp` im Dispatch von `mainedit`;
  `outmem` muss auch aus blitz2-Code (über `extalloc`) heraus funktionieren → verwirft
  blitz2-Frames. Im C-Port: `longjmp` über die `callregs`-/REGSTUB-Grenze (blitz2-c
  `glue.c`) hinweg; blitz2-Zustand (Compiler-Flags, offene Dateien, `Forbid`?) bleibt
  inkonsistent wie im Original. Vor der ersten Hauptschleife ist `memstack` = 0.
- comdata-Einsprünge brauchen Register-Stubs (glue): `openio` (Z-Flag-Rückgabe, für C
  besser: Rückgabe Handle ≠ 0), `extalloc` (d0/d1/a6 rein, d0 raus), `tokone` (a0),
  `refgads` (a0,d0), `actgad` (a0 → d0), `tokall`/`detokallz` (keine Parameter).
- Condition-Code-Rückgaben: `findio` (Z=1 gefunden), `openio` (Z, s. I), `alloc2`/`extalloc`
  (Z=0), `getmousexy` (N).
- Tail-Calls zuhauf (`beepem`, `showall`, `cursor2`, `gotolined0`, `showfname`, `closeio`,
  `detokallc`, `tokit2`, `pstats`, Intuition-`jmp`); Durchfälle `tokall`→`tokall2`,
  `detokallz`→`detokallz2`, `detokall`→`detokall2`→`detokallc`.
- `mousedown`: manueller Stack-Frame mit `addq #8,a7`-Verwerfen und zwei verschiedenen
  Pop-Pfaden → in C lokale Variablen (gespeichertes btop/bbot, curstop/cursoff/thisitem).
- Wort-Paare als Langwort: `move.l bloktopx,btopx` (x+y), `move.l blokbotx,bbotx`,
  `cmp.l` von (x:y) zur Gleichheit, `move.l cursx,-(a7)` in `showprops2` → in C Struktur
  `{WORD x, y;}` bzw. beide Felder kopieren; Reihenfolge der Variablen 6085–6092 nicht
  ändern, falls andere Chunks ebenso `.l` benutzen.
- `nummice` ist `dc.l`, wird aber nur wortweise (oberes Wort, Big Endian) benutzt → in C
  `UWORD nummice` (das untere Wort ist toter Platz; prüfen, ob irgendwo `.l` gelesen wird).
- `tokenise`: Werte 0/1 (`getstuff`) und $FFFF (`tokall`: `not`); nur ≠ 0 zählt –
  außer in 553 (`cmp tokenise,6(a0)`): dort Vergleich mit dem Gadget-Wert → C: Werte
  exakt nachbilden.
- `numtoascii` arbeitet nur auf dem unteren Wort (`printmem` > 64 MB, `openio`).
- Unsigned-Vergleiche (`bcs/bcc/bhi`) vs. signed (`blt/bgt/ble`) in `mousedown`/`sortblox`/
  `orderem` gemischt (Spalte −1 möglich!) – exakt übernehmen.

### H. Einstufung

| Routine | Klasse | Bemerkung |
|---|---|---|
| `gomouse` | UI | Suche Zeile-Index aus Zeilenzeiger wäre KERN (Hilfsfunktion `line_index(item)`) |
| `mousedown`, `oneloop` | UI | Blockkoordinaten-Normalisierung (`.mouseup`: blokbotx<0/≥chcols) als KERN-Helfer abtrennbar |
| `gotoline` | UI | Prüfung 1…numlines KERN-trivial |
| `actgad`, `refgads` | UI | comdata-Stub nötig (GLUE) |
| `blockstart`, `blockend` | KERN + Hook | Koordinatenlogik KERN; `blokdis`/`bloken`/`showall`/`beepem` als UI-Hooks |
| `closewb`, `openwb` | UI | |
| `findio` | UI (OS-intern) | DOS-Strukturen; in C `LockDosList`/`FindDosEntry` oder 1:1 |
| `calciorows`, `openio`, `closeio`, `mycli` | UI | `openio`/`closeio` comdata → GLUE-Stub |
| `tokone` | KERN | GLUE-Stub für comdata; in-place-Tokenizer |
| `tokall`/`tokall2`, `detokallz`/`detokallz2`, `detokall*`, `detokallc` | KERN + Hook | Zeilenschleife KERN; `beepem`, `calcitem`+`showall` als Redraw-Hook; `makeline` ruft `remember` (nur Flag) |
| `help` | KERN + UI | Token-Suche an Spalte (Name/Hilfetext/Token-Nr.) KERN, host-testbar; `ShowTitle`/`Execute` UI |
| `doclean` | GLUE | Rückruf nach blitz2 mit Registersicherung |
| `getstuff` | UI | Parsing von `menitems` KERN-fähig |
| `makemice`, `freemice`, `remember`, `remember2`, `mforget2` | KERN | Liste + AllocMem; `outmem` als Fehler-Hook |
| `showmice` | UI | Label-Extraktion (Text bis `:`/Leerzeichen, max. 12) als KERN-Helfer abtrennbar |
| `a0toname` | KERN | reine Stringlogik (Bug nachbilden oder markieren) |
| `printmem` | UI | `AvailMem` + Statuszeile |
| `alloc2` | KERN | AllocMem-Abstraktion; Fehler → `outmem` (longjmp) |
| `outmem` | GLUE | longjmp + UI (Alert, showall) |
| `extalloc` | GLUE | comdata-Stub, kehrt bei Fehler nicht zurück |
| `doalert` | UI | |

---------------------------------------------------------------------------------------------
## I. Auffälligkeiten / mögliche Bugs des Originals

1. `a0toname` (2996ff): `cmp.b #':',d0` / `cmp.b #'/',d0` testen **d0** statt des kopierten
   Zeichens (`move.b (a0)+,(a1)+` lädt d0 nicht). Normalerweise harmlos (filepath = "",
   filename = alles, Sentinel `' :'` vor `filepath`). Aber d0 beim Einsprung =
   Rückgabewert von `LoadSeg("blitz2:defaultdbug")` (blitz2.s:1441): ist dessen
   Low-Byte zufällig $3A (':') bzw. $2F ('/'), wird der Dateiname zerstört (filename = ""
   bzw. nur letztes Zeichen). BUG(orig).
2. `blockend` (2472): `move d0,btopx` mit undefiniertem d0, bevor überhaupt etwas
   berechnet ist; im Pfad „Start = Ende“ bleibt der Müll in `btopx` (Block ist danach aber
   per `blokdis` aus). Wirkungslos, aber nachbilden/markieren.
3. `blockend`: Cursor auf (0,0) als Ende → `bbotx` = −1 (kein Zeilenumbruch-Sonderfall
   für Zeile 0). Ebenso Ende x = 0 mit y > 0 → Ende = (chcols−1, y−1).
4. `remember2`: `next` des neuen Knotens nicht initialisiert (MEMF_PUBLIC ohne CLEAR).
   `showmice` liest nach dem letzten Knoten `move.l (a4),a4` (a4 = Müll) und beim
   nächsten Durchlauf erneut `(a4)` aus einer **Müll-Adresse** (nur durch `cmp #0,a4`
   geschützt) → möglicher Adressfehler (ungerade Adresse) auf 68000. BUG(orig).
5. `outmem` schließt `filehand`, löscht es aber nicht → ein späteres `outmem` ohne neues
   `doopen` schließt dasselbe Handle erneut.
6. `outmem` vor der ersten Hauptschleife (`memstack` = 0): z. B. `extalloc` aus blitz2-Init
   (`makeiallox`) oder `openreq`-Fehler beim Versionsrequester → `a7 = 0` → Absturz.
7. `extalloc` → `outmem` mitten im Compilerlauf: blitz2-Frames werden verworfen, Speicher/
   Dateien des Compilers bleiben hängen.
8. `openio`: Erfolgs-Rückgabe Z hängt von den Flags nach `WBenchToFront` ab; `mycli`
   verlässt sich darauf (`beq .skip`). Außerdem: Höhe = Editor-Screen-Höhe, obwohl das
   Fenster auf dem Workbench-Screen öffnet (Interlace-Editor auf Non-Interlace-WB →
   Open kann scheitern).
9. `closeio`: `Close(myhandle)` auch wenn `openio` fehlschlug (Handle 0); unter 1.3
   kritisch. `findio` ohne Forbid/LockDosList.
10. `gotoline`: `gotoint` (long) gegen `numlines` nur wortweise verglichen; 65537 → Zeile 1.
    `lea gotoreq,a2` vor `actgad` wirkungslos (actgad setzt a2 = 0).
11. `mousedown`: Hochscrollen beim Ziehen vom Autor als „currently FUCKED“ markiert;
    nach dem Loslassen wird die **Scrollposition vor dem Ziehen** wiederhergestellt
    (`curstop/cursoff/thisitem` vom Stack), die Anzeige springt zurück. Doppelter,
    nie erfüllter Test `curstop+edrows ≥ numlines` (2049–2052 / 2055–2058). Busy-Polling ohne Delay.
    Labels `.loop2`, `.ok`, `.ok2` unbenutzt.
12. Mausklick ohne Ziehen bei vorhandenem Block: `blokdis` setzt `bstx` = −1 → eine per
    F1 gesetzte Startmarke geht verloren.
13. `help`: zeigt den letzten Token, der **vor oder an** der Cursorspalte beginnt (auch
    wenn der Cursor weiter rechts auf Text steht). Unbekannter Token → `findtoke` liefert
    `tokeerr`, a2 zeigt dann hinter diesen String (Müll als Hilfetext). `newsctit`
    (112 B) wird ohne Längenprüfung mit Name+Hilfe gefüllt.
14. `detokallz2`: keine Prüfung, ob die detokenisierte Zeile in `charbuff` passt bzw.
    < 255 Zeichen bleibt (`numchars` ist ein Byte). `detokallz` (comdata+16) lässt
    `thisitem` auf der letzten Zeile stehen und zeichnet nicht neu.
15. `tokone` tokenisiert in-place (a1 = a2) – nur korrekt, solange die Ausgabe nie länger
    als die Eingabe wird.
16. `getstuff`: `AllocMem` ohne NULL-Prüfung, Menüspeicher wird nie freigegeben.
17. `showmice`: Ergebnis von `screenclose`/`screenopen2` nicht geprüft; überschreibt
    `charbuff` (Detokenisieren der Labelzeilen).
18. `printmem`: Wert > 65535 KB wird durch `numtoascii` (unteres Wort) abgeschnitten.
19. `makemice`: `move d1,mousealt` setzt das Flag auf das Labelzeichen (Wortwert mit
    undefiniertem oberen Byte) – nur ≠ 0 zählt, harmlos.
