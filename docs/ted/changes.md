# Änderungen an gemeinsamen Headern

Eine Zeile je Änderung an einer Datei im Besitz des Koordinators
([CONVENTIONS.md](CONVENTIONS.md) §1/§9): Datum, Datei, Änderung, Anlass
(Chunk, asm-Beleg).

| Datum | Datei | Änderung | Anlass |
|---|---|---|---|
| 2026-10-26 | alle | Phase 2a: Header, Gerüste, Glue, Ressourcen, Tests angelegt | PHASE2B-BRIEF.md |

## 2b: Kern-Funktionen für die Bildschirm-Durchläufe (Agent A, Koordinator)

* `fileio.h`: `chmap_showa2item(struct item *, void (*draw)(UBYTE))`,
  `chmap_showall`, `chmap_showtopitem`, `chmap_showbotitem` (ted.s:4050–4104,
  ohne Zeichnen; `draw` darf NULL sein). Die Host-Stubs (`tests/host/stubs.c`)
  rufen sie mit `NULL` auf (O4).
* `tests/host/toklist.c`: Token-Nummern ohne Bit 15 (blitz2 speichert sie so,
  `findtoke`: `bclr #15,d0`). `mkfixtures.py`: neues Orakel `tokall/`.
* `tests/host/CMakeLists.txt`: Tests linken `tedhost`/`tedcore` als Link-Gruppe.

## 2c: Integrations-Review (Koordinator)

| Datum | Datei | Änderung | Anlass |
|---|---|---|---|
| 2026-10-26 | `src/glue.c`, `src/glue.h` | neu `divu_w` (Asm: `divu.w`, echter Trap/Überlauf), `memcont_nostack` (outmem mit `memstack == 0`), `REGSTUB_CCR` (Stub gibt die CCR des C-Adapters zurück), `GLUE_VAR` (Global in Asm, a4-relativ) | Punkt 1/2/3: libnix `__udivsi3` braucht `UtilityBase`; O1 |
| 2026-10-26 | `src/core/fileio.h`/`.c` | `divu_w` im Kern nur noch `TED_HOST`; Kommentar `sortblox` (*r = die **geordneten** Werte), `bloken` schreibt `"block"` (klein) | Punkt 1, Meldung Agent A |
| 2026-10-26 | `src/core/hooks.h` | Kommentar `blokprnt2` (`bltext`, nicht „Block“) | Punkt 5 |
| 2026-10-26 | `src/core/lineedit.c` | `tabrem` über `divu_w`; `chrow` mit `(WORD)chloc` | Punkt 1, review-2c.md |
| 2026-10-26 | `src/input.c` | `showvprop`/`showhprop` über `divu_w` | Punkt 1 (`__umodsi3`) |
| 2026-10-26 | `src/filereq.c` | `#define fq_divu_w divu_w` (freqlib_req.inc unverändert) | Punkt 1 |
| 2026-10-26 | `src/mouse.h`/`.c` | `openio(UWORD *ccr)` → `ULONG` (d0 + CCR von `WBenchToFront` über `callregs`), `mycli` prüft Z; `refgads(…, LONG d0)` (ganzes Register); `outmem` → `memcont_nostack` bei `memstack == 0` | Punkt 2/3 |
| 2026-10-26 | `src/glue.c` | `ezfreq_c`: a3 = `&reqstruct[3]` (Adresse, nicht Inhalt); `openio` mit `REGSTUB_CCR` | Punkt 3, notes/comdata.md |
| 2026-10-26 | `src/core/tokit.c`, `src/core/fileio.c` | Schutzabfragen `tokens == 0`, `freeall`, `freeupa5`, `chmap_scrolldown` nur noch `TED_HOST`; `(UWORD)d6 << 6` | Punkt 2, review-2c.md |
| 2026-10-26 | `src/reqs.c` | `swapd0bord` Index ≥ 16 wie das Original (Original-Schrittweiten) | Punkt 2 |
| 2026-10-26 | `src/screen.c` | `pixrows22 - (UWORD)(blitmod + 2)` (addq.w) in scrolldown/scrollrite | review-2c.md |
| 2026-10-26 | `tools/checklink.py`, `CMakeLists.txt` | ctest `checklink`: keine Bibliotheks-/Arithmetik-Helfer im gelinkten ted (Map) | Punkt 1 |
| 2026-10-26 | `tests/host/CMakeLists.txt` | `TED_XFAIL_TESTS` kein Cache-Eintrag mehr (alter Build hielt den 2a-Wert, grüne Tests liefen als WILL_FAIL rot) | Punkt 4 |
