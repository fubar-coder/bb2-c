# ted-c – TED 1.1 (Editor von Blitz Basic 2) in C

C-Port von `ted.s` (TED 1.1) nach dem Verfahren des Compiler-Ports
`../blitz2-c`. Stand: **Phase 2c** – alle Routinen übersetzt (2b),
Integrations-Review (2c: [notes/review-2c.md](notes/review-2c.md),
comdata-Schnittstelle [notes/comdata.md](notes/comdata.md)); Host- und
Amiga-Prüfungen grün.  **Phase 3** (Emulator-A/B-Tests,
[notes/PHASE3-PLAN.md](notes/PHASE3-PLAN.md), Protokoll
[notes/PHASE3-LOG.md](notes/PHASE3-LOG.md)): Stufe 1 abgeschlossen – alle
13 Tests (Start, Rundreisen, Tippen, Join, Block, Suchen/Ersetzen,
Kompilieren, Fehlerzeile, `.xtra`, Optionen) mit TED-C und Referenz-TED
unter blitz2 1.95 gleich; Stufe 2a abgeschlossen – dieselben Tests mit
blitz2-c (Rc/Cc), alle 52 Vergleiche gleich (`results/ted-ab/report.txt`);
als Nächstes Stufe 2b (breitere Tests).

```
cmake --preset amiga && cmake --build --preset amiga && ctest --preset amiga
#  -> build/amiga/ted, ted.sym (C-Build)
#  -> build/amiga/ref/ted, ted.lst, ted.map, ted.labels  (vasm-Build von ted.s = Referenz)
#  ctest: ref_verify, checkhunk, checkdata, checklabels, checklink
cmake --preset host && cmake --build --preset host && ctest --preset host
#  -> Kernbibliothek (src/core) + Unit-Tests auf dem Host (Fixtures aus den
#     CD-Quellen und bb2tok.py in build/host/fixtures)
python3 -I tools/tedab.py gate         # Phase 3: Gate-Sitzung vorbereiten (ohne --go startet nichts)
python3 -I tools/tedab.py plan --test '0*'               # Sitzungen (je ≤ 50 s)
python3 -I tools/tedab.py run --test 00-start [--go]    # eine Sitzung, ohne Fenster (tools/amiberry.cmake)
python3 -I tools/tedab.py report --all                  # Bericht über alle Sitzungen
```

| Datei | Inhalt |
|---|---|
| `notes/reference.md` | Referenz-Build, Werkzeuge, Binär-Includes (rawed.obj, filereq, Daten) und Entscheidungen |
| `notes/overview.md` | Chunks, Architektur, Querschnitt der Surveys, Risiken |
| `notes/survey-01…07.md` | Schnittstellen-Survey je Chunk |
| `notes/xrefs.md` | Querverweise zwischen den Chunks (`tools/xrefs.py`) |
| `notes/PLAN.md` | Datei-Layout, Header, Glue, Host-Kern, Verifikation |
| `notes/SURVEY-BRIEF.md` | Auftrag der Surveys |
| `notes/CONVENTIONS.md` | Übersetzungskonventionen (Phase 2b) |
| `notes/PHASE2B-BRIEF.md` | Auftrag Phase 2b: Besitz, Prüfung, Aufteilung, offene Entscheidungen |
| `notes/abweichungen.md` | alle `DIFF:`-Stellen |
| `notes/changes.md` | Änderungen an gemeinsamen Headern |
| `notes/comdata.md` | Registerschnittstelle comdata/Rückrufe: Original ↔ Glue ↔ C |
| `notes/review-2c.md` | Phase 2c: Stichprobe auf Befehlsebene, Ergebnisse |
| `notes/PHASE3-PLAN.md` | Plan der Emulator-A/B-Tests |
| `notes/PHASE3-LOG.md` | Protokoll Phase 3 (Werkzeuge, Emulator-Läufe, Abweichungen vom Plan) |

| Verzeichnis | Inhalt |
|---|---|
| `src/core/` | Kern ohne AmigaOS-Header (auch Host): `tedtypes.h`, `tedcore.h`, `hooks.h`, `state.h/.c`, je Chunk `loadsave`, `lines`, `services`, `fileio`, `tokit`, `lineedit`, `keymap` |
| `src/` | Amiga-Teil: `ted.h`, `structs.h`, `glue.h`, `glue_start.c`, `glue.c`, je Chunk `main`, `edit`, `mouse`, `reqs`, `screen`, `input`, `data`, dazu `rawkey`, `filereq` (Kern `../shared/filereq/filereq_core.c`, Umgebung `filereq_env.h`) |
| `tests/host/` | Host-Tests (`test_*.c`), Stubs der Hooks, Fixture-Erzeugung |
| `tests/amiga/` | Phase 3: `teddrive` (Treiber, Weg D), `teddrivetest` (Gate D), `tedprobe` (Gate 0), `tdscript.h` (Skriptsyntax) |
| `tests/emu/` | Phase 3: Testskripte `*.drv` (teddrive) und Gate-Skripte |
| `tests/tools/` | Host-Tests von `tedab.py`/`teddump.py` und der Skriptsyntax |
| `results/ted-ab/` | Phase 3: Ergebnisse (nur Text; Binärdateien unter `build/ted-ab/`) |
| `tools/` | `tedlisting.py`, `xrefs.py`, `checkhunk.py`, `checkdata.py`, `checklabels.py`, `checklink.cmake`; Phase 3: `tedab.py` (Runner), `teddump.py` (Bildschirm-Dumps) |
