# Abweichungen der C-Portierungen von den Originalen

Vollständige Übersicht aller bekannten Stellen, an denen sich die
C-Fassungen (`libs/*.c`, `deflibs/*`, `../blitz2-c`, `../ted-c`) anders verhalten als die
Originale (CD-Binaries bzw. `blitz2.s`). Grundlage: alle `DIFF`-Markierungen
in den Quelltexten, die DIFF-Abschnitte in `cmdlibs-port.md` und
`deflibs.md`, die Projektkonventionen und eine Durchsicht der eigenen Ports
auf nicht markierte Abweichungen. `BUG(orig)` (absichtlich übernommene
Fehler des Originals) steht **nicht** hier, sondern in den Kopfkommentaren
und in `cmdlibs-port.md`.

Regel (Benutzerentscheidung): Fehler der Referenz werden nachgebildet und
dokumentiert, nicht behoben. Abweichungen nur dort, wo sie technisch
unvermeidbar sind, mit Begründung. Die bereits vorhandenen Abweichungen
bleiben bestehen (nicht zurückgebaut), müssen aber hier vollständig
dokumentiert sein.

## Kategorien

| Kürzel | Art | Bedeutung |
|---|---|---|
| **B** | vom Benutzer entschieden | Projektkonvention, für alle Ports festgelegt |
| **T** | technisch unvermeidbar | das Original ist hier undefiniert (Registerreste, Stackinhalt, Zeitverhalten, Speicherlayout) oder in C nicht ausdrückbar |
| **K** | stille Korrektur | das Original wäre nachbildbar, die C-Fassung verhält sich „sicherer“ (Trap statt Sprung ins Leere, Rückkehr statt Absturz, Register bleibt erhalten). Nach der neuen Regel nicht mehr zulässig; Bestand bleibt, ist aber hier aufgeführt |

„beobachtbar?“: ob ein Basic-Programm den Unterschied sehen kann
(Ergebnis, Ausgabe, Absturz) oder nur ein Register-/Speichervergleich.

## 1. Konventionen (gelten für alle Ports)

| Bereich | Original | C-Fassung | Art | Begründung / beobachtbar? |
|---|---|---|---|---|
| a2 (Argumentstapel der Befehle) | Routinen holen Längen/Typen mit `MOVE.x -(a2),dN` und lassen a2 je nach Weg unterschiedlich weit vermindert stehen | alle Routinen bekommen a2 als `BL_REG(a2)`-Parameter, lesen über einen lokalen Zeiger (`BL_POPL`/`BL_POPW`), a2 bleibt unverändert | **B** | Benutzerentscheidung; `preplibst` setzt a2 vor jedem Aufruf neu, der Compiler liest es danach nie (cmdlibs-port.md „Allgemein“). Nicht beobachtbar. Betrifft u. a. stringfunclib, vallib, printlib, editlib, intuifontlib, speaklib, audiolib, screenslib, gtlib, asllib, blitzcoplib, animlib, arexxlib, seriallib, fileiolib, brexxlib (Type) |
| Registererhalt | Routinen ändern beliebige Register (Spec: alles außer a4/a5/a7) | Thunks erhalten mindestens die Register, die das Original erhält, oft mehr (d0-d1/a0-a1 bei Statements) | **B** | Vereinbarung „Thunks erhalten mindestens, was das Original erhält“; zusätzlich erhaltene Register (z. B. d1 bei maxslib/modlib, d2/a2 bei maxlenlib) sind keine Ergebnisse. Nicht beobachtbar |
| Fehlerprüfungen „mindestens gleich“ | Prüfroutinen im Fehlerprüfbereich | ebenso (`BLITZ_ERRCHK`), Meldungen als Assemblerdaten dort | – | keine Abweichung, außer Abschnitt 6 |

## 2. System-Libs

| Library/Routine (Datei:Zeile) | Original | C-Fassung | Art | Begründung / beobachtbar? |
|---|---|---|---|---|
| memlib err1 (memlib.c:76) | Sprung aus localloc/newmem mit gesicherten Registern auf dem Stack, kehrt nie zurück | err1 kehrt zurück, Aufrufer liefert 0 | **K** | nur wenn der Laufzeitfehler-Trap zurückkehrt (normal endet das Programm); praktisch nicht beobachtbar |
| memlib locfree err2 (memlib.c:171) | unbekannter Block: einmalig `TRAP #0` „Unable to Free Memory“ (err2 im normalen Code); kehrt der Trap zurück, läuft der Code in `newmem` weiter, die von locfree gesicherten Register liegen noch auf dem Stack | locfree kehrt nach dem Laufzeitfehler zurück | **K** | nur bei zurückkehrendem Trap; praktisch nicht beobachtbar |
| arrayslib newlocarr/newloclist (arrayslib.c:29) | tauscht a4/a5 um den memlib-Aufruf | übergibt die Basis direkt | **T** | a4/a5 sind in C feste Register; memlib benutzt a5 nicht. Nicht beobachtbar |
| arrayslib Dimensionsrechnung (arrayslib.c:90) | liefert d1.w+1, ändert d2 | d2 erhalten, d1 undefiniert | **B** (Registererhalt) | Compiler benutzt d1/d2 nicht. Nicht beobachtbar |
| maxlenlib (maxlenlib.c:19) | ändert d2 und a2 | erhält d2/a2 | **B** | keine Ergebnisse. Nicht beobachtbar |
| maxslib doallocst (maxslib.c:33) | d1 = -1 nach dem Kopieren | d1 erhalten | **B** | kein Ergebnis |
| maxslib dofreest (maxslib.c:55) | d0 = memlibs d0 | d0 erhalten | **B** | kein Ergebnis |
| modlib domodquick (modlib.c:29) | d1 vertauscht zurück | d1 erhalten | **B** | kein Ergebnis |
| runerrlib Prüfketten (runerrlib.c:238) | läuft nach zurückkehrendem Trap durch die Fehlerkette weiter | C-Prüfung kehrt zurück | **K** | nur bei zurückkehrendem Trap (der Debugger beendet). Praktisch nicht beobachtbar |
| fontlib calcoff (fontlib.c:45) | nach Trap (kein Bitmap/Font) läuft es in den nächsten Trap | zeichnet nichts; unreferenzierte Prüfung `bitmapchk` fehlt | **K** | nur bei zurückkehrendem Trap |
| inputoutputlib defin Einzelzeichen (inputoutputlib.c:44, :232) | `BRA err1` in den Fehlerprüfbereich (ohne Debugger: Sprung hinter die Lib) | Trap und Meldung im normalen Code | **K** | ohne Debugger: Laufzeitfehler statt Sprung ins Leere (beobachtbar nur in diesem Fehlerfall). Technisch möglich wäre der Sprung (vgl. audiolib, screenslib, shapeslib) |
| wbstartuplib AddArg (wbstartuplib.c:74) | ohne Speicher Sprung zu `err` im Fehlerprüfbereich | Laufzeitfehler, NULL, Aufrufer bricht ab | **K** | nur ohne Speicher |
| linklistlib SortList (linklistlib.c:35) | zerstört a3 und d6 | a3 unverändert | **K** | Statement, a3 wird danach nicht gelesen. Nicht beobachtbar |

## 3. Befehls-Libs

| Library/Routine (Datei:Zeile) | Original | C-Fassung | Art | Begründung / beobachtbar? |
|---|---|---|---|---|
| 2dlib Überläufe (2dlib.c:44) | Polyf mit mehr als 16 Punkten / BitMapPoly-Kantenlisten überschreiben den Code hinter polypts/l1 | überschreiben die Daten dahinter | **T** | Speicherlayout; nur bei diesen Überläufen (BUG(orig)) beobachtbar. FloodFill (yofill) ist das Original-Assembler, a3 wie im Original |
| polylib Überläufe (polylib.c:34) | mehr als 16 Punkte / 10 Kanten je Seite treffen den eigenen Code | treffen die Daten dahinter | **T** | Speicherlayout; nur bei Überläufen |
| animlib err6/err7 (animlib.c:45) | `BEQ err7` in den Fehlerprüfbereich bzw. err6 im normalen Code | per JSR erreicht (Rücksprungadresse und gesicherte Register der C-Funktion auf dem Stack) | **T** | GCC lässt Code hinter einem Inline-BRA weg bzw. verschmilzt die Pfade; Ziel und Meldung wie im Original, nur der Stack beim Trap anders; nicht beobachtbar |
| animlib a2 (animlib.c:45) | a2 vermindert | unverändert | **B** | Konvention |
| arexxlib ReplyRexxMsg (arexxlib.c:60, :503) | mit einer Nicht-RexxMsg: holt 15 nie gesicherte Register (Absturz) | kehrt zurück | **K** | beobachtbar nur in diesem Fehlerfall (Absturz vs. nichts) |
| arexxlib CreateRexxMsg (arexxlib.c:60) | kopiert -8(a2) nach -4(a2) auf den Compiler-Stack | kopiert nicht, a2 unverändert | **B** | Konvention; der Compiler liest den Wert nicht |
| asllib Titel (asllib.c:44) | schreibt maxslibs d1 (-1) in die Längenzelle des Titels auf dem Compiler-Stack | Stack und a2 bleiben | **B** | Konvention; nicht beobachtbar |
| audiolib (audiolib.c:43) | a2 vermindert (Lautstärken/Längen) | unverändert | **B** | Konvention |
| bblitlib (bblitlib.c:25) | Einträge ohne nullsub sichern a4/a5 | ebenso (Vermerk: keine Abweichung im Verhalten) | – | – |
| blitlib BlitColl Amiga (blitlib.c:323) | oberes Wort von d0 = Rest aus DisownBlitter | 0 | **T** | Registerrest des OS (undefiniert); #word-Funktion, nicht beobachtbar |
| blitzcoplib CustomCop (blitzcoplib.c:56) | a2 um 4 vermindert | unverändert | **B** | Konvention |
| blitzkeyslib | – | keine Abweichung (DIFF-frei; Binary hat 6 unbenutzte Bytes mehr) | – | – |
| displaylib a2 (displaylib.c:46) | a2 als Argumentstapel vermindert | unverändert | **B** | Konvention |
| displaylib Fehlersprünge (displaylib.c:46) | alle Fehler: Bcc aus dem normalen Code in den Fehlerprüfbereich | per JSR (eine Rücksprungadresse mehr auf dem Stack) | **T** | wie animlib; Ziel gleich, nur Stackinhalt beim Trap |
| displaylib Tabellenindizes (displaylib.c:46) | Sprite-Modus > 5, Modustabelle jenseits Auflösung 2 / Fetch 3, Doublescan > 6 lesen den eigenen Code der Lib | lesen andere Daten | **T** | Speicherlayout; nur bei ungültigen Werten |
| blitzoslib Koordinaten (blitzoslib.c:63) | oberes Wort der an Boxf/Line/Locate übergebenen Koordinaten = Vorzeichen des Box-Felds | Wortwert vorzeichenerweitert | **K** | nachbildbar; unterscheidet sich nur bei Koordinaten um 0/negativ und nur, wenn die gerufene Lib das obere Wort benutzt |
| blitzoslib BCALL (blitzoslib.c:305) | Blitz-Aufrufe aus BoxShow (dobs) lassen a3 = Bitmap der Seite stehen | BCALL stellt a3/a4 (und a6) nach jedem Aufruf wieder her | **K** | **bisher nicht als DIFF markiert**; Statements, a3 wird danach nicht gelesen |
| brexxlib SaveTape/LoadTape (brexxlib.c:60, :594) | `BEQ err1` in den Fehlerprüfbereich | Trap und Meldung im normalen Code | **K** | nur bei „Can't Open File“ ohne Debugger beobachtbar |
| cliargslib ParPath$ (cliargslib.c:24, :117) | verzweigt auf das CCR, das dos.library nach NameFromLock hinterlässt | prüft d0 = 0 | **T** | CCR einer OS-Funktion ist undokumentiert; Ergebnis gleich |
| crunchlib CrM2 (crunchlib.c:49) | verfälschte Daten indizieren in Code/Daten der Lib | ebenso, aber das Layout der C-Lib ist anders | **T** | nur bei verfälschten CrM2-Daten |
| crunchlib Arbeitsbereich (crunchlib.c:49) | löscht 176 Byte auf dem Stack vor der Rückkehr; benutzt a5 als Arbeitsregister | lässt Locals stehen; a5 unverändert | **T** | Stack unterhalb von sp, a5 ist festes Register. Nicht beobachtbar |
| editlib (editlib.c:24) | a2 vermindert | unverändert | **B** | Konvention |
| fileiolib Fehler im normalen Code (fileiolib.c:63, :278) | err1/err2/err3/err6 (und Meldung err7) im Fehlerprüfbereich, ohne Debugger Sprung ins Leere | Trap und Meldung im normalen Code | **K** | nur in diesen Fehlerfällen ohne Debugger |
| fileiolib Register (fileiolib.c:63) | ändert d4-d7, a2, a3 | unverändert; unbenutzte Prüfung `reclenchk` fehlt | **B**/**K** | nicht beobachtbar |
| freqlib Fehlerweg (shared/filereq/filereq_core.c, Kopf und `fq_request`; gilt auch für TED) | ohne Chip-Speicher/Library/Fenster: erstes Long des (nicht initialisierten) FIB-Puffers | 0 | **T** | Inhalt nicht initialisierten Speichers |
| gameiolib | – | keine Abweichung (DIFF-frei) | – | – |
| gtlib GTTags/GTSetAttrs (gtlib.c:53) | poppen die Tag-Paare von a2 | a2 unverändert; unbenutzte Prüfroutine freqchk fehlt | **B** | Konvention |
| iffiolib iffopen (iffiolib.c:34) | a0 nach dos Open verloren | a0 erhalten | **B** (Registererhalt) | **nicht als DIFF markiert**, im Kopf beschrieben; nicht beobachtbar |
| ilbmifflib ilbminit (ilbmifflib.c:46) | a0 nach Fehlschlag = Rest aus dos.library | a0 = Dateiname | **T** | Registerrest; im Kopf beschrieben |
| intuifontlib (intuifontlib.c:28) | a2 vermindert | unverändert | **B** | Konvention |
| maxlenlib/maxslib/modlib | s. System-Libs | | | |
| medlib (medlib.c:56) | LoadMedModule/SetMedVolume lassen a3 auf Objekt+8 bzw. Track | a3 unverändert | **K** | Statements; nicht beobachtbar |
| palettelib Fehler (palettelib.c:55, :288) | AGARed/Green/Blue `err3` und Meldungen err0/err1/errd im Fehlerprüfbereich | Trap und Meldungen im normalen Code | **K** | nur in Fehlerfällen ohne Debugger |
| palettelib leere Palette (palettelib.c:52, :726, :732) | ShowPalette/Use Palette Amiga-Modus mit leerer Palette läuft in den Blitz-Code `b_showpal`, in beiden Zweigen: ab ExecVersion 39 (`agamode`, a0 beliebig) und darunter über `make4bitrgbs` (a0 = Palettenobjekt); schreibt nichts | kehrt in beiden Zweigen zurück | **K** | Ergebnis gleich (nichts geschrieben); nur mit `agamode` und ungeradem a0 auf dem 68000 Adressfehler im Original |
| printlib (printlib.c:23) | a2 vermindert | unverändert | **B** | Konvention |
| screenslib (screenslib.c:57) | a2 gepoppt | unverändert | **B** | Konvention |
| screenslib err1 (screenslib.c:53, :412) | Can't open Screen: `BEQ err1` aus dem normalen Code in den Fehlerprüfbereich | per JSR erreicht (eine Rücksprungadresse mehr auf dem Stack) | **T** | wie animlib/displaylib; Ziel gleich (ohne Debugger ebenfalls Sprung hinter die Lib), nur der Stack beim Trap anders; nicht beobachtbar |
| scrolllib | – | keine Abweichung (DIFF-frei; clsnotint nach dem Binary) | – | – |
| seriallib ReadSerial (seriallib.c:51, :422) | a3 bleibt auf dem 1-Byte-Puffer rb1 in der Lib; a2 = Unit-Slot | a3/a2 unverändert | **K**/**B** | a3: **beobachtbar**, wenn `ReadSerial` in einem String-Ausdruck steht – der Rest des Ausdrucks wird dann ab rb1 gebaut, zwei `ReadSerial` in einer Anweisung ergeben im Original „97b98“, in C „97 98“ (t64 liest deshalb in getrennten Anweisungen, `compilerlauf/README.md` „Serielle Tests“); a2: Konvention |
| seriallib noser (seriallib.c:201) | „Serial Device Not Open“: `TRAP #0` im normalen Code, kehrt der Trap zurück, läuft der Code in den Meldungstext (gesichertes a6 noch auf dem Stack) | Laufzeitfehler, danach kehrt die Routine ohne Ein-/Ausgabe zurück | **K** | nur bei zurückkehrendem Trap (sonst endet das Programm mit der Meldung) |
| speaklib (speaklib.c:28) | a2 gepoppt | unverändert | **B** | Konvention |
| stringfunclib (stringfunclib.c:24, :634, :730, :815) | a2 je nach Weg unterschiedlich vermindert (String$ ≤ 0, Replace$ leer, Cvi/Cvl/Cvq gar nicht) | immer unverändert | **B** | Konvention |
| utillib CreateMsgPort (utillib.c:30, :153, :165, :193) | nach dem Laufzeitfehler (`TRAP #0`, „Failed to allocate memory/signal for Port“) läuft der Code in den Meldungstext; der NoSig-Zweig fällt zuerst in `Error_NoMemory` (holt a0 und die Register ein zweites Mal vom Stack, zweiter Trap) | Laufzeitfehler, danach Rückkehr mit 0 | **K** | nur bei zurückkehrendem Trap; die NoMem-Zweige erreicht das Original ohnehin nicht (memlib globalloc kehrt bei Speichermangel nicht zurück), den NoSig-Zweig nie (`AllocSignal(-1)` liefert nicht 0) |
| trackerlib (trackerlib.c:47) | – | Interrupt-Routine in C, gleiche Register | – | ausdrücklich „keine Abweichung“ |
| vallib (vallib.c:18) | a2 um 2/4 vermindert | unverändert | **B** | Konvention |
| windowslib err6/err7 (windowslib.c:73) | Sprünge in den Fehlerprüfbereich | Trap im normalen Code | **K** | nur in diesen Fehlerfällen ohne Debugger |
| windowslib Register (windowslib.c:73) | ändert a3 (WBlit, inputstr), d7, a4 (Window); WindowTags mit Tag-Paaren schreibt die Titellänge auf den Argumentstapel zurück | nicht | **K**/**B** | nicht beobachtbar |
| windowslib WindowTags &TagList (windowslib.c:73, :2379) | parkt die Titellänge mit `MOVE.l (a7)+,(a2)+` in den 4 Bytes **hinter dem TAG_DONE-Paar der Tagliste des Benutzers** und liest sie dort wieder | liest die Länge vom Argumentstapel, schreibt nichts in die Tagliste | **K** | nachbildbar; **beobachtbar**, wenn hinter der Tagliste Benutzerdaten liegen (z. B. ein Array-Element: Original = Titellänge, C = unverändert) |
| windowslib Koordinaten (windowslib.c:73) | unbekanntes oberes Wort (zcursor) | vorzeichenerweitert | **T** | Registerrest |
| shapeslib (shapeslib.c) | – | keine Abweichung (err10 springt wie im Original in den Fehlerprüfbereich) | – | – |
| shapetrixlib XFlip (shapetrixlib.c, Kopf) | kein Warten nach dem letzten Blit vor dem CPU-Spiegeln | ebenso nachgebildet; Ergebnis zeitabhängig, C-Schleife ist schneller | **T** | Zeitverhalten; im Test bei 13 und 21 Pixel breiten Shapes anders (21 Pixel nur mit Laufzeit-Fehlerprüfung); t141 prüft XFlip nur ohne Schiebe-Blit |
| spriteslib, collslib | – | keine Abweichung | – | – |
| iffmakelib | – | keine Abweichung (Prüffunktionen seit c4a736d im Prüfbereich, s. Abschnitt 6) | – | – |

## 4. deflibs (AmigaOS-Aufrufdeklarationen)

| Library/Routine (Datei:Zeile) | Original | C-Fassung | Art | Begründung / beobachtbar? |
|---|---|---|---|---|
| init aller Libraries/Resourcen (deflibs/deflib.h:20) | `BEQ init+6` springt in die Mitte des `LEA`: `ORI.b #0,(a0)+` mit beliebigem a0, dann OpenLibrary mit dem a1, das der Fehlschlag hinterlässt | Endlosschleife, jedes Mal mit dem richtigen Namen | **T** | a0/a1 nach einem fehlgeschlagenen OpenLibrary sind undefiniert; beobachtbar nur, wenn die Library fehlt (beide hängen) |
| init/finit allgemein (notes/deflibs.md) | zerstört a6, `JMP CloseLibrary` | GCC sichert a6, `JSR` | **B** (Registererhalt) | nicht beobachtbar |
| Resourcen (deflib.h) | `LEA` zeigt 6 Byte in den Namen | nachgebildet (`libname + DEFLIB_RESOURCE_SKEW`) | – | BUG(orig), keine Abweichung |

## 5. Compiler blitz2-c

Erzeugt bytegleiche Programme (Testkorpus und CD-Referenzlauf), daher nur
interne Abweichungen. Konventionen: `-fbaserel` (Globals a4-relativ,
`GLUE_LOAD_A4` in allen Einsprüngen von außen, README), Assembler nur in
`src/glue*.c`, Register mit dauerhaftem Zustand als Globals
(`srcptr`, `numd3`/`numd6`, `getvnamelen`).

| Stelle (Datei:Zeile) | Original | C-Fassung | Art | beobachtbar? |
|---|---|---|---|---|
| errstack/errcont (asmlibs.c:1427, compile.c:184, data.c:1460, data.c:1830, main.c:1354, parse.c:216, parse.c:305, stmts.c:1189, stmts.c:1634) | Fehlerrücksprung wird nicht zurückgesetzt | `errjmp = olderr` | **T** | Stacktrick (a7/errcont) in C nur mit longjmp; nein |
| Stacktricks `addq #4,a7` / `move.l passstack,a7` (asmlibs.c:403, :467, asmlibs.h:69, compile.c:1108, tokens1.c:783, tokens1.c:855) | Rückkehr am Aufrufer vorbei | Rückgabewert / longjmp | **T** | nein |
| Platzhalter-Longs (compile.c:829, stmts.c:432, tokens1.c:256, :509, :635, :1050, tokens2.c:10) | poken Registerreste als Platzhalter | 0 bzw. der erkennbare Wert | **T** | werden immer überschrieben (außer nomemleft); nein |
| Registerergebnisse (eval.h:236, eval.c:126, libcall.h:19, compile.c:1385, tokens1.c:343, tokens2.c:81, parse.c:1157) | Ergebnisse in d2/d3/d6/CCR, Werte vom Stack des Aufrufers | Globals / Parameter / Rückgabewerte | **T** | nein |
| Konstantenfaltung (eval.h:171, eval.c:823, libcall.c:1182) | führt erzeugten Code mit den Compilerregistern aus | `callregs` mit den relevanten Registern | **T** | nein |
| Bereichs-/Überlaufprüfungen (eval.c:485, data.c:1693, parse.c:351, compile.c:198) | ohne Prüfung (Lesen/Schreiben über Tabellen, DIVU durch 0) | C prüft und tut nichts | **K** | nur in Fällen, die laut Kommentar nicht auftreten |
| pushprec (eval.c:929) | legt Operator/Rang ohne Grenzprüfung ab; „Expression too complex“ erst in `popprec`, nachdem über das Stapelende geschrieben wurde | prüft vor dem Schreiben und meldet sofort `pserr` | **K** | Meldung gleich; anders ist nur, dass das Original vorher über `precstackf` hinaus schreibt (nur bei sehr tief geschachtelten Ausdrücken) |
| savefile HUNK_SYMBOL (parse.c:269) | schreibt die auf Langwörter aufgerundete Namenslänge direkt aus dem Label-Knoten: **liest** bis 3 Byte hinter der Allokation und schreibt sie als Füllbytes ins Executable | Kopie mit Nullen aufgefüllt (Füllbytes 0) | **K** | nur mit „Create Debug Info“: die Füllbytes im HUNK_SYMBOL (Original zufällig, C immer 0); sonst nein |
| libcall.c:256, libcall.c:1435 | makeint/Vergleich mit Resten im oberen Byte | FALSE / definierter Vergleich | **T** | nein |
| parse.c:538, parse.h:75, compile.h:32 | err ohne gesetzten Rücksprung | Meldung + `exit(RETURN_FAIL)` | **T** | nur bei internen Fehlern |
| parse.c:797 | `findlast` (toter Code) | läuft die Liste | **T** | nein |
| stmts.c:1664 | flushc nach dem Register-Restore | davor | **T** | nein |
| Start (main.c:31, :1072, :1096, :1134, :1141) | eigener Workbench-/CLI-Start, öffnet/schließt dos.library | libnix-Start (DOSBase, WBStartup, argv) | **T** | Kommandozeile wird von libnix zerlegt (Anführungszeichen) |
| main.c:1529 | SuperState/movec/UserState | `Supervisor()` | **T** | nein |

Die einzelnen Begründungen stehen jeweils am Kommentar `DIFF:` in
`../blitz2-c/src/`.

## 5a. Editor ted-c (TED 1.1)

Referenz ist der vasm-Build von `ted.s` (`../ted-c/notes/reference.md`).
Eigene Liste der TED-Abweichungen: [`../../ted-c/notes/abweichungen.md`](../../ted-c/notes/abweichungen.md)
(dort laufend ergänzt); hier die Zeilen, die den gemeinsamen
Dateirequester betreffen.

| Stelle (Datei:Zeile) | Original | C-Fassung | Art | beobachtbar? |
|---|---|---|---|---|
| `flink` (ted.s:7953; ted-c/src/filereq.c) | der eingebettete Requester-Hunk `filereq` wird beim Start (aus `outside`) zur Laufzeit selbst reloziert (`add.l basis` auf 312 Langwörter im eigenen Code-Hunk, ohne Cache-Leerung; ein zweiter Lauf würde doppelt relozieren – BUG(orig)) | entfällt: der Requester ist C-Code (`shared/filereq/filereq_core.c`), seine Zeiger reloziert LoadSeg | **T** | technisch unvermeidbar: kein eingebetteter Hunk mehr, nichts zu relozieren. Nein (außer dem Doppel-Relozieren, das im Original nie vorkommt) |
| Requester-Datenblock (ted-c/src/filereq.c mit `shared/filereq/filereq_core.c`) | TEDs Kopie von `filereq` unterscheidet sich von der in freqlib in 7 Bytes (Füllbytes hinter Gadget-Texten, z. B. `"OK",0,$06` statt `$00`, und Rest am Hunkende) | Datenblock der freqlib-Fassung | **T** | eine Quelle für beide Programme (Benutzerentscheidung „Variante C“); die Bytes werden nie gelesen. Nein |

## 6. Fehlerprüfungen außerhalb des Fehlerprüfbereichs

`BLITZ_ERRCHK_THUNK`/`BLITZ_ERRCHK_MESSAGE` schalten im Inline-Assembler
mit `.text` zurück; GCC bemerkt das nicht. Steht eine `BLITZ_ERRCHK`-
C-Funktion hinter einem solchen Thunk, gibt GCC keinen neuen
`.section`-Befehl aus und die Funktion landet im normalen Code
(`.previous`/`.pushsection` kennt der Assembler dieser Toolchain nicht).
Verhalten gleich (die Thunks liegen im Prüfbereich und rufen sie), aber ohne
Debugger wird der Code nicht abgeschnitten. Abhilfe: alle Thunks hinter alle
Prüffunktionen (so in shapeslib, shapetrixlib, spriteslib, collslib).
Geprüft durch Übersetzen aller `libs/*.c` mit `-S` und Auswerten der
wirksamen Section jeder `BLITZ_ERRCHK`-Funktion (Stand c4a736d):
**keine** Prüffunktion liegt im normalen Code. (iffmakelib hatte vier –
`bitchk_c`, `bitchk2_c`, `sschk1_c`, `sschk2_c` –, in c4a736d behoben.)

## 7. Stand

Alle Befehls-Libraries sind portiert (c4a736d); die Tabellen enthalten auch
scrolllib, 2dlib, polylib (S12) und gameiolib, blitzkeyslib, animlib,
blitzcoplib, displaylib (S13).

## Zusammenfassung

* **B – vom Benutzer entschieden:** die a2-Konvention (rund 20 Libraries,
  nie beobachtbar) und der Registererhalt „mindestens wie das Original“
  (zusätzlich erhaltene Register sind nie Ergebnisse).
* **T – technisch unvermeidbar:** undefinierte Registerreste und
  OS-Rückgaben (blitlib BlitColl, freqlib, cliargslib CCR, windowslib
  zcursor, ilbmifflib a0, deflibs-Retry), Speicherlayout bei Überläufen und
  ungültigen Indizes (2dlib, polylib, displaylib, crunchlib), Stackinhalt
  bei Fehlersprüngen per JSR (animlib, displaylib, screenslib) und sonstige Stackfragen
  (crunchlib, arrayslib a4/a5), Zeitverhalten (shapetrixlib XFlip) und im
  Compiler alle Stacktricks und Platzhalter. Beobachtbar höchstens in
  Fehlerfällen, bei Überläufen oder verfälschten Daten.
* **K – stille Korrekturen (Bestand):** Traps im normalen Code statt Sprung
  in den abgeschnittenen Fehlerprüfbereich (inputoutputlib, fileiolib,
  palettelib, windowslib, brexxlib, wbstartuplib), Rückkehr statt Absturz
  bzw. weiterlaufender Fehlerkette oder Lauf in den Meldungstext (arexxlib,
  memlib err1/locfree, runerrlib, fontlib, palettelib, seriallib noser,
  utillib CreateMsgPort), erhaltenes a3 (linklistlib, medlib, seriallib,
  windowslib, blitzoslib), die nicht in die Tagliste geschriebene
  Titellänge (windowslib WindowTags &TagList) und Wortkoordinaten
  (blitzoslib), im Compiler einige Bereichsprüfungen (pushprec, savefile).
  Beobachtbar meist nur in Fehlerfällen ohne Debugger bzw. bei
  zurückkehrendem Trap; sichtbar im normalen Lauf sind seriallib
  ReadSerial in String-Ausdrücken und WindowTags &TagList (Speicher hinter
  der Tagliste).
