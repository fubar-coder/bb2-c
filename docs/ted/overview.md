# TED 1.1 (`ted.s`) – Überblick für den C-Port

Quelle: `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/ted.s`
(8072 Zeilen, Devpac, 68000) + Includes `myoffs` (LVOs), `wbheader` (Startcode,
derselbe wie bei blitz2), `aboutreq`, `specialreq` (Requester-Daten).
Referenz-Build und Binär-Includes: [reference.md](reference.md).
Querverweise zwischen den Chunks: [xrefs.md](xrefs.md) (`tools/xrefs.py`).
Auftrag der Surveys: [SURVEY-BRIEF.md](SURVEY-BRIEF.md). Plan: [PLAN.md](PLAN.md).

## 1 Chunks

| # | Zeilen | Umfang | Survey | Inhalt |
|---|---|---|---|---|
| 01 | 1–1015 + `wbheader` (114), `aboutreq` (113), `specialreq` (138) | ~1380 | [survey-01](survey-01.md) | Zeilen-`rs`-Offsets, Init-Einsprung `outside` (Hunk+4) und `.tomain`, Standalone-Start `edstart`, Fail-Kette, `mainedit`/`memcont` (Hauptschleife, `jumptable`-Dispatch), Mauszeiger/Sanduhr mit VBlank-Server, Menüs an/aus, `blocktab`/`blockuntab`, Optionsrequester `defaults`, Drucken `prtfile`, `.xtra`/`.info`, Laden/Speichern (`fileopen*`, `filesave`, `realsave`), About, Suchen/Ersetzen |
| 02 | 1016–1980 | 965 | [survey-02](survey-02.md) | `escape` (Ende), Cursor hoch/runter, `markindent`, Zeile einfügen/löschen, `join`/`split`, `return`, `top`/`bottom`, Scroll-Gadgets, `gotolined0`, `extmenu` (Rückruf in blitz2), `menu`-Dispatch (`menutable`), `new`, Blockoperationen (kill/save/load/copy/forget) |
| 03 | 1981–3062 | 1082 | [survey-03](survey-03.md) | Maus (`gomouse`, `mousedown`: Klick/Ziehen/Scrollen), Goto-Requester, `blockstart`/`blockend`, WB öffnen/schließen, Konsolenfenster `openio`/`closeio` (für laufende Programme), comdata-Dienste (`refgads`, `tokone`, `tokall`, `detokallz`, `actgad`), Hilfe (Token-Hilfe aus der Tokenliste), `doclean`, `getstuff` (Menü COMPILER aus `menitems` bauen), Label-Spalte „mice“, `outmem`/`alloc2`/`extalloc` |
| 04 | 3063–4197 | 1135 | [survey-04](survey-04.md) | Optionsdatei `l:blitzeditor.opts`, Options-Requester-Hilfen, Datei-I/O (`doopen`/`doread`/`fillbuff`/`readline`/`allocline`), `detok`/`findtoke`, Dateirequester-Brücke `ezfreq`, `error`, `openreq`/`closereq` (Requester als Fenster), Suche `isithere2`, Zeilenliste (`freeupa5`, `makeline`, `calcitem`), Block-Hilfen (`getbchar`, `sortblox`, `invertarea`), Zeilenausgabe (`showall`, `showa2item`, `tokprint`), `charstobuff` |
| 05 | 4198–5252 | 1055 | [survey-05](survey-05.md) | **Tokenizer `tokit`**, `alpha`, Scrollen (Blitter), Zeilen anlegen/freigeben (`allocnull`, `freethis`, `freeall`, `donew`), Bildschirm (`calcscreen`, `screenopen`, `makecop`, `screenclose`), Farben, `getblitter`/`putblitter`, `cls`, horizontales Scrollen, eigene Textausgabe (`print*`, `fontcalc`), `calcpos` |
| 06 | 5253–6285 | 1033 | [survey-06](survey-06.md) | Cursor, Prop-Gadgets, `refresh`, Blockgrenzen (`blokfix`), **Zeileneditor `getline`**, `horizpos`, `getinput`/`handled0` (IDCMP, Rawkey über `rawed`), Datenblock: `comdata` (−8…+94), Editor-Globals, Optionsblock `defstart` (= Dateiformat der Optionsdatei), `jumptable`/`menutable`, Fehlertexte, `edstruct`-Kopie |
| 07 | 6286–8072 | 1787 (fast nur Daten) | [survey-07](survey-07.md) | NewScreen/NewWindow/Gadgets, Menüleiste (PROJECT/EDIT/SOURCE/SEARCH), Requester `findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`, `prtreq`, `undobuffer` (nur String-Gadget-Undo, TED 1.1 hat **kein** Editor-Undo), `rawed` (incbin), `flink` + `filerequest` (incbin), Fonts |

Chunk 07 ist groß, aber zu 90 % Intuition-Strukturen; sie lassen sich mit
einer erweiterten Fassung von `blitz2-c/tools/req2c.py` erzeugen (survey-07 G).

## 2 Architektur von TED in einem Bild

```
blitz2 (Hauptprogramm)                     TED (LoadSeg-Segment)
 jsr BADDR(seg)+8 ─ a1=edstruct ─────────► outside (Hunk+4): 28 Bytes edstruct kopieren,
                                           Screen/Fenster, rawed-Init, flink, Puffer
 ◄──────── a0=.tomain, a1=comdata, Z=1 ──
 jsr (a0) ─ a0=Dateiname ────────────────► .tomain → Datei laden → mainedit-Schleife:
                                             getline (Zeileneditor, chmap = Zeile)
                                             → charstobuff/tokit → makeline
                                             → jumptable[d7] (Taste/Menü/Maus/Gadget)
 menu (d6,a5=firstitem,d7=numlines) ◄───── extmenu (Menü COMPILER) ── d0 = Zeile/−1/−2
 loadxtra/savextra (d7=fh,a6=dos)   ◄───── fileopen2 / realsave
 cleanup (edstruct+20)              ◄───── doclean (Wert 0 → nicht gerufen)
 comdata+0…+94 (openreq, getinput,  ─────► Dienste; einige kehren bei Fehler nicht
   error, ezfreq, tokone, extalloc …)       zurück (outmem → longjmp nach memcont)
```

Zentrale Datenmodelle:

* **Zeilenliste**: doppelt verkettet, `{nextitem.l, lastitem.l, numchars.b
  (Länge inkl. 0), chars[] tokenisiert, 0}`, AllocMem(`numchars+9`,
  MEMF_PUBLIC). Köpfe `firstitem`, `enditem`, `thisitem`, `numlines` (Wort).
  Nie leer. blitz2 liest diese Liste direkt (Layout fest).
* **chmap**: Zeichenkarte des Bildschirms (`chcols` = 128 Bytes je Zeile,
  Chip-RAM, wird mit dem Blitter gescrollt). Die **gerade bearbeitete Zeile
  existiert nur in chmap** (Bit 7 = Zeichen aus einem Token-Namen); beim
  Verlassen macht `charstobuff` → `tokit` → `makeline` daraus wieder einen
  Listenknoten. `join`, `split`, `blokload`, `blokcopy` lesen/schreiben chmap.
  → chmap gehört in den Host-Kern.
* **Cursor-/Blockzustand**: `cursx/cursy`, `cursoff/curstop`, `scloc/chloc`,
  `btopx/btopy/bbotx/bboty` (`bboty = -1` = kein Block) – Wortpaare, die der
  Code oft als Langwort kopiert/vergleicht.
* **Tokenliste** von blitz2 (`{next.l, num.w, name\0, help\0, even}`),
  Trenner `colon`/`quote`/`label`/`remark` aus edstruct (Standalone: 255 =
  aus, dann kein Quote-/Remark-Modus im Tokenizer).
* **Optionsblock** `defstart..defend` (50 Bytes) = Dateiformat von
  `l:blitzeditor.opts`.

## 3 Querschnitt aus den Surveys

### 3.1 Register- und Kontrollfluss-Konventionen

* Hauptschleife: a4 = `thisitem`, d7 = Ereigniscode (Taste 1…$FF, Bit 30 =
  Umlaut aus `germs`; −1 Gadget, −2 Menü, −3/−4 Maus), d6 = ID. `jumptable`
  (6-Byte-Einträge `{code.b, flag.b, fn.l}`) wird per `cmp.b` durchsucht
  (−1/−2/−3 = $FF/$FE/$FD – Latin-1 `$FD–$FF` kollidieren, survey-06 I).
* Laden: d7 = FileHandle, a5 = Lesezeiger, d6 = Restbytes (`doread`).
  Blockdurchlauf: a5 = item, d6 = Zeile, d5 = Spalte (`getbchar`).
  Ausgabe: a2/a3/d2/d3 (`showa2item`). Tokenizer: a1/a2/d1/d5.
* **Nicht-lokale Rücksprünge**: `outmem` (a7 = `memstack`, `jmp memcont` – auch
  aus blitz2 heraus über `extalloc`/`openreq`); `addq #4,a7` in `erase`,
  `getfname3`, `doopen` (Abbruch des Aufrufers); `escape` `lea 12(a7)`
  (Stacktiefe Dispatch 4+2 + Menü 4+2); `pchover` im Tokenizer; `mousedown`
  (`addq #8,a7`); `reline` liest Parameter vom Stack; `charstobuff`/`tokit`/
  `enddo` teilen ein Stackwort.
* **Condition Codes als Ergebnis** überall (readline dreiwertig N/Z,
  `getbchar`, `alpha`, `allocnull`, `erase2`, `openio`, `screenopen` …) →
  in C Rückgabewerte.
* blitz2-Einsprünge in comdata sichern teils nichts (`openreq`, `error` nur
  d7, `screenopen2`, `colors`) – die C-Stubs dürfen mehr sichern.

### 3.2 OS und Hardware

| Bereich | Nutzung |
|---|---|
| exec | AllocMem/FreeMem (Zeilen, Puffer, Menüs; Chip für Zeiger/chmap), AddIntServer(VERTB) für die Sanduhr-Animation, FindTask/AllocSignal, OpenDevice/CloseDevice (console), GetMsg/ReplyMsg |
| dos | Open/Read/Write/Close, Rename (.bak), Lock/CurrentDir, DeleteFile, Execute (Hilfe, CLI), Delay, DOS-Liste (`findio`) |
| intuition | OpenScreen (ExtNewScreen mit Tags), OpenWindow, Menüs, Requester **als eigene Fenster** (nie `Request()`), NewModifyProp, RefreshGList, SetPointer/ClearPointer, ShowTitle, DisplayAlert, WBenchToFront/CloseWorkBench/OpenWorkBench |
| graphics | OwnBlitter/WaitBlit/DisownBlitter, SetRGB4, UCopList (Farbverlauf in der Titelleiste), Sprites über SetPointer |
| console.device | nur `RawKeyConvert` (in `rawed.obj`) |
| Hardware direkt | Blitter über `$DFF000` (Scrollen, cls, chmap), CPU schreibt Glyphen direkt in die Bitplanes (planar, kein RTG), `flash` schreibt `$DFF180` (unbenutzt), `mwait` liest CIA `$BFE001` (toter Code) |

### 3.3 Selbstmodifikation und Daten im Code

* Kein Patchen von Befehlen. Aber: `flink` reloziert den eingebetteten
  `filereq`-Hunk zur Laufzeit (einmalig, ohne Cache-Flush); `openreq`
  verschiebt beim ersten Öffnen die Gadgets jedes Requesters (+6/+12) und
  merkt sich das im UserData des ersten Gadgets – **auch bei Requestern von
  blitz2**.
* Viele Daten liegen PC-relativ zwischen dem Code und werden beschrieben
  (Intuition-Strukturen, Flags, Puffer) – in C normale, schreibbare Statics.
* Feste Schrittweiten über Struktur-Grenzen (84/86/44/56/28/54 Bytes,
  survey-07 G.1) und Alias-Labels mitten in Strukturen (`int1..3`, `errorpnt`,
  `viewmode`, `winscreen`, `idcmp`).
* Sentinel-Wort `' :'` vor `filepath`; Wortpaare als Langwort; `bloktopx`
  als Wort, Byte-Puffer und Zeiger; `nummice` `dc.l`, als Wort benutzt.

### 3.4 Einstufung (Zusammenfassung der H-Tabellen)

| Klasse | Routinen (Auswahl) |
|---|---|
| KERN (host-testbar) | Tokenizer `tokit*`/`alpha`/`mytoke`/`tokone`, `detok`/`findtoke`/`detoka5`/`charstobuff`, Hilfe-Suche, Laden `readline`/`doread`/`allocline`/`linka0`, Schreibschleife `realsave`, `.bak`/`.xtra`/`.info`-Logik, Optionsdatei, Zeilenliste (`allocnull`, `makeline`, `freeupa5`, `freethis`, `freeall`, `calcitem`), Kernanteile von `insertline`/`deleteline`/`join`/`split`/`return`/`gotolined0`, Blocklogik (`blokadj`, `sortblox`, `orderem`, `inbloks`, `blokfix`, `getbchar`, `bloksetup`, `blokcopy`, `bloksave`/`blokload`, `blocktab`/`blockuntab`, `markindent`), Suchen/Ersetzen (`isithere2`, `dosearch`, `replace2`), Zeilenpuffer-Operationen von `getline`, `horizpos`/Cursor-Arithmetik, `mice`-Liste, `a0toname`, Druckformatierung, Sondertasten-Mapping von `rawed`, `germs`, Dispatch-Tabellen |
| UI | Bildschirm/Fenster/Menüs/Requester, Zeichnen (`print*`, `showall`, Scroll-Blits, `cursor`, `invertarea`), Props, Maus, IDCMP (`getinput`/`handled0`), Konsolenfenster, Zeiger |
| GLUE (Asm-Stub) | Hunk-Anfang + `outside`/`.tomain`, alle comdata-Einsprünge (Registerkonvention), Rückrufe nach blitz2 (`menurout`, `loadxtra`, `savextra`, `cleanup`), VBlank-Server `myint`, `outmem`-Longjmp über fremde Frames |

Alle Surveys stellen fest: Die Kernroutinen ändern zuerst die Daten und
zeichnen danach – die Ausgabe lässt sich als Hook-Aufrufe am Ende abtrennen.
Ausnahmen (verschränkt): `return` (Scroll mit temporär geändertem
`edrows`/`pixrows`), `freeonly`/`allocfirst` (`cls`), `gadget`/`mousedown`
(Polling-Schleifen), `getline` (Zeichnen nach jedem Tastendruck).

## 4 Risiken

1. **`outmem` als Longjmp über blitz2 hinweg** (comdata `extalloc`,
   `openreq`): verwirft Compiler-Frames (auch die von blitz2-c, inkl. dessen
   `errjmp`-Kette). Vor der ersten Hauptschleife ist `memstack = 0` → Absturz.
   Muss in C exakt so (oder bewusst als `DIFF`) gelöst werden.
2. **Register-Schnittstelle zu zwei Compilern**: TED-C muss mit blitz2 1.95
   (asm, vasm-Build) **und** blitz2-c laufen; jeder Stub lädt sein eigenes a4
   (`-fbaserel`), Rückrufe nach blitz2 über `callregs`.
3. **Hunk-Layout**: Hunk+4 = `outside`, kein libnix-Startcode davor; eigener
   Standalone-Start (wbheader-Ersatz) → `-nostartfiles`, Bibliotheksbasen
   selbst setzen.
4. **Interrupt-Code** (`myroutine`, VBlank): Glue mit a4-Laden, `volatile`,
   Z-Flag-Rückgabe; schreibt in Chip-RAM-Sprite-Daten.
5. **Hardware/Timing**: direkter Blitter (`$DFF000`) zwischen
   OwnBlitter/DisownBlitter, CPU-Zeichnen direkt in planare Bitplanes,
   chmap im Chip-RAM und vom Blitter mitbearbeitet. Läuft nur auf nativen
   Chipset-Screens (kein RTG) – im Emulator nur so testen. Busy-Polls
   (`gadget`, `mousedown` über `getinput2`) und `Delay`.
6. **Rawkey/Konsole**: `rawed` liefert F-Tasten/Cursor/Help als eigene Codes
   (`$81–$8B`, `$1C–$1F`), Qualifier separat, Umlaute über `germs` mit Bit 30;
   `OpenDevice`-Fehler unerkannt; `cmp.b`-Dispatch macht Latin-1 `$FD–$FF`
   zu Maus/Menü/Gadget. Emulator-Tests müssen echte RAWKEY-IntuiMessages mit
   Qualifier erzeugen.
7. **Stack-Tricks** (`addq #4,a7`, `lea 12(a7)`, `reline`-Parameter,
   `charstobuff`↔`enddo`) → in C Rückgabestatus; Gefahr, Abbruchpfade zu
   vergessen (Liste der betroffenen Aufrufer in survey-01/02/04 G).
8. **Byte/Wort-Aliasing** und big-endian-Annahmen (Wortpaare als Long,
   Gadget-Flags `+13`, `comdata+84` zeigt auf ein Flags-Low-Byte) – beim
   Host-Build darf der Kern **keine** dieser Aliasierungen benutzen.
9. **Original-Bugs mit Speicherfolgen** (müssen als `BUG(orig)` erhalten
   bleiben, auf dem Host aber Sanitizer stören): `join` gibt mit falscher
   Größe frei (`+lenitem` fehlt), `detoka5` schreibt hinter `charbuff3`
   (Allokationsende), Lesen über NULL in `bloksave`/`gd03`, Tokenizer liest
   über das Zeilenende, `tabstop = 0` → Division durch null. Für Host-Tests
   braucht es einen Modus, der diese Stellen erkennt statt abzustürzen.
10. **Dateirequester**: Wiederverwendung von `libs-c/libs/freqlib.c` setzt
    voraus, dass der Requester-Kern dort sauber von der Lib-Hülle trennbar ist;
    sein bekannter Überlauf bei hohen Fenstern betrifft TED genauso.

## 5 Wichtigste BUG(orig)-Kandidaten (Auswahl, Details in den Surveys, Abschnitt I)

* `join` FreeMem ohne `lenitem` (bestätigt im Referenzlisting, `$1FE0`) – survey-02.
* `blokfixb` vergleicht `btopy` statt `bbotx` mit `chcols` – survey-06.
* `readline`: letzte Zeile ohne LF nie tokenisiert; Latin-1-Byte vor LF
  verschluckt den Umbruch; zu lange Zeile beendet `blokload` – survey-04.
* `blockuntab` löscht Nicht-Leerzeichen; `tabstop = 0` – survey-01/06.
* `a0toname` vergleicht d0 statt des Zeichens – survey-03.
* `screenopen`-Fehlerpfade rufen CloseScreen mit a6 = ExecBase;
  `freethis`/`freeall` Größe per `add.b` – survey-05.
* `int3` startet mit 4, Puffer zeigt „2“ (tabstop) – survey-07.
* `outside`-Fehlschlag meldet an blitz2 trotzdem Erfolg (Z=1) – survey-01.
