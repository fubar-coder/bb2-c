# Archiv: unveränderte Originale

Hier liegt fremdes Originalmaterial, das der Build oder die Tests als
Referenz brauchen – **unverändert**, nie bearbeitet. Was daraus abgeleitet
wird, entsteht im Build-Verzeichnis (z. B. der Referenz-Build von TED) oder
liegt als erzeugte Testdaten unter `tests/data/`.

| Verzeichnis / Datei | Inhalt | Herkunft |
|---|---|---|
| `sources/amibl2src.readme` | die Readme des Aminet-Archivs `amibl2src.lha` – Beleg für Herkunft und Lizenz der Quellen | `amibl2src.lha` (Aminet `dev/blitz/amibl2src`) |
| `sources/ted-1.1/` | TED 1.1, der Editor von Blitz Basic 2: `ted.s` mit Includes (`myoffs`, `wbheader`, `aboutreq`, `specialreq`, `filereq`) und Binärdateien (`rawed.obj`, Zeichensätze, Mauszeiger, Icon) | `bb2src.zip` → `tedt/` (Stand 14.09.1995), aus `amibl2src.lha` (Aminet `dev/basic`, Uploader Bernd Rösch; Quelltexte von Acid Software, laut Readme unter GNU-Lizenz). `tedt/ted.s` ist das `ted.s` des Archivs mit einfachen Include-Pfaden statt der Devpac-Zuweisung `e:` – so schon im Archiv. |

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
