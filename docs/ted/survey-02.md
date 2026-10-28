# Survey 02 – ted.s Zeilen 1016–1980

Bereich: `escape` … `blokforget` (Ende vor `gomouse`, 1981). Inhalt: Beenden, Cursor
hoch/runter (inkl. Seitenweise mit Shift), `markindent`, Zeile einfügen/löschen,
`join`/`split`/`return`, `top`/`bottom`, V-Prop-Gadget, `gotolined0`/`gotolined02`,
externes Menü (`extmenu` → blitz2 `menurout`), Menü-Dispatch (`menu` über `menutable`),
`new`, Blockoperationen (`blokkill`, `blocksave`/`bloksave`, `blokload`, `blokcopy`,
`blokforget`).

Adressen (vasm-Referenz, `ted.labels`): escape 1BA8, cursdown 1BEE, beepem 1C20,
cursdownok 1C30, movecdown 1C36, cursup 1C68, markindent 1CC4, insertline 1D64,
deleteline 1DBA, freelast 1E54, freeonly 1EA6, join 1EAE, split 2002, return 215A,
top 2232, bottom 2238, gadget 2242, gotolined0 22AA, gotolined02 22B8, gd03 22BA,
extmenu 2362, menu 2396, new 23D6, deftitname 23F8, blokkill 2412, blocksave 2502,
bloksave 2514, blokload 25DC, done3 2670, done2 2684, done4 268C, done 269C,
blalldone 26BE, blokcopy2 26D6, blokcopy 26DE, bcopyok 272C, blokforget 2914.

Wichtige Fremdroutinen (Semantik nur so weit nötig, Details in den anderen Surveys):
`makeline` 3803 (ersetzt Item a4 durch neues Item aus `charbuff`/d5, gibt altes via
`freethis` frei, setzt `thisitem`=a4=neu, passt `firstitem`/`enditem` an),
`makefirst` 3799 (= `charstobuff(a2=charbuff2)` + `makeline`), `charstobuff` 4173
(a2 → `charbuff`, genau `chcols` Zeichen gelesen, Rest-Leerzeichen abgeschnitten,
NUL, d5 = Länge **inkl.** NUL; bei `tokenise` über `tokit`), `freethis` 4582 (gibt
**`thisitem`** frei, nicht a4!), `freeupa5` 3684 (a5 aushängen+freigeben, `numlines--`;
bei `numlines==1` neues Leer-Item via `afirst2`), `allocnull` 4547, `linka0` 3277,
`allocline` 3349, `bloksetup`/`bloksetup2` 3752/3727, `getbchar`/`getbchar2` 3849/3865,
`detoka5` 3759, `blokadj` 3919, `blokfix` 5384, `blokdis` 3903, `bloken` 3890,
`calcitem` 3783, `alloc2`/`outmem` 3033/3038, `erase`/`erase2` 3257.

---------------------------------------------------------------------------------------------
## A. Routinen

Konvention: a4 = aktuelles Item (`thisitem`) beim Eintritt (von `mainedit` gesetzt,
ted.s:145). Alle Routinen hier werden aus `jumptable` (6171) oder `menutable` (6199)
per `jsr (a0)` gerufen, Rückgabewerte/CCs sind für den Aufrufer irrelevant, außer wo
angegeben. „→ X“ = `bra X` (Tail-Call).

- `escape` (1016) : Menü QUIT ($90). `erase` (Abfrage bei `allalt`; Abbruch → `erase`
  macht `addq #4,a7`, d. h. Rückkehr direkt nach `menu`). Gibt das externe Menü frei
  (s. E), `doclean` (Rückruf `cleanup`), `freemice`, `clr allalt`, dann
  **`lea 12(a7),a7`** (verwirft: Rücksprung in `menu` 4 + Flagbyte aus `menu` 2 +
  Rücksprung in `mainedit` 4 + Flagbyte aus `mainedit` 2) und → `freeall`; dessen `rts`
  kehrt hinter `bsr mainedit` (ted.s:101) zurück = Editor-Ende. **Nur über `menu`
  erreichbar** (Stacktiefe fest verdrahtet). Kehrt nicht zum Aufrufer zurück.
- `cursdown` (1041) : Shift (`qualifier&3`): d0=curstop+cursy+edrows, min(numlines−1)
  → `gotolined02`. Sonst: `nextitem(a4)`=0 → `beepem`; sonst Durchfall in
  `cursdownok`.
- `beepem` (1054) : `pointeron`, → `DisplayBeep(myscreen)` (jmp in intuition). Wird von
  vielen Routinen (auch außerhalb, z. B. 328, 903, 2440) als Tail-Ziel benutzt; einmal
  per `bsr` (494). Keine Rückgabe.
- `cursdownok` (1058) : ein: d0=nächstes Item. `thisitem`=d0 (a4 bleibt alt!), fällt in
- `movecdown` (1061) : wenn curstop+edrows ≥ numlines oder cursy+1+vscmarg < edrows →
  `godown` (nur Cursor-Arithmetik); sonst `curstop++`, `scrupall`, → `showbotitem`.
  Zweiter Einsprung aus `return`.
- `cursup` (1074) : Shift: d0=curstop+cursy; <edrows → 0, sonst −edrows → `gotolined02`.
  Sonst `lastitem(a4)`=0 → `beepem`; `thisitem`=prev; curstop=0 oder cursy>vscmarg →
  `goup`; sonst `curstop--`, `scrdownall`, → `showtopitem`.
- `markindent` (1098) : Menü $b1. d0 = Einrückung (führende Spaces) der Zeile a4; leere
  Zeile (chars[0]=0) → `beepem`. Läuft nach oben/unten, solange Zeilen leer (nur
  Spaces) oder Einrückung ≥ d0 sind → d1=obere, d2=untere Zeile. Block := (0,d1)–
  (chcols−1,d2), `bloken`, → `showall`.
- `insertline` (1153) : Menü $51. `allocnull` (Z=1 → `outmem`, nicht-lokal), `allalt=-1`,
  `blokadj(cur)`, `numlines++`, neues Item **vor** a4 einketten (ggf. `firstitem`),
  `thisitem`=neu, d7=cursy → `scrolldown`.
- `deleteline` (1173) : Menü $61. Blockanpassung (s. G/H), dann: kein Nachfolger →
  `freelast`. Sonst `numlines--`, `freethis` (= a4), aushängen, `thisitem`=Nachfolger,
  d7=cursy, `scrollup`, → `showbotitem`.
- `freelast` (1208) : (nur Durchfall/bra aus `deleteline`) letzte Zeile löschen: kein
  Vorgänger → `freeonly`. Sonst `numlines--`, `freethis`, `thisitem`=`enditem`=prev,
  `nextitem(prev)=0`; curstop≠0: `curstop--`, `scrollup`, `scrdownall`, → `showtopitem`;
  sonst `scrollup`, → `goup`.
- `freeonly` (1226) : einzige Zeile: `freethis`, → `allocfirst` (neues Leer-Item +
  `cls`). `numlines` bleibt 1.
- `join` (1229) : Menü $81. Kein Nachfolger → `beepem`. a5=Nachfolger. d0 = Länge der
  aktuellen Bildschirmzeile (`chmap+chloc`, ohne Rest-Spaces). Blockanpassung. Baut in
  `charbuff2`: chmap-Zeile[0..d0) + Nachfolger (roh oder `detok`), auffüllen bis
  `charbuff3`; `charstobuff`, `makeline` (ersetzt a4). Nachfolger aushängen
  (`enditem` ggf.), `FreeMem(a1, numchars)` (**Größe falsch**, s. I), `numlines--`,
  → `showall`.
- `split` (1313) : Shift+Return (aus `return`). d0=cursoff+cursx. Blockanpassung.
  Kopiert chmap-Zeile ab d0 nach `charbuff2` und **überschreibt sie in `chmap` mit
  Spaces**; `charstobuff(a2=chmap-Zeile)`+`makeline` (aktuelle Zeile gekürzt);
  `charstobuff(charbuff2)`, `alloc2` neues Item, Kopie, **nach** a4 einketten
  (`enditem` ggf.), `numlines++`, → `showall`. Cursor bleibt.
- `return` (1402) : Taste 13. Shift → `split`. Sonst `allocnull` (→ `outmem`),
  `allalt`, `blokadj(cur+1)`, `numlines++`, Leer-Item **nach** a4 (`enditem` ggf.),
  `thisitem`=neu, `cursx=0`; war `cursoff`≠0: `clr cursoff`, `refresh`. `showhprop`.
  Wenn cursy+1+vscmarg ≥ edrows: `curstop++`, **temporär** `edrows`=cursy+1,
  `pixrows`=edrows*fontmul (auf Stack gesichert), `scrupall`, zurücksetzen, `rts`.
  Sonst d7=cursy+1, `scrolldown`, → `movecdown`. Kein Zeilen-Splitten, kein Auto-Indent.
- `top` (1453) / `bottom` (1456) : Menü $02/$12, d0=0 bzw. numlines−1 → `gotolined0`.
- `gadget` (1460) : Ereignis 255 (V-Prop). edrows ≥ numlines → `rts`. Merkt Zeile in
  **`bloktopx`** (Scratch!), `cursor` (XOR). Schleife: d0 = VertPot
  (`prop1sinfo+4`)·numlines>>16; geändert → `cursor`, `gotolined02`, `cursor`,
  `showrow`; `getinput2` (pollt, **Busy-Loop**, konsumiert die Nachricht) bis d7≠0;
  dann `cursor`, letzte Position → `gotolined02`.
- `gotolined0` (1490) : ein: d0.w = Zielzeile (0-basiert, **muss < numlines sein**),
  a4 = aktuelles Item. Push −1 (H-Prop aktualisieren), `cursx=0` (cursoff bleibt).
  → `gd03`.
- `gotolined02` (1495) : wie oben, Push 0 (H-Prop nicht). Fällt in
- `gd03` (1496) : Push alter `curstop`; a4 vom aktuellen Index nach d0 laufen (hoch
  oder runter, ohne NULL-Prüfung), `thisitem`=a4. Neue curstop/cursy: Ziel<vscmarg
  oder numlines≤edrows → curstop=0, cursy=Ziel; Ziel ≥ numlines−edrows+vscmarg →
  curstop=numlines−edrows, cursy=Ziel−curstop; sonst cursy=vscmarg,
  curstop=Ziel−vscmarg. `calcpos`; curstop geändert → `showall`; Flag≠0 →
  `showhprop` (Tail). Pop beide Worte. aus: a4=`thisitem`=Ziel-Item.
- `extmenu` (1542) : aus `menu` bei Menü 4. `menurout`=0 → `rts`. Sonst d6.w=d6>>4
  (Punktnummer), a5=`firstitem`, d7.w=`numlines`, `jsr (menurout)` → d0.w; `showme`;
  a4=`thisitem`; d0=−2 → `showall`; d0=−1 → `rts`; sonst → `gotolined0`.
- `menu` (1565) : Ereignis 254, ein: d6.w = Intuition-Menücode. d0=d6&15 (Menü),
  d6=(d6>>1)&$f0 (Punkt 0–15 im High-Nibble). Menü 4 → `extmenu` (vor dem `or`). Sonst
  Schlüssel = Punkt<<4|Menü, linear in `menutable` (6 Bytes je Eintrag: Schlüssel.b,
  Flag.b, Adresse.l; Ende $FF). Gefunden: **Flagbyte auf Stack** (`move.b …,-(a7)`,
  2 Bytes), `jsr (a0)` (berechneter Sprung), Flag≠0 → `showvprop` (Tail).
- `new` (1593) : Menü $00. `erase` (Abbruch → zurück in `menu`), `doclean`, `freeall`,
  kopiert `deftitname` nach `newsctit` (Bildschirmtitel), `showme`, → `donew`.
- `blokkill` (1607) : Menü $11 (auch Shift-131 über `blocksave`). Kein Block → `beepem`.
  `blokop=0`, `allalt`, `bloksetup` (a5/d6 = Blockanfang). Präfix Spalten 0..btopx−1
  (`getbchar2`, Spaces aufgefüllt) → `charbuff2`. Zeilen btopy..bboty−1 per
  `freeupa5` löschen. Bei mehrzeiligem Block `detoka5` (unbedingt, auch ohne
  `tokenise`). Suffix Spalten bbotx+1..chcols−1 anhängen, auffüllen. btopx=0 und
  bbotx=chcols−1 → letzte Zeile auch `freeupa5`; sonst a4=a5, `makefirst`. `blokdis`;
  Cursorzeile ≥ numlines → curstop=numlines−1, cursy=0, `calcitem`, `showall`,
  → `gotolined02`; sonst `calcitem`, → `showall`.
- `blocksave` (1671) : Taste 131. Shift → `blokkill`, → `showvprop`. Sonst Durchfall in
- `bloksave` (1677) : Menü $21. Kein Block → `beepem`. `getbname` (Abbruch: `addq #4,a7`
  → Rückkehr zum Aufrufer von bloksave; d0=1 → `outmem`), d7=Name, `doopen(1006)`
  (Fehler: `addq #4,a7` + `error`), d7=Handle. `bloksetup`, `blokop=-1` (Zeilenende
  bei Textende statt Spaces). Erste Zeile ab btopx zeichenweise (`Write` je 1 Byte,
  Puffer = **Byte in `bloktopx`**), LF; Zwischenzeilen via `detoka5` am Stück + LF
  (nur hier `Write`-Fehler geprüft → `error3`); letzte Zeile zeichenweise bis bbotx.
  → `doclose`. Das `rts` (1736) ist nur in der `cripple`-Version erreichbar.
- `blokload` (1738) : Menü $31. `blokop=0`, `getbname`, a4=`thisitem`, `doopen(1005)`,
  `fillbuff` (0 Bytes → `doclose`). `mousealt`, `allalt`. **`bloktopx` als Langwort** =
  Zeilenende in chmap. Präfix chmap[0..cursor) (Bit 7 gelöscht) → `charbuff`
  (a3), Zeiger auf Cursorposition auf Stack. Erste Zeile: `readline`; ohne EOL →
  `done2`; sonst `bloadadd` + `makeline2` (ersetzt a4). Weitere Zeilen per
  `allocline`+`linka0`. Ende: siehe `done*`. Suffix (Rest der chmap-Zeile, Bit 7
  gelöscht, begrenzt durch `charbuff2`) an `charbuff` → `blalldone`.
- `done3` (1778) : EOF nach LF: Leerzeile (`allocnull`/`linka0`), a3=`charbuff`, → `done`.
- `done2` (1783) : erste Zeile ohne EOL: `bloadadd`, → `done`.
- `done4` (1785) : letzte Zeile ohne EOL: `allocline`+`linka0` (wird sofort durch
  `makeline` in `blalldone` ersetzt – redundant), a3=`charbuff`, `bloadadd`, fällt in
- `done` (1791) : `doclose`, Pop Cursor-Zeiger, Suffix kopieren; fällt in
- `blalldone` (1801) : `charstobuff(a2=charbuff → charbuff, in-place)`, `makeline`,
  `calcitem`, `blokdis`, → `showall`.
  (`done`, `done2..4`, `blalldone` sind **globale** Labels nur für `blokload`.)
- `blokcopy2` (1808) : Taste 132: `bsr blokcopy`, → `showvprop`.
- `blokcopy` (1811) : Menü $01. Kein Block → `beepem`. Cursor im Block (zwischen
  Anfang und Ende, `inbloks`/`inblokf`) → `beepem`. Weiter in
- `bcopyok` (1832) : `bloksetup`; Präfix chmap[0..cursor) → `charbuff2` (d1=Länge),
  **Push d1/a1** (a1 = chmap-Zeiger auf Cursor). Blockzeichen per `getbchar` anhängen
  (über chcols hinaus verworfen). Zeilenwechsel: auffüllen, `charstobuff`, `alloc2`,
  neues Item **vor a4** einketten (`firstitem` ggf.), `numlines++`; Zwischenzeilen
  roh kopieren (tokenisiert, ohne Detok) und ebenfalls vor a4. Letzte Zeile ab
  Spalte 0 neu in `charbuff2`. Blockende (mi): **Pop d1/a1** (d1 = alte Präfixlänge!),
  Suffix chcols−d1 Zeichen aus chmap anhängen, `makefirst` (ersetzt a4). Blockposition
  verschieben, falls Kopie vor/auf Block (s. H). `calcitem`, → `showall`.
- `blokforget` (1976) : Menü $41. Kein Block → `beepem`; `blokdis`, → `showall`.

Tail-Calls in fremde Routinen: `freeall`, `gotolined0/02`, `godown`, `goup`,
`showbotitem`, `showtopitem`, `scrolldown`, `allocfirst`, `showall`, `showhprop`,
`showvprop`, `donew`, `doclose`, `outmem` (via `beq`), DisplayBeep (jmp LVO).

---------------------------------------------------------------------------------------------
## B. Registerkonventionen

- **a4 = aktuelles Item** beim Eintritt (aus `thisitem`, ted.s:145). Routinen setzen
  meist nur `thisitem`, nicht a4 (z. B. `cursdownok`, `insertline`) – `mainedit` lädt
  a4 jede Runde neu. Ausnahme: `gotolined0/02` laufen mit a4 und hinterlassen
  a4=Ziel; `makeline` setzt a4=neues Item.
- **d7 beim Eintritt** = Ereigniscode (jumptable-Schlüssel), **d6** = Gadget-/Menücode.
  Innerhalb: d7 = Startzeile für `scrollup`/`scrolldown`; in `bloksave`/`blokload`
  d7 = DOS-Dateihandle (Name vorher), d6 = Zeichen im `dosbuff`, a5 = Lesezeiger.
- Blockcursor (`bloksetup`): a5 = Item, d6 = Zeile, d5 = Spalte, `getbchar` liefert
  d0, **N=1 Blockende, Z=1 Zeilenende** (Zeile/a5 dann schon weitergeschaltet).
- a6: nach `doopen`/`calldos` = DOSBase (bloksave nutzt danach `write(a6)` direkt);
  `move.l 4,a6` vor jedem FreeMem/`alloc2`.
- Kein `movem`-Sichern in diesem Bereich; `extmenu` sichert vor dem Rückruf nichts
  (blitz2 `menu` muss das selbst tun; nach dem Rückruf wird nur a4 aus `thisitem`
  neu geladen).
- Stack-Konvention Dispatch: `mainedit` pusht Flagbyte (2) und setzt `memstack`=a7
  vor `jsr`; `menu` pusht nochmals Flagbyte (2). Davon hängen ab: `escape`
  (`lea 12`), `erase`/`getbname`/`doopen` (`addq #4` = Rückkehr eine Ebene höher),
  `outmem` (a7=`memstack`, → `memcont`).

---------------------------------------------------------------------------------------------
## C. In diesem Bereich definierte Globals

| Name | Typ | Init | Zweck |
|---|---|---|---|
| `deftitname` (1604) | char[26] + even | `'Blitz Basic 2 Editor V1.0',0` | Titel bei `new` |

Sonst nur Code. Benutzte Fremd-Globals (zur Info): `bloktopx` (6085, Wort) wird hier
als Scratch missbraucht (s. G); `menutable` (6199) ist eine Code-Zeiger-Tabelle
(30 Einträge à 6 Bytes, $FF-Ende) – wird hier interpretiert.

`menutable`-Belegung (Schlüssel Punkt<<4|Menü, Flag=showvprop danach):
Menü 0: $00 new(0) $10 fileopen(1) $20 filesave(0) $30 defaults(0) $40 doabout(0)
$50 prtfile(0) $60 mycli(0) $70 closewb(0) $80 openwb(0) $90 escape(0).
Menü 1: $01 blokcopy(1) $11 blokkill(1) $21 bloksave(0) $31 blokload(0) $41
blokforget(0) $51 insertline(1) $61 deleteline(1) $81 join(1) $91 blocktab(1)
$a1 blockuntab(1) $b1 markindent(0).
Menü 2: $02 top(1) $12 bottom(1) $22 gotoline(1).
Menü 3 (SEARCH): $03 find $13 next $23 previous $33 replace $43 replaceall (alle 1).
Menü 4: extern (`extmenu`).

`jumptable` (6171, gleiches Format, Ende 0): 28 cursup(1), 29 cursdown(1),
13 return(1), 255 gadget(1), 254 menu(0), 253 mousedown(1), 139 help(0),
129 blockstart(0), 130 blockend(0), 131 blocksave(0), 132 blokcopy2(0).

---------------------------------------------------------------------------------------------
## D. Selbstmodifizierender Code / Daten im Code

- Kein Patchen von Befehlen in diesem Bereich.
- `new` überschreibt `newsctit` (6312, im Code-Segment, PC-relativ per `lea`): Titel
  wird zu `deftitname` (kürzer, passt).
- `bloksave`: `move.l #bloktopx,d2` – absolute (relozierte) Adresse einer Code-Variablen
  als Write-Puffer.
- `escape` gibt das von `getstuff` (2786) zur Laufzeit per AllocMem gebaute externe
  Menü frei (Kopie von `menu2`-Daten).
- Keine Einsprünge in `incbin`-Blöcke hier (indirekt: `getbname` → `ezfreq` →
  `filerequest+36`).

---------------------------------------------------------------------------------------------
## E. Heap-/Listenstrukturen

**Zeilen-Item** (rsreset ted.s:15): `+0 nextitem.l`, `+4 lastitem.l`, `+8 numchars.b`
(Länge **inkl.** NUL), `+9 chars[]` (tokenisiert wenn `tokenise`, Token = 2 Bytes mit
Bit 7 im ersten). Allokation `numchars+lenitem` (lenitem=9), MEMF_PUBLIC (d1=1).
Globals: `firstitem`, `enditem`, `thisitem`, `numlines` (Wort). Doppelt verkettet,
`lastitem(first)=0`, `nextitem(end)=0`. Zeilennummern werden nirgends gespeichert,
sondern als curstop+cursy bzw. per Ablaufen berechnet.

- `allocnull`: 10 Bytes, `move #$100,numchars` → numchars=1, chars[0]=0.
- Erzeugen/Einketten hier: `insertline` (vor a4), `return` (nach a4), `split` (nach
  a4), `blokcopy` (vor a4), `blokload` (`linka0`: nach a4, a4 weiter). Ersetzen:
  `makeline` (join, split, blokkill, blokload, blokcopy). Löschen: `freethis`
  (deleteline/freelast/freeonly), `freeupa5` (blokkill), direkt FreeMem (join).

**Externes Menü** (aus `getstuff`, freigegeben in `escape`): `menu2` (6554) ist die
Menu-Struktur „SEARCH“; ihr `NextMenu` (+0) zeigt auf eine Kopie des Menu-Kopfs
(30 Bytes = `menu2name-menu2`, Menu-Struct) mit `FirstItem` (+18) → Kette von
54-Byte-Blöcken (= `m2i1text-menu2item1`: MenuItem 34 + IntuiText 20, verkettet über
+0 NextItem). `escape` gibt nur dieses eine Menü frei, setzt `menu2.NextMenu` nicht
zurück und ruft kein `ClearMenuStrip`.

**Zeichenpuffer** (Allokation ted.s:41: doslen(2048)+3·chcols+6): `charbuff`,
`charbuff2`, `charbuff3` je chcols+2 Bytes; `charbuff4` zeigt auf das Ende (unbenutzt).
`charbuff3[0]` = Länge+1 nach `detoka5`, Text ab `charbuff3+1`. In `join`/`split`/
`blokkill`/`blokcopy` dient `charbuff2` als Zeilenbaupuffer, Auffüllen „bis
`charbuff3`“ = chcols+2 Zeichen.

**chmap**: Bildschirm-Zeichenkarte, Zeile = `chmap+chloc`, chcols Bytes, Bit 7 =
Darstellungsflag (wird beim Übernehmen mit `bclr #7`/`and #127` gelöscht). Für die
aktuelle Zeile ist chmap die **maßgebliche Kopie** (Item ist ggf. veraltet, siehe
`anyalt` in `mainedit`): `join`, `split`, `blokload`, `blokcopy` lesen dort; `split`
schreibt dort.

**Mice-Liste**: Knoten 8 Bytes (`next.l`, `item.l`), `freemice` in `escape`. Wird nach
jedem Befehl bei `mousealt` neu aufgebaut (`showmice`), `remember`/`mforget2` setzen
nur `mousealt`.

---------------------------------------------------------------------------------------------
## F. Externe Aufrufe

- exec: `FreeMem` (escape: Menü-Blöcke; join: Item), `AllocMem` über `alloc2`/`allocnull`.
- dos: `Write` (bloksave, direkt `jsr write(a6)`), indirekt `Open`/`Read`/`Close`
  (`doopen`/`fillbuff`/`doclose`).
- intuition: `DisplayBeep` (beepem), indirekt `ShowTitle` (`showme`).
- **Rückruf in blitz2**: `extmenu` → `menurout` (edstruct+16, blitz2.s:1498):
  ein d6.w = Menüpunkt, a5 = `firstitem`, d7.w = `numlines`; aus d0.w: −1 nichts,
  −2 `showall`, sonst Zeilennummer → `gotolined0`. Obere Wörter von d6/d7 undefiniert
  (`lsr.w`/`move.w`). Der Rückruf kann `extalloc` (comdata+88) rufen; Fehlschlag →
  `outmem` setzt a7=`memstack` und verwirft die blitz2-Stackrahmen.
- `doclean` → `cleanup` (edstruct+20, normal 0) in `escape`/`new`.
- V-Prop: liest `prop1sinfo+4` (PropInfo.VertPot) direkt, während Intuition es ändert.
- Eingabe: `getinput2` (GetMsg ohne Wait) in `gadget`-Schleife; Ereignis-Codes 254/255.
- `qualifier&3` = Shift links/rechts (Rawkey-Qualifier der letzten Taste).
- Blitter indirekt über `scrollup`/`scrolldown`/`scrupall`/`scrdownall` (`getblitter`).

---------------------------------------------------------------------------------------------
## G. Was in C Sonderbehandlung braucht

- **Dispatch-Stack**: `escape` (`lea 12(a7)`), `erase`/`getbname`/`doopen`
  (`addq #4,a7`), `outmem` (a7=`memstack`). In C: Handler geben Status zurück
  (`ED_ABORT`, `ED_QUIT`) oder `setjmp/longjmp` für `outmem` (Punkt in `mainedit`
  vor dem Handleraufruf). `escape` → Rückgabe „Editor beenden“ an `mainedit`.
- **`menurout`-Aufruf**: Register-Rückruf in blitz2 (d6/a5/d7 → d0) → Asm-Stub in
  `glue*.c`; muss alle Register ab dort definiert setzen und nichts erwarten.
- `menutable`/`jumptable`: in C Struct-Array `{uint8 key, flag; void (*fn)(void);}`;
  Flag-Byte als lokale Variable statt Stack.
- `gotolined0`/`gotolined02`: zwei Einsprünge mit gepushtem Flag → C-Funktion mit
  Parameter `int update_hprop`; `gd03` intern.
- Durchfälle: `cursdown`→`cursdownok`→`movecdown`, `blocksave`→`bloksave`,
  `done4`→`done`→`blalldone`, `blokcopy`→`bcopyok`, `deleteline`→`freelast`→`freeonly`
  (nur per `beq`). Mehrfacheinsprünge `movecdown` (return), `beepem` (global).
- **`bloktopx`-Aliasing**: Wort-Variable, in `gadget` als Wort-Scratch, in `bloksave`
  als 1-Byte-Schreibpuffer (High-Byte!), in `blokload` als **Langwort** (überschreibt
  `bloktopx`+`bloktopy`). Kollidiert mit laufender Maus-Blockmarkierung (2023ff.). In C
  eigene lokale Variablen; `bloktopx/bloktopy` nach blokload ggf. bewusst zerstören
  (Verhaltensgleichheit) oder BUG(orig)-Marker.
- Byte-Push auf a7 (`move.b …,-(a7)`) belegt 2 Bytes – für Stacktiefen in `escape`
  relevant.
- CCs über Ebenen: `getbchar` (N/Z), `allocnull` (Z=1 Fehler), `erase2` (Z=1 OK),
  `inbloks`/`inblokf` (Z=1 durch `moveq #0,d3`, sonst Z vom `cmp`/`bhi`),
  `readline` (N=1 nichts, Z=1 ohne EOL), `fillbuff` (Z=1 0 Bytes).
- Vorzeichen: Vergleiche fast alle **unsigned** (`bcs/bcc/bhi/bls`) auf Wörtern;
  `bboty=-1` = „kein Block“ wird mit `cmp #-1` geprüft. `markindent`/`join` nutzen
  signed `blt`/`bmi`.
- `numchars` ist Byte; `join` addiert per `add.b` (Überlauf bei 255 möglich).
- Lesen über NULL: `bloksave` (letzte Blockzeile = letzte Textzeile, bbotx hinter
  Textende → `detoka5` auf a5=0), `gd03` (Ziel ≥ numlines). Auf dem Amiga still
  „funktionierend“, in C/Host abfangen.

---------------------------------------------------------------------------------------------
## H. Einstufung

| Routine | Klasse | Bemerkung / Schnitt für Host-Kern |
|---|---|---|
| escape | UI/GLUE | Stacktrick; Menüfreigabe + `freeall` (KERN-Teil: `freeall`) |
| cursdown/cursup | UI | Kern-Anteil nur `thisitem`=next/prev und Seitenarithmetik |
| cursdownok/movecdown | UI | Scroll-Entscheidung (curstop/vscmarg) rein arithmetisch |
| beepem | UI | Hook `ui_beep()` |
| markindent | KERN | rein Liste; danach `bloken`+`showall` → Hook `ui_block_changed`, `ui_redraw_all` |
| insertline | KERN+UI | Kern: alloc, `blokadj`, numlines, Einketten vor a4, thisitem. Danach `scrolldown(cursy)` → Hook `ui_line_inserted(row)` |
| deleteline | KERN+UI | Kern: Blockanpassung, Aushängen/Freigeben, thisitem. UI je Fall: Mitte `scrollup`+`showbotitem`; letzte Zeile `scrollup`+(`scrdownall`+`showtopitem` bzw. `goup`); einzige Zeile `allocfirst` (enthält `cls`!). Kern gibt Fall zurück |
| freelast/freeonly | KERN+UI | wie oben; `allocfirst` = `afirst2` (KERN) + `cls` (UI) |
| join | KERN+UI | Eingabe = chmap-Zeile (Host-Puffer). Kern bis `numlines--`; danach `showall` |
| split | KERN+UI | wie join; schreibt chmap (muss im Kern bleiben, da chmap die aktuelle Zeile ist) |
| return | KERN+UI | Kern: alloc, `blokadj(cur+1)`, Einketten, thisitem, cursx=0. UI: `refresh`, `showhprop`, Scroll mit temporärem edrows/pixrows, `scrolldown`. `curstop++` ist Zustand, gehört zum Kern |
| top/bottom | KERN | → gotolined0 |
| gadget | UI | Busy-Loop mit Ereignisabfrage |
| gotolined0/02/gd03 | KERN+UI | Kern: Item-Lauf + curstop/cursy-Berechnung (pure). UI: `calcpos`, `showall` bei curstop-Änderung, `showhprop` |
| extmenu | GLUE | Register-Rückruf; danach UI (`showme`, `showall`) |
| menu | KERN | Tabellensuche pure; Aufruf + `showvprop` UI |
| new | KERN+UI | `erase` (Requester), `freeall`/`donew` (donew mischt `blokdis`, `allocfirst`→`cls`, `showhprop/showvprop`) |
| blokkill | KERN+UI | Kern bis inkl. `blokdis`-Zustand (bboty=−1); `blokdis` zeichnet Statuszeile + Menüflags → Hook. Ende `showall`/`gotolined02` |
| blocksave | UI | Qualifier-Weiche |
| bloksave | KERN | Dateischreiben (abstrahierbar: `Write`); `getbname` (Requester), `error` (Requester) → Callbacks |
| blokload, done*, blalldone | KERN | Datei lesen; Eingabe chmap-Zeile/Cursorspalte; Requester per Callback; Ende `blokdis`+`showall` |
| blokcopy/blokcopy2/bcopyok | KERN+UI | Kern: Kopie + Blockverschiebung; `showall`/`showvprop` danach |
| blokforget | KERN+UI | `blokdis` + `showall` |

Allgemein: Die Kernoperationen lassen sich trennen, wenn (1) chmap/chloc/chcols/
cursx/cursoff/curstop/cursy als Editor-Zustand im Kern liegen (kein OS),
(2) `blokdis`/`bloken` in Zustandsänderung (bboty=−1/bstx=−1, Menü-Flags-Bits) und
Anzeige (`blokprnt2`) zerlegt werden, (3) Redraw-Aufrufe am Ende als Hooks
(`ui_scroll_up(row)`, `ui_scroll_down(row)`, `ui_scr_up_all()`,
`ui_scr_down_all()`, `ui_show_top_item()`, `ui_show_bot_item()`,
`ui_redraw_all()`, `ui_refresh()`, `ui_show_hprop/vprop()`, `ui_beep()`) bleiben –
in allen Routinen dieses Bereichs liegt das Zeichnen **nach** der Listenänderung,
einzige Verschränkung: `return` (Scroll mit temporär geändertem edrows/pixrows nach
`curstop++`), `freeonly` (`cls` in `allocfirst`), `gadget` (Schleife).
`goup`/`godown`/`calcpos` ändern nur Variablen (cursy, scloc, chloc) – im Kern
belassen (scloc ist ein Bitmap-Offset, als Zahl harmlos).

**Blockkoordinaten-Logik** (pure, gut testbar; btopx/btopy/bbotx/bboty, bboty=−1 kein
Block):
- `blokadj(d0)`: d0 ≤ bboty → bboty++; d0 ≤ btopy → btopy++ (insertline: cur,
  return: cur+1).
- deleteline: cur < btopy → beide −1; btopy ≤ cur ≤ bboty → bboty−1, wenn dann
  bboty < btopy: bboty=0, dann beide −1 (→ bboty=−1, btopy−1); cur > bboty → nichts.
- join (cur < bboty, d1=cur+1, d0=Länge aktuelle Zeile): d1<btopy → beide −1;
  d1=btopy=bboty → btopx+=d0, bbotx=btopx+Breite, `blokfix`, beide −1;
  d1=btopy<bboty → btopx+=d0, `blokfix`, beide −1; d1=bboty>btopy → bbotx+=d0,
  `blokfix`, bboty−1; sonst bboty−1.
- split (d0=Spalte): cur>bboty nichts; cur<btopy beide +1; btopy<cur<bboty bboty+1;
  cur=btopy<bboty: d0>btopx → bboty+1, sonst btopx−=d0, beide +1;
  cur=bboty>btopy: d0>bbotx nichts, sonst bbotx−=d0, bboty+1;
  cur=btopy=bboty: d0>bbotx nichts; d0>btopx → bbotx−=d0, bboty+1; sonst beide x −d0,
  `blokfix`, beide +1.
- blokcopy (nur wenn Kopie vor/auf Blockende): d1=bboty−btopy; bbotx=chcols−1 →
  d1++, und bei cur≥btopy btopx−=d0; sonst cur<btopy nichts weiter; cur=bboty (=btopy)
  → x beide +(bbotx−btopx+1), `blokfix`; cur=btopy<bboty → btopx+=bbotx−d0+1,
  `blokfix`; dann btopy+=d1, bboty+=d1.

---------------------------------------------------------------------------------------------
## I. Auffälligkeiten / mögliche Bugs des Originals

1. **`join` FreeMem mit falscher Größe** (1305–1309): `moveq #0,d0; add.b numchars(a1),d0`
   – es fehlt `+lenitem` (9). Im Binary so assembliert (1FE0: `7000 d0290008`). Pro
   Join bleiben 9 Bytes (gerundet) als Leck. Kommentar „was move.b“ deutet auf
   Nachbesserung hin. BUG(orig).
2. `join` gibt die angehängte Zeile ohne `mforget2` frei → `mousealt` nicht gesetzt;
   hatte die Zeile ein Label, bleibt ein Mice-Knoten mit hängendem Zeiger bis zum
   nächsten Neuaufbau.
3. `join`/`split`/`blokcopy`/`blokload`: keine Längenprüfung. Ergebniszeilen > chcols
   werden von `charstobuff` stumm auf chcols abgeschnitten; `join` (Präfix + `detok`
   der Folgezeile) und `blokload` (`bloadadd`) können über `charbuff2` hinaus in
   `charbuff3` schreiben, `join` bei expandierenden Token sogar bis ans Allokationsende.
4. `blokcopy` `.oneline`: `movem.l (a7)+,d1/a1` stellt die **Präfixlänge** wieder her,
   nicht die aktuelle Länge in `charbuff2` → es werden immer chcols−Präfix
   Suffixzeichen angehängt; die Zeile ist um die Fragmentlänge zu lang und wird
   abgeschnitten (meist nur Rest-Spaces, bei langen Zeilen echter Textverlust).
5. Nach `blokfix` → `blokdis` (bboty=−1) ändern `join` (−1), `split`/`blokcopy` (+d1)
   bboty weiter → bboty=−2/0…: „Geisterblock“ ohne aktivierte Menüpunkte.
6. `deleteline` auf einem einzeiligen Block setzt bboty über `clr`+`sub` auf −1, ohne
   `blokdis` (Menüpunkte/Statustext „block“ bleiben, `bstx` unverändert).
7. `escape`: externes Menü wird freigegeben, während der Menüstreifen noch am Fenster
   hängt (kein `ClearMenuStrip`), `menu2.NextMenu` nicht genullt. Hat `menitems` einen
   Titel aber keine Punkte, enthält die Kopie noch `FirstItem=menu2item1` (statisch!)
   → FreeMem auf Programmdaten (getstuff 2808ff.).
8. `blokkill` ruft `detoka5` auch ohne `tokenise`; `bloksave` `.bok` ebenfalls. Bei
   Bytes ≥$80 im Nicht-Token-Modus läuft `findtoke` mit `tokens`=0 ab Adresse 0.
9. `bloksave`: nur `Write` der ganzen Zwischenzeilen wird geprüft; Einzelbyte-Writes
   nicht. Schreibt byteweise unbuffered (langsam). Liest ggf. über das Listenende
   (s. G).
10. `blokload`: Präfix/Suffix aus chmap sind Klartext, geladene Zeile tokenisiert;
    danach `charstobuff` in-place über `charbuff` (Quelle=Ziel) – bei `tokenise`
    wird gemischter Inhalt erneut durch `tokit` geschickt (prüfen, ob `tokit`
    Token-Bytes durchreicht). `done4` alloziert die letzte Zeile, die `blalldone`
    sofort ersetzt (überflüssige Alloc/Free). `bloktopx/bloktopy` werden zerstört.
11. `gadget`: Busy-Polling ohne `WaitPort`; die beendende Nachricht (z. B. Taste)
    wird verschluckt.
12. `gotolined0` setzt `cursx=0`, aber nicht `cursoff` (auskommentiert 1492) – bei
    horizontal gescrolltem Bild steht der Cursor auf Spalte cursoff.
13. `gd03`: keine Bereichsprüfung; d0 ≥ numlines (oder negativ, z. B. Rückgabe <−2
    aus `menurout`) läuft über NULL.
14. `menu`: nur Menü 0–15/Punkt 0–15 unterscheidbar (Bit 4 der Menünummer, Bits 9–10
    der Punktnummer ignoriert); Unterpunkte ignoriert.
15. `new` setzt den Titel auf „Blitz Basic 2 Editor V1.0“, obwohl der Starttitel
    „Blitz Basic 2 - Ted Version 1.1“ ist (Kosmetik).
16. `split`/`blokcopy` erzeugen neue Zeilen per `alloc2` ohne `remember`-Prüfung
    (beginnt die neue Zeile mit Label, wird `mousealt` nicht gesetzt).
17. `freelast` bei curstop≠0: `scrollup` und unmittelbar `scrdownall` (doppelte
    Blitteroperation; nur Optik).
18. `join` `.nextbot`: `cmp btopy(pc),d1; beq .nextboth` ist toter Code (d1≠btopy
    dort garantiert).
