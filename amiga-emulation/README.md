# Emulationsdaten (nicht im Repository)

Für die Emulator-Tests (`tools/lib/amiga_emulation`) liegen hier ein
Kickstart-ROM und zwei Hardfiles. Sie sind nicht eingecheckt (Kickstart und
AmigaOS dürfen nicht weitergegeben werden); jeder legt seine eigenen
Kopien hier ab.

| Datei | Inhalt | Größe | SHA-256 (Stand 27.10.2026) |
|---|---|---|---|
| `A1200.47.115.rom` | Kickstart 3.2 (47.115), A1200 | 524288 | `27ddaf0a460e2ee85935139cbac1df234c283b3979c50526d118e9a55c61100a` |
| `Workbench_OS3.2-AB3-AGA.vhd` | AmigaOS 3.2, Workbench nativ (PAL HighRes Laced, 256 Farben) – Standard | 67176960 | – |
| `Workbench_OS3.2-AB3.vhd` | AmigaOS 3.2, Workbench auf RTG (uaegfx, 1680×1050) | 67176960 | – |

Die Hardfiles hat jeder selbst eingerichtet, deshalb gibt es für sie keine
Prüfsumme; sie müssen nur die Bedingungen unten erfüllen.

## Konfiguration

`tools/lib/amiga_emulation/default.uae` ist die Standard-Konfiguration
(A1200, AGA, 68020/68882, 8 MB Chip, 128 MB Z3-Fast, das AGA-Image). Tests
ändern sie nur durch Schlüssel=Wert-Unterschiede. Die RTG-Variante:

```
gfxcard_size=16
gfxcard_type=ZorroIII
hardfile2 / uaehf0: Workbench_OS3.2-AB3.vhd statt Workbench_OS3.2-AB3-AGA.vhd
```

TED ist nur mit der AGA-Variante testbar (auf RTG lägen seine Bitplanes im
Grafikkartenspeicher, wo der Blitter sie nicht erreicht).

## Bedingungen an die Hardfiles

* **`user-startup`** führt `TEST:run.script` aus, wenn `TEST:` eingebunden
  ist, und fragt das Laufwerk abgesichert ab (ohne `TEST:` sonst Requester
  „Datenträger einlegen“, der den Start blockiert):

  ```
  Assign >NIL: TEST: EXISTS
  If NOT WARN
    If EXISTS TEST:run.script
      Execute TEST:run.script
    EndIf
  EndIf
  ```
  Keine weiteren Laufwerke voraussetzen, die nicht immer eingebunden sind.
* **Schreibgeschützt:** die Hardfiles werden `ro` eingebunden; mehrere
  Instanzen teilen sich dasselbe Image, nichts verändert es. `T:` und
  `ENV:` liegen im RAM. Jeder Schreibversuch auf `SYS:` öffnet den
  Requester „Datenträger Workbench ist schreibgeschützt“ und blockiert den
  Lauf bis zum Zeitlimit – auch durch Systemprogramme: `ShowConfig`
  schreibt auf `SYS:` und hängt deshalb (geprüft 27.10.2026). Tests
  schreiben nur nach `TEST:` oder `RAM:`.
* Ein Image ändern: mit einer eigenen, beschreibbaren Konfiguration
  starten.
