# Archiv: unveränderte Originale

Hier liegt fremdes Originalmaterial, das der Build oder die Tests als
Referenz brauchen – **unverändert**, nie bearbeitet. Was daraus abgeleitet
wird, entsteht im Build-Verzeichnis (z. B. der Referenz-Build von TED) oder
liegt als erzeugte Testdaten unter `tests/data/`.

| Verzeichnis / Datei | Inhalt | Herkunft |
|---|---|---|
| `sources/amibl2src.readme` | die Readme des Aminet-Archivs `amibl2src.lha` – Beleg für Herkunft und Lizenz der Quellen | `amibl2src.lha` (Aminet `dev/blitz/amibl2src`) |
| `sources/blitz2-1.95/` (in git) | der Compiler Blitz Basic 2 v1.95 (`version equ 195`): `blitz2.s` mit den Dateien, die er einbindet (`myoffs`, `wbheader`, `options2.src.new`, `asmmac`, `typereq`, `options.src`, `calcreq`, `obj.info`) und `GenAm.opts` (`nocase`) | `bb2src.zip` (Stand 14.09.1995, Hauptverzeichnis), aus `amibl2src.lha` wie unten; nur die Dateien, die `blitz2.s` braucht. `myoffs` und `wbheader` sind dieselben wie in `ted-1.1` (je Fassung vollständig). |
| `sources/ted-1.1/` (in git) | TED 1.1, der Editor von Blitz Basic 2: `ted.s` mit Includes (`myoffs`, `wbheader`, `aboutreq`, `specialreq`, `filereq`) und Binärdateien (`rawed.obj`, Zeichensätze, Mauszeiger, Icon) | `bb2src.zip` → `tedt/` (Stand 14.09.1995), aus `amibl2src.lha` (Aminet `dev/basic`, Uploader Bernd Rösch; Quelltexte von Acid Software, laut Readme unter GNU-Lizenz). `tedt/ted.s` ist das `ted.s` des Archivs mit einfachen Include-Pfaden statt der Devpac-Zuweisung `e:` – so schon im Archiv. |

## Heruntergeladen (nicht in git)

Fremdes Material ohne klare Lizenz für die Weitergabe – vor allem die
Beiträge Dritter auf der CD – liegt nicht im Repository. Für die Tests holt
es `cmake -P cmake/fetch-originals.cmake` (oder das Build-Ziel
`fetch-originals`); nur dieser Schritt braucht Netz, der normale Build
nicht. Die Downloads landen in `.cache/downloads/` und werden über ihre
SHA-256 geprüft.

| Verzeichnis | Inhalt | Download | SHA-256 |
|---|---|---|---|
| `cd/ultimate-blitz-basic-2.1/` | die CD „Ultimate Blitz Basic 2.1“ (1997, Acid Software), 4310 Dateien | <https://archive.org/download/ultimate-blitz-basic-v2_1/Ultimate%20Blitz%20Basic%202.1%20%281997%29%28Acid%20Software%29.iso> | `205a16fa83403d61a785733cec2518bcc1fe5a6fae5b7da4da6329bd32c99166` |
| `aminet/amibl2src/` | `amibl2src.lha`, darin `bb2src.zip` entpackt nach `bb2src/` und `bblibsrc.lha` (Blitz-Quellen der Libraries, tokenisiert, 209 Dateien) nach `bblibsrc/` | <https://aminet.net/dev/blitz/amibl2src.lha> | `b71e6445d0838b15416f5b55c5be422f07bd4fae5da5c065ce329042067628ff` |

* Die CD hat nur ISO-9660-Namen (kein Rock Ridge, kein Joliet), also
  Großbuchstaben – so bleiben sie. Der Linux-Kernel zeigt sie beim
  Einbinden klein (`map=normal`); die Kopie in bb2 hat deshalb kleine
  Namen. Einzige Änderung: die drei Namen mit Latin-1-Zeichen werden UTF-8
  (`BUI6/BLITZBANK/CATALOGS/FRANçAIS`, `BUI6/FRONTENDV1.0/CATALOGS/ESPAñOL`,
  `BUI7/FRONTENDV1.2/CATALOGS/ESPAñOL`).
* Das Skript prüft, dass `sources/ted-1.1/` und `sources/blitz2-1.95/`
  genau den Dateien in `bb2src.zip` des Aminet-Archivs entsprechen.
* `bblibsrc/` ist der Inhalt des Verzeichnisses `bblibsrc/` von
  `bblibsrc.lha` (eigene Marke `bblibsrc.sha256`, wird also auch in einem
  schon entpackten Archiv nachgeholt). Lesbar als Text macht ihn das
  Build-Ziel `bblibsrc` (`build/<preset>/bblibsrc/NAME`, README,
  Abschnitt „Lesbare Quellen der Libraries“).
* `RWELibSourc.lha` bleibt gepackt: spätere Libraries von Red When
  Excited (287 Dateien), nicht Teil von blitz2 1.95 und
  seinen acidlibs, also keine Referenz für diesen Port; die Token-Liste
  von blitz2 1.95 (`tests/data/ted-tokens.txt`) kennt nicht alle ihre
  Tokens.

## Lizenzlage der Originale

Stand 28.10.2026, nach Recherche:

* **Quelltexte von Blitz Basic 2 und TED** (`bb2src.zip` in
  `amibl2src.lha`): `sources/amibl2src.readme` sagt „Thanks to Mark Sibly
  and Simon Armstrong and RWE that they give the Source and allow to
  enhance my loved Blitzbasic2“ und „The Source is under GNU see Acid
  Homepage at www.acid.co.nz“ – ohne Versionsnummer und ohne Lizenztext
  im Archiv. Damals gab es nur die GPL 2; nennt ein Programm keine
  Version, darf man nach GPL 2 § 9 jede veröffentlichte Version wählen.
  Die Homepage von Acid ist nicht mehr erreichbar.
* **AmiBlitz3**, die spätere Weiterentwicklung derselben Code-Linie, steht
  auf SourceForge unter der Apache License 2.0
  (<https://sourceforge.net/projects/amiblitz3/>: „Acid Soft was so
  friendly to release the BB2 source as Opensource.“). Der Kopf des
  Compiler-Quelltexts dort (`Sourcecodes/Amiblitz3/Compiler/AmiBlitz3.ab3`)
  sagt: „Auf Bestreben von Bernd Roesch durfte die Amiga-Version
  disassembliert und weiterentwickelt werden.“ Wann und auf welcher
  Grundlage der Wechsel zu Apache stattfand, ist nicht mehr
  nachzuvollziehen. **Im Code-Repository selbst** (SVN, nach git
  konvertiert, 602 Commits ab 10.02.2009) gibt es weder eine Lizenzdatei
  noch einen Commit zu einem Lizenzwechsel; `Docs/Amiblitz3.guide`,
  Abschnitt „Program Licenses“, sagt seit dem ersten Commit bis zum
  letzten Stand unverändert: „The "AmiBlitz3" and "PED" programs are
  relased under the GNU General Public License […] either version 2 of
  the License, or (at your own option) any later version.“, „The
  libraries and additional tools are released under their own original
  or updated licenses.“ und „No-one is claiming any intellectual or any
  other copyright over software developed in AmiBlitz3. You are free to
  release anything you create in AmiBlitz3 under any license you wish
  (at the time of writing there are no GPL licensed libraries).“ Die
  Apache-Angabe steht also nur in den Projekt-Angaben auf SourceForge.
  Dieses Projekt baut auf dem GPL-Stand auf; die letzte Aussage entspricht
  inhaltlich seiner Laufzeit-Ausnahme.
* **Library-Anleitungen** der Zeit tragen den Vermerk „Used with the
  permission of Acid Software“ (z. B. AmiBlitz3
  `Docs/AcidLibs/SORTLIB.guide`).
* **Rechteinhaber:** Acid Software und Red When Excited bestehen nicht
  mehr; eine Stelle, die über die Lizenz der Originale verbindlich
  entscheiden könnte, ist nicht bekannt. Urheberrechte erlöschen dadurch
  nicht – deshalb baut dieses Projekt auf der dokumentierten
  GNU-Freigabe auf.

Regeln:

* Nichts hier ändern. Ein neuer Stand ist ein neues Verzeichnis mit
  Versionsnummer (`ted-1.1`, später z. B. `blitz2-1.95`).
* Jedes Verzeichnis steht in dieser Tabelle mit seiner Herkunft.
* Git behandelt alles hier als binär (keine Zeilenende-Umwandlung);
  Assembler-Quellen bekommen Diffs.
