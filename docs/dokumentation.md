# Blitz Basic 2.1 – was die CD-Dokumentation über Compiler, Präprozessor und Editor sagt

Stand 26.10.2026. Ausgewertet wurde die lokale Kopie der „Blitz Basic CD“
(Guildhall Leisure / BUI, zusammengestellt von Kevin Winspear) unter
`bb2/cd/`. Die Pfade unten sind relativ zu `bb2/cd/`.

| Bestandteil | Version / Datum | Beleg |
|---|---|---|
| Compiler `blitz2/blitz2` | „Blitz Basic 2 Version 2.10 (C)opyright 1995 Acid Software, Release Version“ | Zeichenkette im Binary |
| Handbuch `blitz2_manual` | Blitz Basic 2.1, Klartext (OCR-Scan) | – |
| Editor `blitz2/ted` | TED 1.1 (laut `bum10b/blitz2/releasenotes`) | – |
| NewTED `extras/newteditor/ted` | V2.1, 20.10.95 | `extras/newteditor/readme.ted` |
| SuperTED `bum10a/ted223/ted`, `bss3/superted` | 2.23 (BUM10, 25.9.96) und 2.5 (23.6.96) | `bss3/superted/documentation/superted.guide` |
| Lib-Entwicklung `extras/libsdev/documentation` | September 1994 (Mark Sibly, Simon Armstrong) | – |
| BUM9 / BUM10 | 13.5.96 / 25.9.96 (Ted Bailey) | `bum9b/bum9docs/bum9_readme`, `bum10b/bum10examples1/bum10docs/bum10_readme` |
| `blitz2.guide` | Bibliotheksbefehle (Jurgen Valks, 1994); kaum Compiler-Inhalte | – |

**Legende**

* **[Doku]** steht so in einem Dokument der CD (Datei und Zeile angegeben).
* **[Bin]** ist eine eigene Auswertung der Binärdateien auf der CD:
  Disassembly mit `m68k-amigaos-objdump -d -m68000 blitz2/blitz2` oder ein
  Hex-Dump. Adressen sind VMA im `.text`-Hunk (Datei-Offset = VMA + `$20`).
  Nichts davon wurde ausgeführt.
* **[Schluss]** ist eine eigene Folgerung und nicht belegt.

**Hinweis zum Handbuch:** `blitz2_manual` ist ein OCR-Text. Der Backquote
`` ` `` erscheint fast immer als `'`, `{`/`}` teils als `(`, `)` oder `t`,
`1` als `I` oder `]`, `0` als `O`. Zitate unten sind stillschweigend korrigiert, wo die
Bedeutung eindeutig ist. Das Handbuch ist inhaltlich die Vorlage der „BPG“,
die die Compiler-Spec zitiert (D-BPG-…).

---

## 1 Makros

### 1.1 Dokumentiert

* **Definition und Aufruf** [Doku] `blitz2_manual` Z. 3280–3283: „A macro is
  defined in a Macro name..End Macro structure. The code between these two
  commands is not compiled but placed in the compiler's memory. When the
  compiler reaches a !macroname it then inserts the code …“ Referenz:
  Z. 6603–6612 („All text following Macro, up until the next End Macro, will be
  included in the macro's contents“).
* **Parameter** [Doku] Z. 3311–3337: Parameter in `{…}`. Ein Backquote vor
  „a digit or letter (1-9, a-z)“ markiert die Einsetzstelle. Ab dem zehnten
  Parameter gilt: `` `a `` ist der zehnte, `` `b `` der elfte usw. Parameter
  „can be any text“. Z. 3340–3352: Klammerfalle bei `!distance{x*10+20,…}`.
  Die Ersetzung ist also rein textuell.
* **`` `0 ``** [Doku] Z. 3359–3375: „returns the number of parameters passed“;
  Beispiel `Vadd` mit `CNIF `0=2 … CELSE … CERR … CEND`.
* **Rekursion** [Doku] Z. 3384–3396: „Macros are recursive and can call
  themselves“. Beispiel `dolist` mit `CSIF "`2">""` und Selbstaufruf; ein
  fehlender Parameter wird also zu leerem Text.
* **Einzeilige Form** [Doku] Z. 3408: `Macro project …:End Macro` (ohne
  Leerzeichen vor `:End Macro`), Aufruf mitten in einem Ausdruck
  (`Circlef !project{x,y,z},3,…`). Z. 4556: `Macro rndpt Rnd(640),Rnd(512):End Macro`.
* **`` `@ ``** ist im Handbuch nur indirekt erwähnt [Doku] Z. 11884:
  „Duplicate Label: … May also occur with macros where a label is not
  preceded by a \@.“ (OCR; gemeint ist vermutlich `` `@ ``).
* **Grenzen und Fehler** [Doku] Anhang 1, Z. 11792–11814:
  * „Macro Buffer Overflow: The Options requester … contains a macro buffer
    size … May also be caused by a recursive macro call which generates
    endless code.“
  * „Macro already Defined: … may have been defined in one of the included
    resident files as well as somewhere in the source code.“
  * „**Can't create Macro inside Macro**: Macro definitions must occur in the
    primary code.“
  * „Macro without End Macro“, „Macro too Big: Macro's are limited to the
    buffer sizes defined in the Options requester.“
  * „**Macros Nested too Deep: Eight levels of macro nesting** is available in
    Blitz 2. Should never happen!!“
  * „Macro not Found: The macro has not been defined previous to the
    !macroname{ } call.“ Makros müssen also **vor** dem Aufruf definiert sein.
* **Makros in Residents** [Doku] Z. 3010–3043 und Z. 887–891: Ein Resident
  enthält NewTypes, Makros und Konstanten (Abschnitt 3.3).
* **Leerzeichen vor `{`** [Doku] `extras/libsdev/documentation` Z. 252–294:
  `!libheader {#acidsoundlib,init,1,0,0}`, `!libs {#acidsoundlib,$1380}`;
  Handbuch Z. 3396: `!dolist {a,b,…}`.
* **Compiler-Optionen** [Bin] Im Optionsfenster des Compilers gibt es unter
  „Debugger:“ die Schalter „Long Arrays“, „**Expand Macros**“ und „**Quoted
  Macros**“ (Zeichenketten in `blitz2/blitz2`). Das Handbuch nennt sie
  nicht. AB3 führt sie in `ptr_xtradata` als „obsolet“ (AmiBlitz3.ab3
  Z. 24937–24941).

### 1.2 Aus dem Binary (Expansion, `blitz2` `$3A3A`–`$3CA2`) [Bin]

* Die Aufrufsuche läuft über die Anweisung bis `:` oder `;` außerhalb von
  Strings und sucht `!`; Token-Wörter (Byte ≥ `$80`) werden als zwei Bytes
  übersprungen. Nach jeder Expansion beginnt die Suche wieder am
  Anweisungsanfang (`$3C9C bra $3A46`). Das entspricht Spec 06 §6.3.1.
* Ersetzung nach `` ` `` (`$3B4C`):
  * `` `{ `` → konstanter Ausdruck bis `}` (`jsr $BF6E`). Also gab es
    `` `{ausdruck} `` schon in 2.10; das Handbuch beschreibt es nicht.
  * `` `@ `` → Expansionszähler dezimal (`jsr $53DC`).
  * Ist (*x* OR `$20`) ≥ `$7A`, kommt ein Fehler (`jmp $85DA`), also ist
    auch **`` `z `` unzulässig**, entgegen „a-z“ im Handbuch.
  * Ziffern und Buchstaben ergeben *v* = 0…34. Für *v* = 0 wird die Anzahl
    **zweistellig** eingesetzt (`divu #10`). Ist *v* größer als die Anzahl,
    wird nichts eingesetzt. Andere Zeichen (Satzzeichen) fallen auf den Zweig
    für *v* = 0, ersetzt wird also die Anzahl.
* Token-Wörter im Rumpf werden als zwei Bytes kopiert. Eine Behandlung von
  `;` (Kopfkommentar `;…:@:`) **gibt es nicht**.
* Für jede Expansion wird ein Puffer in der Größe des Macro-Puffers per
  `AllocMem` angelegt (`$3AF0`–`$3AFC`); bei Überlauf: `$81E6`.

### 1.3 Folgerungen [Schluss]

* Die Quirks I-22 (Satzzeichen nach `` ` `` wirken wie `` `0 ``) und I-25
  (`` `z `` unzulässig) und die zweistellige `` `0 `` gab es schon in 2.10.
  Sie sind keine Regressionen von AB3, sondern alte Eigenheiten.
* Der Kopfkommentar der `Macro`-Zeile (Spec 06 §6.2.4) ist eine spätere
  Erweiterung.
* Verschachtelte Makro-**Definitionen** waren laut Doku verboten. AB3
  speichert sie dagegen als Text und legt sie bei der Expansion an
  (Spec 06 §6.2.3). Ob 2.10 den Fehler tatsächlich meldet, ist nicht
  geprüft; die Fehlermeldung steht im Binary.

---

## 2 Bedingte Kompilierung

### 2.1 Dokumentiert

* [Doku] `blitz2_manual` Z. 3220–3233: „CNIF – compiler on if numeric
  comparison is true … CSIF … CELSE – switch compiler from previous state
  on=>off off=>on, CEND – end of conditional block (restores previous
  state)“, „**CNIF/CEND blocks can be nested.**“
* [Doku] Z. 3235–3238: „the CNIF directive only works with constant
  parameters – for example '5', '#test' – and not with variables.“
* [Doku] Referenz Z. 6566–6590: „CNIF Constant Comparison Constant …
  Comparison should be one of '<', '>', '=', '<>', '<=' or '>='“; „CSIF
  "String" Comparison "String" … Both strings must be quote enclosed literal
  strings“; „CSIF is of most use in macros for checking macro parameters.“
  Die Syntaxhilfe im Binary lautet genauso: `CNIF` / „Constant Comparison
  Constant“, `CSIF` / „"String" Comparison "String"“.
* [Doku] Z. 6592–6602 und Z. 2293–2300: `CERR "text"` erzeugt einen
  Compilerfehler, gedacht für Makros und bedingte Kompilierung.
* [Doku] Fehler (Z. 11778–11783): „CNIF/CSIF without CEND“, „CEND without
  CNIF/CSIF...“.
* Ein Definiertheitstest `@#name` kommt in der Doku **nicht** vor.

### 2.2 Aus dem Binary [Bin]

* `getchar` (`$7A2E`–`$7BD4`) behandelt die Tokens `$801C`–`$801F` direkt:
  `$801C` → `$C47E` (CNIF), `$801D` → `$C492` (CSIF), `$801E` CELSE,
  `$801F` CEND. Die Nummern sind dieselben wie in AB3 (Spec 05 §5.1).
* Ist *compile* falsch, wird jedes Zeichen außer `0` verworfen. Im
  übersprungenen Bereich zählt CNIF/CSIF nur *nest* hoch (`$7B4E`).
* **CELSE** schaltet um, wenn *nest* = 0 ist (`$7B58`–`$7B62`); den Stapel
  prüft es nicht. Mehrfaches CELSE schaltet also mehrfach um (I-28), und ein
  CELSE ohne CNIF schaltet still auf Überspringen (I-65).
* **CEND** stellt den Zustand vom Stapel wieder her; ein leerer Stapel führt
  zum Fehler (`$7B6C`–`$7B9A`).
* **CNIF** (`$C47E`) wertet einen **ganzen konstanten Ausdruck** aus
  (`bsr $BF6A`) und prüft mit `D1=5` auf ≠ 0. Das ist derselbe Mechanismus
  wie in AB3 (`do_cnif`, `D1=5` → `cif_ne`). Danach muss die Anweisung enden
  (`$C50E`–`$C512`).
* **CSIF** (`$C492`) liest String, Operator (`bsr $CCA8`) und String; die
  Strings laufen über `get1bytemain` (`$7C0E`).

### 2.3 Folgerungen [Schluss]

Die Doku vereinfacht: „Constant Comparison Constant“ ist in Wahrheit ein
beliebiger konstanter Ausdruck mit Test auf ≠ 0, wie in AB3. Die
Quirks I-28 und I-65 stammen aus 2.10.

---

## 3 Konstanten, Includes, Residents, Optionen, Lexik

### 3.1 Konstanten

* [Doku] Z. 3168–3200: `#width=320`. Konstanten „can be used in assembler“,
  „can be used with conditional compiling evaluations“, „**can only hold
  integer values**“, „can be altered through the source at compile time but
  NOT at runtime“.
* [Doku] Fehler Z. 11604–11627:
  * „Constant not defined“, „Constant Not Found“.
  * „**Constant already defined: Constants can only be defined once, i.e.
    cannot change their value through the code.**“
  * „Fractions Not allowed in Constants: … they are usually rounded and no
    error is generated.“
  * „Illegal Constant Expression: A constant may only hold whole numbers,
    either a decimal place, text or a variable name has been included“.
  * „Label has been used as a Constant“ (Z. 11877).
* [Doku] Systemkonstanten Z. 1629–1639: `Pi`, `On` = −1, `Off` = 0,
  `True` = −1, `False` = 0.
* [Doku] Z. 3030: In der Resident-Anleitung steht `xconst= 1 0` (OCR, wohl
  `#xconst=10`).

### 3.2 Includes

* [Doku] Z. 6528–6564:
  * „INCLUDE … The file **must be in tokenised form** (ie: saved from the
    Blitz editor) – **ascii files may not be INCLUDE'd**. INCDIR may be used
    to specify a path for Filename. **Filename may be optionally quote
    enclosed to avoid tokenisation problems.**“
  * „XINCLUDE … only ever included once.“
  * „INCBIN … INCDIR may be used …“
  * „INCDIR Pathname … prefixed to any filenames specified by any of
    INCLUDE, XINCLUDE or INCBIN.“
* [Doku] Fehler Z. 11680–11684: „Already Included“, „Can't open Include“,
  „Error Reading File“.
* [Doku] Z. 11531–11533: „Any reference to an include file or a macro could
  mean the error is there and not on the line referenced.“

### 3.3 Resident-Dateien

* [Doku] Z. 887–891 (Menü CREATE RESIDENT): „A resident is a file including
  all constants and macro definitions as well as newtype definitions … a
  dramatic increase in compile speed“.
* [Doku] Z. 3010–3043: Man schreibt ein Programm mit den NEWTYPEs, Makros
  und Konstanten, führt **COMPILE&RUN** aus und wählt dann **CREATE
  RESIDENT**. Der Resident gibt also den Zustand nach dem Übersetzen
  wieder. Eingebunden wird er „by entering the name of the resident file
  into **one of the 'RESIDENT' fields** of the compiler options
  requester“. `AMIGALIBS.RES` enthält „all the structures, constants and
  macros“ des Betriebssystems.
* [Doku] Fehler Z. 11784–11790: „**Clash in Residents: Residents … must not
  include the same Macro and Constant definitions.**“, „Can't Load Resident:
  … Check the pathname.“
* [Doku] `extras/libsdev/documentation` Z. 195–197: „Keep your .res files in
  the root of blitzlibs:, reference all resident files in compiler options
  as blitzlibs:filename.res“.
* [Bin] In den `.xtra`-Dateien gibt es **8 Resident-Felder zu je 68 Bytes**
  ab Offset `$36`; belegt sind Felder bei `$36`, `$7A`, `$BE` in
  `bss3/redmapeditor/examples/source/demo*.bb.xtra`. AB3 sieht dafür
  ebenfalls 544 Bytes vor (`Dc.w $0220 ; 68*8`, AmiBlitz3.ab3 Z. 24960).

### 3.4 Compiler-Optionen und `.xtra`

* [Doku] Z. 928–956: Create Icons, Enable Runtime Errors, Make Smallest Code
  (zwei Durchläufe, „automatically selected when creating executable
  files“), Debug Info, Buffer Sizes, Object Maximums, Resident.
* [Doku] Z. 6614–6622: `Runerrson`/`Runerrsoff` „override the settings in
  Compiler Options“.
* [Doku] `bss3/superted/documentation/superted.guide` Z. 717–725: „'.xtra'
  (since those are settings files for Blitz source code)“. Z. 1045–1053:
  „The .xtra files … are handled by a function inside Blitz 2 itself and
  not SuperTED“; die Zwischendateien sind „only around 1k per document“.
* [Bin] Fenstertexte im Compiler (Reihenfolge wie im Binary):
  „Create Icons for Executable Files“; „Runtime Error Debugger…“,
  „Debugger:“, „Long Arrays:“, „Expand Macros:“, „Quoted Macros:“; „Make
  Smallest Code“; „Create Debug Info …“. Dann die Puffer „Object, Libs,
  System, Data, Macro, String Buffer“ mit den Vorgaben 16384, 16384, 4096,
  4096, 8192, 10240; dann „Object Maximums“, „Resident“. Unter „Runtime
  Error Options“ stehen AUTO RUN, INTERUPT CHECKING, ASSEMBLER CHECKING,
  OVERFLOW CHECKING. Das ist dieselbe Reihenfolge wie `ptr_xtradata` in AB3.
* [Bin] **Aufbau der `.xtra`**, abgeleitet aus Hex-Dumps von
  `bui8/jackexplains/filerequester.bb2.xtra`, `blitz2_pddisk_*/…` und
  `bss3/redmapeditor/…`; die Feldnamen sind **[Schluss]**:

  | Offset | Länge | Inhalt |
  |---|---|---|
  | `$00` | 1 | `$01` (in allen Dateien gesehen; Versions-Byte?) |
  | `$01` | 1 | Create Icons (`$80`/`$00`) |
  | `$02` | 14 × 2 | Schalter (`$0080` = an) |
  | `$1E` | 6 × 4 | Puffer Object, Libs, System, Data, Macro, String (z. B. Macro `$2000` = 8192, String `$2800` = 10240) |
  | `$36` | 8 × 68 | Resident-Namen |
  | `$256` | 2 × 192 | Pfad und Name der ausführbaren Datei (z. B. `df0:caffeinefree`, `CaffeineFree`) |
  | `$3D6` | 128 | CLI-Argument (`viewlibs.bb2.xtra`: `commands`) |
  | `$456` | Rest | Wortpaare (Nummer, Maximum) der Object Maximums; ihre Anzahl erklärt die schwankenden Größen 1054–1250 Bytes |

  Auffällig: Manche Resident-Felder beginnen mit `$00`, gefolgt von
  `litzlibs:…`. Ob das ein abgeschaltetes Feld ist, ist offen.

### 3.5 USEPATH, Direktiven, Kommentare, Namen

* **USEPATH** [Doku] Z. 1784–1822: textuelles Einsetzen vor `\feld`. „UsePath
  is a compiler directive … affects the compiler as it reads through your
  program top to bottom … governed by the closest previous usepath in the
  listing.“ Referenz Z. 6490–6505 (Schreibweise dort `USEPATH`).
* **Direktiven in 2.10** [Bin] Die Syntaxhilfe der Systemlib im Binary nennt:
  NEWTYPE, End, Let, Dim, Goto, Gosub, Return, Statement, Function,
  MouseWait, If, While, **Macro**, Select, Case, Default, AsmExit,
  **XINCLUDE**, **INCLUDE**, (Platzhalter „Jimi“), SHARED, For, Next, To,
  Step, Data, Read, **CNIF**, **CSIF**, **CELSE**, **CEND**, **CERR**, Then,
  **Else**, Even4, Even8, Dc, Ds, Even, Dcb, GetReg, IncBin, …, DEFTYPE,
  BLITZ, AMIGA, QAMIGA, …, die 68000-Mnemonics **nur in Großschreibung**,
  dann INCDIR, WBStartup, USEPATH, CloseEd, NoCli, Runerrson, Runerrsoff.
  Nach dieser Reihenfolge ist `Macro` = `$800D`, `XINCLUDE` = `$8012`,
  `INCLUDE` = `$8013`, CNIF…CEND = `$801C`–`$801F`, `Else` = `$8022`, wie
  heute. **Es fehlen** `OPTIMIZE`, `SYNTAX`, `USEPATH2`, `USELASTPATH` und
  die kleingeschriebenen PPC-/FPU-Mnemonics.
* **Kommentare** [Doku] Z. 3618–3623: „Blitz2 uses the semicolon character.
  Any text after a semicolon on a line will be ignored by the Blitz2
  compiler.“ Ebenso Z. 3959–3970. [Bin] `getchar` (`$7BB4`–`$7BC4`) liest
  nach `;` außerhalb eines Strings bis zum Zeilenende; eine Prüfung auf
  `:@` gibt es nicht.
* **Namen** [Doku] Z. 3553–3568: „names can be of any length; they must
  start with a letter … or an underscore; must only contain alphanumeric
  chars and underscores; **must not start with the same letters as any
  Blitz2 command**. Also, label and variable names in Blitz2 are always
  treated as **case-sensitive**“.
* **Zeilenfortsetzung `@@`, Escape-Sequenzen** sind im Handbuch **nicht**
  dokumentiert. Strings haben keine Escape-Möglichkeit; dokumentiert ist
  nur „No Terminating Quote“ (Z. 11636).

### 3.6 Das CMake-Zeichen `~`

* [Doku] Z. 3424–3450: „A special character known as the cmake character
  can be used to evaluate constant expressions and insert the literal
  result into your code.“ Beispiel `NPrint var~`1~` mit `!Ivar{2+1}` →
  `var3`.
* [Bin] `getchar` vergleicht mit **`~` = `$7E`** (`$7A7A cmpw #126`), nicht
  mit `$7F`. Es wertet einen konstanten Ausdruck aus (`$7ACA bsr $BF6E`),
  erwartet ein schließendes `~` (`$7ACE`, sonst Syntaxfehler) und schreibt
  den Wert dezimal in einen Puffer. Das Pufferende ist `$01`; beim Lesen von
  `$01` springt es an die gesicherte Position zurück (`$7A94`). `$02` ist
  das Ende einer USEPATH-Einfügung (`$7A82`). Ein `~` innerhalb einer
  Einfügung beendet sie (Flag `$79AC`).
* [Schluss] Die Prüfung liegt in `getchar` vor jeder String-Unterscheidung,
  und auch String-Literale werden über `getchar` gelesen (§4.3). Ein `~` war
  damit vermutlich **überall** aktiv, auch in Strings und Namen. Das ist
  nicht geprüft.

---

## 4 Editor TED und Dateiformat

### 4.1 Tokenisierung

* [Doku] `superted.guide` Z. 813–824: „Blitz 2 treats keywords as 'Token's.
  These are special characters which are recognized as separate from
  standard ASCII characters. The Tokenise option allows you to switch off
  tokenisation … when importing a large ASCII file … With this option
  ticked, Blitz commands will look like and are treated the same as
  standard non-Blitz words.“
* [Doku] `extras/newteditor/readme.ted` Z. 22, 47: NewTED 2.1 kann „Save
  entire file as ASCII“, die Option „Tokenise – Tokenise into Blitz
  commands“ und eine eigene Farbe für Tokens. [Bin] TED 1.1 hat schon
  „Tokenise“, „Token Colour:“ und „Token Unavailable“.
* [Doku] `internet/frac/read.me` Z. 1–45: Der Autor schrieb den Readme mit
  eingeschalteter Tokenisierung: „The capitalisation is a bit strange in this
  file 'cos I forgot to turn off Tokenising“. Im Text stehen mitten im Satz
  `On`, `To`, `AND`, `Use`, `Free`, `Point`, `Used`, `Line`, `If`, `For`.
  [Schluss] TED erkannte Schlüsselwörter **ohne Rücksicht auf
  Groß-/Kleinschreibung**, auch in reinem Text, und speicherte sie als
  Token. Angezeigt wurden sie in der Schreibweise der Token-Tabelle.
* [Doku] `blitz2_manual` Z. 6535 u. a.: Dateinamen „may be optionally quote
  enclosed to avoid tokenisation problems“. [Schluss] Ein Dateiname ohne
  Anführungszeichen konnte also Tokens enthalten.
* [Doku] `blitz2_manual` Z. 573: „Any file ending with the suffix .bb2 is a
  source file“. [Bin] Alle TED-Versionen enthalten `.xtra` und `.bak`;
  NewTED und SuperTED filtern mit `(#?.bb|#?.bb2)`.

### 4.2 Zeilen

* [Doku] `bss3/superted/extras/autodocs/ted.doc` (`TEDDetoke`,
  `TEDGetLabelList`): „SuperTED's line structure is as follows: *PrevLine,
  *NextLine, **linelength.b**, <actual tokenised text>“. Eine Zeile ist
  also im Speicher höchstens 255 Bytes lang (tokenisiert).
* [Doku] ARexx `GETLINE`/`GETWORD` liefern „detokenised“ Text
  (`superted.guide` Z. 334–335, 506–507).

### 4.3 Umlaute

* [Doku] `superted.guide` Z. 1007–1019 („Foreign Users“): „any 'umlauts' …
  will not be accepted by SuperTED. This is due to the fact that **all
  characters with ASCII values of 128 or higher are treated as tokens**
  (Blitz commands) … A few of the more common characters are handled“.
* [Bin] **Umlaut-Tabelle in allen drei TED-Binaries**: `blitz2/ted` @
  `$5DCE`, `extras/newteditor/ted` @ `$77A8`, `bum10a/ted223/ted` @ `$7E34`,
  jeweils `E4 1A F6 1B FC 1C C4 1D D6 1E DC 1F 00`. Das heißt ä→`$1A`,
  ö→`$1B`, ü→`$1C`, Ä→`$1D`, Ö→`$1E`, Ü→`$1F`. Die Tabelle endet mit `00`:
  **ß (`$19`) und © (`$18`) fehlen** in diesen Versionen; die kommen erst in
  `ted2.bb2` dazu (bb2-corpus.md §3.2).
* [Bin] **Gegenrichtung im Compiler 2.10**: `blitz2/blitz2` hat bei VMA
  `$7BEE` die Tabelle `gs` mit 26 × `$00`, dann **`E4 F6 FC C4 D6 DC`**. Das
  heißt `$1A`→ä, `$1B`→ö, `$1C`→ü, `$1D`→Ä, `$1E`→Ö, `$1F`→Ü – **dieselbe
  Reihenfolge wie TED**. Benutzt wird sie nur in `get1bytemain`
  (`$7C0E`–`$7C28`): Ist man in einem String und das Zeichen < `$20`, wird
  es über die Tabelle abgebildet; `$00`–`$19` werden zu 0.
* [Bin] **String-Literale laufen in 2.10 über diese Abbildung**: Die
  Literal-Routine (`$ADC0`–`$ADF2`) liest jedes Zeichen mit `bsr $7C0E` und
  schreibt es mit `bsr $7428` in den Code. Bei 0 springt sie zu `$8224`
  (vermutlich „No Terminating Quote“).
* [Schluss] Blitz 2.10 gab die Umlaut-Bytes alter Quellen also als Latin-1
  ä ö ü Ä Ö Ü aus. Andere Steuerzeichen `$01`–`$19` beendeten einen String
  wie das Zeilenende. AB3 hat die Tabelle mit **vertauschten** ä/ö und Ä/Ö
  (I-73), und `litstring` benutzt sie gar nicht (I-72). Beides sind spätere
  Änderungen, nicht das Verhalten von Blitz 2.1.

### 4.4 Unbekannte Tokens

* [Doku] `blitz2_manual` Z. 11672–11674: „Token Not Found: 'token number':
  When loading source, **Blitz 2 replaces any unfound tokens with ?????**,
  compiling your code with these unknown tokens present will generate the
  above error.“ Ebenso `bum9a/blitz2/releasenotes`: „Blitz will report the
  error 'Token Not Found' and the command will appear as ????? instead of
  text“.
* [Bin] Fehlertexte „Token Not Found : $ffff.“ und „Library not Found :
  $ffff.“

---

## 5 Bibliothekssystem

* [Doku] `blitz2/releasenotes` Abschnitt 4: In 2.1 wird der Befehlssatz aus
  drei Quellen geladen. „acidlibs – a single file containing standard Blitz
  commands; deflibs – commands compiled from third party libs found in
  blitzlibs:; userlibs – all libs found in directories branching off
  blitz2:userlibs“. Neu in 2.1 ist „restructured library handling“. [Bin]
  Im Binary stehen `blitz2:acidlibs`, `blitz2:deflibs`, `blitz2:userlibs/`.
* [Doku] `extras/libsdev/documentation` Z. 43–69 und 366–396 (Stand 1994,
  vor 2.1): Es wird zuerst `Blitz2:Deflibs` geladen, sonst alle
  Unterverzeichnisse von `Blitzlibs:` (System, Amigalibs, Basic, Userlibs).
  „Deflibs is simply all the separate Blitzlibs: libraries joined together
  as one continuous file.“ `Makedeflibs` erzeugt sie; „Reload All Libs“
  (Menü, Handbuch Z. 911–914) lädt neu.
* **Lib-Nummern** [Doku] `extras/libsdev/documentation`:
  * „Each library in Blitz 2 has it's own unique identifying number“
    (Z. 406).
  * Entwicklungs-Libs 10…50 (Z. 411).
  * BUM7-Notiz Z. 118–124: „Do NOT use library numbers above 40 … we left
    the odd numbers for our own expansion“; vergeben wurden u. a. Elmore 99,
    97, 95, RI 63, 58, 51, Neil 50, 49, 48.
  * Amigalibs über `fdconvert`: „first available should be #217“ (Z. 85–88);
    Befehle bekommen ein angehängtes `_` (Z. 92–94).
* **Token-Nummer** [Doku] `blitz2_manual` Z. 11663–11666: „token number =
  libnumber*128 + token offset“ (bei `TokeJsr`). Z. 6729–6730: „A full list
  of all token numbers … will be available shortly“.
* **Folgen von Änderungen** [Doku]:
  * `blitz2.guide` Z. 3263–3265 (CIA-Tracker): Befehle „are only included
    because they served a purpose in debugging a long time ago, and **to
    remove them would cause problems with the Blitz tokens**“.
  * `extras/libsdev/userlibdocs/xbonesciatrklib.doc` Z. 445 u. a.:
    „ModuleToChip still tokenises, but performs no function“.
  * `bum10b/bum10examples1/bum10docs/xbciatrackerlib.doc` Z. 102–103:
    „WARNING! Some of the tokens have changed … you MUST save off any source
    that uses this library in ASCII format.“
  * `internet/tritonblitz/readme`: „save all your sources as ASCII. The new
    libraries have probably other IDs … sonst werden die Tokens nach der
    Neuinstallation … nicht mehr erkannt.“
  * `bum9a/bum9examples2/deflibman/libman.doc`: LibMan zeigt
    Nummernkollisionen („Clashes (i.e. double libnums)“) und lässt sie mit
    „Auto“ weg.
* **Blick nach vorn** [Doku] `extras/libsdev/documentation` Z. 112–116 (Mark
  Sibly): „we are probably going to get rid of tokenising in Blitz3 so that
  1. we don't get all the clashing that has been going on and 2. users can
  use what ever ascii editor they please.“
* [Schluss] Eine automatische Umsetzung alter Token-Nummern (wie
  `replacedat` in AB3, I-33) ist für 2.1 nirgends erwähnt. Der empfohlene
  Weg war, die Quelle als ASCII zu speichern und neu zu tokenisieren. Die
  Tabellen in `bb2/tokens/` (bereits vorhanden) zeigen die Nummern von 2.1,
  z. B. die String-Befehle in Lib 185 und `Edit$`/`Inkey$` in Lib 186.

---

## 6 Versionsgeschichte und Unterschiede zu späteren Versionen

* **2.1 gegenüber früher** [Doku] `blitz2/releasenotes`: „online help,
  restructured library handling, bug free parameter passing, comprehensive
  AGA and 3.0 support“. Früher gab die Help-Taste nur die Syntax in der
  Titelzeile aus.
* **BUM9 (13.5.96)** [Doku] `bum9a/blitz2/releasenotes`: neue Debugger,
  NewTED 2.1 (ab WB 2.0), neue Fremd-Libs; der Compiler bleibt gleich.
  `MakeDefLibs.bb2` und LibMan erzeugen kleine Deflibs.
* **BUM10 (25.9.96)** [Doku] `bum10b/blitz2/releasenotes`: „Blitz2 compiler
  (unchanged from BUM9)“, „acidlibs (unchanged from BUM9)“, SuperTED 2.23;
  neue Libs in `userlibs/beta`. `bum10_readme`: „it will probably be the
  last one ever“.
* **SuperTED 2.5** (Red When Excited, 1996): ARexx-Port `TED_REXX1…9`,
  mehrere Dokumente, `.xtra` über Temporärdateien in `T:`.
* **Unterschiede zu AmiBlitz 2/3**, soweit aus der CD erkennbar
  [Bin/Schluss]:
  * Es gab kein `OPTIMIZE`, kein `SYNTAX` (New-Syntax-Modus), keine
    Escape-Sequenzen `\\…`, keine Zeichenliterale `@"…"`, kein `:@`, keinen
    Kopfkommentar für Makros, kein `USEPATH2`/`USELASTPATH` und keine
    kleingeschriebenen Mnemonics.
  * Das CMake-Zeichen war `~` (`$7E`), nicht `$7F` (Spec I-05).
  * Die Umlaut-Tabelle `gs` war richtig herum, und String-Literale benutzten
    sie (Spec I-72, I-73).
  * Der Macro-Puffer war 8192 Bytes groß (AB3: 40 000).
  * Der Editor tokenisierte ohne Rücksicht auf Groß-/Kleinschreibung (AB3:
    case-sensitiv, Spec 03 §3.1).

---

## 7 Abgleich mit der Compiler-Spec

Bewertung: **bestätigt** = BB2.1 wie Spec; **widerspricht** = BB2.1-Doku
oder -Binary anders; **beantwortet** = klärt einen offenen Punkt;
**Herkunft** = zeigt, ob ein Quirk alt oder neu ist.

| Spec-Stelle | BB2.1-Doku / Binary sagt | Bewertung |
|---|---|---|
| 04 §4.1, I-73 („`gs` … Altlast aus BlitzBasic 2“, „schon in Blitz 2, `amiblitz2.bb2`“) | 2.10-Binary: `gs` = `E4 F6 FC C4 D6 DC` (`$1A`=ä, `$1B`=ö …), gleich wie TED 1.1/2.1/2.23 [Bin] | **widerspricht**: In 2.10 waren Compiler und Editor einig. Die Vertauschung stammt aus einer späteren Quelle (`amiblitz2.bb2`); I-73 ist eine Regression |
| 04 §4.1, I-72 (`litstring` bildet `$1A`–`$1F` nicht ab) | 2.10 liest Literale über `get1bytemain` und damit über `gs` (`$ADC0`) [Bin] | **beantwortet** die offene Frage aus bb2-format.md §5 und bb2-corpus.md §3.3: Blitz 2.1 gab ä ö ü Ä Ö Ü aus. I-72 ist eine Regression |
| 04 §4.1 (`$00`–`$19` → 0 in Strings) | dasselbe in 2.10 [Bin] | bestätigt; in 2.10 betraf das auch Literale (Steuerzeichen beendeten den String, [Schluss]) |
| 04 §4.3 `:@` | 2.10-`getchar` überspringt nach `;` bis Zeilenende, ohne `:@`; die Doku kennt nur `;` [Bin, Doku Z. 3618] | Herkunft: `:@` ist eine spätere Erweiterung |
| 04 §4.5.2 Escapes | keine Erwähnung; nur „No Terminating Quote“ [Doku] | Herkunft: spätere Erweiterung (New-Syntax) |
| 04 §4.6 `True`/`On`/`False`/`Off`/`Pi` | Z. 1629–1639 [Doku] | bestätigt |
| 04 §4.7, B4 (Namensregeln, „darf nicht mit Befehlsnamen beginnen“) | wörtlich so in Z. 3553–3568; Namen case-sensitiv [Doku] | bestätigt die Herkunft von B4. Vermutlich eine Folge des TED-Tokenizers (§4.1), [Schluss] |
| 08 B1, I-38 (`UsePath`, `DefType`, `Nprint` in der Doku) | TED erkannte Schlüsselwörter unabhängig von der Schreibweise (frac/read.me) [Doku-indirekt, Schluss] | **erklärt** B1: In BB2 war die Schreibweise in der Doku egal, weil TED normalisierte. Erst der case-sensitive ASCII-Tokenizer macht diese Beispiele kaputt |
| 04 §4.9 USEPATH | textuell, „top to bottom“, nächstes vorheriges USEPATH gilt; Ende-Marke `$02` [Doku, Bin] | bestätigt; `USEPATH2`/`USELASTPATH` gab es in 2.10 nicht |
| 04 §4.10, I-05 (`~…~` mit `$7F`, „vermutlich nur aus alten `.bb2`“) | Doku Z. 3424–3450 mit `~`; Binary vergleicht mit `$7E` [Doku, Bin] | **beantwortet**: Das Zeichen war `~` selbst, auch in `.bb2`. Mit `$7F` ist es auch für alte `.bb2` unerreichbar. I-05 ist eine Regression |
| 04 §4.11 `@@` | nicht dokumentiert | offen; vermutlich spätere Erweiterung |
| 05 §5.1 Tokens `$801C`–`$801F` | gleiche Nummern, Verarbeitung in `getchar` [Bin] | bestätigt |
| 05 §5.3 (CNIF = Ausdruck ≠ 0) | Doku: „Constant Comparison Constant“ mit `< > = <> <= >=`; Binary: Ausdruck + `D1=5` (≠ 0) wie AB3 | Doku vereinfacht, Verhalten bestätigt |
| 05 §5.3 `@#name` | nicht dokumentiert | Herkunft: vermutlich später ([Schluss], Binary nicht geprüft) |
| 05 §5.4 CSIF-Operatoren (auch `=<`, `=>`, `><`) | Doku nennt nur `< > = <> <= >=` | Doku unvollständig (wie AB3-Doku) |
| 05 §5.2, I-28 (mehrfaches CELSE) | Doku: „toggle“; Binary schaltet bei *nest* = 0 jedes Mal um | Herkunft: Quirk aus 2.10 |
| 05 §5.6, I-65 (CELSE ohne CNIF still) | Binary prüft nur *nest*, nicht den Stapel | Herkunft: aus 2.10 |
| 05 §5.6, I-68 (Neudefinition von Konstanten ohne Meldung) | „Constant already defined: Constants can only be defined once“ (Z. 11612) [Doku]; Text auch im Binary | **widerspricht** der Doku von 2.1; ob 2.10 den Fehler tatsächlich meldete, ist offen |
| 05 §5.6 (Brüche → „Illegal constant expression“) | „Illegal Constant Expression: … a decimal place, text or a variable name“; aber auch „Fractions Not allowed … usually rounded and no error“ | teils bestätigt; die Doku widerspricht sich selbst |
| 06 Einleitung, §6.1.1 (Resident: Typen, Makros, Konstanten) | „contains a pre-compiled list of NewTypes, macros and constants“; Erzeugung nach COMPILE&RUN [Doku] | bestätigt |
| 06 §6.1.1 („Clash in Residents“ prüft nur Typen/Makros, nicht Konstanten) | „must not include the same Macro and Constant definitions“ [Doku] | **widerspricht** der 2.1-Doku für Konstanten |
| 06 §6.1.1, I-75 (höchstens 7 Residents) | „one of the 'RESIDENT' fields“; `.xtra` hat 8 Felder zu 68 Bytes [Doku, Bin] | Hinweis: BB2 hatte 8 Plätze; die Grenze 7 in AB3 liegt im Text-Header (`ParseAB3Header`) |
| 06 §6.1 Tabelle (`` `1 ``…`` `9 ``, `` `a ``…`` `y ``) und I-25/B3 (`` `z ``) | Doku: „1-9, a-z“; Binary: `` `z `` → Fehler wie AB3 | Doku-Fehler schon in 2.1; Verhalten seit 2.10 gleich |
| 06 §6.1 `` `0 `` zweistellig | Doku: „number of parameters“; Binary: `divu #10`, zwei Ziffern | bestätigt (Doku ungenau) |
| 06 §6.1 `` `@ `` | nur im Fehlertext (Z. 11884, OCR „\@“); Binary unterstützt es | bestätigt |
| 06 §6.1 `` `{ausdruck} `` | nicht dokumentiert; Binary unterstützt es | bestätigt; seit 2.10 vorhanden |
| 06 §6.2.3 (verschachtelte Definition als Text, wird bei Expansion angelegt) | „Can't create Macro inside Macro: Macro definitions must occur in the primary code“ [Doku] | **widerspricht**: In 2.1 war das ein Fehler. Ob AB3 die Meldung noch erzeugt, ist offen |
| 06 §6.2.3 Einzeilige Form (I-16) | Beispiele nur ohne Leerzeichen vor `:End Macro` (Z. 3408, 4556) | bestätigt die Spec: Die Doku-Form funktioniert; I-16 betrifft nur `… : End Macro` |
| 06 §6.2.3 Größe (40 000 Bytes) | Puffer im Optionsfenster, Vorgabe 8192 [Bin]; „Macro too Big“, „Macro Buffer Overflow“ [Doku] | Grenze anders, Mechanismus gleich |
| 06 §6.2.4 Kopfkommentar | keine Behandlung von `;` in der 2.10-Expansion [Bin] | Herkunft: später |
| 06 §6.3.1 (Suche bis `:`/`;`, Neustart am Anweisungsanfang) | gleich im Binary (`$3A46`–`$3A90`, `$3C9C`) | bestätigt |
| 06 §6.3.3 klassischer Modus, Leerzeichen vor `{` | `!dolist {…}`, `!libheader {…}` in Doku und Libs-Quellen | bestätigt |
| 06 §6.4 (v > n → leer) | `dolist`-Beispiel mit `CSIF "`2">""` setzt das voraus | bestätigt |
| 06 §6.4 Grenzen (keine Tiefe, nur Pufferüberlauf) | „Macros Nested too Deep: Eight levels“ [Doku] | **widerspricht**/offen: In 2.1 gab es eine Tiefengrenze 8 (wofür genau, ist offen) |
| 06 §6.5 (`[`/`]` nicht gezählt, I-24) | keine Aussage | offen |
| 08 A5 / I-05 (CMake) | siehe 04 §4.10 | Regression bestätigt |
| 08 B3 | siehe `` `z `` | Doku-Fehler stammt aus der BB2-Doku |
| 11 I-03 (Zeilen > 250 Bytes) | SuperTED: `linelength.b` (≤ 255) [Doku] | Herkunft: Der Editor ließ längere Zeilen nicht zu; die 250-Grenze passt dazu ([Schluss]) |
| 11 I-33 (`replacedat`) | keine automatische Umsetzung; Empfehlung „save as ASCII“ [Doku] | Herkunft: später; alte Quellen ohne passende Libs zeigen `?????` |
| 11 I-52 (Libs > 127 Einträge) | „token number = libnumber*128 + token offset“ [Doku] | bestätigt die Rechnung |
| 11 I-54 (`$8012` = XINCLUDE) | Reihenfolge in der Systemlib: …, XINCLUDE, INCLUDE … [Bin] | bestätigt |
| 11 I-71 (Schlüsselwort als Text in `.bb2`) | Tokenisierung in TED abschaltbar; „Save as ASCII“ [Doku] | erklärt, wie solche Dateien entstehen konnten |

---

## 8 Antworten auf offene Punkte in `ab3c/docs/bb2-converter`

| Offener Punkt | Antwort aus der CD | Art |
|---|---|---|
| Was macht Blitz 2 mit `$1A`–`$1F` in String-Literalen? (README, corpus §3.3/§8.1) | Der Compiler 2.10 bildet sie beim Übersetzen auf Latin-1 ab: `$1A` ä, `$1B` ö, `$1C` ü, `$1D` Ä, `$1E` Ö, `$1F` Ü. `$00`–`$19` werden zu 0, das wirkt wie das Zeilenende | [Bin]; eine Amiberry-Probe steht noch aus |
| TED-Tabelle vs. `gs` | In 2.10 stimmen beide überein (TED-Reihenfolge). Die Vertauschung in `gs` ist jünger | [Bin] |
| `$19` ß, `$18` © | Fehlen in TED 1.1, NewTED 2.1, SuperTED 2.23 und in `gs` von 2.10; erst in `ted2.bb2`. Der Compiler 2.10 macht aus `$18`/`$19` im String 0 | [Bin], Folge [Schluss] |
| Bytes ≥ `$80` in Strings | SuperTED: „all characters with ASCII values of 128 or higher are treated as tokens“; `getchar` 2.10 setzt ≥ `$80` immer zu einem Wort zusammen, auch im String. Latin-1 in Strings ist also keine Blitz-2.1-Form | [Doku], [Bin] |
| Schlüsselwörter als Text (`Macro`, `End NEWTYPE`, `XINCLUDE`) | TED konnte die Tokenisierung abschalten; NewTED speichert „entire file as ASCII“. Dateien mit Text-Schlüsselwörtern entstanden so oder mit Fremdeditoren. Die Doku verlangt für INCLUDE „tokenised form – ascii files may not be INCLUDE'd“ | [Doku], [Schluss] |
| `.xtra`-Layout (README Nr. 8) | Aufbau siehe §3.4 (Offsets aus Hex-Dumps, passt zur Reihenfolge von `ptr_xtradata`). Abweichung: BB2 hat 14 Schalterwörter, AB3 listet 15 | [Bin]/[Schluss] |
| Kopf `BB22` | Auf der CD nicht erklärt; keine Datei mit diesem Kopf (Treffer nur zufällig in Binärdaten) | offen |
| Alte Token-Nummern | Doku: Nummer = Lib × 128 + Index; nicht gefundene Tokens erscheinen als `?????` und ergeben „Token Not Found“. Lib-Wechsel: Quelle vorher als ASCII sichern. Nummern von 2.1: `bb2/tokens/acidlibs.txt` | [Doku] |
| `~…~` „kommt im Korpus nicht vor“ (corpus §2) | Gesucht wurde `$7F`; in Blitz 2 ist es aber `~` (`$7E`). Der Korpus muss nach `~` in Makro-Rümpfen durchsucht werden | [Bin] → neue Suche nötig |

---

## 9 Offene Punkte

1. **Amiberry-Probe mit Blitz 2.1:** Bestätigen, dass `NPrint "a␚␛␜␝␞␟b"`
   die Bytes `61 E4 F6 FC C4 D6 DC 62` ausgibt und dass ein Steuerzeichen
   `$01`–`$19` im String einen Fehler ergibt. Bisher ist das nur aus dem
   Disassembly erschlossen.
2. **`~` in Strings und Namen in 2.10:** Wird `"a~b"` als CMake-Ausdruck
   behandelt? Folgt aus dem Code, ist aber nicht geprüft.
3. **„Constant already defined“ und „Can't create Macro inside Macro“:**
   Werden diese Fehler in 2.10 tatsächlich gemeldet? Die Texte stehen im
   Binary; der Aufrufer ist nicht gesucht.
4. **„Macros Nested too Deep“ (8 Ebenen):** Worauf bezieht sich die Grenze?
   Die Expansion arbeitet iterativ; vermutlich auf Makros in
   Include-/Expansionsstapeln. Nicht untersucht.
5. **`.xtra`:** Bedeutung der 14 Schalterwörter im Einzelnen; das Byte
   `$01` an `$00`; Resident-Felder mit `$00` am Anfang; Bedeutung der
   Wortpaare ab `$456` (Lib-Nummer oder Objekt-Typ?). Unterschied 14 ↔ 15
   Wörter zu AB3 klären, bevor AB3/ab3c eine BB2-`.xtra` liest.
6. **TED-Tokenizer:** Tokenisierte TED auch Präfixe (Grund für „must not
   start with the same letters as any Blitz2 command“) und auch in
   Kommentaren und Strings? frac/read.me deutet auf Erkennung ohne
   Rücksicht auf Groß-/Kleinschreibung hin. Prüfbar mit `blitz2/ted` in
   Amiberry.
7. **`@@`, `@#name`:** In 2.10 vorhanden? (`cmpw #64` bei `$983A`, `$D670`
   nicht untersucht.)
8. **Kopf `BB22`:** Auf der CD nicht erklärt.
