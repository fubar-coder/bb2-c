# Archiv: unveränderte Originale

Hier liegt fremdes Originalmaterial, das der Build oder die Tests als
Referenz brauchen – **unverändert**, nie bearbeitet. Was daraus abgeleitet
wird, entsteht im Build-Verzeichnis (z. B. der Referenz-Build von TED) oder
liegt als erzeugte Testdaten unter `tests/data/`.

| Verzeichnis | Inhalt | Herkunft |
|---|---|---|
| `sources/ted-1.1/` | TED 1.1, der Editor von Blitz Basic 2: `ted.s` mit Includes (`myoffs`, `wbheader`, `aboutreq`, `specialreq`, `filereq`) und Binärdateien (`rawed.obj`, Zeichensätze, Mauszeiger, Icon) | `bb2src.zip` → `tedt/` (Stand 14.09.1995), aus `amibl2src.lha` (Aminet `dev/basic`, Uploader Bernd Rösch; Quelltexte von Acid Software, laut Readme unter GNU-Lizenz). `tedt/ted.s` ist das `ted.s` des Archivs mit einfachen Include-Pfaden statt der Devpac-Zuweisung `e:` – so schon im Archiv. |

Regeln:

* Nichts hier ändern. Ein neuer Stand ist ein neues Verzeichnis mit
  Versionsnummer (`ted-1.1`, später z. B. `blitz2-1.95`).
* Jedes Verzeichnis steht in dieser Tabelle mit seiner Herkunft.
* Git behandelt alles hier als binär (keine Zeilenende-Umwandlung);
  Assembler-Quellen bekommen Diffs.
