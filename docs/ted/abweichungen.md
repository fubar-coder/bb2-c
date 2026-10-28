# Abweichungen der C-Fassung von TED 1.1

Referenz: vasm-Build von `ted.s` ([reference.md](reference.md)). Regel
(Benutzerentscheidung): Fehler des Originals werden nachgebildet
(`BUG(orig)`), Abweichungen nur, wo technisch unvermeidbar – im Code mit
`DIFF:` und Begründung markiert und hier vollständig aufgeführt.
Kategorien wie in [`../../libs-c/notes/abweichungen.md`](../../libs-c/notes/abweichungen.md)
(**B** vom Benutzer entschieden, **T** technisch unvermeidbar, **K** stille
Korrektur – nicht zulässig). Die Zeilen, die den mit libs-c geteilten
Dateirequester betreffen, stehen zusätzlich dort in Abschnitt 5a.

Stand: Phase 2c (Integrations-Review). Alle „sichereren“ Stellen der Phase
2b sind auf dem Amiga wieder original (Abschnitt „In Phase 2c
zurückgenommen“ unten); Schutzabfragen gibt es nur noch im Host-Build
(`#ifdef TED_HOST`, mit `ORIG_HAZARD` markiert) – sie ändern das Verhalten
der Amiga-Fassung nicht und sind keine Abweichungen.

| Stelle | Original | C-Fassung | Art | beobachtbar? |
|---|---|---|---|---|
| `flink` (ted.s:7953; src/filereq.c) | reloziert den eingebetteten Requester-Hunk `filereq` beim Start selbst (312 RELOC32, ohne Cache-Leerung; ein zweiter Lauf würde doppelt relozieren – BUG(orig)) | entfällt: der Requester ist C-Code aus `../shared/filereq/filereq_core.c`, LoadSeg reloziert | **T** | nein |
| Requester-Datenblock (src/filereq.c) | TEDs `filereq` weicht in 7 Füllbytes von freqlib ab (`"OK",0,$06` statt `$00` …, Rest am Hunkende) | Datenblock der freqlib-Fassung (eine Quelle, „Variante C“) | **B**/**T** | nein (Bytes werden nie gelesen) |
| `ezfreq` → `filerequest+36` (src/filereq.c) | a0 = `reqstruct`, der Requester liest den Block direkt | `filerequest()` kopiert die 4 Langwörter nach `fq_args` und ruft `fq_request()` | **T** | nein (der Block wird während des Requesters nicht geändert) |
| `jumptable`/`menutable` (src/input.c) | gepackte 6-Byte-Einträge `{code.b, flag.b, fn.l}` | Array `struct jtentry` (Funktionszeiger) in gleicher Reihenfolge, gleiche Endmarken | **T** | nein (niemand sonst liest die Tabellen) |
| `escape` (ted.s:1016) | `lea 12(a7),a7` + `bra freeall`: kehrt hinter `bsr mainedit` zurück | `edquit = TRUE`, `mainedit` kehrt direkt nach dem Handler zurück | **T** | nein |
| `erase`/`getfname3`/`doopen`/`getbname` | `addq #4,a7`: Rücksprung am Aufrufer vorbei | Rückgabewert, jeder Aufrufer prüft und kehrt zurück | **T** | nein |
| `outmem`/`memcont` | `move.l memstack,a7 / bra memcont` | `longjmp(*memstack)` auf den `setjmp` in `mainedit` (verwirft wie das Original auch blitz2-Rahmen); `memstack == 0` (O1): `memcont_nostack` (src/glue.c) macht es wörtlich (a7 = 0, memcont, `bra mainedit`) – Absturz wie im Original, nur die gerufenen C-Routinen benutzen den Stapel anders | **T** | nein (Absturzstelle bei O1 evtl. eine andere) |
| Sentinel `dc ' :'` vor `filepath` (ted.s:6139) | `.tomain` liest `filepath[-2]` (= ':') bei leerem Pfad | leerer Pfad wird ausdrücklich geprüft (gleiches Ergebnis) | **T** | nein |
| edstruct-Kopie (ted.s:24) | 14 Wörter am Stück nach `tokens…remark` | feldweise in einzelne Globals | **T** | nein |
| chmap-Blits in `scrollup`/`scrolldown`/`eraseline`/`cls` | Blitter auch auf der Zeichenkarte | `chmap_*` im Kern mit der CPU (gleiche Bytes), Bitplanes weiter per Blitter | **T** | nein (Timing: Blitter und CPU arbeiten im Original parallel) |
| feste Schrittweiten über Strukturen (survey-07 G.1) | `ga4 + n*84`, `hp1 + n*86`, `ga10 + n*44`, `bo14 + n*28/56` | Zeigertabellen `intgads`, `hps`, `colgads`, `colbords` (src/data.c) | **T** | nein |
| comdata-Einsprünge (src/glue.c) | sichern teils nichts (openreq, getinput, error …) | REGSTUB sichert alle Register außer den Ergebnissen | **B** (wie libs-c: „mindestens, was das Original erhält“) | nein |
| `edstart`/wbheader, `dosopen` schlägt fehl (src/main.c `edstart_c`) | `wb_exit`-`rts` kehrt nach `outside` zurück, a1 undefiniert (survey-01 I.2) | `edstart_stub` setzt immer a1 = 0: `outside` läuft als Standalone weiter (ohne DOSBase) | **T** | nein (dos.library fehlt nie) |
| Menüpunkte: Füllbyte hinter `Command` (src/data.c) | `dc.b 'X '`: Füllbyte = Leerzeichen | `struct MenuItem` hat dort Padding, C initialisiert es mit 0 | **T** | nein (nie gelesen; auch nicht in den COMPILER-Kopien von `getstuff`) |
| `sleepon` Pfad „schon aktiv“ (src/main.c) | nach `movem` Restore zerstört `makemenus` a0/d0 des blitz2-Aufrufers (survey-01 I.7) | Glue sichert alle Register (Zeile „comdata-Einsprünge“) | **B** | nein |
| `swapd0bord` (ted.s:3238; src/reqs.c) | Knopfindex ≥ 16 (Farbwert ≥ 4 aus beschädigter Optionsdatei): tauscht die Bytes 4/5 bei `bo14 + (d0*56).w` und +28 | gleiche Rechnung relativ zu `defaultreqbo14` (Original-Schrittweiten); die C-Daten liegen anders, dort stehen andere Variablen | **T** (Datenlayout) | nur bei beschädigter `l:blitzeditor.opts` |

## In Phase 2c zurückgenommen (Amiga wieder wie das Original)

| Stelle | bis 2b | jetzt (Amiga) | Host |
|---|---|---|---|
| `divu` (alle 11 `divu` von ted.s; `divu_w`, `tabrem`, `showvprop`/`showhprop`, Dateirequester `fq_divu_w`) | C-Division; durch 0 wie Überlauf, sonst `__udivsi3`/`__umodsi3` aus libnix20 (brauchen `UtilityBase` → Absturz an jeder Stelle) | `divu_w` in src/glue.c = der Befehl `divu.w` selbst (Überlauf: V, Register unverändert; durch 0: Trap 5 wie im Original); ctest `checklink` stellt sicher, dass kein Arithmetik-Helfer gelinkt wird | C-Fassung, Abbruch bei 0 (O3) |
| `findtoke`, `tokit` `.tsttoke` mit `tokens == 0` | unbekanntes Token / kein Treffer | läuft den „Knoten“ bei Adresse 0 ab (Wort bei 4, Name ab 6, `next` = Langwort bei 0) – erreichbar: TED standalone lädt eine tokenisierte Datei | Schutzabfrage |
| `freeall` mehr Runden als Zeilen | Abbruch | liest das „Item“ bei 0, `FreeMem(0, …)`, weiter | Schutzabfrage |
| `freeupa5` `.first` bei inkonsistenter Liste | kein Schreibzugriff | `clr.l 4` (überschreibt den ExecBase-Zeiger, wie das Original) | Schutzabfrage |
| `chmap_scrolldown` | Lesen vor chmap entfiel | liest die Bytes vor chmap wie der Blit | Schutzabfrage |
| `swapd0bord` Index ≥ 16 | ignoriert | tauscht wie das Original (Zeile oben, T wegen Datenlayout) | – (nur Amiga) |
| `outmem` mit `memstack == 0` (O1) | `longjmp` durch NULL | a7 = 0 + memcont (Zeile oben) | – |
| `openio` → `mycli` (Z-Flag) | Handle != 0 | `WBenchToFront` über `callregs`: d0 und CCR wie im Original; comdata+60 gibt die Flags weiter (`REGSTUB_CCR`) | – |
