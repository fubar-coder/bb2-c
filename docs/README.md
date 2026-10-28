# Dokumentation: Analyse und Übersetzung

Befunde aus der Analyse der Originalquelltexte (`blitz2.s`, `ted.s`,
Libraries) und Notizen zur Übersetzung nach C: Bestandsaufnahmen,
Reviews, Abweichungen vom Original, Konventionen. Hier stehen die
Begründungen für Code-Kommentare wie `BUG(orig)`, `DIFF`, `ted.s:NNN`
oder `blitz2.s:NNN`.

**Übernommen aus dem Arbeitsrepository bb2, Stand `ee6174b`
(28.10.2026), unverändert.** Pfade, Befehle und Zeilenverweise in den
Dateien beziehen sich deshalb auf den Aufbau von bb2 (z. B.
`libs-c/libs/gtlib.c`, `quelltext/amibl2src/…/blitz2.s`); die Zuordnung
steht unten. Die Zeilennummern der Dateien sind dieselben wie in bb2.

| hier | bb2 |
|---|---|
| `architektur.md`, `dokumentation.md`, `formate.md`, `token-tabelle.md` | gleichnamig im Hauptverzeichnis |
| `blitz2/` | `blitz2-c/notes/`; `CONVENTIONS.md` aus `blitz2-c/`, `bb2-readme.md` = `blitz2-c/README.md` |
| `ted/` | `ted-c/notes/`; `bb2-readme.md` = `ted-c/README.md` |
| `libs/` | `libs-c/notes/`; `bb2-readme.md` = `libs-c/README.md` |

Nicht übernommen: die Arbeitsaufträge (`*BRIEF*.md`), die detokenisierten
Library-Quellen (`libs-c/bb2src/*.bb`, aus Material ohne klare Lizenz),
Korpus und Ergebnisse (`compilerlauf/`), Token-Dumps (`tokens/`),
`toolchain.md` und `ci-exploration.md`.

Zuordnung der Pfade in den Texten:

| bb2 | hier |
|---|---|
| `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s` | `archive/sources/blitz2-1.95/blitz2.s` |
| `…/bb2src.zip.dir/tedt/ted.s` | `archive/sources/ted-1.1/ted.s` |
| `blitz2-c/src/` | `programs/blitz2/src/` |
| `ted-c/src/` | `programs/ted/src/` |
| `libs-c/libs/` | `libs/acidlibs/` |
| `libs-c/deflibs/`, `libs-c/include/` | `libs/deflibs/`, `libs/include/` |
| `cd/` | `archive/cd/ultimate-blitz-basic-2.1/` (Großbuchstaben, siehe `archive/README.md`) |
