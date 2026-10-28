# Survey 04 – ted.s:3063–4197

Bereich: Optionen (`l:blitzeditor.opts`), Optionsrequester-Hilfen, Datei-I/O und
Zeilenliste (Laden), Token-Lookup/Detokenizer, Dateirequester-Brücke, modale
Requester-Fenster (`openreq`/`closereq`/`error`), Suche (`isithere`),
Block-Hilfen, Zeilenausgabe (`showa2item`/`tokprint`), `charstobuff`.
Endet vor `tokit2` (4195, nur Einsprung erwähnt).

Kurzbezeichnungen: `item` = Zeilenknoten (s. E), `chmap` = Zeichenkarte des
Bildschirms (`chcols` Bytes je Zeile), `PUBLIC` = MEMF_PUBLIC (1).

---

## A. Routinen

### Optionen / Optionsrequester

- `loaddefs (3063)` : ein – ; aus – ; zerstört d0–d3/d7/a0–a6, a5 ; Open(`defname`, 1005 MODE_OLDFILE), Read(defend-defstart = 50 Bytes) roh nach `defstart`, Close; Rückgabewert von Read ungeprüft (Teil-/Kurzlesen lässt Rest unverändert). Dann a5=`defstart`, **Tail-`bra setdefs`** (auch wenn Open fehlschlägt).
- `savedefs (3078)` : Open(`defname`, 1006 MODE_NEWFILE), Write 50 Bytes ab `defstart`, **Tail-`jmp close(a6)`**; Fehler still ignoriert.
- `setdefs (3092)` : ein a5 = 50-Byte-Optionsblock (Layout wie `defstart`, s. C) ; aus – (Tail `bra setprops`) ; zerstört d0–d2, a0–a2, a5 (+25 Wörter). Ablauf:
  1. `tokens`==0 → `clr 6(a5)` (tokenise im **Quellblock** gelöscht!).
  2. Gadget-Flags (Offset +12, Bit 7 = SELECTED) setzen: `defaultreqga7` ⇐ `(a5)==6` (6-Pixel-Font), `ga8` ⇐ `6(a5)<<7` (tokenise), `ga9` ⇐ `8(a5)<<7` (makeicons) – Wert wird ungeprüft per `lsl #7` geodert (s. I).
  3. Farbwahl-Gadgets: für i=0..3 (normcol, labcol, tokcol, propcol): `swapd0bord(alt_i | 4*i)`, `swapd0bord(neu_i | 4*i)` (alt = aktuelle Globals ab `normcol`, neu = `10(a5)+2i`) → alter Knopf „hoch“, neuer „gedrückt“.
  4. `numtoascii2` von hscmarg/vscmarg/tabstop (18/20/22(a5)) in die Stringgadget-Puffer `defaultreqla47/48/49`.
  5. 25 Wörter a5 → `defstart` kopieren (a5 danach +50).
  6. `int1/2/3+2` (StringInfo.LongInt, Low-Word) ⇐ hscmarg/vscmarg/tabstop.
  7. d0=propcol, `bra setprops`.
- `numtoascii2 (3140)` : wie `numtoascii`, plus `clr.b (a0)` (a0 bleibt auf der 0).
- `numtoascii (3144)` : ein d0.w (vorzeichenlos), a0 = Puffer ; aus a0 hinter letzter Ziffer, **kein** Nullbyte ; zerstört d0–d2/a1. Dezimal ohne führende Nullen, 0 → "0". Global genutzt (2558, 3027, 5269, 5310).
- `powersof10 (3165)` : Daten im Code, `dc 10000,1000,100,10,1`.
- `setprops (3167)` : ein d0 = Farbregister 0..3 ; setzt `HorizPot` (PropInfo+2) von `defaultreqhp1..3` = `v*65536/15` aus `palit[d0*3+k]`, v==15 → $FFFF (sonst divu-Überlauf) ; zerstört d0–d2/a0/a1. Rein Daten, kein OS.
- `dispprops (3189)` : RefreshGList(`defaultreqga1`, `req_window`, `defaultreq`, 3) ; **Tail-`jmp`** ; a6=int.
- `propadj (3196)` : ein a5 = `palit`-Eintrag (3 Wörter r,g,b), d4 = Farbregister ; liest `HorizPot>>12` der 3 Prop-Gadgets; wenn ≠ (a5)/2/4(a5): Werte zurückschreiben und **Tail** `SetRGB4(&screen->ViewPort (myscreen+44), d4, r, g, b)` ; sonst rts. Kein CC-Ergebnis.
- `dispgadd0 (3228)` : ein d0 = Index (0..) ab `defaultreqga10`, Schrittweite `ga11-ga10` ; RefreshGList(gad, req_window, defaultreq, 1), Tail-`jmp`.
- `swapd0bord (3238)` : ein d0.w = Knopfindex (Farbe | 4*Kategorie, 0..15) ; tauscht FrontPen/BackPen (Border+4/+5) der **beiden** Borders des Knopfes (`bo14 + d0*56` und `+28`) ; interner `bsr .skip` + Durchfall (Routine läuft zweimal durch `.skip`). Zerstört d0 (mulu), d1, a0.
- `swapbords (3247)` : tauscht Pens aller 32 Borders ab `defaultreqbo14` (Schritt 28) – stellt nach Abbruch/Neuaufbau den „alle hoch“-Zustand her bzw. invertiert ihn.

### Rückfrage / Fehler / Requester

- `erase (3257)` : `bsr erase2`; bei ne **`addq #4,a7` → kehrt in den Aufrufer des Aufrufers zurück** (bricht z. B. `fileopen` ab). Bei eq normales rts.
- `erase2 (3262)` : ein `allalt` (Text geändert) ; aus **Z=1 wenn OK** (nicht geändert oder Gadget-ID 1 gewählt), sonst Z=0 ; bei OK zusätzlich `mousealt=-1`. Öffnet `erasereq` per `openreq`, Schleife `getinput` bis d7==-1 (Gadget); Schließen macht `handled0` (ENDGADGET). Z=1 via `cmp d6,d6`.
- `error (3511)` : **comdata+12** ; ein a0 = Fehlertext (→ `errorpnt`, IText des `errorreq`) ; sichert nur d7 ; wartet auf Gadget-ID 1 (andere Gadgets: weiter warten) ; zerstört d0–d6/a0–a2/a6 (getinput). Kehrt immer zurück.
- `openreq (3580)` : **comdata+0** ; ein a0 = Intuition-`Requester`-Struktur (nur Vorlage) ; aus – (Kommentar „return 0 if couldn't open“ stimmt nicht: Fehlschlag → **`beq outmem`**, kehrt nicht zurück, setzt a7=`memstack`) ; zerstört d0/d1/a0/a1/a6. Ablauf: `req`=a0; NewWindow `reqwindow` W/H = Req.Width+12 / Height+15; Left=(edcols*8−W)/2, Top=(newscreen.Height−H)/2 (`lsr`, s. I); FirstGadget = Req.ReqGadget (+16); **einmalige Patch-Relokation** der Gadgets (+6 x, +12 y), markiert über UserData-Hiword des 1. Gadgets (+40 = -1); OpenWindow; Fensterstapel `req_old2←req_old←req_window←neu`; PrintIText(rp, Req.ReqText(+24), 6, 12); DrawBorder(rp, Req.ReqBorder(+20), 6, 12); **Tail `bra pointeron`**.
- `closereq (3643)` : **comdata+4** ; `movem.l d0-d1/a0-a2/a6` gesichert ; falls `req_window`≠0: UserPort leeren (GetMsg/ReplyMsg), ClearPointer, CloseWindow, Stapel zurück (`req_window←req_old←req_old2←0`), `flushmw` ; **immer** `makemenus`. Kein CC-Ergebnis.
- `flushmw (3672)` : leert UserPort von `mywindow` ; zerstört d0/a0–a2/a6. Auch von 4903 gerufen.

### Datei-Requester-Brücke

- `getbname (3485)` : ein a2 = Titel ; a0=`blokpath`, a1=`blokname` → `getfname3`.
- `getfname (3489)` : a2=`filetext`, Durchfall in `getfname2`.
- `getfname2 (3490)` : ein a2 = Titel ; a0=`filepath`, a1=`filename`, Durchfall in `getfname3`.
- `getfname3 (3493)` : `bsr ezfreq`; ne → d7 = d0 (Zeiger auf vollen Dateinamen) und rts ; Z=1 und d0==1 → **`beq outmem`** (kein Speicher) ; sonst (Abbruch) **`addq #4,a7` → Aufrufer des Aufrufers**. Drei Einsprünge (getfname/getfname2/getfname3 + getbname).
- `ezfreq (3502)` : **comdata+24** ; ein a0 Pfadpuffer, a1 Namenspuffer, a2 Titel ; füllt `reqstruct` {a0,a1,a2,0}, a0=`reqstruct`, **Tail `bra filerequest+36`** (Code im `incbin filereq`, s. D) ; aus d0 (Name / 0 / 1), Z=1 bei Abbruch/Fehler (vom Filereq-Code gesetzt).

### Datei-I/O und Laden in die Zeilenliste (Kern)

Registerkonvention während eines Ladevorgangs: **d7 = FileHandle, a5 = Lesezeiger in `dosbuff`, d6 = Restbytes im Puffer** (alle drei dürfen vom Aufrufer zwischen `doopen` und `doclose` nicht angefasst werden).

- `doopen (3307)` : ein d7 = Dateiname, d2 = Modus (1005/1006) ; aus d7 = Handle, `filehand` = Handle ; bei Fehlschlag **`addq #4,a7` + Tail `bra error`** (error1-Text) → kehrt nach der Meldung in den Aufrufer des Aufrufers zurück. Puffer wird **nicht** gefüllt (trotz Kommentar) – Aufrufer ruft `fillbuff`.
- `doclose (3302)` : `filehand=0`, **Tail** Close(d7).
- `fillbuff (3337)` : a5=`dosbuff`, d6 = Read(d7, dosbuff, doslen=2048) ; aus **Z=1 wenn 0 Bytes (EOF) oder Fehler** ; Fehler (d0<0): `error(error2)`, d6=0. Zerstört d0–d3/a0/a1/a6.
- `doread (3325)` : aus d0.b = nächstes Byte, **N=0,Z=1** (`cmp d0,d0`) ; EOF: d0=-1, **N=1**. Aufrufer testen nur `bmi`. Lädt bei Bedarf über `fillbuff` nach.
- `readline (3375)` : liest eine Zeile nach `charbuff2` ; aus d5 = Länge **inkl. Nullbyte** (ggf. nach Tokenisierung), d0 + CC: **N=1 (d0=-1): nichts mehr gelesen (EOF)** ; **Z=1 (d0=0): Zeile ohne Zeilenende** (EOF mitten in der Zeile oder Pufferüberlauf) ; **Z=0,N=0 (d0=1): normale Zeile**. Regeln:
  - Byte mit Bit 7 → Token-Paar: dieses und das **nächste Byte ungeprüft** übernehmen (auch 0/10/9).
  - LF (10) → wird zu 0, setzt d4=-1 („ASCII-Datei, tokenisieren“) und beendet die Zeile ; ein 0-Byte beendet die Zeile ohne d4 (tokenisiertes Format).
  - TAB (9) → Leerzeichen bis Spalte ≡ 0 mod `tabstop` (Spalte = Bytes seit Zeilenanfang, Token-Paare zählen 2).
  - Überlauf: Ende bei `charbuff3`; dann a0=`charbuff3-3` → Zeile abgeschnitten (max. 127 Zeichen + 0), Rest der Datei-Zeile wird zur nächsten Zeile.
  - Tokenisieren nur wenn d4≠0 **und** `tokenise`≠0: d5-- (ohne 0), `mytoke` (in place in charbuff2, neues d5 inkl. 0).
  - CR wird nicht behandelt (bleibt im Text).
  - CC-Rückgabe kommt aus `move (a7)+,d0` am Ende.
- `mytoke (3447)` : sichert d1–d4/d6–d7/a0–a6 ; d1=d5 (Länge ohne 0), a1=a2=`charbuff2` → `tokit2` (4195; in place) ; aus d5 = neue Länge inkl. 0 ; d0 zerstört.
- `allocline (3349)` : ein d5 = Länge inkl. 0, `charbuff2` ; AllocMem(d5+lenitem, PUBLIC) über `alloc2` (Fehlschlag → `outmem`, nicht zurück) ; setzt `nextitem=0`, `numchars=d5.b`, kopiert bis inkl. 0 ; **`lastitem` bleibt uninitialisiert** (Aufrufer setzt ihn: `fileopen` explizit, `linka0`) ; aus a0 = neues item. Erstes Nicht-Leerzeichen == `label` → **Tail `beq remember`** (stub: `mousealt=-1`). Zerstört d0/d1/a1/a6.
- `linka0 (3277)` : ein a0 = neues item, a4 = Vorgänger ; hängt a0 hinter a4 ein, `numlines++`, ggf. `enditem=a0` (setzt dann `nextitem(a0)` **nicht** – verlässt sich auf allocline/allocnull) ; aus a4 = a0, `thisitem` = a0. Zerstört d0/a1.
- `bloadadd (3293)` : hängt `charbuff2` (token-paar-bewusst, inkl. 0) an a3 an ; aus a3 zeigt **auf** das Nullbyte. Nur von `blokload`.

### Token-Lookup / Detokenizer (Kern)

- `detok (3455)` : ein a0 = tokenisierte Zeile (0-terminiert), a1 = Ziel ; aus Ziel 0-terminiert, a1 auf der 0, a0 hinter Quelle ; zerstört d0/a3. Kein Längenlimit (s. I). Rekursion nur per `bra detok` (Schleife).
- `findtoke (3472)` : ein d0.w = Tokenwort (Bit 15 wird gelöscht) ; aus **a3 = Zeiger auf Namen** (0-terminiert) ; unbekannt → `tokeerr` ("?????"). Liste `tokens` (aus edstruct): Knoten {+0 next.l, +4 Nummer.w, +6 Name}. Setzt `tokens`≠0 voraus. Kein CC-Ergebnis.
- `detoka5 (3759)` : ein a5 = item ; detokenisiert `chars(a5)` nach `charbuff3+1`, **ohne 0-Terminator**, Längenbyte `charbuff3[0] = n+1` ; zerstört d0/a0/a1/a3. Lokaler `bsr .loop` (Unterprogramm im Unterprogramm).

### Suche

- `isithere (3523)` : d5=0, Durchfall in `isithere2`.
- `isithere2 (3530)` : ein a4 = item, d5 = Startspalte, Suchtext `findreqla16` (64 Bytes, 0-terminiert), Option „Groß/klein beachten“ = Bit 7 von `findreqga8+13` (Gadget-Flags SELECTED) ; tokenisiert → erst `detok` nach `charbuff` ; aus **d0.w = Fundspalte (N=0) oder -1 (N=1)** ; zerstört d0/d2/a0–a3/**a5**. Vergleich: Buchstaben (laut `alpha`/`alphad0`) mit `and #$DF` hochgestuft, wenn nicht case-sensitiv. Leerer Suchtext → sofort Fund bei d5.

### Zeilenliste: Freigabe, Ersetzen, Navigation

- `freeupa5 (3684)` : ein a5 = item ; aushängen (`firstitem`/`enditem` angepasst, `numlines--`), Label-Zeile → `mforget2` (stub), FreeMem(numchars+lenitem), `memalt`=Größe ; **Tail `jmp freemem`**. Sonderfall `numlines==1`: `afirst2` (neue Leerzeile wird erste/letzte/this), dann a5 freigeben, `numlines` bleibt 1. **`thisitem` wird sonst nicht angepasst** (Aufrufer). a1 = freigegebener Knoten.
- `calcitem (3783)` : a4 = `firstitem` + (curstop+cursy) Schritte, `thisitem=a4` ; keine Nullprüfung ; zerstört d0.
- `makeline2 (3794)` : d5 = a3−charbuff+1, `bra makeline`.
- `makefirst (3799)` : a2=`charbuff2`, `charstobuff`, Durchfall in `makeline`.
- `makeline (3803)` : ein a4 = zu ersetzendes item (**muss == `thisitem` sein**), `charbuff` = neuer Inhalt, d5 = Länge inkl. 0 ; neues item allozieren (alloc2/outmem), Label → `remember`, an Stelle von a4 einhängen (first/end aktualisiert), Text kopieren, **`freethis`** (gibt `thisitem` frei), a4=`thisitem`=neu. Zerstört d0/d1/a0/a1/a6.
- `allocfirst (3041)` : `afirst2`, **Tail `bra cls`**.
- `afirst2 (4043)` : `allocnull` (10 Bytes, leere Zeile) ; aus Z=1 bei Fehlschlag (dann nichts gesetzt), sonst first/end/this = a0, Z=0.

### Block

Block-Koordinaten (Globals, Zeilennummer absolut = curstop+cursy, Spalte in detokenisierten Zeichen): `btopx/btopy`, `bbotx/bboty`; `bboty==-1` = kein Block.

- `bloksetup2 (3727)` : ein a4 = this item ; aus **a5 = item der Zeile `btopy`, d6 = btopy, d7 = curstop+cursy** ; läuft per next/last zur Blockanfangszeile (vorzeichenlose Vergleiche). Endet über globales Label `found (3750)` (nur rts, kein Fremdeinsprung).
- `bloksetup (3752)` : `bloksetup2`, d5=`btopx` ; tokenise → **Tail `bra detoka5`**.
- `getbchar (3849)` : Zustand **a5 = item, d6 = Zeile, d5 = Spalte (0-basiert vor Inkrement)** ; aus d0 = Zeichen und CC: **N=1 (d0=-1): Blockende**, **Z=1 (d0=0): Zeilenende** (a5=next, d6++, d5=0 – Aufrufer muss bei tokenise `detoka5` neu rufen), sonst Zeichen. Ende = (d6>bboty) oder (d6==bboty && d5>bbotx); `d5>=chcols` → Zeilenwechsel. Hinter dem Zeilentext: `blokop`≠0 → Zeilenwechsel, sonst Leerzeichen (Auffüllen bis chcols).
- `getbchar2 (3865)` : zweiter Einsprung ohne Blockendeprüfung (von 1617/1636).
- `newline (3884)` : globales Label, nur aus getbchar.
- `bloken (3890)` : Menüpunkte Block (`menu4item1/2/4/a/b`, Flags+12 Bit 4 ITEMENABLED) setzen, `bltext`="block", Durchfall in `blokprnt2` ; zerstört Register (pstats).
- `blokprnt2 (3899)` : a1=`statsloc`+44, **Tail `bra pstats`** (Statuszeile).
- `blokdis (3903)` : Gegenteil; `bstx=-1`, `bboty=-1`, `bltext`=5 Leerzeichen ; sichert d0/a0–a2 nur um blokprnt2.
- `blokadj (3919)` : ein d0 = eingefügte Zeilennummer ; wenn Block aktiv und d0≤bboty: bboty++, und wenn d0≤btopy: btopy++ (vorzeichenlos).
- `showprops2 (3929)` : ein d2/d3 = Spalte/Zeile ; setzt cursx=clamp(d2−cursoff, 0, chcols−1), cursy=d3 temporär, `showhprop`+`showvprop`, stellt cursx/cursy (als Long) und d2/d3/d5 wieder her.
- `sortblox (3947)` : normalisiert btop ≤ bbot (Zeile vorzeichenlos, Spalte vorzeichenbehaftet), lädt d0–d3 = btopx,btopy,bbotx,bboty (vor dem Tausch!; nach Tausch geordnet).
- `orderem (3965)` : dasselbe nur in Registern d0/d1 ↔ d2/d3.
- `invertarea (3974)` : `orderem`, Durchfall in `invertarea2`.
- `invertarea2 (3977)` : ein d0,d1 → d2,d3 (Spalte/Zeile, d0/d2 absolute Spalten) ; invertiert zeichenweise (`not.b`) direkt in **beiden** Bitplanes `bitmaps[0..1]`, `fonthite2+1` Pixelzeilen je Zeichen, Fließtext-Bereich (über Zeilenende bis `edcols`, dann nächste Zeile ab Spalte 0) ; Spalten <cursoff übersprungen. Zerstört d0–d4/a0/a1. Kein OS – **direkter Chip-RAM-Zugriff**.
- `getmousexy (4015)` : liest `myscreen` MouseY(+16)/MouseX(+18) ; aus d0=x, d1=y, d2=x>>3, d3=(y−topskip)/fonthite (**Hiword = Rest von divu!**) und N=0 ; außerhalb des Textbereichs: d3=-1, **N=1**.

### Bildschirmausgabe der Zeilen

- `showall (4050)` : `storepos`, `cls`, von `thisitem` cursy Zeilen zurück (keine Nullprüfung), zeigt bis `edrows` Zeilen (godown), **Tail `bra restorepos`** ; zerstört **d7**, d0/d1/a2.
- `showtopitem (4073)` : Zeile in Bildschirmzeile 0 (cursy Schritte zurück) neu ausgeben ; fehlt ein Vorgänger → nichts. Tail restorepos.
- `showbotitem (4088)` : Zeile in Bildschirmzeile edrows−1 ausgeben, falls vorhanden. Tail restorepos.
- `showthisitem (4102)` : a2=`thisitem`, Durchfall in `showa2item`.
- `showa2item (4104)` : ein a2 = item, cursy/chloc = Zielzeile ; schreibt `chmap[chloc + d2]` und zeichnet via `print` (cursx=d3, `color`) ; d2 = Zeichenkartenspalte, d3 = Bildschirmspalte ; 1. Zeichen == `label` → `normprint` (nicht als Token), sonst alles über `tokprint` ; sichert cursx auf dem Stack. Durchfall/`beq` in `endofline`.
- `endofline (4124)` : wenn Zeile im Block (btopy..bboty): Rest der Bildschirmzeile mit Leerzeichen per `print` füllen ; `cursx` vom Stack (Stackpaar mit showa2item – **nur aus showa2item anspringbar**). Löscht `chmap`-Rest **nicht**.
- `normprint (4142)` : `chmap[d2]=d0`, Durchfall `normprint2`.
- `normprint2 (4143)` : nur wenn d2≥cursoff und d3<edcols: `print` an cursx=d3, d3++.
- `tokprint (4152)` : tokenise && Bit7 → Tokenwort aus d0 und nächstem Byte (a2++), `findtoke`, `color=tokcol`, Namen zeichenweise: `chmap[d2]=c|$80` (Bit 7 markiert Token-Zeichen), `normprint2`, d2++ ; danach `color=normcol`, d2-- ; sonst `normprint`. **Zerstört a5** (exg-Trick, a5 = Namenszeiger danach).
- `charstobuff (4173)` : ein a2 = Quelle (chmap-Zeile, `chcols` Bytes) ; tokenise → **Tail `bra tokit`** (mit Stack-Argument (a7)=charbuff, d1=chcols−1, a1=charbuff) ; sonst Kopie nach `charbuff`, hinten Leerzeichen gekappt, 0 angehängt (Label `enddo 4190`, auch Ende von `tokit`) ; aus d5 = Länge inkl. 0. **Stack-Kopplung**: `move.l a1,-(a7)` hier, `sub.l (a7)+,d5` in `enddo` – tokit muss mit genau diesem Stackwort in `enddo` enden.
- `tokit2 (4195)` : `move.l a1,-(a7)`, `bra tokit3` (Grenze, Rest im nächsten Chunk).

---

## B. Persistente Registerkonventionen

| Register | Bedeutung | Gültig in |
|---|---|---|
| d7 | FileHandle (bzw. vorher Dateiname für `doopen`) | doopen…doclose, readline, fillbuff |
| a5 | Lesezeiger in `dosbuff` | doread/fillbuff/readline (Laden) |
| d6 | Restbytes in `dosbuff` | doread/fillbuff/readline |
| d5 | Zeilenlänge inkl. 0 | readline → allocline; charstobuff → makeline |
| a4 | aktuelles/vorheriges item | linka0, makeline, isithere, bloksetup2, calcitem |
| a5, d6, d5 | Block-Cursor: item, Zeile, Spalte | bloksetup → getbchar(2) → detoka5 |
| a2/a3/d2/d3 | Ausgabe: Quelle, chmap-Zeile, chmap-Spalte, Bildschirmspalte | showa2item/normprint/tokprint |
| a6 | wird nicht erhalten; Macros `calldos`/`callint` laden dos/int frisch | überall |

Kollisionen: a5 ist zugleich Dateizeiger (Laden) und Block-item (Block speichern 1683ff. benutzt `bloksetup` + `doopen` – dort wird `fillbuff` nicht gerufen, Schreiben geht nicht über a5). `tokprint` und `isithere2` zerstören a5.

Von blitz2 gerufen (comdata): `openreq` (+0, **sichert nichts**), `closereq` (+4, `movem d0-d1/a0-a2/a6`), `error` (+12, sichert nur d7), `ezfreq` (+24, Filereq-Code sichert d1–d7/a0–a6 per `movem` am Code-Anfang+4). blitz2-Wrapper müssen davon ausgehen, dass openreq/error d0–d6/a0–a6 zerstören.

---

## C. Globals, in diesem Bereich definiert

| Label | Zeile | Typ | Init | Zweck |
|---|---|---|---|---|
| `powersof10` | 3165 | 5×w | 10000..1 | numtoascii |
| `reqstruct` | 3509 | 4×l | 0 | Parameter für Filereq-Code {Pfad, Name, Titel, 0} |
| `reqwindow` | 3565 | NewWindow (48 B) | s. u. | Vorlage Requester-Fenster |
| `req_gads` | 3569 | l (in reqwindow, +18) | 0 | NewWindow.FirstGadget |
| `req_screen` | 3572 | l (reqwindow+30) | 0 | NewWindow.Screen |
| `req_old`, `req_old2` | 3577/3578 | l | 0 | Stapel geöffneter Requester-Fenster (Tiefe 3 inkl. `req_window`) |

`reqwindow`: Left/Top/Width/Height werden bei jedem `openreq` überschrieben (Offsets 0/2/4/6); IDCMP = $20|$40|$200|$400 (GADGETDOWN, GADGETUP, CLOSEWINDOW, RAWKEY); Flags = 2|8|$1000 (WINDOWDRAG, WINDOWCLOSE, ACTIVATE); Type 15 (CUSTOMSCREEN); Titel 0.

Außerhalb definiert, hier wesentlich benutzt (für Header):
- Optionsblock `defstart..defend` (ted.s:6099, 50 Bytes = 25 Wörter, **Dateiformat von `l:blitzeditor.opts`**, roh, big-endian):
  `+0 fonthite(8)` `+2 fonthite2(7)` `+4 fontmul(0)` `+6 tokenise(0)` `+8 makeicons(1)` `+10 normcol(1)` `+12 labcol(2)` `+14 tokcol(3)` `+16 propcol(0)` `+18 hscmarg(12)` `+20 vscmarg(6)` `+22 tabstop(2)` `+24 palit 4×(r,g,b)` = (8,8,8)(15,15,15)(0,0,0)(15,13,0) `+48 origscrows(0)`.
- Puffer (ted.s:41–63, ein AllocMem): `dosbuff` 2048, dann `charbuff`, `charbuff2`, `charbuff3` je `chcols+2` (=130), `charbuff4` = Ende der Allokation (Größe 0!, s. I).
- Optionsrequester-Daten (ted.s:7310ff.): `defaultreqhp1..3` PropInfo (Flags 11, HorizPot +2 wird geschrieben), `defaultreqga1` Liste der 3 Prop-Gadgets, `defaultreqga4..6` Integer-Stringgadgets mit StringInfo (`int1..3` = StringInfo.LongInt, `defaultreqla47..49` Puffer, MaxChars 3), `defaultreqga7/8/9` Toggle-Gadgets (Flags +12 Bit 7 SELECTED gepatcht), `defaultreqga10..` Farbknöpfe (Schritt `ga11-ga10`), Borders `defaultreqbo14..` 16 Knöpfe × 2 Borders à 28 Bytes (Border 16 + 6 Koordinatenpaare), Pens an +4/+5 werden getauscht.
- Menü: `menu4item1/2/4/a/b` Flags (+12) Bit 4.
- Suchrequester: `findreqla16` (Suchtext), `findreqga8` Flags (+12/+13 Bit 7).

---

## D. Selbstmodifizierender Code / Daten im Code

- **`bra filerequest+36`** (3507): Einsprung in `incbin filereq` (ted.s:7967). Datei = AmigaDOS-Hunk: 24 B HUNK_HEADER + HUNK_CODE ($3E9) + Länge → Code beginnt bei +32. Code+0 = `lea $b3c(pc),a0` (eingebauter Default-Parameterblock), **Code+4 (= +36) = `movem.l d1-d7/a0-a6,-(a7)`, `move.l a0,a5`** → ezfreq übergibt eigenen Block in a0. Wird von `flink` (7953) zur Laufzeit reloziert; in C: Filerequester neu schreiben oder Hunk zur Laufzeit laden (GLUE).
- **`openreq` patcht die übergebenen Requester-Daten**: Gadget-Left/Top aller Gadgets +6/+12 beim ersten Öffnen, Merker `UserData`-Hiword (Gadget+40) des ersten Gadgets = -1. Betrifft auch Requester aus blitz2 (comdata+0) – die Strukturen sind nach dem ersten Aufruf verändert.
- Statische Strukturen zur Laufzeit beschrieben (nicht Code): `reqwindow`, Gadget-Flags (`ga7/8/9`+12), PropInfo HorizPot, StringInfo LongInt/Puffer, Border-Pens, MenuItem-Flags, `bltext`. Alles per `(pc)`-Adressierung gelesen, aber absolut beschrieben – in C normale `static`-Daten.
- `powersof10`, `reqstruct`, `reqwindow`, `req_old*` liegen zwischen Code (PC-relativ).
- Kein Schreiben auf Befehle in diesem Bereich.

---

## E. Heap-/Listenstrukturen

**Zeilenknoten (item)** (ted.s:16–20, `lenitem` = 9):

| Off | Größe | Feld | Bedeutung |
|---|---|---|---|
| 0 | l | `nextitem` | nächste Zeile / 0 |
| 4 | l | `lastitem` | vorige Zeile / 0 |
| 8 | b | `numchars` | Länge **inkl.** Nullbyte (≤255; FreeMem-Größe = numchars+9!) |
| 9 | n | `chars` | Text, tokenisiert (Token = 2 Bytes, 1. Byte Bit 7), 0-terminiert |

- Allokation `numchars+lenitem`, MEMF_PUBLIC, über `alloc2` (setzt `memalt=-1`, bei 0 → `outmem`). Leerzeile `allocnull`: 10 Bytes, `numchars=1`, `chars[0]=0` (Wortschreibzugriff `$100` auf Offset 8).
- Globale Listenköpfe: `firstitem`, `enditem`, `thisitem`, `numlines` (Wort). Invariante: nie leer (mind. eine Zeile; `freeupa5` erzeugt Ersatz).
- Einfügen: `linka0` (nach a4), Ersetzen: `makeline` (a4==thisitem), Entfernen: `freeupa5`/`freethis`.
- Label-Zeilen (erstes Nicht-Leerzeichen == `label`, Default 255 = aus): `remember`/`mforget2` sind hier nur Stubs (`mousealt=-1`); die Maus-Labelliste wird später komplett neu aufgebaut (`showmice`, 2916).

**`dosbuff`-Puffer**: 2048 Bytes, Zustand (a5, d6) in Registern, `filehand` global (für `outmem`-Aufräumen).

**charbuff-Bereiche** (je 130 Bytes, zusammenhängend):
- `charbuff`: Zeile zum Einfügen/Ersetzen (makeline), detok-Ziel von `isithere`.
- `charbuff2`: Leselinie (readline, in-place tokit), Quelle `makefirst`.
- `charbuff3`: Längenbyte + detokenisierte Blockzeile (`detoka5`), Überlaufgrenze von readline.

**Token-Liste** (`tokens`, aus edstruct): einfach verkettet {next.l, nummer.w, name\0}.

**Requester-Fensterstapel**: `req_window` / `req_old` / `req_old2` (keine Heap-Struktur, Fenster von Intuition).

---

## F. Externe Aufrufe

| Lib | LVO | Wo | Zweck |
|---|---|---|---|
| dos | Open/Read/Write/Close | loaddefs, savedefs, doopen, fillbuff, doclose | Optionsdatei, Quelltext, Block |
| exec | AllocMem (über alloc2/allocnull), FreeMem | allocline, makeline, freeupa5 | Zeilenknoten |
| exec | GetMsg/ReplyMsg | closereq, flushmw | Ports leeren |
| intuition | OpenWindow, CloseWindow, ClearPointer, PrintIText, DrawBorder | openreq, closereq | Requester-Fenster |
| intuition | RefreshGList | dispprops, dispgadd0 | Optionsrequester (mit `defaultreq` als Requester-Argument!) |
| graphics | SetRGB4 | propadj | Palette (ViewPort = Screen+44) |
| Filereq | `filerequest+36` | ezfreq | Dateiauswahl |

- Interne UI-Aufrufe aus diesem Bereich: `getinput` (erase2, error), `pointeron` (openreq), `makemenus` (closereq), `print`, `cls`, `storepos`/`restorepos`, `godown`, `pstats`, `showhprop`/`showvprop`, `outmem`, `remember`/`mforget2`, `freethis`, `allocnull`, `alpha`/`alphad0`, `tokit`/`tokit2`.
- **Direkter Speicherzugriff**: `invertarea2` schreibt per `not.b` in die Bitplanes (`bitmaps`, Chip-RAM), `getmousexy` liest Screen-Felder (+16/+18), `showa2item` schreibt in `chmap`. Kein `$dff000`, kein Blitter, keine Interrupts in diesem Bereich.
- Rückrufe in blitz2 (edstruct): keine in diesem Bereich (`loadxtra` erst in `fileopen`, außerhalb).
- comdata-Einträge hier: +0 openreq, +4 closereq, +12 error, +24 ezfreq.

---

## G. Sonderbehandlung in C

1. **Abbruch über den Stack** (`addq #4,a7`): `erase`, `getfname3`, `doopen` (dort + `bra error`). In C: Rückgabewert (bool) und `if (!erase()) return;` in jedem Aufrufer (fileopen, blokload, …). Genau prüfen, welche Aufrufer das betrifft (Aufrufer der Aufrufer sind teils Menü-Handler mit Stackaufbau).
2. **`outmem`** setzt `a7=memstack` und springt nach `memcont` (Hauptschleife): nicht-lokaler Sprung → `longjmp` (in `alloc2`, `openreq`, `getfname3`). Achtung: Aufruf über comdata aus blitz2 (openreq) würde blitz2-Stack verwerfen → in C eigener Fehlerpfad (Rückgabe 0) für den comdata-Wrapper.
3. **CC-Rückgaben**: readline (N/Z dreiwertig → `enum {RL_EOF=-1, RL_NOEOL=0, RL_OK=1}`), doread (−1/Byte), fillbuff (bool), erase2 (bool), isithere2 (int, −1), getbchar (−1/0/Zeichen – Zeichen ≥$80 im Nicht-Token-Modus: s. I), getmousexy (bool + 4 Ausgaben), afirst2 (Zeiger/NULL), ezfreq (Z vom Fremdcode).
4. **Register-Zustand über Aufrufe** (d7/a5/d6 beim Laden, a5/d6/d5 beim Blockdurchlauf): in C als Struct `struct rdstate {BPTR fh; UBYTE *p; LONG left;}` bzw. `struct bcur {item *a5; UWORD y, x;}`.
5. **`charstobuff` → `tokit` → `enddo`** teilen ein Stackwort (Pufferanfang); in C: tokit gibt Länge zurück, `charstobuff` ruft es normal.
6. **`showa2item`/`endofline`** teilen das gesicherte `cursx` auf dem Stack → in C eine Funktion.
7. comdata-Wrapper (Asm-Stub, GLUE): openreq/closereq/error/ezfreq mit a0/a1/a2-Parametern, Register-Sicherung wie Original (closereq: d0–d1/a0–a2/a6; error: nur d7 – Stub darf mehr sichern).
8. `ezfreq` → Filereq-Code (incbin): GLUE oder Neuimplementierung (asl/reqtools wäre Verhaltensänderung).
9. Fallstricke:
   - `numchars` ist **Byte**, Länge inkl. 0; FreeMem-Größe daraus berechnet → in C `UBYTE` beibehalten.
   - `move #$100,numchars(a0)` = Byte-Paar-Schreiben (numchars=1, chars[0]=0).
   - `getmousexy` d3: Hiword enthält Rest (divu) – nur Wort verwenden.
   - `move.l cursx,-(a7)` sichert cursx **und** cursy (benachbarte Wörter) – in C beide.
   - Token-Wort: `(hi<<8|lo) & 0x7FFF` (findtoke `bclr #15`).
   - Vergleiche: btopy/bboty-Vergleiche vorzeichenlos (`bcs/bhi/bls`), Spalten teils vorzeichenbehaftet (`bge`, `bgt`, `ble`); `bboty==-1` → $FFFF unsigned (blokadj prüft vorher explizit).
   - `setdefs` `lsl #7` auf Flag-Wörter: nur 0/1 erlaubt.
   - Gadget-Flags-Bit 7 wird teils am Byte +13 (`btst #7`), teils am Wort +12 (`$80`) getestet – identisch (Big-Endian).
   - `tokprint` zerstört a5, `showall` zerstört d7.
   - `setprops` divu-Überlauf bei 15 → Sonderfall $FFFF beibehalten.

---

## H. Einstufung

| Routine | Klasse | Bemerkung / Trennstelle |
|---|---|---|
| loaddefs, savedefs | KERN (Datei) | danach `setdefs` (UI-Anteil) |
| setdefs | UI | Datenteil (Kopie nach defstart, LongInt) abtrennbar: `defs_apply(src)` + Hook `defs_ui_sync()` für Gadget-Flags/Borders/Puffer/Props |
| numtoascii(2) | KERN | |
| setprops | UI-Daten (kein OS) | Formel KERN-fähig |
| dispprops, dispgadd0, propadj | UI | |
| swapd0bord, swapbords | UI-Daten | |
| erase, erase2, error | UI | Abbruchsemantik s. G1 |
| openreq, closereq, flushmw | UI | |
| getbname/getfname*/ezfreq | UI/GLUE | Filereq-incbin |
| doopen, doclose, fillbuff, doread | KERN (Datei) | Fehler → `error` (UI) per Callback |
| readline | **KERN** | tabstop, tokenise als Parameter; mytoke = Tokenizer-Kern |
| mytoke | KERN | |
| allocline, linka0, bloadadd | **KERN** | `remember` = Flag setzen (Hook) |
| detok, findtoke, detoka5 | **KERN** | |
| isithere(2) | **KERN** | Suchtext/Option als Parameter statt Gadget-Daten |
| freeupa5, calcitem, makeline(2), makefirst | **KERN** | `memalt`/`mousealt`-Flags = Anzeige-Hinweise; makefirst hängt an charstobuff |
| afirst2 | KERN | |
| allocfirst | KERN + UI | `cls` abtrennen |
| bloksetup(2), getbchar(2), newline | **KERN** | |
| sortblox, orderem, blokadj | KERN | |
| bloken, blokdis, blokprnt2 | UI | Blockzustand (`bboty=-1`, `bstx=-1`) von Menü/Statuszeile trennen |
| showprops2 | UI | |
| invertarea(2), getmousexy | UI | |
| showall, showtopitem, showbotitem, showthisitem, showa2item, endofline, normprint(2), tokprint | UI | Füllen von `chmap` (Zeichenkarte, Bit 7 = Tokenzeichen) ist Kern-Logik: `render_line(item, chmap_row)` + `print`-Callback |
| charstobuff | KERN | Quelle ist chmap-Zeile (Puffer) – host-testbar |

---

## I. Auffälligkeiten / mögliche Bugs (BUG(orig)-Kandidaten)

1. **readline: letzte Zeile ohne LF wird nicht tokenisiert** (3424ff.): d4 wird nur im LF-Pfad (→ `.skip3`) gesetzt; im `.oops`-Pfad (EOF/Überlauf) ist d4 immer 0. ASCII-Dateien ohne abschließendes LF: letzte Zeile bleibt Klartext; ebenso abgeschnittene überlange Zeilen.
2. **readline: Bytes ≥$80 in ASCII-Dateien** (ISO-Latin-1-Umlaute) werden als Token-Paar behandelt: das Folgebyte wird ungeprüft übernommen, auch LF/0/TAB → Zeilenumbruch nach Umlaut am Zeilenende geht verloren (zwei Zeilen verschmelzen), TAB wird nicht expandiert.
3. **readline: Token-Anfangsbyte direkt vor EOF** → Zeile endet `…$8x,0`; `detok`/`tokprint` lesen das Nullbyte als Token-Low-Byte und laufen über das Zeilenende hinaus.
4. **readline: `divu tabstop`** – `tabstop` kommt ungeprüft aus der Optionsdatei bzw. dem Integer-Gadget (`int3`); 0 → Division-durch-Null-Trap.
5. **readline: Tab-Expansion vor der Überlaufprüfung** – kann bis zu `tabstop−1` Bytes über `charbuff3` hinaus schreiben (in charbuff3, harmlos, außer bei großem tabstop: dann über das Allokationsende).
6. **readline: Pufferüberlauf liefert Z=1 („kein EOL“)** – `blokload` (1772ff.) bricht bei Z=1 das Laden ab (`done4`): eine zu lange Zeile im Block beendet den Blockimport; der Rest der Datei wird ignoriert. `fileopen` lädt dagegen weiter.
7. **CR (13) wird nicht entfernt** (DOS/Windows-Dateien behalten CR im Text).
8. **detok / detoka5 ohne Längenprüfung**: tokenisierte Zeile bis 127 Bytes kann detokenisiert >130 Bytes ergeben → `isithere2` überschreibt `charbuff2`; `detoka5` schreibt hinter `charbuff3` in `charbuff4`, das **am Ende der Allokation liegt** (ted.s:41–63 reserviert nur 3×(chcols+2)) → Heap-Überschreiber. Längenbyte `charbuff3[0]` läuft bei >254 über.
9. **getbchar (Nicht-Token-Modus)**: `move.b chars-1(a5,d5),d0` setzt N für Zeichen ≥$80 → Aufrufer werten Umlaute als **Blockende** (bmi). Token-Modus (`tst d0` auf Wort) ist korrekt.
10. **isithere2: Startspalte d5 > Zeilenlänge** → Suche beginnt hinter dem Nullbyte, liest bis zur nächsten 0 im Speicher (mögliche Fehltreffer/Lesen fremder Daten). Abhängig von Aufrufern prüfen (938/974: Weitersuchen nach Fund).
11. **freeupa5 bei numlines==1**: schlägt `allocnull` fehl, bleibt `firstitem/enditem/thisitem` auf dem gerade freigegebenen Knoten (Dangling Pointer).
12. **makeline gibt `thisitem` frei, ersetzt aber a4** – stillschweigende Voraussetzung a4==thisitem; jeder Aufrufer mit a4≠thisitem hätte Leck + Use-after-free.
13. **allocline lässt `lastitem` uninitialisiert**; `linka0` setzt bei Einfügen am Listenende `nextitem(a0)` nicht (verlässt sich auf allocline).
14. **openreq**: Kommentar „return 0 if couldn't open“ falsch – Fehlschlag → `outmem` (Stack-Reset; aus blitz2 über comdata fatal). Zentrierung mit `lsr` → bei Requester breiter als `edcols*8` bzw. höher als Screen: riesige Koordinate → OpenWindow schlägt fehl. Requester ohne Gadgets: `tst 40(a1)` mit a1=0 liest Adresse 40. Mehr als 3 verschachtelte Requester: ältestes Fenster geht aus dem Stapel verloren (Leck). `req` wird bei `closereq` nicht zurückgesetzt.
15. **Gadget-Relokation in openreq** ist einmalig über UserData des 1. Gadgets markiert; wird ein Gadget-Satz in mehreren Requestern geteilt oder hat UserData≠0, unterbleibt/verdoppelt sich die Verschiebung.
16. **RefreshGList mit `defaultreq` als Requester-Argument** (dispprops, dispgadd0), obwohl die Gadgets in einem Fenster hängen (openreq baut Fenster, kein Requester) – funktioniert offenbar, ist aber formal falsch.
17. **setdefs**: `clr 6(a5)` verändert den Quellblock (bei loaddefs = `defstart` selbst, bei Abbruch des Optionsrequesters = Kopie in `charbuff`); Flags per `lsl #7` ohne Maskierung (Wert ≠0/1 aus beschädigter Optionsdatei setzt fremde Gadget-Flag-Bits). `fontmul`, `fonthite`, `origscrows` werden roh aus der Datei übernommen (fontmul hängt von `bmapmul` beim Speichern ab).
18. **loaddefs** prüft weder Read-Ergebnis noch Dateigröße; **savedefs** meldet Fehler nicht.
19. **showall/showtopitem** setzen voraus, dass cursy Vorgänger existieren (showall ohne Nullprüfung).
20. **bloken/blokdis** ändern MenuItem-Flags ohne ClearMenuStrip/SetMenuStrip (in 1.3 üblich, aber formal unsauber).
21. `numlines` ist ein Wort (>65535 Zeilen läuft über) – theoretisch.
22. Tab-Spaltenberechnung in readline zählt Token-Paare als 2 Spalten (bei tokenisierten Dateien mit TABs falsche Einrückung).
