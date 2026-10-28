# Token-Tabelle von Blitz Basic 2.1

Stand 26.10.2026. Quelle: Blitz-Basic-CD (`cd/`), Libraries
`blitz2/deflibs` (Standard), `extras/bigdeflibs/deflibs` (große Variante) und
`blitz2/acidlibs`. Die Dateien haben **dasselbe BigLib-Format** wie die
BlitzLibs von AmiBlitz3 (Compiler-Spec 09); `biglib_dump.py --tokens` liest
sie unverändert.

Die eingebauten Befehle (Lib 0/1, Wörter `$80xx`) stehen nicht in den
Libraries, sondern im Compiler `blitz2/blitz2` (siehe `formate.md`).

## Dateien

| Datei | Inhalt |
|---|---|
| `tokens/deflibs.txt` | `blitz2/deflibs`, 992 Tokens, Libs 2 und 200–255 (OS-Aufrufe `…_`) |
| `tokens/bigdeflibs.txt` | `extras/bigdeflibs/deflibs`, 1408 Tokens: wie `deflibs` ohne Lib 200, dazu Libs 11–111 (Fremd-Libs: Listen, Requester, Module, `Request`, Dateisystem …) |
| `tokens/acidlibs.txt` | `blitz2/acidlibs`, 619 Tokens, Libs 74–194 (Acid-Befehle: Strings Lib 185, `Edit$`/`Inkey$` Lib 186 …) |
| `tokens/ab3.txt` | AmiBlitz3 zum Vergleich (`ab3c -dump token` mit `AmiBlitz3-Original/BlitzLibs`), 4653 Zeilen |
| `tokens/vergleich.txt` | jedes BB2.1-Token mit Status gegenüber AB3 |
| `tokens/corpus_unbekannt.txt` | die AB3-unbekannten Wörter des Aminet-Korpus mit BB2.1-Namen |

Erzeugt mit `tools/compare_tokens.py` und `tools/resolve_unknown.py`:

```
T=../AmiBlitz3/Docs/Compiler-Spec/verification/tools
python3 -I $T/biglib_dump.py --tokens cd/blitz2/acidlibs > tokens/acidlibs.txt
python3 -I tools/compare_tokens.py tokens/ab3.txt tokens/deflibs.txt \
    tokens/bigdeflibs.txt tokens/acidlibs.txt > tokens/vergleich.txt
python3 -I tools/resolve_unknown.py \
    ../ab3c/tests/external/aminet/bb2-corpus-files.csv \
    tokens/deflibs.txt tokens/bigdeflibs.txt tokens/acidlibs.txt
```

## Vergleich mit AmiBlitz3 (Lib/Index → Name)

| Status | Anzahl | Bedeutung |
|---|---|---|
| gleich | 1706 | gleicher Name unter gleicher Nummer |
| anderer Name | 223 | Nummer gibt es in AB3, aber mit **anderem** Befehl |
| fehlt | 102 | Nummer gibt es in AB3 nicht |

Die 223 Fälle mit anderem Namen sind meist **verschobene Indizes**: AB3 hat
in eine Lib einen Befehl eingefügt oder entfernt, alle folgenden rücken.
Beispiele:

| Wort | BB2.1 | AB3 |
|---|---|---|
| `$A589` (75/9) | `Min` | `CopyByte` |
| `$A58D` (75/13) | `Max` | `BLoad` |
| `$9C0C` (56/12) | `ModuleToChip` | `GetModuleName$` |
| `$B581` (107/1) | `Null` | `CreateMsgPort` |
| `$BC0E` (120/14) | `True` | `Vallong` |
| `$F581` (235/1) | `GetIcon_` | `FreeFreeList_` |
| `$A781` (79/1) | `ReqFileRequest` | `Edit$` |

Folge für den Konverter: Ein Token-Wort einer BB2.1-Datei, das AB3 kennt, ist
**kein** Beleg dafür, dass es dasselbe bedeutet. Eine `.bb2`-Datei lässt sich
nur mit der Tabelle der Version übersetzen, mit der sie gespeichert wurde.
Welche Version das war, steht nicht in der Datei (Kopf `BB22` siehe
`formate.md`); der Konverter braucht die Version als Option oder eine
Plausibilitätsprüfung (z. B. Parameterzahl).

## Die alten Wörter aus dem Aminet-Korpus

`docs/bb2-converter/bb2-corpus.md` §5 listet Wörter, die AB3 nicht kennt
(CSV: 57 verschiedene). Mit BB2.1 sind **37** davon benannt
(`tokens/corpus_unbekannt.txt`):

| Wörter | Lib | BB2.1 | Dateien |
|---|---|---|---|
| `$DC81`–`$DC99` | 185 (acidlibs) | `Left$`, `Right$`, `Mid$`, `Hex$`, `Bin$`, `Chr$`, `Asc`, `String$`, `Instr`, `Replace$`, `Mki$` … `CaseSense` | bis 100 je Wort |
| `$DD01`–`$DD03` | 186 (acidlibs) | `Edit$`, `Inkey$`, `Edit` | 57/90/16 |
| `$B58D`, `$B58F`–`$B591` | 107 (bigdeflibs) | `Request`, `ScreenWidth`, `ScreenHeight`, `ActiveWindow` | 13/21/21/2 |
| `$A596`, `$A599` | 75 | `NextBank`, `FillMem` | 7/3 |
| `$B38C` | 103 | `ClickMouse` | 1 |
| `$C70D` | 142 | `BlitzFont` | 1 |
| `$FD28` | 250 | `BestModeIDA_` | 2 |

Die Zuordnung in `bb2-corpus.md` §5, die aus dem Kontext erschlossen war
(Lib 185 → heute Lib 6 gleicher Index, Lib 186 → Lib 79), stimmt damit
überein. **Nicht** BB2.1 sind 20 Wörter: Lib 5, 29, 39, 109/27–48, 123, 147
– sie stammen aus späteren oder fremden Libraries (AmiBlitz 2, Fremd-Libs).

## Bezug zu `replacedat` und den defekten Resident-Makros

`replacedat` setzt in AmiBlitz3 nur FPU-Varianten um (Befund vom 26.10.,
Change `macros` in ab3c); alte Lib-Nummern bildet AB3 **nicht** ab. Die
Wörter in den fünf defekten Makros von `all.res` sind genau BB2.1-Wörter:

| Wort in `all.res` | BB2.1 |
|---|---|
| `$DC86` | `Chr$` (185/6) |
| `$DC87` | `Asc` (185/7) |
| `$B58D` | `Request` (107/13) |
| `$B593` | `ShowRequesters` (107/19) |

Die Makros (`PSD_NAME_FRONTMOST`, `Toolbar_KeyButton`, `checklib`, `cout`,
`screen_`) wurden also mit einer Blitz-2-Tabelle tokenisiert und nie
umgesetzt. Ein Konverter für `.bb2` muss die BB2.1-Tabelle selbst
mitbringen; `replacedat` hilft dafür nicht. Das korrigiert die Annahme in
`bb2-corpus.md` §5 und Anforderung 4 in `docs/bb2-converter/README.md`.

## Offen

* Ob Dateien des Korpus bekannte, aber **verschobene** Wörter enthalten
  (z. B. `$A589` als `Min`): dafür die Token-Zählung je Datei
  (`tests/external/aminet/tools/analyze.py`) gegen `vergleich.txt` laufen
  lassen.
* Ältere Versionen (2.0, 1.x) und AmiBlitz 2 haben eigene Tabellen; die CD
  enthält nur 2.1 (die `bum*`-Disketten eventuell Zwischenstände).
