# libs-c – Blitz-Basic-2-Libraries in C

Die Libraries von Blitz Basic 2 (System-Libs wie `ldivlib`, später die
Befehls-Libs) als lesbare C-Quelltexte, aus denen sich **funktionsgleiche
Library-Dateien im BB2-Format** bauen lassen. Grundlage sind die
BB2-Originale in `../quelltext/amibl2src/files/amiblitz2source/bblibsrc.lha.dir/bblibsrc/`.

Zwei Wege (siehe Absprache):

* **Original-Libraries** (`../cd/blitz2/deflibs`, `acidlibs`): damit muss ein
  Compiler bytegleiche Programme erzeugen. Hier wird nichts nachgebaut.
* **C-Libraries** (dieses Verzeichnis): gleiches Format, gleiche Token,
  gleiche Aufrufkonvention, **funktional gleich** – geprüft im Emulator
  gegen das Original. Der Maschinencode ist ein anderer (GCC statt
  Handassembler), daher sind damit erzeugte Programme nicht bytegleich.

## Bauen und prüfen

```
cmake --preset amiga && cmake --build --preset amiga
#  -> build/amiga/libNAME.a   Static Library (Grundlage, später direkt mit ld nutzbar)
#  -> build/amiga/NAME.obj    Library-Datei im BB2-Format
python3 -I tools/biglib.py extract ../cd/blitz2/acidlibs 64735 ref/ldivlib.obj
python3 -I tools/biglib.py compare ref/ldivlib.obj build/amiga/ldivlib.obj
python3 -I tools/amiga_test.py "[libtest]"         # Amiberry (snitch-Filter), s. u.
```

## Wie eine Library in C aussieht

Eine BB2-Library war ein Blitz-Programm: Die Makros aus `libmacs.res`
(`../cd/extras/blitzlibs/libmacs.bb`) erzeugten mit `Dc` Kopf und
Token-Tabelle vor dem Assembler-Code. `include/blitzlib.h` bildet jedes
dieser Makros nach und erzeugt **dieselben Bytes** – als Daten in der
Section `.blitzlib.head`, die `ld/blitzlib.ld` an Offset 0 des einzigen
Code-Hunks legt:

```c
/* !syslibheader{#ldivlib,$cb00,$cb02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_ldivlib, 0xcb00, 0xcb02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qdivide, 0, 0);           /* $CB00 quickdiv */
BLITZ_NULLSUB(ldivide, 0, 0);           /* $CB01 longdiv */
BLITZ_LIBFIN();

LONG ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1)) { ... }
```

* **Register:** Argumente als GCC-Registerparameter (`BL_REG(d0)`),
  Ergebnis in d0. a5 (Globals) und a4 (Locals) sind feste Register
  (`-ffixed-a4 -ffixed-a5`) und in C als `blitz_globals`/`blitz_locals`
  (globale Registervariablen) erreichbar.
* **Prüfroutinen** (nur mit Debugger): `BLITZ_ERRCHK` legt sie hinter
  `BLITZ_ERROR_AREA`. Verweise aus dem normalen Code dorthin zeigen ohne
  Debugger hinter das Code-Ende (die Originale tun das teils auch, z. B.
  memlib err1 → mess1); bis 27.10.2026 warnte `mkblitzlib.py` davor, seitdem
  prüft das niemand mehr automatisch.
* **Reihenfolge im Fehlerprüfbereich:** Das GNU as dieser Toolchain kennt
  kein `.pushsection`/`.popsection`. `BLITZ_ERRCHK_THUNK` (ebenso
  `BLITZ_ERRCHK_MESSAGE` und `BLITZ_ERRCHK_AB`) schaltet deshalb mit
  `.section .blitzlib.errchk` um und am Ende fest auf `.text` zurück. GCC
  merkt davon nichts: Steht so ein Makro zwischen zwei `BLITZ_ERRCHK`-
  C-Funktionen, glaubt GCC sich noch in `.blitzlib.errchk`, gibt keinen
  Section-Wechsel aus, und die zweite Prüffunktion landet im normalen Code.
  Darum alle `BLITZ_ERRCHK_THUNK`s hinter die `BLITZ_ERRCHK`-C-Funktionen
  stellen (Meldungen davor oder dahinter, nicht dazwischen).
* **Treue:** Algorithmen werden bitgenau übernommen (z. B. Division durch 0,
  Überlauf), nicht durch C-Operatoren ersetzt.

Weitere Makros in `include/blitzlib.h` (Einzelheiten in den Kommentaren dort):

| Makro | Zweck |
|---|---|
| `BLITZ_THUNK`, `_Z`, `_PRE`, `_POST`, `_POP` | Einsprung, der Register sichert / CCR setzt / Stack-Argumente abräumt, ruft eine C-Funktion |
| `BLITZ_ALIBJSR(tok, r)`, `_FORM`, `BLITZ_BLIBJSR` | Routine einer anderen Library aufrufen (d0/d1/a0/a1 über `struct bl_regs`) |
| `BLITZ_ALIBJSR_A6(tok, base, x)` | dasselbe mit geladenem a6 (z. B. floatquicklib `$D300` braucht die ffplib-Basis), d0 hin und zurück |
| `BLITZ_BASEJSR(base, off, r)`, `BLITZ_BASEJSR1/2(base, off, x0[, x1])` | `JSR off(a6)` mit a6 = Basis (mathffp, memlib-Tabelle, Systemlibrary); 1/2 = Operanden in d0/d1 |
| `BLITZ_ALIBJSR_CCR(tok, r, "cc")`, `BLITZ_ALIBJSR_CCR_D0A0(tok, "cc", &d0, a0)` | Aufruf einer Routine, die im CCR antwortet (iffiolib: Z = ok); Ergebnis `Scc` direkt nach dem Aufruf |
| `BLITZ_ALIBJSR_D4`, `BLITZ_BLIBJSR_D4(tok, d0, d1, d2, d3)` | Aufruf mit Argumenten in d0–d3 (z. B. spriteslib ShowSprite) |
| `BLITZ_ENTRY_C(f)` | Code-Label als `char e_f[]` für Tabelleneinträge mit Adressrechnung |
| `BLITZ_AMIGAONLY(code)`, `BLITZ_BLITZONLY`, `BLITZ_BLITZONLY_CHK(chk)` | Adresse + 1 in `!subs`: nur Amiga-Modus / nur Blitz-Modus (err1 = 1 bzw. Prüfung + 1) |
| `BLITZ_ERRCHK_AB(entry, blitz, amiga)` | Fehlerprüfung mit Blitz-/Amiga-Variante (`$a001`-Präfix) im Fehlerprüfbereich |
| `BLITZ_ERRCHK_THUNK(entry, cfunc, save)` | Thunk einer Fehlerprüfung im Fehlerprüfbereich; `save` = Register, die die Originalprüfung erhält |
| `BLITZ_ERRCHK_MESSAGE(label, "Text")` | Fehlermeldung als Assembler-Daten im Fehlerprüfbereich (GCC erlaubt keinen Code und keine Daten in derselben benannten Section) |
| `BLITZ_RUNERR(msg)` | Laufzeitfehler (`TRAP #0`) |
| `BLITZ_INLINE`, `BLITZ_AB_ROUTINE`, `BLITZ_ASM_ROUTINE` | Inline-Routine, Blitz-/Amiga-Variante, Assembler-Routine |

`include/blitzhelp.h` (von `blitzlib.h` eingebunden) enthält kleine
gemeinsame Hilfen: `BL_SETW`/`BL_SETB` (Wort-/Byte-Operation, Rest des
Registers bleibt), `BL_EXTL`, `BL_SWAP`, `bl_divu_w`/`bl_divs_w` (DIVU/DIVS
als Befehl, Überlauf lässt das Register stehen), `BL_POPL`/`BL_POPW` (lokaler
Cursor auf dem a2-Argumentstapel), `LVO_SPFix` … `LVO_SPCeil` (mathffp) und
`TOK_globalloc` … bzw. `MEMBASE_localloc` … (oft aufgerufene Routinen der
System-Libs), `bl_mulu_w`/`bl_muls_w` (MULU.w/MULS.w als Befehl, sonst ruft
GCC teils `__mulsi3`), Custom-Chip-Zugriffe `BL_CUSTOM`, `BL_CUSTOMW`/`BL_CUSTOML`
(Register relativ zur Chipbasis), `BL_BLITBUSY`/`BL_BLITWAIT`,
`BL_INTENA`/`BL_INTS_OFF`/`BL_INTS_ON` sowie Tokens mehrfach aufgerufener
Befehls-Libs (`TOK_allocst`/`TOK_freest`, `TOK_setint`/`TOK_clrint`/`TOK_vwait`,
`TOK_execversion`, `TOK_iffopen` … `TOK_iffread`, `TOK_ilbminit` …
`TOK_ilbmbitmap`, `TOK_getpal`/`TOK_getpal2`, `TOK_convertkey` …
`TOK_qconvertkey`, `TOK_showsprite`, `TOK_setevfilter`, `TOK_extbitmap`).
`include/blitzexec.h` hat `BL_FREESIGNAL(sig)` (FreeSignal mit
null-erweiterter Signalnummer wie die Originale; die NDK-Inline nimmt ein
BYTE). Was nur eine Library braucht, bleibt in deren Quelltext.

## Werkzeuge

| Datei | Zweck |
|---|---|
| `tools/detok.py` | tokenisierte BB2-Quelltexte (`bblibsrc`) lesbar machen |
| `cmake -P tools/mklibnums.cmake` | `include/libnums.h` aus dem originalen `libnums.bb` erzeugen (`LIB_ldivlib` = `#ldivlib`) |
| `tools/biglib.py` | Libs aus BigLib-Dateien ziehen (`extract`), anzeigen (`dump`), Deklarationen vergleichen (`compare`), ersetzen (`replace`), neu zusammensetzen (`join`) |
| `tools/mkdeflibs.py` | einmaliger Generator: `deflibs/NAME.c` aus `cd/blitz2/deflibs` (s. u., `notes/deflibs.md`) |
| `tools/srcdecl.py` | Deklarationen (libmacs-Makros) eines Blitz-Quelltexts host-seitig auswerten und mit einer Lib-Binary vergleichen (`dump`/`compare`/`report`), siehe `notes/cmdlibs-decl.md` |
| `tools/amiga_test.py` | `tests/libs_tests` (snitch) in Amiberry, optional mit Filter; seriell, Timeout 30 s |
| `tools/disasm.py` | Code einer Library disassemblieren, Einsprünge und Relocs markiert |
| `tools/asmcmp.py` | Assembler-Routinen Original/C-Build befehlsweise vergleichen |

`amiga_test.py` nutzt `tools/amiberry.cmake` (Emulator ohne Fenster,
Konfiguration `A1200-AB3.uae`, 68020, Laufwerk `AB3C:`) und legt die Testdateien unter
`ab3c/build/amiberry/bb2-libs-c/` ab.

## Stand: alle 25 System-Libs

Alle Deklarationsteile sind mit den Originalen identisch
(`tools/biglib.py compare`); alle Tests im Emulator bestehen. Die Tests
sind ein snitch-Programm (`tests/libs_tests`, gleiche Einrichtung wie ab3c);
die Testlogik selbst ist C:

| Testfall (Tag) | Datei | Libraries | Prüfungen |
|---|---|---|---|
| `libtest ldivlib`/`lmullib` `[libtest]` | `libtest.c` | ldivlib, lmullib | je Routine 20 729 Fälle |
| `[sys]` | `systest.c` | memlib, datalib, chipbaselib, strings1lib, staticslib, varslib, exitslib, clrlib | 49 |
| `[mathstr]` | `systest_mathstr.c` | strings2lib, ffplib, strcomplib, floatquicklib, modlib, maxlenlib | 56 864 |
| `[runtime]` | `systest_runtime.c` | arrayslib, maxslib, errtraplib, rundebuglib | 113 |
| `[system]` | `systest_system.c` | wbstartuplib, intlib, switchlib, runerrlib, debuglib | 178 |
| `[cmd]` | `systest_cmdlibs.c` | memacclib, sortlib, mathtranslib, utillib, environlib (Befehls-Libs, s. u.) | 2 057 |

```
python3 -I tools/amiga_test.py                    # alle
python3 -I tools/amiga_test.py "[mathstr]"        # eine Gruppe
```

Referenz-Logs: `tests/ref-logs/` (snitch-Lauf, Strukturvergleich,
Befehlsvergleich).

Jeder Aufruf wird zusätzlich darauf geprüft, dass die C-Fassung mindestens
alle Register erhält, die das Original erhält. Routinen, die Assembler
bleiben, vergleicht `ctest --preset amiga` (`asmcmp_*`, Liste in
`CMakeLists.txt`) Befehl für Befehl mit dem Original.

**Assembler bleibt** nur, wo C es nicht ausdrücken kann: Stackpointer
sichern/setzen (exitslib, Teile von runerrlib, errtraplib), Exception- und
Trap-Handler mit RTE (errtraplib, rundebuglib, runerrlib, debuglib),
Sprungtabellen und Datenblöcke mit festem Layout (memlib, debuglib),
Inline-Routinen und Blitz-/Amiga-Präfixe (chipbaselib, intlib). Alles andere
ist C.

**Nicht im Emulator ausführbar** (nur Struktur-/Befehlsvergleich): alles,
was den Blitz-Modus einschaltet (switchlib goblitz) oder nur aus der
Debugger-/Trap-Zustandsmaschine erreicht wird (Teile von debuglib,
runerrlib, rundebuglib TRAP #0), sowie der Workbench-Start von wbstartuplib.
Die Einzelheiten stehen in den Kopfkommentaren der Libraries.

**Binary neuer als die Quelle:** `switchlib` (Tastatur über die
Device-Liste) und `debuglib` (zusätzliches Token `$D509`, erweiterte
Handler-Tabelle) – Referenz ist das Binary der CD.

**Für den Vergleich mit AmiBlitz3** (als `BUG(orig)` erhalten, Auswahl):
`MOD` auf Quick/Float rechnet nur mit den ganzzahligen Teilen (die
Library liefert bei Quick `Quotient<<16|Rest`, bei Float einen Long – der
Compiler liest das Ergebnis aber als Word bzw. Long, blitz2.s:9459/9463,
daher ist das kein sichtbarer Fehler);
Quick 0 → Float `$00000080`; `lmultiply`/`arrmult` addieren
Teilprodukte per SWAP (nur bei Überlauf sichtbar); `newlist` schreibt bei
Größe 0 (z. B. Dimension −1) 8 Bytes hinter den Block; Division durch 0 liefert
`$7FFFFFFF` unabhängig vom Vorzeichen.

## Stand: Befehls-Libs (acidlibs)

Die "einfachen" Befehls-Libraries, Reihenfolge nach Größe. Deklarationen
mit `tools/biglib.py compare` gegen das CD-Binary
(`biglib.py extract ../cd/blitz2/acidlibs NUM ref/NAME.obj`), Funktion
zweifach geprüft:

* **BASIC-Ebene:** Testprogramme `../compilerlauf/corpus/tests/` t09–t13, t20, t21, t30, t31
  sowie `tests-runerr/` t32–t36 (nur mit Fehlerprüfung)
  (jedes Token, Randfälle); `compilerlauf/tools/corpus_check.py` übersetzt
  sie mit Original-Compiler + Original-Libs und mit dem C-Port + `acidlibs-c`
  (der CMake-Target `acidlibs` nimmt jede `libs/*.c` auf) und vergleicht
  die Ausgaben, mit und ohne Laufzeit-Fehlerprüfung.
* **Routinen-Ebene:** `[cmd]` in `tests/libs_tests` (Ergebnisse, Speicher,
  a3, Registererhalt wie bei den System-Libs).

| Nr | Library | Token | Deklarationen | Tests | Assembler |
|---:|---|---|---|---|---|
| 180 | memacclib | Poke Peek Peeks$ Call | identisch | t09, `[cmd]` | `Call` (JMP mit allen Registern) |
| 110 | sortlib | Sort SortUp SortDown | identisch | t10, `[cmd]` | CCR von mathffp `SPCmp` → -1/0/1 |
| 192 | mathtranslib | ACos … HTan, `^` (Float) | identisch | t11, `[cmd]` | Inline-Routinen `JSR lvo(a6)` |
| 194 | utillib | CreateMsgPort DeleteMsgPort Wait TaskName$ | identisch | t12, `[cmd]` | – |
| 193 | environlib | WBWidth … WBExtViewMode, cr_vbr | identisch | t13, `[cmd]` | `MOVEC VBR` |
| 78 | datelib | SystemDate Date$ NumDays DateFormat Days … Secs | identisch | t30 | DIVU/DIVS als Befehl (Überlaufverhalten) |
| 128 | linklistlib | FirstItem AddItem … SortList (14 des Binarys) | identisch | t31, t32–t36 (Laufzeitfehler) | Fehlertexte als Daten, Thunk der Fehlerprüfungen |
| 185 | stringfunclib | Left$ Right$ Mid$ … Replace$ Cvi/Cvl/Cvq | identisch | t20 | Inline `Chr$`, `Len`; Thunks |
| 120 | vallib | Val Int Sgn Frac … Rnd Abs QAngle | identisch | t21 | Inline QFrac QWrap QAbs VPos True False |
| 124 | fileiolib | OpenFile ReadFile WriteFile CloseFile KillFile Fields Put Get FileOutput FileInput Lof FileSeek Loc Eof DosBuffLen WriteMem ReadMem Exists | identisch | t50, t53, t54–t57 (Laufzeitfehler) | BRA-Tabelle der I/O-Handler; Thunks |
| 106 | cliargslib | NumPars Par$ FromCLI ParPath$ | identisch | t51, t58–t59 (Laufzeitfehler) | – (Thunks) |
| 76 | banklib | Bank InitBank FreeBank LoadBank SaveBank BankLoc BankSize AllocMem FreeMem | identisch | t52 | – (Thunks) |
| 164 | inputoutputlib | DefaultInput DefaultOutput PopInput PopOutput (+ 7 Kanalroutinen) | identisch | t41, t43–t45 (Laufzeitfehler) | Kanal-Verteiler `JSR off(a6)`, BRA.w-Köpfe der Handler, Trap von defin |
| 187 | printlib | Print NPrint Str$ Format FloatMode UStr$ | identisch | t40 | BRA.w-Kopf des Str$-Handlers |
| 186 | editlib | Edit$ Inkey$ Edit | identisch | t41, t42, t46–t49 (Laufzeitfehler) | – |
| 74 | crunchlib (`#romuluscrunchlib`) | Implode Deplode CrMDecrunch PPDecrunch | identisch | t60, Host-Vergleich (unicorn) | – |
| 92 | arexxlib | CreateRexxMsg DeleteRexxMsg FillRexxMsg … RexxEvent IsRexxMsg RexxError | identisch | t61 | – |
| 90 | seriallib | OpenSerial WriteSerial … SerialEvent ReadSerialMem WriteSerialMem | identisch | t62, t63 (Laufzeitfehler), t64 (Empfang mit Gegenstelle, `compilerlauf/tools/serialrun.py`) | – |
| 158 | screenslib | Screen WbToScreen LoadScreen FindScreen … ShowBitMap ScreenTags | identisch | t81, t86–t89 (Laufzeitfehler) | Inline ViewPort/Show/Hide/Beep/MoveScreen/SMouseX/Y |
| 142 | fontlib | LoadBlitzFont BitMapOutput Colour Locate CursX CursY BitMapInput | identisch | t82, t84 (BitMapInput nicht testbar) | BRA.w-Köpfe der Handler, `doinput1` |
| 134 | intuifontlib | LoadFont (Binary ohne `Text`) | identisch | t82, t85 | `loadchk` (CCR-Prüfung) |
| 130 | rawkeylib | – (convertkey, unconvert, qconvertkey für andere Libs) | identisch | indirekt (t72 über windowslib) | – |
| 138 | windowslib | Window WaitEvent Event … WindowTags (75) | identisch | t70–t72, t75, t76, t78, t79 | BRA-Tabelle der Handler; Inline RastPort |
| 136 | menuslib | MenuTitle SetMenu MenuItem SubItem ShapeItem ShapeSub MenuGap SubItemOff MenuState MenuColour MenuChecked | identisch | t73, t71, t77 | – |
| 108 | mouselib | Mouse Pointer MouseArea MouseX MouseY MouseXSpeed MouseYSpeed PositionMouse | identisch | t74 | Aufruf ShowSprite mit d0–d3 |
| 162 | iffiolib | (keine; Routinen +1…+8 für andere Libs) | identisch | indirekt t100, t103 | – (Thunks, CCR-Ergebnis per tst.w) |
| 161 | ilbmifflib | ILBMInfo ILBMWidth ILBMHeight ILBMDepth ILBMViewMode DecodeILBM | identisch | t100, t103 | – (Thunks; ALibJsr mit Z-Flag) |
| 114 | iffmakelib | SaveBitmap SaveScreen SaveShape SavePalette | identisch | t100, t104, t107 | – (Thunks) |
| 132 | brexxlib | Record PlayBack SaveTape LoadTape AbsMouse RelMouse MouseButton ClickButton MacroKey TapeTrap QuietTrap Type QuickPlay XStatus PlayWait FreeMacroKey | identisch | t101, t106 | – (Thunks; Input-Handler in C) |
| 117 | blitzoslib | InitPage FreePage AddBox … BlitzEvent SpotBox (31 Tokens) | identisch | keiner (Original stürzt im Testaufbau ab) | BLibJsr-Aufrufe per Inline-Asm (BCALL), DIVS mit Überlauf |
| 116 | audiolib | DiskPlay DiskBuffer LoadSound Sound Volume Filter LoopSound InitSound SoundData PeekSound DecodeSound SetPeriod | identisch | t110, t114, t115, t119 (DiskPlay nicht testbar) | Inline SoundData/PeekSound, `IFFCALL` |
| 96 | trackerlib | LoadModule PlayModule StopModule | identisch | t111, t116, t117 | DBF-Warteschleifen, $A001-Prefixe |
| 82 | medlib | LoadMedModule StartMedModule PlayMed StopMed JumpMed SetMedVolume GetMedVolume GetMedNote GetMedInstr SetMedMask DecodeMedModule | identisch | t112, t118 | – |
| 88 | speaklib | Speak SetVoice Translate$ PhoneticSpeak VoiceLoc | identisch | t113 (nur Init-Fehler) | – |
| 140 | gadgetslib | TextGadget … SetTextGadget (GadgetList) | identisch | t90, Host-Vergleich | – |
| 141 | gtlib (`#mygadtoolslib`) | GTButton … GTFreeGadget (Binary-Stand) | identisch | Host-Vergleich | AddGList mit Position -1 als Long |
| 80 | asllib (`#myasllib`) | ASLFileRequest$ ASLFontRequest ASLScreenRequest ASLPathRequest$ | identisch | Host-Vergleich | – |
| 152 | freqlib | FileRequest$ | identisch | Host-Vergleich | Datenblock (Bytes + Relocs) des Requesters |
| 160 | bitmaplib | BitMap LoadBitMap ScreensBitMap CopyBitMap ShapesBitMap CludgeBitMap | identisch | t120, t124, t125 | – |
| 156 | palettelib | LoadPalette Cycle RGB Red Green Blue PalRGB SetCycle StopCycle AGARGB … MixPalette | identisch (Binary: Red/Green/Blue #long) | t121–t123, t126–t129 | Blitz/Amiga-Präfixe ($a001), RGB springt direkt in SetRGB4 |
| 157 | fadelib | FadeIn FadeOut ASyncFade DoFade FadeStatus | identisch | t123 | – |
| 150 | blitlib | Blit BlitMode CookieMode EraseMode InvMode SolidMode Block BlitColl | identisch | t130, t134–t136 | Blitz/Amiga-Präfixe, Inline-Modi |
| 146 | bblitlib | Buffer BBlit UnBuffer BBlitMode ShowStencil FlushBuffer | identisch | t131, t134, t137 | Blitz/Amiga-Präfixe, MULS/MULU-Inline |
| 148 | qblitlib | Queue QBlit UnQueue QBlitMode FlushQueue | identisch | t132, t134, t138 | Blitz/Amiga-Präfixe, Inline FlushQueue |
| 122 | stencillib | Stencil SBlit SBlitMode | identisch | t133, t134, t139 | Blitz/Amiga-Präfixe |
| 154 | shapeslib | LoadShape MakeCookie ShapeWidth ShapeHeight Handle AutoCookie CopyShape GetaShape SaveShapes LoadShapes MidHandle DecodeShapes InitShape | identisch | t140, t144, t145, t146, t149 | – (Thunks; Blitz/Amiga-Präfix GetaShape) |
| 118 | shapetrixlib | Scale XFlip YFlip Rotate | identisch | t141 (XFlip zeitabhängig, s. Notizen) | – |
| 112 | spriteslib | GetaSprite ShowSprite InFront InFrontF InFrontB SaveSprites LoadSprites SpriteMode | identisch | t142, t147 | – |
| 104 | collslib | SetCollOdd SetColl SetCollHi ShapesHit ShapeSpriteHit SpritesHit RectsHit SColl PColl DoColl | identisch | t143, t148 | – |
| 100 | scrolllib | Scroll Plot Point Cls BlockScroll | identisch | t150, t159 | – |
| 119 | 2dlib (#sis2dlib) | Circle Circlef Box Boxf Line FreeFill AllocFill ReMap FloodFill Poly Polyf BitPlanesBitMap ClipBlit ClipBlitMode BitMapWindow BitMapOrigin | identisch | t151–t155, t158 | yofill (FloodFill) als Original-Assembler |
| 121 | polylib (#chunkylib) | ChunkyCLS ChunkyPlot ChunkyVlin ChunkyPoly BitMapPoly RenderToBitmap RenderToChunky RenderPoly | identisch | t156, t157 | – |
| 190 | gameiolib | Joyx Joyy Joyr Joyb Gameb | identisch | t160, t165 (Laufzeitfehler) | – |
| 98 | blitzkeyslib | BlitzKeys RawStatus BlitzRepeat BlitzQualifier HandleRawKey BlitzKey | identisch | t161 | BRA-Kopf des Eingabe-Handlers |
| 94 | animlib | LoadAnim InitAnim NextFrame Frames | identisch | t162, t168 | Inline Frames; err1–7-Stubs |
| 144 | blitzcoplib | Slice CopLoc CustomCop Show ShowF ShowB CopLen ColSplit ShowBlitz FreeSlices DisplayOff DisplayOn SetBPLCON0 SetBLWait (Binary ohne Help) | identisch | t163, t166 | $A001-Köpfe |
| 143 | displaylib | InitCopList CreateDisplay DisplayBitMap … DisplayGenlock (24) | identisch | t164, t167, t169 | Fehler-Stubs, _checkcoplist |

Befunde und `BUG(orig)`-Eigenheiten je Library: `notes/cmdlibs-port.md`.
Alle Abweichungen der C-Fassungen von den Originalen (DIFF, Konventionen,
Compiler): `notes/abweichungen.md`.

Offen: die übrigen Befehls-Libraries (deflibs/acidlibs). Das Einbinden der
C-Libraries in einen echten Compilerlauf (C-Port `../blitz2-c`) ist für den
Testkorpus erledigt (`corpus_check.py`, Konfiguration clib).

## deflibs: AmigaOS-Aufrufe

`blitz2:deflibs` (39 Libraries mit den Deklarationen `Open_`, `AllocMem_` …,
Nummern 2, 200, 219–255) liegt als C-Quelle unter `deflibs/NAME.c` vor,
einmalig erzeugt von `tools/mkdeflibs.py`; seitdem sind die C-Dateien die
Quelle. Je Aufruf eine Zeile `BLITZ_OSCALL(baselib, lvo, "Name_", "(help)",
regs…)`, Kopf `BLITZ_OSLIBHEADER`, init/finit in C (`deflibs/deflib.h`,
inklusive der `BUG(orig)`-Endlosschleife und des falschen Namenszeigers der
Resourcen).

```
cmake --build --preset amiga --target deflibs   # -> build/amiga/deflibs-c
python3 -I ../compilerlauf/tools/corpus_check.py --deflibs build/amiga/deflibs-c
```

Alle 39 Deklarationsteile sind mit dem Original identisch (`biglib.py
compare`), die Reihenfolge auch. Einzelheiten und Testergebnisse:
`notes/deflibs.md`.
