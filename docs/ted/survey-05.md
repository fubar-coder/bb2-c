# Survey 05 – ted.s Zeilen 4198–5252

Bereich: Tokenizer `tokit` (+ Einsprung `tokit2`/`tokit3` aus Chunk 04), `putchar`/`pchover`,
`alpha`/`alphad0`, vertikales Blitter-Scrolling (`scrollup`/`scrolldown`/`eraseline`),
Cursorzeilen-Arithmetik, Zeilen-Alloc/Free (`allocnull`/`freethis`/`freeall`), `donew`,
Bildschirm öffnen/schließen inkl. Gadget-Patches und User-Copperliste (`makecop`), Farben,
Blitter-Klammer (`getblitter`/`putblitter`), `cls`, horizontales Scrolling (`scrollrite`/
`scrollleft`), eigene Textausgabe (`print*`, `qprint`, `fontcalc`) mit den Fonts `chars.b`/
`chars8.b`, `calcpos`/`calcphys`.

Adressen (Referenzlisting): tokit $460E, tokit3 $4618, tokskip $4626, putchar $4772,
alpha $4786, scrollup $47B6, scrolldown $48D8, eraseline $495C, allocnull $4A50, donew $4A7E,
calcscreen $4B8A, screenopen $4C3A, makecop $4EAA, screenclose $4F7A, getblitter $5048,
cls $50A8, scrollrite $511C, scrollleft $51D0, printa0 $52C6, print2 $5340, print4 $5364,
qprint $53B6, fontcalc $53CE, calcphys $5424.

---------------------------------------------------------------------------------------------
## A. Routinen

### Tokenizer

Kontext (Chunk 04, hier nötig): `charstobuff` (4173) pusht `a1` (Zielpuffer-Start), setzt
`d1 = chcols-1` und springt per **`bne tokit`** (kein `bsr`!) hinein, wenn `tokenise`≠0 → das
`rts` von `enddo`/`tokit` kehrt direkt zum Aufrufer von `charstobuff` zurück.
`tokit2` (4195) pusht `a1` selbst und springt nach `tokit3`. Gemeinsamer Ausgang `enddo` (4190,
Chunk 04): `clr.b (a1)+ ; d5 = a1 - (a7)+ ; rts`.

- `tokit`=`tokit4` (4198/4202) : ein: a2=Quelle, a1=Ziel, d1.w=**Index des letzten Zeichens**,
  Stack: [Zielstart][Rücksprung]. Sucht rückwärts letztes Zeichen ≠ ' ' (Vergleich auf **rohes**
  Byte, $A0 zählt nicht als Leerzeichen). Alles Leerzeichen → `addq #4,a7` (Zielstart weg),
  `clr.b (a1)`, d5=1, rts (a1 zeigt AUF die 0, nicht dahinter – anders als enddo!).
  Sonst → `tokskip`. Mehrere Einsprünge (tokit/tokit4 identisch, tokit3).
- `tokit3` (4204) : Einsprung von `tokit2`: erst `subq #1,d1` → Aufrufer von tokit2 übergeben
  d1 = **Länge** (nicht letzter Index). Aufrufer: `tokone` (d1=strlen), `tokall` (d1=numchars-1
  = strlen ohne NUL), `mytoke` (d1=d5).
- `tokskip` (4210) : Hauptschleife. Pusht a4 (Stack: [a4][Zielstart][Ret]). d0.w=0 =
  Anzahl verbrauchter Quellzeichen, `invflag`=0 (Quote-Modus). Lokale Zustände `.loop2`,
  `.quoteon`, `.pch`, `.hendrix`, `.ppp2`, `.remloop`, `.notrem`, `.hexloop`, `.nothex`,
  `.tsttoke`, `.loop3/.loop4` (Namensvergleich), `.next`, `.notoke`, `.unto2`, `.found`,
  `.putina3`. **Kein normales Ende**: verlassen wird nur über `putchar`→`pchover` bzw.
  `.found`→`pchover2` → `bra enddo`. aus: d5 = Ausgabelänge inkl. NUL, a1 = hinter NUL,
  a4 restauriert; zerstört d0,d2,d3,d4,a2,a3. `.remloop` ist eine Endlosschleife, die nur durch
  pchover endet.
- `putchar` (4319) : `(a1)+ = d2`, d0++, wenn d0 > d1 (unsigned, `bhi`) → `pchover`.
  **Darf nur innerhalb tokskip gerufen werden** (Stackannahme).
- `pchover` (4324) : `addq #4,a7` (verwirft Rücksprung von putchar) → `pchover2` (4325):
  `move.l (a7)+,a4` → `bra enddo`. Nicht-lokales Verlassen über zwei Ebenen.
- `alpha` (4328) : ein d2.b ; aus Z=1 wenn 'A'–'Z' oder 'a'–'z' (via `cmp d2,d2`), sonst Z=0;
  zerstört nichts. Auch von Suchen (3552) benutzt.
- `alphad0` (4340) : wie alpha für d0 (exg). Benutzer: Suchroutine 3545.

Tokenizer-Algorithmus (exakt):
1. `.loop2`: d2=(a2)+ & $7F. d2==`quote` → `.quoteon`: invflag=~invflag, putchar, weiter.
2. `.hendrix`: invflag≠0 → putchar (Zeichen in Anführungszeichen roh). Buchstabe → `.tsttoke`.
3. `.ppp2` (Nicht-Buchstabe): `_` `.` `\` → `.unto2`. ==`remark` → Rest bis d1 kopieren.
   ==`$` → `.hexloop`: '$' und folgende Hexziffern kopieren. Dann `.nothex`: aktuelles
   Zeichen **ungeprüft** putchar (auch Quote/Remark/`_`!), nächstes lesen: Quote→`.quoteon`,
   kein Buchstabe→`.ppp2`, Buchstabe→`.tsttoke`. Alle übrigen Zeichen (Ziffern, Operatoren,
   Leerzeichen) laufen ebenfalls über `.nothex`.
4. `.tsttoke`: a2 zurück auf Wortanfang, Tokenliste ab `tokens` linear durchgehen. Knoten:
   +0 next (l), +4 Tokenwort (w), +6 Name (NUL-term.; in blitz2 folgt danach noch der
   Hilfetext). Vergleich `or.b #32` auf beiden Seiten. Name zu Ende → `.found`: Zeichen danach
   (& $7F) Buchstabe oder `_` → nächster Knoten (Ziffern dürfen folgen). Treffer → Ausgabe
   `(Hi | $80), Lo`, d0 += Namenslänge, d0>d1 → Ende (Token wurde schon geschrieben),
   a2 hinter Namen, `.loop2`.
5. Kein Treffer (`.notoke`) → `.unto2`: erstes Zeichen und alle folgenden **Buchstaben**
   kopieren; Quote → `.quoteon`; erstes Nicht-Buchstaben-Zeichen → `.ppp2` (also Ziffer nach
   Bezeichner beendet ihn: `x1print` → x 1 Print).

#### Vergleich mit `compilerlauf/tools/bb2tok.py`
bb2tok.py bildet die Zustandsmaschine korrekt nach (inkl. `.nothex`-Quirk, `or $20`,
Ziffern nach Namen, `_` `.` `\`). Nicht modelliert:
- **Quote/Remark-Zeichen sind Laufzeitvariablen** (`quote`, `remark`, aus edstruct kopiert in
  `outside` ted.s:24ff, blitz2.s:280 `':".;'`). Initialwert 255 → ohne blitz2 (standalone)
  gibt es keinen Quote- und keinen Remark-Modus (Byte & $7F kann nie 255 sein). bb2tok
  hart `"`/`;`.
- **Tokenliste = verkettete Liste aus blitz2 zur Laufzeit** (`tokens`), Reihenfolge wie dort
  verkettet (inkl. zur Laufzeit geladener Libs). bb2tok nutzt feste Tabellen. Ein Knoten mit
  leerem Namen würde sofort matchen (`.loop4`: erstes Byte 0 → `.found`); bb2tok filtert per
  Regex `\S+`.
- **Lesen über d1 hinaus**: Namensvergleich (`.loop4`) und `.found` lesen ab a4 ohne
  Längenprüfung. Bei Quelle = Zeilenpuffer (NUL-terminiert) harmlos. Bei Quelle = chmap-Zeile
  (`charstobuff`, 128 Spalten, kein Terminator) und Text bis Spalte 127 wird **in die nächste
  chmap-Zeile** weitergelesen (z. B. „…Pri“ + nächste Zeile „nt…“ → Token Print, danach
  pchover2). bb2tok behandelt das Zeilenende als Leerzeichen.
- **Trailing-Space-Trim auf Rohbyte**: `$A0` am Zeilenende wird nicht entfernt, aber als ' '
  ausgegeben. bb2tok: `rstrip(b' ')` vor dem Maskieren → identisch. (ok)
- **Bit 7 wird überall gelöscht**, auch in Strings/Kommentaren (ä=228 → 'd'). Grund: in der
  chmap markiert Bit 7 „Token-Farbe“ (print2/tokprint). Umlaute werden bei Eingabe über
  `germs` (6036) auf 26–31 abgebildet. bb2tok maskiert ebenfalls (ok), Umlaut-Mapping fehlt dort.
- **In-place-Tokenisierung**: `tokone` und `mytoke` rufen mit a1==a2. Funktioniert, weil
  Tokens ≥2 Zeichen lang sind (Ausgabe überholt Eingabe nie). Ein 1-Zeichen-Tokenname würde
  das nächste Quellzeichen überschreiben, bevor es gelesen wird.
- **Leere Liste** (`tokens`=0): `.loop3` liest Knoten bei Adresse 0 (Name ab Adresse 6 =
  ExecBase-Bytes). Wird nur verhindert, weil `tokenise` nur bei tokens≠0 gesetzt wird
  (`getstuff` 2781) – `tokall` (über comdata) prüft das nicht.
- `invflag` wird auch von der Blockmarkierung (2033, 2201–2221) als ganz anderes Flag benutzt;
  tokit setzt es auf 0/toggelt es (Seiteneffekt auf globalen Zustand).
- Leere Zeile: asm `a1` zeigt danach auf die NUL (nicht dahinter); Länge d5=1 wie enddo.

### Vertikales Scrollen (Blitter, a5=$dff000, a6=GfxBase)
- `scrupall` (4345) : d7=0, **Durchfall** in `scrollup`.
- `scrollup` (4346) : ein d7.w = erste Bildschirmzeile; schiebt Zeilen d7+1..edrows-1 um eine
  nach oben, löscht letzte Zeile. d6=edrows-d7-1 (keine Prüfung; d7≥edrows → d6<0 →
  Riesenblit). chmap: Blit A=chmap+(d7+1)*128 → D=chmap+d7*128, Mods 0. Bitplanes je
  `scup2`. Letzte chmap-Zeile mit $2020 füllen. Tail-`bra putblitter`. zerstört d0–d7,a0,a1,
  **a5**, a6.
- `scup2` (4407) : intern; a0=Plane. Kopie (wenn Höhe≠0, `cmp #64,d4`), dann letzte Zeile mit
  BLTADAT=0 bzw. $FFFF (wenn Bildschirmzeile curstop+edrows-1 im Block btopy..bboty) füllen,
  BLTCON0=$1F0. Tail-`jmp waitblit(a6)`.
- `scrdownall` (4433) : d7=0, Durchfall in `scrolldown`.
- `scrolldown` (4434) : ein d7 ; Zeilen d7..edrows-2 eine nach unten (Descending, BLTCON1=2),
  danach **Durchfall** in `eraseline` (bei d6=0 direkt `beq eraseline`). d5=Offset letztes Wort
  des Editbereichs (pixrows22-blitmod-2).
- `scdown2` (4523) : intern, ein Plane-Blit, Tail-`jmp waitblit`.
- `eraseline` (4479) : ein d7, Blitter schon besessen (nur Sprungziel/Durchfall). Füllt
  Bitplane-Zeile d7 (beide Planes) mit 0 bzw. $FFFF (Block), chmap-Zeile d7 mit $2020.
  **d7 wird mit chcols multipliziert (zerstört)**. Tail-`bra putblitter`.

### Cursor/Position
- `godown` (4530) / `goup` (4538) : cursy±1, scloc±=fontmul (long), chloc±=chcols. Keine
  Grenzprüfung. zerstört d0. Mehrfach Tail-Call-Ziel (1064, 1225).
- `storepos` (4976) / `restorepos` (4981) : cursx+cursy (als long) / scloc / chloc ↔ *2-Kopien.
  restorepos häufig Tail-Ziel (`bra restorepos`, `beq restorepos` 4067).
- `calcpos` (5234) : scloc=cursy*fontmul, chloc=cursy*chcols. zerstört d1.
- `calcphys` (5244) : aus a0=bitmaps[0]+scloc+cursx, a1=bitmaps[1]+scloc+cursx (cursx als
  Wort, vorzeichenerweitert).

### Zeilenliste
- `allocnull` (4547) : AllocMem(lenitem+1=10, MEMF_PUBLIC) ; aus d0=a0=Item, **Z=1 bei
  Fehler** (tst.l), sonst Z=0 (letzter `move #$100`). Init next=last=0, Wort $0100 an
  numchars → numchars=1, chars[0]=0. memalt=-1. a6=ExecBase.
- `donew` (4560) : „alles löschen“: color=normcol, `blokdis`, cursx/cursy/scloc/chloc/curstop/
  cursoff/allalt=0, filepath/filename="", numlines=1, `allocfirst` (→allocnull + **cls**),
  showhprop, showvprop; aus d0=-1 → **immer Z=0**. Kommentar „z=1 if problem“ stimmt nicht
  (Fehler von allocnull wird ignoriert). Gibt alte Zeilen NICHT frei (Aufrufer: freeall vorher).
- `freethis` (4582) : gibt `thisitem` frei (nicht aushängen!). Erstes Nicht-Space-Zeichen ==
  `label` → `mforget2` (setzt nur mousealt=-1). FreeMem(a1, lenitem + numchars) mit
  **`add.b`** (siehe I). memalt=Größe. Tail-`jmp freemem`.
- `freeall` (4602) : gibt numlines Items ab firstitem frei (über nextitem, vor FreeMem gelesen).
  `dbf` mit numlines-1 → numlines=0 wäre 65536 Durchläufe. Listen-Globals bleiben stehen.

### Bildschirm
- `calcscreen` (4619) : GetScreenData(scdata,84,WBENCHSCREEN,0); Breite &$FFF0 (Kommentar sagt
  „multiple of 8“, real 16). topskip=12, botskip=8 hart. Setzt newwindow.TopEdge, viewmode
  (=WB ViewPort.Modes, scdata+76), newscreen/newwindow Width, totsccols=Breite/8,
  edcols=totsccols-mousecols-2, edrows=(H-topskip-botskip)/fonthite-1, pixrows=edrows*fonthite,
  newwindow.Height=pixrows+fonthite+botskip+1 (Statuszeile), newscreen.Height=+topskip.
- `screenopen` (4668) : calcscreen, OpenScreen → myscreen, winscreen (=NewWindow.Screen),
  reqstruct+12. Liest RastPort.BitMap (Screen+88): bmapmul=BytesPerRow, blitmod=bpr-edcols,
  pixrows2 (Wort, abgeschnitten)/pixrows22 (long)=bpr*pixrows, fontmul=bpr*fonthite,
  combitmap=Plane0, bitmaps[0..1]=Plane0/1+topskip*bpr, statsloc=Plane1-Zeile
  (topskip+pixrows+botskip+1). chmap=AllocMem(chcols*edrows, **MEMF_CHIP**). Patcht Gadgets/
  Borders (s. C), OpenWindow → mywindow/mywindow2, SetMenuStrip(menu1), myrast=Window.RPort,
  DrawBorder×2, `colors`, `pstats`(Statuszeilentext), mousealt=-1, pr_WindowPtr (Task+184)
  sichern → oldmywindow und auf mywindow setzen, `pointeron`, screensopen=-1, `makecop`.
  aus: **d0=0/Z=1 = OK**, d0=-1/Z=0 = Fehler. Fehlerpfade `.wfail/.cfail/.sfail` (s. I).
- `screenopen2` (4661) : screenopen, showvprop, showhprop, Tail-`bra showall`. Ignoriert
  Fehler von screenopen. **Von blitz2 gerufen** (`mydata`+4 = comdata-4).
- `makecop` (4823) : nur wenn `copok`=0. col2 = RGB4 von Farbe 2 aus palit. AllocMem(12,
  PUBLIC|CLEAR) = struct UCopList → `coplist`; UCopperListInit(ucl,24); für Zeile 0..11:
  CWait(ucl,v,0),CBump, CMove(ucl,$184 (COLOR02),cols[v]),CBump; CWait(10000,255) (ohne CBump).
  ucl->Next = vp->UCopIns; vp->UCopIns = ucl (Screen+44+20); Tail-`jmp RemakeDisplay`
  (Intuition). Farbverlauf in Pen 2 über die Titelleiste. zerstört a5,a6,d6,d7.
- `pstats` (4880) : ein a3=NUL-String, a1=Ziel (1 Plane); je Zeichen qprint, a1++. Tail-Ziel
  (3031, 3901, 5273).
- `freechmap` (4888) : FreeMem(chmap, chcols*edrows); a6=Exec danach. Tail-`jmp freemem`.
- `screenclose` (4895) : screensopen=0, pr_WindowPtr zurück, freechmap, flushmw, ClearPointer,
  ClearMenuStrip, CloseWindow, Tail-`jmp CloseScreen` (gibt UCopList mit frei; `coplist`
  bleibt hängend, unbenutzt). **Von blitz2 gerufen** (`mydata`+0 = comdata-8).
- `mwait` (4913) : Busy-Wait bis CIA-A PRA ($BFE001) Bit 7 = 0 (Feuerknopf Port 2).
  **Kein Aufrufer** – toter Debug-Code.
- `halfcolors` (4917) : SetRGB4 Farben 0–3 mit halbierten palit-Werten. Von blitz2 (comdata+52).
- `colors` (4940) : SetRGB4 Farben 0–3 aus palit, dann 17–19 aus mpalit (Mauszeiger-Sprite).
  Von blitz2 (comdata+48) und intern. Beide: keine Registersicherung (d0–d3,d6,d7,a0,a4–a6).

### Blitter / Löschen / horizontales Scrollen
- `getblitter` (4964) : OwnBlitter, WaitBlit, **a5=$DFF000**, BLTAFWM/ALWM=$FFFF, BLTCON1=0.
  aus: a5=custom, a6=GfxBase (Aufrufer verlassen sich darauf für `jsr waitblit(a6)`).
- `putblitter` (4973) : WaitBlit, Tail-`jmp DisownBlitter`.
- `cls` (4986) : chmap komplett mit $2020 (Blitter-Fill), dann Editbereich beider Planes
  (`cls2`, pixrows Zeilen × edcols/2 Wörter) mit 0. Cursor bleibt. Tail-`bra putblitter`.
  `cls2` (5006) startet nur den Blit (kein Wait; Aufrufer wartet).
- `scrollrite` (5019) : Bitplanes um 1 Wort (2 Zeichen) nach rechts, descending
  (BLTCON=$09F0_0002), dann linke 2 Spalten aus chmap+cursoff neu zeichnen (print3), Zeile für
  Zeile. Caller muss cursoff vorher angepasst haben. a5 wird danach als chmap-Zeiger
  wiederverwendet. color=normcol, Tail-`bra restorepos`.
- `scrollleft` (5069) : analog nach links (ascending, BLTCON1 aus getblitter=0), rechte 2
  Spalten (edcols-2, edcols-1) neu zeichnen.
- `scrite2`/`scleft2` (5064/5108) setzen A/D und **fallen/springen** in `hscmods` (5112):
  Mods=blitmod+2, BLTSIZE=pixrows<<6 | (edcols-2)/2 (startet Blit).

### Textausgabe
- `print3` (5125) : d0 mit Bit 7 → color=tokcol sonst normcol, Bit 7 löschen, a0=`print4`,
  Tail in `printa0`. Zeichnet nur, schreibt nicht in chmap.
- `print` (5132) : a0=`print2`, **Durchfall** in `printa0`. „print d0 at cursx,cursy in color“.
- `printa0` (5135) : ein d0=Zeichen, a0=Zeichenroutine. movem d2-d4/a2. Bestimmt `inverse`
  (0/-1) aus Blockgrenzen (y=curstop+cursy unsigned, x=cursoff+cursx **signed**):
  y<btopy → nein; y==btopy: x<btopx → nein, sonst wenn y<bboty → ja, sonst x>bbotx → nein/ja;
  btopy<y<bboty → ja; y==bboty: x≤bbotx → ja; y>bboty → nein; bboty==-1 → kein Block.
  Dann `prinv`: `jsr (a0)` (berechneter Aufruf). zerstört d0,d1,a0,a1.
- `prinv` (5163) : Label mitten in printa0, nur intern.
- `print2` (5167) : schreibt d0 (|$80 wenn color≠normcol) nach chmap[chloc+cursoff+cursx],
  **Durchfall** in `print4`.
- `print4` (5178) : calcphys, fontcalc; pro Glyphenzeile: Plane0 = glyph & (color Bit0 ? FF:0)
  ^ inverse, Plane1 = glyph & (color Bit1 ? FF:0) ^ inverse. fonthite2+1 Zeilen, Glyphen-Stride
  16, Bitmap-Stride bmapmul. CPU-Schreibzugriffe in Chip-RAM.
- `qprint` (5205) : d0 Zeichen, a0 Ziel, 1 Plane, ohne Farbe/Inverse. zerstört d0,d1,a0,a2.
- `fontcalc` (5214) : ein d0 (0–127) ; aus a2=Glyphe. font8: +(c&15)+(c>>4)*128;
  font6 (wenn fonthite==6): +(c&15)+(c>>4)*96. zerstört d0,d1.

---------------------------------------------------------------------------------------------
## B. Registerkonventionen

- Tokenizer: a2=Quelle (läuft), a1=Ziel (läuft), d1.w=letzter Index, d0.w=Zähler, d2=Zeichen,
  a3=Tokenknoten, a4=Lookahead (gesichert), Stack hält Zielstart (für d5). Ausgabe d5.
- Blitter-Routinen: **a5=$DFF000, a6=GfxBase** zwischen getblitter und putblitter; alle
  Unterroutinen (`scup2`, `scdown2`, `cls2`, `hscmods`, `eraseline`) setzen das voraus.
  Damit wird a5 in allen Scrollroutinen zerstört.
- d7 = Bildschirmzeile bei scrollup/scrolldown/eraseline (wird zerstört).
- Zeichenausgabe: d0=Zeichen; printa0 erhält d2-d4/a2 (movem), Rest frei.
- Exec-Aufrufe über `move.l 4,a6`; danach bleibt a6=ExecBase (relevant für .cfail/.wfail).
- Keine Routine hier hat `movem`-Sicherung für blitz2-Aufrufe: `screenclose`, `screenopen2`
  (mydata), `colors`, `halfcolors` (comdata) werden von blitz2 gerufen und zerstören fast alle
  Register inkl. a5/a6 → Erwartung des Aufrufers in blitz2 prüfen (Glue).
- `tokone` (comdata+56, Chunk 04) → `tokit2`: ebenfalls ohne Sicherung; `mytoke` sichert.

---------------------------------------------------------------------------------------------
## C. Globals in diesem Bereich

| Name | Zeile | Typ/Init | Zweck |
|---|---|---|---|
| `scdata` | 4617 | ds.b 84 | Kopie WB-Screen (GetScreenData); +12 Width (gepatcht &$FFF0), +14 Height, +76 ViewPort.Modes |
| `blitmod` | 4666 | w 0 | Blitter-Modulo = BytesPerRow-edcols |
| `oldmywindow` | 4811 | l 0 | gesicherter pr_WindowPtr |
| `screensopen` | 4812 | w 0 | -1 wenn Screen offen (gelesen 2555, 2606) |
| `coplist` | 4814 | l 0 | UCopList (nur geschrieben) |
| `cols` | 4816 | 11 w | Farbverlauf $357…$246 für Titelleiste |
| `col2` | 4819 | w 0 | 12. Eintrag von `cols` = echte Farbe 2 (zur Laufzeit) |
| `copok` | 4821 | w 0 | ≠0 → kein makecop (toggelt in `defaults` 411/413) |
| `pixrows22` | 5017 | l 0 | bpr*pixrows als long |

Liegen mitten zwischen Code (Daten im Code-Hunk), in C normale Globals.

Zur Laufzeit gepatchte Strukturen (definiert außerhalb, Offsets wichtig):
- `newscreen` (NewScreen): +4 Width, +6 Height; `viewmode` (= +12 ViewModes).
- `newwindow` (NewWindow): +2 TopEdge=topskip, +4 Width, +6 Height; `winscreen` (+30 Screen).
- `reqstruct+12` = Screen-Zeiger.
- `newgads` (Gadget, vert. Prop rechts): +4 LeftEdge=edcols*8+3, +6 TopEdge=2,
  +8 Width=10, +10 Height=pixrows-4.
- `newgads2` (Gadget, horiz. Prop unten): +4 LeftEdge=3, +6 TopEdge=pixrows+2,
  +8 Width=edcols*8-6, +10 Height=4.
- `prop2sinfo+6` (PropInfo.HorizBody) = edcols*65536/chcols.
- Border-XY-Tabellen: `ngc1+10`, `ngc2+6`, `ngc2+10` = pixrows-1; `ngc3`,`ngc4`,`ngc4+4` =
  edcols*8-1; `ngc3+10`,`ngc4+6`,`ngc4+10` = 7. Borders `newgadbord`+0 / `nextngad`+0 =
  edcols*8 (LeftEdge), `newgadbord2+2` / `nextngad2+2` = pixrows (TopEdge).
- Reihenfolge wichtig: newgads+4/+10 werden erst gelesen (für Borders), dann `addq/subq`
  angepasst – screenopen ist nur wegen der vorherigen absoluten Neuzuweisung idempotent.

Gelesene Globals (außerhalb definiert): quote/remark/label/colon (Bytes, Init 255, aus edstruct),
tokens, invflag, inverse, color (Wort; Bits über `color+1` getestet), normcol, tokcol,
fonthite/fonthite2/fontmul, bmapmul, chcols (128, nie geändert), edcols, edrows, pixrows,
chmap, bitmaps[2], cursx/cursy/scloc/chloc(+2), cursoff, curstop, btopx/btopy/bbotx/bboty,
palit/mpalit, memalt, mousecols (0/12), totsccols, topskip/botskip, statsloc, combitmap.

---------------------------------------------------------------------------------------------
## D. Selbstmodifizierender Code / Daten im Code

- Kein Schreiben auf Befehle in diesem Bereich.
- PC-relative Daten im Code-Hunk werden beschrieben: `scdata` (auch per `and` gepatcht),
  `blitmod`, `col2` (verlängert die Tabelle `cols` um einen Eintrag – `cols`+`col2` müssen in C
  ein Array [12] sein), `oldmywindow`, `screensopen`, `coplist`, `pixrows22`.
- Gepatchte Intuition-Strukturen (s. C).
- Fonts `font6`/`font8` = `incbin chars.b` (768 B) / `chars8.b` (1024 B), direkt hintereinander:
  je 128 Zeichen, 16 Zeichen pro „Fontzeile“, Glyphenzeilen-Stride 16 Byte, 8 px breit.
- Keine Einsprünge in incbin-Code.

---------------------------------------------------------------------------------------------
## E. Heap-/Speicherstrukturen

- **Zeilenitem** (Offsets aus rs-Definition Zeile 16–20): +0 nextitem (l), +4 lastitem (l),
  +8 numchars (b, **inkl. NUL**), +9 chars[]. lenitem=9. Allokation numchars+9, MEMF_PUBLIC.
  Leeres Item: 10 Byte, numchars=1, chars=„\0“ (Wortschreibzugriff $0100 auf +8).
- **chmap**: edrows × chcols(128) Byte, **Chip-RAM** (wird per Blitter bearbeitet). Zeile r =
  chmap+r*128; sichtbare Spalten cursoff..cursoff+edcols-1. Byte = Zeichen, Bit 7 = in
  Nicht-Normalfarbe (Token) gezeichnet. Leer = ' '.
- **Bitplanes**: Screen-BitMap, 2 Planes, bitmaps[i] = Plane i + topskip*bpr; Zeichenzelle
  8 px × fonthite, Byteadresse = bitmaps[i] + cursy*fontmul + cursx. Statuszeile in Plane 1
  ab `statsloc`.
- **UCopList** 12 Byte (Next, FirstCopList, CopList), AllocMem PUBLIC|CLEAR, gehört nach
  Einhängen dem ViewPort (CloseScreen gibt frei).
- `memalt` wird bei jeder Alloc/Free gesetzt (-1, 2 oder Größe) → Anzeige freier Speicher.

---------------------------------------------------------------------------------------------
## F. Externe Aufrufe / Hardware

- exec: AllocMem, FreeMem, FindTask(0) (pr_WindowPtr @184 setzen/zurücksetzen).
- intuition: GetScreenData, OpenScreen, CloseScreen, OpenWindow, CloseWindow, SetMenuStrip,
  ClearMenuStrip, ClearPointer, DrawBorder, RemakeDisplay.
- graphics: OwnBlitter, DisownBlitter, WaitBlit, SetRGB4, UCopperListInit (-594), CWait (-378),
  CMove (-372), CBump (-366) – die vier letzten per Zahl, nicht per Equate.
- **Direkter Hardwarezugriff**:
  - Blitter über `$DFF000` (a5): BLTCON0 $040 (Werte $09F0 = A→D, $01F0 = D=BLTADAT-Fill),
    BLTCON1 $042 (0 / 2=DESC), BLTAFWM/ALWM $044, BLTAPT $050, BLTDPT $054, BLTSIZE $058,
    BLTAMOD $064, BLTDMOD $066, BLTADAT $074. Immer zwischen OwnBlitter/DisownBlitter, jeder
    Blit mit WaitBlit abgeschlossen (sauber). BLTSIZE nur OCS (10 Bit Höhe, 6 Bit Breite).
  - Blitter-Fill/Copy auch auf **chmap** (Zeichenspeicher, nicht nur Grafik).
  - CPU schreibt Glyphen direkt in die Screen-Bitplanes (print4, qprint) → nur planare
    Chip-RAM-Bitmaps; kein RTG.
  - CIA-A `$BFE001` Bit 7 in `mwait` (tot).
  - Copper nur über OS (UCopList), Farbregister COLOR02 ($184) per CMove.
- Timing: keine Delays/VBlank; nur Busy-Wait in `mwait` (unbenutzt) und WaitBlit.
- Rückrufe in blitz2: keine in diesem Bereich. Von blitz2 gerufen: screenclose, screenopen2,
  colors, halfcolors (und indirekt tokit über tokone/tokall).

---------------------------------------------------------------------------------------------
## G. Sonderbehandlung in C

- Tokenizer: nicht-lokales Verlassen (pchover: `addq #4,a7`, Stackpop über Ebenen) → in C als
  Schleife mit `goto done`/Rückgabewert; putchar als Makro/inline mit Längenprüfung.
  Einsprünge tokit (d1=letzter Index, Zielstart auf Stack) vs. tokit2 (d1=Länge) als zwei
  C-Funktionen auf einen Kern; charstobuff springt per `bne` hinein (kein Rücksprung dorthin).
  Rückgabe: Länge inkl. NUL (d5) **und** Endzeiger a1 (unterschiedlich bei leerer Zeile).
  Quote/Remark-Zeichen und Tokenliste als Parameter (Host-Test!). Lesen über d1 hinaus
  (chmap-Fall) nachbilden oder bewusst `BUG(orig)` dokumentieren.
- `alpha`: Z-Flag-Rückgabe → `bool`.
- Blitter-Routinen: in C mit `custom.h` (`custom->bltcon0` …) + OwnBlitter/WaitBlit möglich,
  kein Inline-Asm nötig. Empfehlung: chmap-Blits (Zeile kopieren, mit $20 füllen) in C als
  memmove/memset in einer KERN-Schicht, Bitplane-Blits getrennt (UI). Achtung: chmap-Blits
  laufen derzeit parallel/vor den Bitplane-Blits – Reihenfolge egal (unabhängige Daten).
- BLTSIZE-Breite 0 = 64 Wörter wird ausgenutzt (chcols=128 → chcols/2=64 → `&63`=0); an zwei
  Stellen fehlt das `&63` (s. I) – in C chcols als Konstante 128 behandeln.
- Byte/Wort-Aliasing: `color` (Wort) Bits über `color+1` getestet; numchars per Wortzugriff
  mit chars[0] zusammen initialisiert; `cursx` long-Zugriff löscht/kopiert cursx+cursy
  (cursx2/cursy2 ebenso) → Layout `cursx,cursy` bzw. `cursx2,cursy2` als Paar erhalten oder
  explizit beide Felder.
- `inverse` Wort, aber als Byte-Maske `eor.b` benutzt (0/$FF).
- Berechneter Aufruf `jsr (a0)` in printa0 → Funktionszeiger.
- Flag-Rückgaben: allocnull (Z=1 Fehler), screenopen (Z=1 Erfolg, d0=0), donew (immer
  „Erfolg“), alpha (Z).
- blitz2-Einsprünge (screenclose, screenopen2, colors, halfcolors) brauchen Glue mit der
  Registererwartung von blitz2 (dort prüfen).
- Mischung signed/unsigned in printa0 (y: bcs/bhi, x: blt/bgt/ble) exakt übernehmen.

---------------------------------------------------------------------------------------------
## H. Einstufung

| Routine | Klasse | Bemerkung |
|---|---|---|
| tokit/tokit2/tokit3/tokit4/tokskip | KERN | rein; Tokenliste+quote/remark als Eingabe |
| putchar/pchover/pchover2 | KERN | intern im Tokenizer |
| alpha/alphad0 | KERN | |
| godown/goup/storepos/restorepos/calcpos | KERN (Zustand) | reine Arithmetik auf Cursor-Globals, host-testbar wenn fontmul/chcols gesetzt |
| allocnull/freethis/freeall | KERN | AllocMem/FreeMem abstrahierbar; freethis→mforget2 setzt nur Flag |
| donew | KERN+UI | Hook: blokdis, cls (via allocfirst), showhprop/showvprop als Callbacks |
| fontcalc | KERN | reine Adressrechnung, Fontdaten nötig |
| print2 | KERN+UI | chmap-Schreiben (KERN) abtrennbar vom Zeichnen (print4) |
| scrollup/scrolldown/eraseline/cls | UI (+KERN-Anteil) | chmap-Teil als memmove/memset trennbar |
| scrollrite/scrollleft/hscmods/scrite2/scleft2 | UI | |
| print/print3/printa0/prinv/print4/qprint/pstats/calcphys | UI | Block-Inverse-Entscheidung in printa0 ist reine Logik (abtrennbar) |
| getblitter/putblitter | UI | Hardware; C mit custom.h |
| calcscreen/screenopen/screenopen2/screenclose/makecop/freechmap | UI | calcscreen-Geometrie als reine Funktion testbar |
| colors/halfcolors | UI | von blitz2 gerufen → GLUE-Wrapper |
| mwait | UI | toter Code (CIA) |

---------------------------------------------------------------------------------------------
## I. Auffälligkeiten / mögliche Bugs (BUG(orig))

1. **screenopen Fehlerpfade rufen CloseScreen mit a6=ExecBase** (4806/4807): `.cfail` nach
   AllocMem (a6=Exec) und `.wfail` nach freechmap (a6=Exec) → `jsr -66(a6)` = exec-Vektor
   statt intuition CloseScreen. Bei Chip-RAM-Mangel/OpenWindow-Fehler Absturzgefahr.
2. `screenopen2` ignoriert den Fehler von screenopen und zeichnet weiter (showvprop/showall).
3. `makecop`: AllocMem-Ergebnis nicht geprüft; letzter CWait ohne CBump (CEND unvollständig);
   UCopperListInit mit 24 für 24 Befehle + Ende-Wait (knapp).
4. `scrolldown` chmap-Blit (4453–4457): BLTSIZE ohne `&63` → `d6<<6 | 64` → bei geradem d6
   eine Zeile zu viel (liest bei d7=0 eine Zeile vor chmap). Folgenlos, weil eraseline Zeile
   d7 danach löscht. `scrollup` 4400–4403 ebenso ohne `&63`, korrekt nur weil 64|64=64.
5. `freethis`/`freeall`: Größe `moveq #lenitem,d0 ; add.b numchars,d0` – Byte-Addition,
   überläuft bei numchars ≥ 247 (FreeMem mit falscher Größe). Allokation (1378 u. a.) nutzt
   `add` Wort.
6. `donew`: Kommentar „z=1 if problem“, liefert aber immer d0=-1/Z=0; allocnull-Fehler
   (firstitem bleibt alt/ungültig) wird verschluckt.
7. `halfcolors`/`colors`: `moveq #0,d2` doppelt statt `d3` (Tippfehler; obere Hälfte von d3
   undefiniert, SetRGB4 benutzt nur die unteren 4 Bit).
8. `calcscreen`: Kommentar „multiple of 8“, Maske $FFF0 = 16. Keine Untergrenze für edrows.
   Bei Breite ≥ 1040 px wird edcols ≥ 128 = chcols → `divu` (prop2sinfo) läuft über,
   chmap-Zeilen zu kurz für den sichtbaren Bereich.
9. `scrollup`/`eraseline`: neue Zeile wird bei Block über die **ganze Breite** invertiert
   ($FFFF), ohne btopx/bbotx – an erster/letzter Blockzeile ggf. falsch invertierte Zellen
   jenseits des Zeilenendes, bis neu gezeichnet.
10. Tokenizer liest beim Namensvergleich über das Zeilenende (chmap-Fall: in die nächste
    chmap-Zeile, s. A); `tokens`=0 → Zugriff ab Adresse 0.
11. `fontcalc` ohne Bereichsprüfung: Zeichen ≥128 lesen hinter `chars.b` in `chars8.b` bzw.
    hinter `chars8.b` hinaus (print2/print4 maskieren Bit 7 nicht, nur print3).
12. `freeall` mit numlines=0 → dbf-Unterlauf (65536 Freigaben).
13. `scrollup`/`scrolldown` ohne Prüfung d7 < edrows (negatives d6 → BLTSIZE riesig).
14. `pixrows2` nur als Wort gespeichert (bpr*pixrows > 65535 abgeschnitten; im Bereich nicht
    gelesen).
15. `mwait` testet $BFE001 Bit 7 (Joystick-Feuer Port 2, nicht linke Maustaste Bit 6); toter
    Code.
16. Nach `tokit` mit leerer Zeile zeigt a1 auf die NUL, sonst dahinter –
    Inkonsistenz, falls ein Aufrufer a1 weiterverwendet.
