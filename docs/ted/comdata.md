# comdata und Rückrufe: Registerschnittstelle Original ↔ Glue ↔ C

Stand Phase 2c. Quelle: `ted.s` (Zeilen unten), [architektur.md](../../architektur.md) §3.2/3.3,
Aufrufstellen in `blitz2.s` (CD-Quelltext) und `../blitz2-c/src/main.c`
(`ed_*`). Glue: `src/glue.c` (`REGSTUB`, `REGSTUB_CCR`, Adapter `*_c`).

## 1 Mechanik

* **`REGSTUB(name)`**: `movem.l d0-d7/a0-a6,-(sp)`, a4 = TEDs a4,
  `name_c(frame)`, alle Register aus dem (vom Adapter geänderten) Rahmen
  zurück, `tst.l d0`, `rts`. Ergebnis: alle Register außer den
  ausdrücklichen Ausgaben bleiben erhalten (Zeile „comdata-Einsprünge“ in
  [abweichungen.md](abweichungen.md), Kategorie **B**); die Flags sind die
  von `tst.l d0`.
* **`REGSTUB_CCR(name)`** (neu in 2c): wie oben, aber der Adapter liefert
  die CCR, mit der der Einsprung zurückkehrt (`move.w …,ccr` nach dem
  `movem`), für Routinen, deren Flags nicht `tst.l d0` entsprechen.
* Rückrufe nach blitz2 laufen über `callregs` (Register aus/in einen
  Rahmen, Rückgabe CCR).

## 2 comdata (TED → blitz2)

„Original“ = was die asm-Routine an Registern/Flags hinterlässt; „von
blitz2 benutzt“ = was die Aufrufstellen in `blitz2.s` danach lesen.

| Off. | Routine (ted.s) | Eingang | Original: Ausgang / zerstört | von blitz2 benutzt | Glue/C | Befund 2c |
|---|---|---|---|---|---|---|
| −8 | `screenclose` 4895 | – | d0/d1/a0/a1/a6 zerstört | nichts (1609) | REGSTUB, `screenclose()` | OK |
| −4 | `screenopen2` 4661 | – | d0… zerstört (screenopen-Ergebnis verworfen) | nichts (1664) | REGSTUB, `screenopen2()` | OK |
| 0 | `openreq` 3580 | a0 = Requester-Vorlage | Tail `bra pointeron`: d0 = Rest von `DrawBorder` (undefiniert; Fehlschlag → `outmem`, kehrt nie mit 0 zurück), a6 = IntuitionBase | nichts (1973, 2538, 4586, 15170, 15489, 15747, 15987) | REGSTUB, `openreq(a0)`; d0 = d0 des Aufrufers | OK (d0 im Original ohne definierten Wert; blitz2-c `ed_openreq` liest d0 – dort nie geprüft) |
| 4 | `closereq` 3643 | – | sichert d0-d1/a0-a2/a6 selbst | nichts (2471) | REGSTUB | OK |
| 8 | `getinput` 5845 | – | d7.l = Ereignis (−1 Gadget, −2 Menü, −3/−4 Maus, Taste), d6.w = Gadget-ID/Menünummer (oberes Wort bleibt), Z=0; a6 = exec, a0-a2/d0/d1 zerstört | d6.w, d7.w (`cmp #-1,d7`, `cmp #1,d6`) | REGSTUB, `getinput(&d6)`: d7 ganz, d6 per `SETW`, d0 = −1 (Z=0) | OK |
| 12 | `error` 3511 | a0 = Text | d7 gesichert; d6 = 1, sonst zerstört | nichts (7667 sichert d7/a6 selbst) | REGSTUB, `error(a0)` | OK |
| 16 | `detokallz` 2670 | – | zerstört | – (v1.95 ruft nicht) | REGSTUB | OK |
| 20 | `tokall` 2648 | – | zerstört | – | REGSTUB | OK |
| 24 | `ezfreq` 3502 | a0 Pfad, a1 Name, a2 Titel | d0 = Name/0, Z=1 bei 0 (Requester endet mit `move.l`), **a3 = `reqstruct+12`** (Adresse; filerequest+36 sichert d1-a6) | Z (`beq`), d0 (1526, 1564, 4127) | REGSTUB (`tst.l d0` = gleiches Z), `ezfreq()` | **korrigiert**: a3 war der *Inhalt* von `reqstruct[3]`, jetzt die Adresse |
| 28 | `refgads` 2629 | a0 Gadget, d0.l Anzahl | `jmp refreshglist`: d0/d1/a0/a1 zerstört, a1 = req_window, a2 = 0, a6 = IntuitionBase | nichts; 15510/15628 rufen dreimal über a5 (bleibt in beiden erhalten) | REGSTUB, `refgads(a0, LONG d0)` | **korrigiert**: d0 jetzt das ganze Register (vorher `UWORD`, 0xFFFF statt −1 bei oberem Wort ≠ 0) |
| 32 | `getinput2` 5833 | – | nichts da: d7 = 0, Z=1; sonst wie getinput (Z=0 wenn verstanden) | d7.l (`tst.l d7`), d6.w (2832, 15187) | REGSTUB, d0 = d7 ? −1 : 0 | OK |
| 36…44 | Daten `combitmap`, `myscreen`, `extint` | | | | `struct mydata` (structs.h) | OK |
| 48 | `colors` 4940 | – | zerstört | – | REGSTUB | OK |
| 52 | `halfcolors` 4917 | – | zerstört | – | REGSTUB | OK |
| 56 | `tokone` 2636 | a0 = Zeile (0-terminiert) | in place tokenisiert; d5 = Länge (tokit2), a1/a2 … zerstört; Zeile ≥ 32768 Bytes: `rts` ohne tokit2 (d5 unverändert) | nichts (3841) | REGSTUB, `SETW(d5, tokone(a0))` | OK (Fall ≥ 32768: d5.w = 0 statt unverändert – nicht erreichbar, blitz2-Zeilen < 256) |
| 60 | `openio` 2548 | – | Fehlschlag: d0 = 0, Z=1; Erfolg: d0 und CCR von `WBenchToFront`, a6 = IntuitionBase | nichts (1697) | **REGSTUB_CCR**, `openio(&ccr)` ruft `WBenchToFront` über `callregs` (LVO −342) | **korrigiert** (vorher Handle + `tst.l`; DIFF entfällt) |
| 64 | `closeio` 2597 | – | zerstört | nichts (1722) | REGSTUB | OK |
| 68 | `actgad` 2409 | a0 Gadget | d0 = `ActivateGadget`-Ergebnis | nichts (15176) | REGSTUB, d0 | OK (Flags: Original die von intuition, hier `tst.l d0`; nicht benutzt) |
| 72 | `pointeron` 209 | – | sichert a0-a1/d0-d3/a6 | – (blitz2 1490 `jmp`) | REGSTUB | OK |
| 76 | `sleepon` 223 | – | sichert a0-a1/d0-d3/a6; Pfad „schon aktiv“ zerstört a0/d0 über `makemenus` | – (1494) | REGSTUB | **B** (Zeile `sleepon` in abweichungen.md) |
| 80/84 | Daten `mywindow2`, (Gadget-Adresse) | | | | structs.h | OK |
| 88 | `extalloc` 3049 | d0 Größe, d1 Flags, **a6 des Aufrufers** (`jsr allocmem(a6)`) | d0 = Speicher, Z=0; Fehlschlag: `closereq` + `outmem` (kehrt nicht zurück) | d0 (`doallocmem` 3379 = `jmp`) | REGSTUB, `extalloc(d0, d1)` mit `AllocMem` über SysBase | OK (blitz2 ruft mit a6 = exec; ein anderes a6 würde im Original eine andere Bibliothek treffen – nicht erreichbar) |
| 92/94 | Daten `bmapmul`, `req_window` | | | 1939, 2546 | `struct mydata` | OK |

Restrisiko für Phase 3: der Stub erhält Register, die das Original
zerstört. An den Aufrufstellen oben (jede `comdata`-Stelle in `blitz2.s`,
jeweils die folgenden Befehle durchgesehen, nicht der ganze weitere
Kontrollfluss) wird kein zerstörtes Register vor dem Neuladen gelesen;
blitz2-c übergibt/liest nur die Spalte „von blitz2 benutzt“. Ein A/B-Test
mit blitz2 v1.95 (PHASE3-PLAN.md) deckt den Rest ab.

## 3 Rückrufe (TED → blitz2)

| Rückruf | ted.s | Original | C (`glue.c`) | Befund |
|---|---|---|---|---|
| `menurout` | 1542 `extmenu` | `lsr #4,d6`, a5 = `firstitem`, d7.w = `numlines`, a4 = `thisitem` (Handler-Konvention); d0.w zurück (−1/−2/Zeile) | `call_menurout(d6 >> 4)`: d6/d7 oberes Wort 0 (im Original Reste), a4/a5 wie oben, Rückgabe `(WORD)d0` | OK |
| `cleanup` | 2771 `doclean` | `jsr (cleanup)`, alle Register gesichert | `call_cleanup()`: Register 0, TEDs Register über `callregs` gesichert | OK |
| `loadxtra`/`savextra` | 710/795 | d7 = Handle, a6 = DOSBase, a5 = Routine | `call_xtra`: d7, a5, a6 wie Original | OK |
| VBlank `myint` | 306 | `is_Code`: a1 = is_Data, Rückgabe d0 = 0 (Z=1) | `myint_stub`: a4 gesichert/geladen, `moveq #0,d0` | OK |
| Hunk+4 `outside` / `.tomain` / `edstart` | 24/87/136 | siehe glue.h | glue_start.c | unverändert (2a, checkhunk) |
