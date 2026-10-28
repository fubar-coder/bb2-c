# Survey 06 – ted.s Zeilen 5253–6285

Bereich: Cursor-Blinken/Statuszeile/Props (`cursor`…`showhprop`), `refresh`, Block-Randkorrektur
(`inbloks`/`inblokf`/`blokfix`), der **Zeileneditor** `getline` (Zeichen einfügen/löschen, Tab,
Cursor links/rechts, Shift-Varianten, Löschen bis Zeilenende), `horizpos`/`cursleft`/`cursrite`,
die **IDCMP-Schleife** `getinput`/`getinput2`/`handled0` (Rawkey über `rawed+32+4`, Requester-
Tastatur Return/Esc, Umlaute `germs`), `refme`, dann der **ganze Datenblock** ab 6022:
`mydata`/`comdata`-Tabelle, Editor-Globals, Defaults-Block (Datei `l:blitzeditor.opts`),
`jumptable`/`menutable`, Fehlertexte, `edstruct`-Kopierbereich.

Adressen (vasm-Referenzlisting, `tedlisting.py addr`): `cursor` $543E, `showrow` $5458,
`getline` $566C, `getinput2` $5BB8, `getinput` $5BD6, `handled0` $5C00, `germs` $5DAE,
`mydata` $5DBA, `comdata` $5DC2, `req_window` $5E20 (= comdata+94 ✓), `topskip` $5E24,
`jumptable` $61C2, `menutable` $6206, `tokens` $62F8, `remark` $6313, `rawed` $73E0.

Alles liegt in der einen Code-Section; Globals werden PC-relativ gelesen und absolut geschrieben.

---------------------------------------------------------------------------------------------
## A. Routinen

Konventionen in diesem Bereich: „Spalte“ = Bildschirmspalte `cursx` (0..edcols-1);
„abs. Spalte“ = `cursx+cursoff` (0..chcols-1); „Zeilennr.“ = `curstop+cursy`.
Zeichenzelle = 1 Byte breit (8 Pixel), 2 Bitplanes, `fonthite` Rasterzeilen à `bmapmul` Bytes,
`fontmul` = Bytes je Textzeile in einer Plane.

### Anzeige
- `cursor` (5253) = `cursor2` (5254) : zwei Labels, gleiche Adresse. ein: `cursx`,`scloc`,
  `bitmaps`. `calcphys` → a0/a1 = Zelle in Plane 0/1; `not.b` über `fonthite` Rasterzeilen in
  beiden Planes (XOR-Cursor, Aufruf zweimal = wieder aus). zerstört d0,a0,a1. Kein Rückgabewert.
- `showrow` (5263) : `numtext`="     \0" (5 Leerz.), d0=Zeilennr.+1 → `numtoascii`(d0,a0=numtext);
  Ausgabe mit `pstats` (a3=numtext) direkt in die Bitmap bei `statsloc+5` („Line:“). Tail-`bra
  pstats`. zerstört d0-d2,a0,a1,a3 (+qprint).
- `showvprop` (5275) : `showrow`, dann NewModifyProp(newgads=vprop, mywindow, req=0,
  flags d0=13 (AUTOKNOB|FREEVERT|PROPBORDERLESS), HorizPot d1=-1, VertPot d2, HorizBody d3=-1,
  VertBody d4, NumGad d5=1). VertBody = (edrows<<16)/numlines bzw. -1 wenn edrows>=numlines;
  VertPot = (Zeilennr.<<16)/(numlines-1) bzw. -1 wenn Zeilennr.>=numlines-1 (kein /0).
  **Tail-`jmp newmodifyprop(a6)`** mit a6=`int` → kehrt mit a6=IntuitionBase zurück.
  zerstört d0-d5,a0-a2,a6.
- `showhprop` (5305) : `numtext`="   \0", Spalte (abs.+1) bei `statsloc+18` („Column:“);
  NewModifyProp(newgads2=hprop, flags 11 (AUTOKNOB|FREEHORIZ|PROPBORDERLESS), HorizPot =
  (abs.Spalte<<16)/(chcols-1) bzw. -1 am Ende, VertPot -1, HorizBody = `prop2sinfo+6`
  (unverändert), VertBody -1, d5=1). Tail-`jmp` wie oben.
- `refresh` (5337) : ganzen Textbereich aus `chmap` neu zeichnen (horizontal ab `cursoff`).
  `storepos`; scloc=chloc=cursy=0; je Zeile cursx=0, a5=chmap+chloc+cursoff, edcols× `print3`,
  `godown`; `color`=normcol; **Tail-`bra restorepos`**. zerstört d0,d1,d6,d7,a0,a5 (+print).
  Zeichnet den Cursor nicht.

### Blockgrenzen (Block = btopx/btopy..bbotx/bboty, `bboty==-1` = kein Block)
- `inbloks` (5360) : Z=1 genau dann, wenn Zeilennr.==btopy und abs.Spalte<=btopx (unsigned).
  out: Z, d2 = Zeilennr. (bei Zeilen-Mismatch) bzw. abs.Spalte; d3=0 nur bei Z=1. Auch von
  Chunk 2/3 (1821) gerufen.
- `inblokf` (5372) : dto. mit bboty/bbotx („Cursor <= letztes Blockzeichen“).
- `blokfix` (5384) : wenn btopx>=chcols: btopx=0, btopy++; wenn dann btopy>bboty → Tail-`bra
  blokdis` (Block aufheben), sonst weiter wie `blokfixb`. zerstört d2.
- `blokfixb` (5393) : **zweiter Einsprung** (von `blokfix`, `char127`). bbotx<0 (signed):
  bbotx=chcols-1, bboty--, wenn bboty<btopy → `blokdis`. Sonst: einzeiliger Block mit
  bbotx<btopx (signed) → `blokdis`; bbotx>=chcols → bbotx=chcols-1 (siehe I.1: Bug).
  Rückkehr per rts oder Tail-`bra blokdis` (anderer Chunk, Z.3903).

### Zeileneditor
- `getline` (5416) : **Schleife bis ein nicht lokal behandeltes Ereignis kommt.** ein: nichts
  (Globals). `anyalt`=0; `cursor` an; → `getline3`.
- `getline2` (5420) : Rücksprung-Ziel aller lokalen Aktionen: `cursor` an, `showhprop`, fällt
  in `getline3`.
- `getline3` (5422) : `getinput` → d7/d6; `cursor` aus; Arbeitsregister:
  a0/a1 = Plane 0/1 + scloc (Zeilenanfang am Schirm), a2 = chmap+chloc (Zeile in der
  Zeichenkarte, chcols Bytes, unabh. von cursoff), d0 = cursx. Verteilung:
  - d7<0 (Gadget/Menü/Maus) → `charhi`; Bit 30 gesetzt (Umlaut aus `germs`) → löschen, `.isasc`;
    d7<32 → `charlow`; d7>=127 → `charhi`; sonst `.isasc`.
  - `.isasc` (Einfügen, immer Insert-Modus): anyalt=d7; Block: `inbloks`→btopx++,
    `inblokf`→bbotx++, `blokfix`; Bitmap-Zeile ab cursx um 1 Zelle nach rechts (beide Planes,
    fonthite Rasterzeilen), chmap ab abs.Spalte um 1 nach rechts (Byte chcols-1 fällt weg),
    `print` d7 (Farbe = aktuelles `color`), `cursrite`, → getline2.
- `charlow` (5484) : d7=31 (Cursor links, `$1F`):
  ohne Shift (`qualifier&3`) `cursleft`; mit Shift: abs.Spalte≠0 → d1=0 → `curspos`; abs.
  Spalte==0 → `.tababove`: Zeilennr.==0 → getline2; sonst führende Leerzeichen der Vorzeile
  zählen (`thisitem→lastitem(+4)→chars`, Ende bei Byte 0 → keine Bewegung) → `curspos`.
- `charnot31` (5521) : d7=30 (rechts): ohne Shift `cis30skip` (= `cursrite`); mit Shift: d1 =
  1+Index des letzten Nicht-Leerzeichens in a2 (max chcols-1, 0 bei leerer Zeile) → fällt in
  `curspos`.
- `curspos` (5537) : **Sprungziel** mit d1 = abs. Zielspalte: `horizpos`, → getline2.
- `cis30skip` (5540) : `cursrite`, → getline2.
- `charnot30` (5543) : d7=8 (Backspace): abs.Spalte==0 → getline2; sonst (a0-a2 gesichert)
  `cursleft`, d0=cursx, d7=8, → `char127` (anyalt wird 8).
- `charnot8` (5555) : d7=9 (Tab). d0=d2=abs.Spalte.
  - mit Shift („delete tab“): anyalt=9; d0 bis zum vorigen Vielfachen von `tabstop` (divu,
    Rest 0) zurück, bei <0 → getline2; Block: d4=d2-d0, inbloks→btopx-=d4, inblokf→bbotx-=d4
    (kein `blokfix`); chmap[d2..] nach chmap[d0..] kopieren, Rest mit ' ' füllen; **push d0**,
    → `reline`. Löscht auch Nicht-Leerzeichen (I.3).
  - ohne Shift: d0 bis zum nächsten Vielfachen von tabstop vor (>=chcols → getline2, nichts);
    anyalt=9; **push d0**; d0=Anzahl; Block (d2 gesichert): +d0 auf btopx/bbotx, `blokfix`;
    chmap ab d2 um d0 nach rechts (Überlauf fällt weg), Lücke ' '; fällt in `reline`.
- `reline` (5645) : **Stack-parametrisiert**: erwartet bei Eintritt das Wort (a7) = abs.
  Zielspalte. Zeichnet die sichtbare Zeile (chmap+chloc+cursoff, edcols Zeichen) mit `print3`,
  cursx gerettet; pop d1 → `curspos`. Benutzer: Tab löschen, Tab einfügen, Menü $f8e1.
- `charnot9` (5658) : alle anderen d7<32 → `glinedone` (Rückgabe an `mainedit`, u. a. 13, 27,
  28/29 Cursor hoch/runter, Ctrl-Codes).
- `charhi` (5662) : d7=127 → fällt in `char127`, sonst `charnot127`.
- `char127` (5665) : Löschen unter Cursor (Del; auch Ziel von Backspace). anyalt=d7; Block:
  inbloks und abs.Spalte≠btopx → btopx--; inblokf und bbotx+1<chcols → bbotx--; dann
  `blokfixb` (nur wenn inblokf Z=1 war; bei Z=0 → direkt Verschieben). Bitmap ab cursx+1 um 1
  nach links, chmap ab abs.Spalte+1 nach links, chmap[chcols-1]=' '; letzte sichtbare Zelle
  (cursx=edcols-1) mit `print3` aus chmap neu; → getline2.
- `charnot127` (5718) : d7=-1 und d6=1 (hprop-Gadget, Down oder Up): d1 =
  (HorizPot `prop2sinfo+2` × chcols)>>16 → `curspos`. Sonst:
- `charnotgad` (5726) : d7=-2 und d6=$f8e1 (Menü 1, Punkt 7, kein Sub; in `menutable` als $71
  fehlend – „bis Zeilenende löschen“): anyalt=d6; ab abs.Spalte ' ' bis chcols; **push**
  abs.Spalte; → `reline`. Block wird nicht angepasst.
- `charnotmenu` = `glinedone` (5738/5740) : rts. **Einziger Ausgang von getline.**
  aus: d7 = Ereignis (s. B/G), d6 = ID, `anyalt`≠0 wenn Zeile geändert, `qualifier`;
  d0,a0-a2 Müll. a6 = exec (aus getinput) oder int (aus showhprop) – nicht verlassen.

### Cursorbewegung
- `horizpos` (5742) : ein d1 = abs. Zielspalte. d4=alter cursoff. d1 < edcols-hscmarg →
  cursoff=0, cursx=d1. Sonst d2=edcols/2, d0=d1-d2, d3=chcols-edcols: d0>d3 (signed) →
  cursoff=d3, cursx=d1-d3; sonst cursoff=d0 (auf gerade aufgerundet, dann cursx=d2-1) bzw.
  cursx=d2. Hat sich cursoff geändert → **Tail-`bra refresh`**, sonst rts. Zeichnet den Cursor
  nicht. zerstört d0,d2-d4 (+refresh: d6,d7,a5). Auch von `find`-Pfad (956) gerufen.
- `cursleft` (5775) : ein d0=cursx. cursx>=hscmarg → cursx--. Sonst cursoff≠0 → cursoff-=2,
  `scrollrite` (Bitmap 1 Wort = 2 Zellen), cursx++ (netto 1 nach links). Sonst cursx≠0 →
  cursx--, sonst nichts. zerstört d1 (+scrollrite).
- `cursrite` (5789) : ein d0=cursx. cursoff+edcols>=chcols (rechter Rand der Karte sichtbar):
  cursx<edcols-1 → cursx++ sonst nichts. Sonst: cursx<hscmarg oder cursx<edcols-hscmarg →
  cursx++; sonst cursoff+=2, `scrollleft`, cursx--. zerstört d1,d2.
- `flash` (5810) : Debug: 65536× d0 nach COLOR00 ($dff180). d0 erhalten. **Unbenutzt.**

### Eingabe (IDCMP)
- `pickwindow` (5827) : out a2 = `req_window` falls ≠0, sonst `mywindow`. zerstört d0.
- `getinput2` (5833) : **comdata+32**. a6=exec; Port = pickwindow→UserPort (+86); ein
  GetMsg; Nachricht → **Tail-`bne handled0`** (Ergebnis wie dort); keine → d7=0, d0=0 (Z=1).
  Verarbeitet höchstens 1 Nachricht; eine ignorierte liefert ebenfalls d7=0.
- `getinput` (5845) : **comdata+8**. a6=exec; `ginp1` (5852, interner Schleifenkopf):
  pickwindow (bei jedem Durchlauf neu, Requester kann inzwischen zu sein), WaitPort/GetMsg bis
  Nachricht, `handled0`, Z=1 → `ginp1`. out: d7/d6, d0=-1, Z=0, **a6=exec**. zerstört
  d0,d1,a0-a2 (keine movem-Sicherung – auch nicht für blitz2-Aufrufe).
- `handled0` (5866) : ein d0 = IntuiMessage, a6=exec. Antwortet (ReplyMsg) in allen Pfaden.
  aus: d0=-1/Z=0 „verstanden“ mit d7 (und d6), oder d0=0/Z=1 und d7=0 „ignoriert“.
  | Klasse | Ergebnis |
  |---|---|
  | $20 GADGETDOWN / $40 GADGETUP | d7=-1, d6=GadgetID(+38). String-Gadget (Typ-Lowbyte +17 ==4) mit Code 9 (Tab) → ignoriert. ENDGADGET (Activation +15 Bit 2) und Klasse $40 → ReplyMsg, **`closereq`**, d0=-1 (Requester ist beim Rücksprung schon zu!) |
  | $2000 MENUVERIFY | ignoriert |
  | $100 MENUPICK | Code -1 (MENUNULL) ignoriert, sonst d7=-2, d6=Code (NextSelect nicht ausgewertet) |
  | $8 MOUSEBUTTONS | Code $68 SELECTDOWN → d7=-3, alles andere → d7=-4 |
  | $400 RAWKEY | Key-up (Bit 7) ignoriert; `qualifier`=Qual(+26); d0=Code,d1=Qual → `bsr rawed+32+4` → d7=Byte (0 → ignoriert). Mit Requester: 13 → erstes Gadget der Fensterliste (+62) mit ENDGADGET und BOOLGADGET (Typ 1) → `refme` (Blinken), Klasse als $40 behandeln (→ schließt). 27 → **letztes** solches Gadget, ebenso; andere Tasten ignoriert. Ohne Requester: `germs`-Tabelle, Treffer → Ersatzcode \| Bit 30 |
  | $200 CLOSEWINDOW | wie Esc (`.doesc`) |
  | sonst | ignoriert |
- `refme` (6003) : ein a0=Gadget (Kommentar „d0.b=pl…“ ist tot). Flags(+12)|=$80 (SELECTED),
  RefreshGList(a0, req_window, 0, 1), Delay(4), Flags&=~$80, RefreshGList. Sichert
  d0-d1/a0-a2/a6 (movem).

### Daten mit Code-Bezug (Dispatch, Details in C)
- `jumptable` (6171): von `mainedit` (Z.156) durchsucht, `cmp.b` gegen d7 → **berechneter
  `jsr (a0)`** (Flag-Byte auf Stack, `memstack`=a7, Flag≠0 → danach `showvprop`).
- `menutable` (6199): von `menu` (Z.1577) durchsucht → `jsr (a0)`, Flag≠0 → Tail `showvprop`.

---------------------------------------------------------------------------------------------
## B. Persistente Registerkonventionen

- Kein Register überlebt die Hauptschleife in diesem Bereich; alles läuft über Globals.
- **Rückgabe von `getinput`/`getinput2`/`getline`:** d7 = Ereignis, d6 = ID:
  d7 = 1..$FF Taste (zero-extended Long; ggf. **Bit 30** = „druckbares Ersatzzeichen“ aus
  `germs`, nur innerhalb getline relevant), 0 = nichts (nur getinput2), -1 Gadget (d6=ID),
  -2 Menü (d6=Intuition-Menücode), -3 Maus gedrückt, -4 Maus losgelassen/andere Taste.
  Tastencodes: ASCII via RawKeyConvert; Cursor hoch/runter/rechts/links $1C/$1D/$1E/$1F;
  F1–F10 $81–$8A; Help $8B; Del 127; Backspace 8; Tab 9; Return 13; Esc 27.
- Innerhalb `getline3`…: a0/a1 = Bitmap-Zeilenanfang Plane 0/1, a2 = chmap-Zeile, d0 = cursx,
  d1 = Zielspalte für `curspos`, d2 = abs. Startspalte (Tab), d7/d6 = Ereignis. `print`/`print3`
  sichern d2-d4/a2 (→ a2 und d7 überleben `print3` in `reline`).
- a6: `getinput*` setzen/hinterlassen exec; `showvprop`/`showhprop` hinterlassen `int`;
  `refme` erhält a6.
- `inbloks`/`inblokf`: Rückgabe nur über **Z-Flag** (Aufrufer `bne`).
- `handled0`/`getinput2`: Rückgabe über d0 **und** Z-Flag (getinput testet `beq`).
- Von blitz2 über comdata gerufen (diesem Bereich): `getinput` (+8), `getinput2` (+32) – **ohne**
  movem; blitz2 erwartet nur d6/d7 (blitz2.s 1420 u. a.). Die Einträge +72/+76
  (`pointeron`/`sleepon`, andere Chunks) sichern a0-a1/d0-d3/a6.

---------------------------------------------------------------------------------------------
## C. Globals (definiert 6022–6282)

### `germs` (6022), 12 Bytes, `ngerms equ 6` (Z.2)
Paare (Latin-1 → Zeichensatzcode): ä 228→26, ö 246→27, ü 252→28, Ä 196→29, Ö 214→30,
Ü 220→31. Ergebnis bekommt Bit 30 (unterscheidet von Steuercodes 26..31 = Cursortasten!).
Umlaute liegen im TED-Zeichensatz also auf 26..31.

### `mydata`/`comdata` (6027) – Dienst-/Datentabelle für blitz2 (a1 bei Init, Z.81)
| Off | Label/Inhalt | Typ | blitz2 nutzt |
|---|---|---|---|
| -8 | `screenclose` | Code | ja (CloseEd) |
| -4 | `screenopen2` | Code | ja |
| 0 | `openreq` | Code | ja |
| 4 | `closereq` | Code | ja |
| 8 | `getinput` | Code | ja |
| 12 | `error` | Code | ja |
| 16 | `detokallz` | Code | nein |
| 20 | `tokall` | Code | nein |
| 24 | `ezfreq` | Code | ja |
| 28 | `refgads` | Code | ja |
| 32 | `getinput2` | Code | ja |
| 36 | `combitmap` dc.l 0 | Daten (gesetzt 4698) | ja |
| 40 | `myscreen` dc.l 0 | Daten | ja |
| 44 | `extint` dc.l 0 | IntuitionBase (Z.37) | ja |
| 48 | `colors` | Zeiger | (grep: nein) |
| 52 | `halfcolors` | Zeiger | (grep: nein) |
| 56 | `tokone` | Code | ja |
| 60 | `openio` | Code | ja |
| 64 | `closeio` | Code | ja |
| 68 | `actgad` | Code | ja |
| 72 | `pointeron` | Code | ja |
| 76 | `sleepon` | Code | ja |
| 80 | `mywindow2` dc.l 0 | Editorfenster (4772) | ja |
| 84 | dc.l 0 → zur Init (extern) `defaultreqga9+13` (Z.82) | Zeiger auf Flags-Lowbyte eines Default-Requester-Gadgets („Icons“, Bit 7) | grep: nein |
| 88 | `extalloc` | Code | ja |
| 92 | `bmapmul` dc 0 | Wort: Bytes/Bitplane-Zeile | ja |
| 94 | `req_window` dc.l 0 | aktuelles Requester-Fenster | ja |

→ C: **gepackte Struktur mit festen Offsets**, Long bei Offset 94 (nur 2-aligned; mit m68k-gcc
ok). `bmapmul`/`req_window` sind zugleich normale Editor-Globals.

### Editor-Globals (6057–6097), Wort = `dc`
| Name | Typ | Init | Zweck |
|---|---|---|---|
| topskip, botskip | W | 0 | Pixel von Intuition oben/unten belegt |
| chcols | W | 128 | Spalten der Zeichenkarte (= max. Zeilenlänge) |
| totsccols | W | 80 | Zeichenspalten am Schirm gesamt |
| edrows | W | 0 | Textzeilen am Schirm |
| pixrows, pixrows2 | W | 0 | editierbare Pixelzeilen / ×fonthite |
| edcols | W | 0 | editierbare Spalten |
| mousecols | W | 0 | Spalten für „mice“ (Lesezeichen-Spalte) |
| bitmaps | 2×L | 0 | Plane 0, Plane 1 |
| cursx, cursy | W,W | 0 | Cursor am Schirm – **als Long kopiert** (storepos, 724, 3929) |
| scloc | L | 0 | Byte-Offset Cursorzeile in Plane |
| chloc | W | 0 | Offset Cursorzeile in chmap |
| cursx2,cursy2,scloc2,chloc2 | W,W,L,W | 0 | Sicherung (storepos/restorepos) |
| cursoff, curstop | W,W | 0 | horiz. Scroll-Offset / erste sichtbare Zeile (`clr.l cursoff` 725) |
| color | W | 1 | aktuelle Zeichenfarbe |
| qualifier | W | 0 | Qualifier der letzten Taste |
| chmap | L | 0 | Zeichenkarte (chcols×edrows) |
| anyalt | W | 0 | Zeile geändert (≠0) |
| firstitem, enditem, thisitem | L | 0 | Zeilenliste erste/letzte/aktuelle |
| numlines | W | 0 | Anzahl Zeilen |
| bloktopx/y, blokbotx/y | 4×W | 0 | Block in Zeilen-Koordinaten (Long-Paare kopiert 2151ff.; **auch als Long-Zeiger** 1754/1793 und als 1-Byte-Puffer für Write 1694ff. missbraucht) |
| btopx/y, bbotx/y | 4×W | 0 | Block „real“ (Spalte/Zeile); bboty=-1 = kein Block; Long-Paare (2013ff.) |
| invflag, inverse | W | 0 | Invertierungs-Flags |
| memstack | L | 0 | a7 für Speicherfehler-Longjmp (`outmem` → `memcont`) |
| filehand | L | 0 | offene Datei, bei Speicherfehler geschlossen |
| memalt | W | 0 | Speicher geändert → `printmem` |

### Defaults-Block `defstart`..`defend` (6099–6120), 50 Bytes, **binär** von/nach `l:blitzeditor.opts`
Reihenfolge fest (loaddefs/savedefs Z.3062ff., `defaults` kopiert wortweise):
fonthite 8, fonthite2 7 (=fonthite-1, dbf), fontmul 0, tokenise 0, makeicons 1, normcol 1,
labcol 2, tokcol 3, propcol 0, hscmarg 12, vscmarg 6, tabstop 2, palit 4×RGB-Wort-Tripel
(8,8,8 / 15,15,15 / 0,0,0 / 15,13,0), origscrows 0 (unbenutzt). → C: Struktur big-endian,
ohne Padding, `sizeof==50` prüfen.

### Sonstige Daten
| Name | Typ | Init | Zweck |
|---|---|---|---|
| defname | str | 'l:blitzeditor.opts' | Defaults-Datei |
| mpalit | 3×3 W | 15,15,0/15,0,0/0,0,0 | Palette Mauszeiger |
| statsloc | L | 0 | Bitmap-Adresse Statuszeile |
| statstext | str | 'Line:      Column:    Largest Mem (K):      ' | Statuszeile |
| bltext | str | 'Block' | Statusanzeige |
| numtext | 3×W | 0 | Zahlenpuffer (auch als Long beschrieben) |
| defalt | W | 0 | Defaults geändert → beim Ende speichern |
| req | L | 0 | aktueller Requester (openreq) |
| allalt | W | 0 | Quelle geändert |
| (ohne Label) | W | ' :' | **Sentinel vor filepath** (s. G) |
| filepath | 192 B | "" | Pfad Quelle |
| filename | 64 B | "" | Name Quelle |
| filetext, filetext2 | str | | Requestertitel Laden/Speichern |
| blokpath/blokname | 192/64 B | "" | Pfad/Name Block |
| bloktext, bloktext2 | str | | Requestertitel Block |
| blokop | W | 0 | Blockoperation läuft |
| mywindow, myrast | L | 0 | Hauptfenster, RastPort |
| charbuff..charbuff4, dosbuff | L | 0 | Zeiger in einen AllocMem-Block (doslen 2048 + 3×(chcols+2)) |
| nummice, mice | L,L | 0 | Lesezeichen Anzahl/Liste |
| mousealt | W | 0 | Lesezeichen geändert → showmice |
| tokeerr | 2 str | '?????','Token Unavailable' | Ersatz Name/Hilfe (Z.3482) |
| error1/2/3 | str | Couldn't open / reading / writing | Fehlertexte |
| returncode | W | 0 | Exit-Code (-1 bis normales Ende) |
| external | L | 0 | edstruct-Zeiger von blitz2 (0 = eigenständig) |

### `jumptable` (6171) – Tabelle von Code-Zeigern
Eintrag 6 Bytes: `.b Taste, .b Flag(showvprop danach), .l Routine` (Long nur 2-aligned);
Ende: Byte 0.
| Code | Flag | Routine |
|---|---|---|
| 28 | 1 | cursup |
| 29 | 1 | cursdown |
| 13 | 1 | return |
| 255 (=-1) | 1 | gadget (vprop; prüft d6 **nicht**) |
| 254 (=-2) | 0 | menu |
| 253 (=-3) | 1 | mousedown |
| 139 ($8B Help) | 0 | help |
| 129–132 (F1–F4) | 0 | blockstart, blockend, blocksave, blokcopy2 |

### `menutable` (6199) – Tabelle von Code-Zeigern
Gleiches Format, Ende: Byte 255. Schlüssel = `(Punkt<<4)|Menü` (aus Intuition-Code: Menü =
Code&15, Punkt = (Code>>5)&15, Subitem ignoriert; Menü 4 → `extmenu`, Z.1563).
| Menü | Punkte (Flag) |
|---|---|
| 0 Projekt | 0 new(0), 1 fileopen(1), 2 filesave(0), 3 defaults(0), 4 doabout(0), 5 prtfile(0), 6 mycli(0), 7 closewb(0), 8 openwb(0), 9 escape(0) |
| 1 Edit | 0 blokcopy(1), 1 blokkill(1), 2 bloksave(0), 3 blokload(0), 4 blokforget(0), 5 insertline(1), 6 deleteline(1), [7 = $f8e1 in getline], 8 join(1), 9 blocktab(1), $a blockuntab(1), $b markindent(0) |
| 2 | 0 top(1), 1 bottom(1), 2 gotoline(1) |
| 3 Suchen | 0 find, 1 next, 2 previous, 3 replace, 4 replaceall (alle 1) |
| 4 | extern (COMPILER, blitz2 `menu`) |

### `edstruct`-Kopierbereich (6267–6282)
`outside` (Z.24) kopiert 14 Wörter = 28 Bytes von edstruct (a1) nach `tokens`:
| Off | Label | Init (eigenständig) | Bedeutung |
|---|---|---|---|
| 0 | tokens | 0 | Token-Liste (≠0 → tokenise=1, getstuff) |
| 4 | menitems | 0 | Menütexte COMPILER |
| 8 | loadxtra | 0 | Rückruf nach Laden |
| 12 | savextra | 0 | Rückruf nach Speichern |
| 16 | menurout | 0 | Menü-Rückruf (extmenu) |
| 20 | cleanup | 0 | Rückruf in `doclean` |
| 24–27 | colon, quote, label, remark | je 255 | Trennzeichen `:` `"` `.` `;` (255 = aus) |
| 28 | dc.b 0 | | Terminator (wird nicht mitkopiert) |
`returncode` und `external` stehen direkt davor (nicht Teil der Kopie).

---------------------------------------------------------------------------------------------
## D. Selbstmodifizierender Code / Daten im Code / incbin-Einsprünge

- Kein Patchen von Befehlen in diesem Bereich. Alle Globals liegen in der Code-Section und
  werden beschrieben (C: normale schreibbare Daten).
- `comdata+84` wird zur Init mit einer Adresse **ins Innere** einer Gadget-Struktur gefüllt
  (`defaultreqga9+13`).
- `refme` schreibt in Gadget-Strukturen (Flags +12) der Requester.
- **`rawed` (incbin `rawed.obj`, Z.7109, $73E0)**: AmigaDOS-Ladedatei (HUNK_HEADER $3F3, 1 Hunk,
  HUNK_CODE $9C Langwörter = 624 Bytes, **keine Relocs**, HUNK_END) → Code ab Dateioffset 32,
  positionsunabhängig. Sprungleiste:
  - `rawed+32+0` (`bra` init; Aufruf Z.39): FindTask(0), AllocSignal(-1), baut MsgPort+IOStdReq
    im eigenen Hunk (82 Bytes genullt, ln_Type 5, mn_Length $30), OpenDevice("console.device",
    unit -1, ioreq, 0) – **Ergebnis ungeprüft**; ConsoleDevice = io_Device. Baut dann zwei
    105-Byte-Tabellen (Codes 0..$68 ohne/mit Shift) – **danach nie benutzt** (tot).
  - `rawed+32+4` (Z.5928): ein d0.w=Rawcode, d1.w=Qualifier; aus d0.w = Zeichen (0 = keins).
    $50–$59 → +$31 ($81–$8A, F-Tasten), $4C–$4F → −$30 ($1C–$1F, Cursor), $5F → $8B (Help),
    sonst RawKeyConvert (console −48) mit statischem InputEvent (Class 1 RAWKEY, Code, Qual,
    EventAddress 0 → keine Dead-Keys), Puffer 1 Byte, keymap 0 (Default); Rückgabe -1 → 0,
    sonst erstes Byte (bei 0 Zeichen: 0). Sichert a0-a2/a6, zerstört d1. Spezialtasten
    ignorieren Qualifier.
  - `rawed+32+8` (Z.128): `lea ioreq; move.l 4,a6; jmp CloseDevice(a6)`. Signal wird nicht
    freigegeben.
  Schreibt in seine eigenen Daten (InputEvent, Puffer, Device-Basis) → C: komplett als C-Modul
  neu (`OpenDevice`/`RawKeyConvert`), kein Asm nötig.

---------------------------------------------------------------------------------------------
## E. Heap-/Listenstrukturen (soweit hier benutzt)

- Zeile (`rsreset` Z.14): `nextitem` L +0, `lastitem` L +4, `numchars` B +8, `chars` +9
  (Alloc = Länge+9). `.tababove` liest `thisitem→lastitem→chars` und erwartet ein **0-Byte am
  Ende** (vgl. makeline/charstobuff in anderen Chunks).
- `chmap`: chcols × edrows Bytes, Zeichen der sichtbaren Zeilen in voller Breite (Bit 7 =
  Token-Farbe für `print3`). Zeile y bei chmap + y·chcols (= `chloc`).
- `dosbuff`-Block: doslen(2048) + 3·chcols + 6; charbuff = +2048, charbuff2/3/4 je +chcols+2.
- IntuiMessage: Class +20 (L), Code +24, Qualifier +26, IAddress +28. Window: FirstGadget +62,
  UserPort +86. Gadget: Flags +12 (W, $80 SELECTED), Activation-Lowbyte +15 (Bit 2 ENDGADGET),
  GadgetType-Lowbyte +17 (1 BOOL, 4 STR), GadgetID +38. PropInfo: HorizPot +2, VertPot +4,
  HorizBody +6.

---------------------------------------------------------------------------------------------
## F. Externe Aufrufe

- exec: WaitPort, GetMsg, ReplyMsg (getinput*, handled0). (rawed: FindTask, AllocSignal,
  OpenDevice, CloseDevice.)
- intuition: NewModifyProp (−468; showvprop/showhprop, a6=`int`), RefreshGList (−432; refme).
- dos: Delay(4) (refme).
- console.device: RawKeyConvert (−48) in rawed.
- Hardware: `flash` schreibt COLOR00 $dff180 (unbenutzt). Sonst direkte Bitmap-Schreibzugriffe
  (cursor, Zeichen schieben in getline/char127, pstats/qprint).
- IDCMP Hauptfenster (newwindow, Z.6318): RAWKEY|GADGETUP|MENUPICK|MOUSEBUTTONS|GADGETDOWN,
  Flags BACKDROP|BORDERLESS|ACTIVATE (kein Close-Gadget).
- Rückrufe nach blitz2 (Daten hier, Aufruf woanders): `menurout` (extmenu 1542: d6=Punkt,
  a5=firstitem, d7=numlines → d0.w), `loadxtra`/`savextra` (jsr (a5), d7=Handle, a6=DOSBase,
  Z.710/795), `cleanup` (doclean 2772, d0-d7/a0-a6 gesichert).

---------------------------------------------------------------------------------------------
## G. Sonderbehandlung in C

1. **getline** ist ein Zustandsautomat mit Sprüngen kreuz und quer (getline2/curspos/reline/
   char127 als Join-Punkte). C: eine Schleife `for(;;) { getinput(); switch… }`, `curspos(d1)`
   = `horizpos(d1); continue;`. Einziger Ausgang = `glinedone`.
2. **`reline` liest seinen Parameter vom Stack** (vom Aufrufer gepushtes Wort, beim Tab-Einfügen
   liegt zwischendurch noch d2 darüber). C: `reline(col)`.
3. **Mehrfach-Rückgabe** d7/d6 + d0/Z aus getinput/handled0; Bit 30 in d7 als Flag; d7 negativ
   für Nicht-Tasten. C: Rückgabe-Struct oder Globals `ev_code`/`ev_id`; Bit 30 als eigenes Flag.
   Für blitz2 (comdata+8/+32) braucht es Register-Glue (d6/d7 aus, a6=exec egal).
4. **Byte-Vergleich in mainedit/jumptable** (`cmp.b d0,d7`): -1/-2/-3 werden zu 255/254/253 –
   in C entsprechend `(UBYTE)code` vergleichen, sonst Verhalten ändern (siehe I.6).
5. **Tail-Calls mit a6-Wechsel**: showvprop/showhprop enden in `jmp NewModifyProp`, horizpos in
   `bra refresh`, refresh in `bra restorepos`, blokfix in `bra blokdis`, getinput2 in `bne
   handled0` – in C normale Aufrufe.
6. **Long-Aliasing von Wortpaaren**: cursx/cursy, cursx2/cursy2, cursoff/curstop,
   btopx/btopy, bbotx/bboty, bloktopx/bloktopy, blokbotx/blokboty werden als Long kopiert/
   gelöscht; bloktopx(+y) außerdem als Zeiger (1754) und Byte-Puffer (1694). C: Struct-Paare
   bzw. die Zugriffe einzeln ausschreiben.
7. **Sentinel `dc ' :'` vor `filepath`** (6139): `outside` prüft `-2(a0)==':'` nach dem Kopieren;
   bei leerem filepath trifft das genau dieses Byte → kein '/'. C: explizit „leerer Pfad“ prüfen.
8. comdata als gepackte Tabelle mit festen Offsets (inkl. −8/−4 vor dem Symbol, Wort bei 92,
   Long bei 94); Defaults-Block als 50-Byte-Datei-Image.
9. `memstack`-Longjmp (outmem → memcont) setzt Rahmen von mainedit voraus → setjmp/longjmp.
10. `refme`/handled0 ändern Gadget-Flags in fremden (blitz2-)Requester-Strukturen.
11. divu-Semantik: VertBody/VertPot/HorizPot nutzen 32/16-Division mit 16-Bit-Quotient; per
   Konstruktion kein Überlauf (Zähler < Nenner) – in C `(UWORD)((ULONG)x<<16 / n)`.
   Tab-Logik nutzt `divu` nur für den Rest (`x % tabstop`; tabstop=0 aus Defaults-Datei →
   Division durch 0-Trap im Original).

---------------------------------------------------------------------------------------------
## H. Einstufung

| Routine | Klasse | Bemerkung |
|---|---|---|
| cursor/cursor2 | UI | Bitmap-XOR |
| showrow, showhprop, showvprop | UI | Zahlformat (numtoascii) KERN, Rest UI |
| refresh | UI | |
| inbloks, inblokf, blokfix, blokfixb | KERN | reine Logik auf Globals; blokdis-Tail = Hook |
| getline/2/3, charlow…glinedone, reline, curspos | KERN+UI gemischt | Trennbar: **Zeilenpuffer-Operationen** auf `a2`-Zeile (Einfügen mit Verlust des letzten Bytes, Löschen, Tab ein/aus, Shift-Links/Rechts-Zielspalte, Löschen bis Ende, Blockkorrektur) sind KERN; Bitmap-Verschiebung + print/print3 + cursor als Ausgabe-Hooks (`hook_shift_right(x)`, `hook_shift_left(x)`, `hook_drawcell(x)`, `hook_redraw_line()`); getinput als Eingabe-Hook |
| horizpos, cursleft, cursrite | KERN+Hook | Spalten-/Offsetlogik KERN; `refresh`/`scrollleft`/`scrollrite` als Hooks |
| flash | (entfällt) | unbenutzt, Hardware |
| pickwindow, getinput, getinput2, handled0, refme | UI | comdata-Einsprünge brauchen GLUE (Register d6/d7) |
| rawed (+0/+4/+8) | UI | als C neu; Sonderkey-Mapping ($4C–$5F) KERN-testbar |
| jumptable/menutable-Dispatch | KERN | Tabellen in C als Struct-Arrays mit Funktionszeigern |
| germs | KERN | Tabelle |

---------------------------------------------------------------------------------------------
## I. Auffälligkeiten / mögliche Bugs des Originals

1. **`blokfixb` .ok2/.ok4 (5403–5413): falsches Register.** Bei mehrzeiligem Block (btopy≠bboty)
   springt `bne .ok4` mit d2=**btopy** statt bbotx; dann `cmp chcols,d2`: beginnt der Block ab
   Zeile ≥ chcols (128), wird bbotx auf chcols-1 gesetzt (Blockende → Zeilenende), sonst
   bleibt ein bbotx≥chcols unkorrigiert. `blokfix` läuft bei **jedem** eingefügten Zeichen,
   sobald ein Block existiert. → `BUG(orig)`.
2. Einfügen (`.isasc`, Tab einfügen) verwirft stumm das Zeichen in Spalte chcols-1, auch wenn es
   kein Leerzeichen ist (kein „Zeile voll“).
3. Shift-Tab (Tab löschen) löscht bis zur vorigen Tab-Position **beliebige** Zeichen, nicht nur
   Leerzeichen; ruft danach kein `blokfix`; Blockgrenzen zwischen neuer und alter Spalte
   werden nicht verschoben.
4. Menü $f8e1 (Löschen bis Zeilenende) passt den Block nicht an.
5. `horizpos`: ist `hscmarg` > edcols/2 (Defaults frei einstellbar), wird d0=d1-edcols/2
   negativ → negativer `cursoff`. Zweig `cursoff=chcols-edcols` kann einen **ungeraden**
   cursoff liefern, während cursleft/cursrite in 2er-Schritten (Wort-Scroll) arbeiten.
6. Byte-Vergleich in `mainedit`: Latin-1-Zeichen $FD/$FE/$FF aus RawKeyConvert (Alt-Kombi-
   nationen) würden als mousedown/menu/gadget ausgeführt; $81–$84/$8B (nicht druckbar) analog.
   Druckbare Latin-1-Zeichen ≥ $A0 außer den 6 Umlauten werden nie eingefügt (landen in
   mainedit, ignoriert). Umlaute nur ohne offenen Requester.
7. `handled0` CLOSEWINDOW ohne `req_window` → `.doesc` liest `62(0)` (Nullzeiger); praktisch
   unerreichbar, da das Hauptfenster kein CLOSEWINDOW-IDCMP hat.
8. `handled0` schließt bei ENDGADGET selbst den Requester (`closereq`) – Aufrufer (auch blitz2)
   bekommt d7=-1/d6=ID erst danach. Esc wählt das **letzte**, Return das **erste**
   Bool-ENDGADGET.
9. `getinput2` liefert d7=0 auch dann, wenn eine ignorierte Nachricht gelesen wurde und weitere
   warten (nur eine Nachricht pro Aufruf).
10. `jumptable`-Eintrag `gadget` reagiert auf jede Gadget-ID außer hprop (1), prüft d6 nicht.
11. `refme`-Kommentar „d0.b=pl if select…“: Parameter ungenutzt.
12. `rawed`: OpenDevice-Fehler ungeprüft (Absturz bei RawKeyConvert mit Basis 0), Signal nie
    freigegeben, zwei 105-Byte-Tabellen gebaut und nie gelesen, unerreichbares `moveq #0,d0; rts`
    am Hunk-Ende.
13. `flash` ist toter Debug-Code.
14. `char127`-Blockkorrektur: `blokfixb` nur, wenn `inblokf` traf; nach `btopx--` allein wird
    nicht korrigiert (unschädlich, da btopx > Cursorspalte ≥ 0).
15. `origscrows` im Defaults-Block wird nirgends benutzt (belegt 2 Bytes der Datei).
16. MENUPICK: Mehrfachauswahl (NextSelect) wird ignoriert; MENUVERIFY wird nur beantwortet.
