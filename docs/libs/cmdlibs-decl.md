# Befehls-Libraries: Quelltext (bblibsrc) vs. CD-Binary (acidlibs)

Stand: Vergleich der **Deklarationsteile** (Header, Token-Einträge mit
Art/Formen/Args/Libs/Name/Hilfetext, libfin) der 58 Befehls-Libs in
`cd/blitz2/acidlibs` (BB2.1; identisch mit `cd/bum9a/blitz2/acidlibs` und
`cd/bum10b/blitz2/acidlibs`) mit den Quelltexten in
`quelltext/amibl2src/files/amiblitz2source/bblibsrc.lha.dir/bblibsrc/`.
Der Code selbst ist **nicht** verglichen.

```
python3 -I libs-c/tools/srcdecl.py report cd/blitz2/acidlibs <bblibsrc>
python3 -I libs-c/tools/srcdecl.py compare cd/blitz2/acidlibs <bblibsrc>/gtlib
```

## Wie die Libs gebaut wurden

Die Quelltexte sind normale BB2-Programme, die mit dem Blitz-Assembler
übersetzt wurden; die Deklarationsmakros (`!libheader`, `!astatement`,
`!args`, `!libs`, `!subs`, `!name`, `!nullsub`, `!libfin` …) stehen in
`cd/extras/blitzlibs/libmacs.bb` (identisch in `cd/bum9b|bum10a/blitzlibs/
libmacs.bb2`, dort auch `.res`), die Lib-Nummern in `libnums.bb`
(`INCLUDE "blitzlibs:libnums.bb"` am Ende von libmacs). Die Makros erzeugen
nur `Dc`-Daten in genau dem Format, das `biglib.parse_lib` liest. Deshalb
wertet `srcdecl.py` die Makros host-seitig aus (kein Amiberry nötig):

* Konstanten aus libmacs.bb, libnums.bb (spätere Definition gewinnt), dann
  `#x=…` des Quelltexts.
* BB2-Eigenheit: `#` ist in Assembler-Ausdrücken optional (`la6`,
  `shapeslib`, `#word|push` ergeben die Konstanten) - im Binary bestätigt.
* Labels = gesetzter Code-Offset, `0` = ungesetzt (wie `biglib.without_code`).
* Handgeschriebener Eintragskopf `Dc 3 LSL 8+3,0,0` (windowslib WaitEvent)
  wird erkannt.

Gegenprobe: 24 von 25 Systemlib-Quellen ergeben "identical"; Ausnahme
debuglib (Binary hat einen nullsub mehr, `last` anders - bekannt, siehe
libs/debuglib.c: Binary neuer als Quelle).

## Ergebnis

* **52 identisch**, **1 bis auf die Lib-Nummer identisch** (Lib 1),
  **5 mit Abweichungen**, **0 ohne Quelle**.
* Lib-Nummer stammt immer aus `#name` + libnums.bb, außer wo angegeben.

| Nr | Quelle | Einträge Src/Bin | Ergebnis |
|---:|---|---|---|
| 1 | dhandlerlib | 2/2 | identisch bis auf Lib-Nr.: `#dhandlerlib` ist in libnums.bb nicht definiert, Binary hat 1 |
| 74 | crunchlib (`#romuluscrunchlib`) | 4/4 | identisch |
| 76 | banklib | 13/13 | identisch |
| 78 | datelib | 11/11 | identisch |
| 80 | asllib (`#myasllib`) | 6/6 | identisch |
| 82 | medlib | 18/18 | identisch |
| 88 | speaklib | 7/7 | identisch |
| 90 | seriallib | 13/13 | identisch |
| 92 | arexxlib | 16/16 | identisch |
| 94 | animlib | 10/10 | identisch |
| 96 | trackerlib | 9/9 | identisch |
| 98 | blitzkeyslib | 7/7 | identisch |
| 100 | scrolllib | 7/7 | identisch |
| 104 | collslib | 12/12 | identisch |
| 106 | cliargslib | 4/4 | identisch |
| 108 | mouselib | 9/9 | identisch |
| 110 | sortlib | 4/4 | identisch |
| 112 | spriteslib | 14/14 | identisch |
| 114 | iffmakelib | 5/5 | identisch |
| 116 | audiolib | 18/18 | identisch |
| 117 | blitzoslib | 32/32 | identisch |
| 118 | shapetrixlib | 4/4 | identisch |
| 119 | 2dlib (`#sis2dlib`) | 17/17 | identisch |
| 120 | vallib | 16/16 | identisch |
| 121 | polylib (`#chunkylib=121` im Quelltext) | 8/8 | identisch |
| 122 | stencillib | 7/7 | identisch |
| 124 | fileiolib | 25/25 | identisch |
| 128 | linklistlib | 18/15 | **Quelle neuer**: SetItem, CountList, CountItem am Ende zusätzlich |
| 130 | rawkeylib | 5/5 | identisch |
| 132 | brexxlib | 23/23 | identisch |
| 134 | intuifontlib | 8/7 | **Quelle neuer**: Token `Text` am Ende zusätzlich |
| 136 | menuslib | 17/17 | identisch |
| 138 | windowslib | 75/75 | identisch |
| 140 | gadgetslib | 39/39 | identisch |
| 141 | gtlib (`#mygadtoolslib`) | 37/40 | **anderer Stand**, s. u. |
| 142 | fontlib | 15/15 | identisch |
| 143 | displaylib | 25/25 | identisch |
| 144 | blitzcoplib | 20/19 | **Quelle neuer**: Token `Help` am Ende zusätzlich |
| 146 | bblitlib | 11/11 | identisch |
| 148 | qblitlib | 10/10 | identisch |
| 150 | blitlib | 10/10 | identisch |
| 152 | freqlib | 1/1 | identisch |
| 154 | shapeslib | 21/21 | identisch |
| 156 | palettelib | 33/33 | **Args**: Red/Green/Blue Quelle `!args{2}` (#word), Binary 3 (#long) |
| 157 | fadelib (`libheader{157`) | 6/6 | identisch |
| 158 | screenslib | 20/20 | identisch |
| 160 | bitmaplib | 11/11 | identisch |
| 161 | ilbmifflib | 9/9 | identisch |
| 162 | iffiolib | 9/9 | identisch |
| 164 | inputoutputlib | 12/12 | identisch |
| 180 | memacclib | 4/4 | identisch |
| 185 | stringfunclib | 28/28 | identisch |
| 186 | editlib | 3/3 | identisch |
| 187 | printlib | 7/7 | identisch |
| 190 | gameiolib | 5/5 | identisch |
| 192 | mathtranslib (2. Definition =192) | 16/16 | identisch |
| 193 | environlib | 9/9 | identisch |
| 194 | utillib | 6/6 | identisch |

## Details zu den Abweichungen

* **141 gtlib** - Quelle und Binary sind getrennte Weiterentwicklungen
  (Quelle-Kommentare bis "November 1994 gtsetattrs fixed by Steve"):
  * nur Quelle: `GTMakeCycleTag` (Token 31, "Simon was here!");
  * nur Binary: `DetachGTList`, `GTGimmeCludge`, `GTSetHighlight`,
    `GTFreeGadget` (Token 31-34);
  * `GTList` Hilfe: Quelle "A Gadg**T**ools GadgetList", Binary "A GadTools …";
  * `AttachGTList`: Binary hat eine Fehlerprüf-Routine (subs err1 gesetzt),
    Quelle nicht;
  * `GTSetAttrs`: Quelle `!astatement`, Binary Art $303 (Statement *und*
    Funktion, Typ #long - wie windowslib WaitEvent).
  * `oldgtlib` ist älter (ohne GTStatus/GTArrowSize/GTMakeCycleTag;
    AttachGTList ohne intuitionlib-Basis in a6 - "presumed a6 had gtlib").
* **156 palettelib**: nur die Argumenttypen von Red/Green/Blue (Binary
  nimmt #long). `palettelib.aga` ist ein älterer AGA-Entwurf (18 Einträge,
  ohne AGARGB/AGAPalRGB/AGARed…).
* **128/134/144**: Quelle hat neue Tokens am Ende (Token-Nummern der
  übrigen bleiben gleich) - Quelle ist also neuer als BB2.1.

## Weitere Quelldateien (nicht auf der CD bzw. Varianten)

* Varianten derselben Nummer: `sis2dlib.bb` (älter als 2dlib: ohne
  ClipBlitMode/BitMapWindow/BitMapOrigin, Line/ClipBlit mit weniger Formen),
  `olddisplaylib`, `displaylibfuk` (ältere displaylib-Stände), `oldgtlib`,
  `palettelib.aga`.
* `renderlib` = polylib bis auf `#renderlib` (libnums: 87, Kommentar
  `; #renderlib=121`): Deklarationen identisch zu Lib 121.
* Nicht in acidlibs: alglib (#algebralib=9), cd32lib (113), chunkylib (72 aus
  libnums; 29 Einträge, ChunkyCLS… - die Tokens von 121 kommen aus polylib),
  ctrlclib und xprlib (`#ctrlclib`/`#xprlib` nicht in libnums).
* `bb2src.zip.dir/linklistlib` ist byte-gleich mit bblibsrc/linklistlib.
* Die RWE-Quellen (`RWELibSourc.lha.dir/Libs`) enthalten keine der
  acidlibs-Nummern.

## Versionshinweise

Alle Dateien im Archiv tragen dasselbe Datum (14.09.1995), also kein
Hinweis. Kommentare in den Quellen reichen bis "bum8" (animlib, 2dlib,
utillib, blitzoslib) bzw. "March 1995" (fileiolib); bum9a/bum10b liefern
dieselbe acidlibs wie die 2.1-CD. Die Quellen sind daher grob der
2.1-Stand, bei linklistlib/intuifontlib/blitzcoplib/gtlib etwas danach.

## Offene Punkte

* Code-Teile sind nicht verglichen; für die Portierung gilt wie bei den
  Systemlibs: Binary ist die Referenz (`tools/disasm.py`).
* Lib 1 (dhandlerlib): Mit welcher Nummer-Definition gebaut wurde, ist
  unklar (libnums.bb kennt `#dhandlerlib` nicht).
