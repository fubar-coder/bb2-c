# Blitz Basic 2.1 – what the CD documentation says about compiler, preprocessor and editor

As of 26.10.2026. Analysed was the local copy of the "Blitz Basic CD"
(Guildhall Leisure / BUI, compiled by Kevin Winspear) under
`archive/cd/ultimate-blitz-basic-2.1/`. The paths below are relative to it (there in upper case).

| Component | Version / date | Evidence |
|---|---|---|
| Compiler `blitz2/blitz2` | "Blitz Basic 2 Version 2.10 (C)opyright 1995 Acid Software, Release Version" | string in the binary |
| Manual `blitz2_manual` | Blitz Basic 2.1, plain text (OCR scan) | – |
| Editor `blitz2/ted` | TED 1.1 (according to `bum10b/blitz2/releasenotes`) | – |
| NewTED `extras/newteditor/ted` | V2.1, 20.10.95 | `extras/newteditor/readme.ted` |
| SuperTED `bum10a/ted223/ted`, `bss3/superted` | 2.23 (BUM10, 25.9.96) and 2.5 (23.6.96) | `bss3/superted/documentation/superted.guide` |
| Lib development `extras/libsdev/documentation` | September 1994 (Mark Sibly, Simon Armstrong) | – |
| BUM9 / BUM10 | 13.5.96 / 25.9.96 (Ted Bailey) | `bum9b/bum9docs/bum9_readme`, `bum10b/bum10examples1/bum10docs/bum10_readme` |
| `blitz2.guide` | library commands (Jurgen Valks, 1994); hardly any compiler content | – |

**Legend**

* **[Doku]** is stated like this in a document of the CD (file and line given).
* **[Bin]** is our own analysis of the binary files on the CD:
  disassembly with `m68k-amigaos-objdump -d -m68000 blitz2/blitz2` or a
  hex dump. Addresses are VMA in the `.text` hunk (file offset = VMA + `$20`).
  None of it was executed.
* **[Schluss]** is our own conclusion and not proven.

**Note on the manual:** `blitz2_manual` is an OCR text. The backquote
`` ` `` almost always appears as `'`, `{`/`}` partly as `(`, `)` or `t`,
`1` as `I` or `]`, `0` as `O`. Quotes below are silently corrected where the
meaning is unambiguous. 

---

## 1 Macros

### 1.1 Documented

* **Definition and call** [Doku] `blitz2_manual` lines 3280–3283: "A macro is
  defined in a Macro name..End Macro structure. The code between these two
  commands is not compiled but placed in the compiler's memory. When the
  compiler reaches a !macroname it then inserts the code …" Reference:
  lines 6603–6612 ("All text following Macro, up until the next End Macro, will be
  included in the macro's contents").
* **Parameters** [Doku] lines 3311–3337: parameters in `{…}`. A backquote before
  "a digit or letter (1-9, a-z)" marks the insertion point. From the tenth
  parameter on: `` `a `` is the tenth, `` `b `` the eleventh etc. Parameters
  "can be any text". Lines 3340–3352: bracket pitfall with `!distance{x*10+20,…}`.
  So the replacement is purely textual.
* **`` `0 ``** [Doku] lines 3359–3375: "returns the number of parameters passed";
  example `Vadd` with `CNIF `0=2 … CELSE … CERR … CEND`.
* **Recursion** [Doku] lines 3384–3396: "Macros are recursive and can call
  themselves". Example `dolist` with `CSIF "`2">""` and a self-call; so a
  missing parameter becomes empty text.
* **Single-line form** [Doku] line 3408: `Macro project …:End Macro` (without
  a space before `:End Macro`), call in the middle of an expression
  (`Circlef !project{x,y,z},3,…`). Line 4556: `Macro rndpt Rnd(640),Rnd(512):End Macro`.
* **`` `@ ``** is mentioned in the manual only indirectly [Doku] line 11884:
  "Duplicate Label: … May also occur with macros where a label is not
  preceded by a \@." (OCR; presumably `` `@ `` is meant).
* **Limits and errors** [Doku] appendix 1, lines 11792–11814:
  * "Macro Buffer Overflow: The Options requester … contains a macro buffer
    size … May also be caused by a recursive macro call which generates
    endless code."
  * "Macro already Defined: … may have been defined in one of the included
    resident files as well as somewhere in the source code."
  * "**Can't create Macro inside Macro**: Macro definitions must occur in the
    primary code."
  * "Macro without End Macro", "Macro too Big: Macro's are limited to the
    buffer sizes defined in the Options requester."
  * "**Macros Nested too Deep: Eight levels of macro nesting** is available in
    Blitz 2. Should never happen!!"
  * "Macro not Found: The macro has not been defined previous to the
    !macroname{ } call." So macros must be defined **before** the call.
* **Macros in residents** [Doku] lines 3010–3043 and lines 887–891: a resident
  contains NewTypes, macros and constants (section 3.3).
* **Space before `{`** [Doku] `extras/libsdev/documentation` lines 252–294:
  `!libheader {#acidsoundlib,init,1,0,0}`, `!libs {#acidsoundlib,$1380}`;
  manual line 3396: `!dolist {a,b,…}`.
* **Compiler options** [Bin] In the options window of the compiler there are, under
  "Debugger:", the switches "Long Arrays", "**Expand Macros**" and "**Quoted
  Macros**" (strings in `blitz2/blitz2`). The manual does not mention
  them. AB3 lists them in `ptr_xtradata` as "obsolete" (AmiBlitz3.ab3
  lines 24937–24941).

### 1.2 From the binary (expansion, `blitz2` `$3A3A`–`$3CA2`) [Bin]

* The call search runs over the statement up to `:` or `;` outside of
  strings and looks for `!`; token words (byte ≥ `$80`) are skipped as
  two bytes. After each expansion the search starts again at the
  start of the statement (`$3C9C bra $3A46`).
* Replacement after `` ` `` (`$3B4C`):
  * `` `{ `` → constant expression up to `}` (`jsr $BF6E`). So
    `` `{ausdruck} `` already existed in 2.10; the manual does not describe it.
  * `` `@ `` → expansion counter in decimal (`jsr $53DC`).
  * If (*x* OR `$20`) ≥ `$7A`, an error follows (`jmp $85DA`), so
    **`` `z `` is not allowed** either, contrary to "a-z" in the manual.
  * Digits and letters give *v* = 0…34. For *v* = 0 the count is inserted
    **as two digits** (`divu #10`). If *v* is greater than the count,
    nothing is inserted. Other characters (punctuation) fall onto the branch
    for *v* = 0, so the count is inserted.
* Token words in the body are copied as two bytes. A handling of
  `;` (header comment `;…:@:`) **does not exist**.
* For each expansion a buffer of the size of the macro buffer is allocated via
  `AllocMem` (`$3AF0`–`$3AFC`); on overflow: `$81E6`.

### 1.3 Conclusions [Schluss]

* The quirks (punctuation after `` ` `` acts like `` `0 ``; `` `z `` not
  allowed) and the two-digit `` `0 `` already existed in 2.10.
  They are not regressions of AB3, but old peculiarities.
* The header comment of the `Macro` line is a later
  extension.
* Nested macro **definitions** were forbidden according to the docs. AB3,
  in contrast, stores them as text and creates them during expansion. Whether 2.10 actually reports the error is not
  checked; the error message is in the binary.

---

## 2 Conditional compilation

### 2.1 Documented

* [Doku] `blitz2_manual` lines 3220–3233: "CNIF – compiler on if numeric
  comparison is true … CSIF … CELSE – switch compiler from previous state
  on=>off off=>on, CEND – end of conditional block (restores previous
  state)", "**CNIF/CEND blocks can be nested.**"
* [Doku] lines 3235–3238: "the CNIF directive only works with constant
  parameters – for example '5', '#test' – and not with variables."
* [Doku] reference lines 6566–6590: "CNIF Constant Comparison Constant …
  Comparison should be one of '<', '>', '=', '<>', '<=' or '>='"; "CSIF
  "String" Comparison "String" … Both strings must be quote enclosed literal
  strings"; "CSIF is of most use in macros for checking macro parameters."
  The syntax help in the binary reads the same: `CNIF` / "Constant Comparison
  Constant", `CSIF` / ""String" Comparison "String"".
* [Doku] lines 6592–6602 and lines 2293–2300: `CERR "text"` generates a
  compiler error, intended for macros and conditional compilation.
* [Doku] errors (lines 11778–11783): "CNIF/CSIF without CEND", "CEND without
  CNIF/CSIF...".
* A definedness test `@#name` does **not** occur in the docs.

### 2.2 From the binary [Bin]

* `getchar` (`$7A2E`–`$7BD4`) handles the tokens `$801C`–`$801F` directly:
  `$801C` → `$C47E` (CNIF), `$801D` → `$C492` (CSIF), `$801E` CELSE,
  `$801F` CEND. The numbers are the same as in AB3.
* If *compile* is false, every character except `0` is discarded. In the
  skipped area CNIF/CSIF only increments *nest* (`$7B4E`).
* **CELSE** toggles if *nest* = 0 (`$7B58`–`$7B62`); it does not check
  the stack. So multiple CELSE toggle multiple times, and a
  CELSE without CNIF silently switches to skipping.
* **CEND** restores the state from the stack; an empty stack leads
  to an error (`$7B6C`–`$7B9A`).
* **CNIF** (`$C47E`) evaluates a **whole constant expression**
  (`bsr $BF6A`) and checks with `D1=5` for ≠ 0. This is the same mechanism
  as in AB3 (`do_cnif`, `D1=5` → `cif_ne`). After that the statement must end
  (`$C50E`–`$C512`).
* **CSIF** (`$C492`) reads string, operator (`bsr $CCA8`) and string; the
  strings go through `get1bytemain` (`$7C0E`).

### 2.3 Conclusions [Schluss]

The docs simplify: "Constant Comparison Constant" is in truth an
arbitrary constant expression with a test for ≠ 0, as in AB3. The
multiple-CELSE and CELSE-without-CNIF quirks come from 2.10.

---

## 3 Constants, includes, residents, options, lexis

### 3.1 Constants

* [Doku] lines 3168–3200: `#width=320`. Constants "can be used in assembler",
  "can be used with conditional compiling evaluations", "**can only hold
  integer values**", "can be altered through the source at compile time but
  NOT at runtime".
* [Doku] errors lines 11604–11627:
  * "Constant not defined", "Constant Not Found".
  * "**Constant already defined: Constants can only be defined once, i.e.
    cannot change their value through the code.**"
  * "Fractions Not allowed in Constants: … they are usually rounded and no
    error is generated."
  * "Illegal Constant Expression: A constant may only hold whole numbers,
    either a decimal place, text or a variable name has been included".
  * "Label has been used as a Constant" (line 11877).
* [Doku] system constants lines 1629–1639: `Pi`, `On` = −1, `Off` = 0,
  `True` = −1, `False` = 0.
* [Doku] line 3030: the resident instructions say `xconst= 1 0` (OCR, probably
  `#xconst=10`).

### 3.2 Includes

* [Doku] lines 6528–6564:
  * "INCLUDE … The file **must be in tokenised form** (ie: saved from the
    Blitz editor) – **ascii files may not be INCLUDE'd**. INCDIR may be used
    to specify a path for Filename. **Filename may be optionally quote
    enclosed to avoid tokenisation problems.**"
  * "XINCLUDE … only ever included once."
  * "INCBIN … INCDIR may be used …"
  * "INCDIR Pathname … prefixed to any filenames specified by any of
    INCLUDE, XINCLUDE or INCBIN."
* [Doku] errors lines 11680–11684: "Already Included", "Can't open Include",
  "Error Reading File".
* [Doku] lines 11531–11533: "Any reference to an include file or a macro could
  mean the error is there and not on the line referenced."

### 3.3 Resident files

* [Doku] lines 887–891 (menu CREATE RESIDENT): "A resident is a file including
  all constants and macro definitions as well as newtype definitions … a
  dramatic increase in compile speed".
* [Doku] lines 3010–3043: one writes a program with the NEWTYPEs, macros
  and constants, runs **COMPILE&RUN** and then chooses **CREATE
  RESIDENT**. So the resident reflects the state after
  compiling. It is included "by entering the name of the resident file
  into **one of the 'RESIDENT' fields** of the compiler options
  requester". `AMIGALIBS.RES` contains "all the structures, constants and
  macros" of the operating system.
* [Doku] errors lines 11784–11790: "**Clash in Residents: Residents … must not
  include the same Macro and Constant definitions.**", "Can't Load Resident:
  … Check the pathname."
* [Doku] `extras/libsdev/documentation` lines 195–197: "Keep your .res files in
  the root of blitzlibs:, reference all resident files in compiler options
  as blitzlibs:filename.res".
* [Bin] In the `.xtra` files there are **8 resident fields of 68 bytes each**
  from offset `$36` on; fields at `$36`, `$7A`, `$BE` are occupied in
  `bss3/redmapeditor/examples/source/demo*.bb.xtra`. AB3 also reserves
  544 bytes for this (`Dc.w $0220 ; 68*8`, AmiBlitz3.ab3 line 24960).

### 3.4 Compiler options and `.xtra`

* [Doku] lines 928–956: Create Icons, Enable Runtime Errors, Make Smallest Code
  (two passes, "automatically selected when creating executable
  files"), Debug Info, Buffer Sizes, Object Maximums, Resident.
* [Doku] lines 6614–6622: `Runerrson`/`Runerrsoff` "override the settings in
  Compiler Options".
* [Doku] `bss3/superted/documentation/superted.guide` lines 717–725: "'.xtra'
  (since those are settings files for Blitz source code)". Lines 1045–1053:
  "The .xtra files … are handled by a function inside Blitz 2 itself and
  not SuperTED"; the intermediate files are "only around 1k per document".
* [Bin] Window texts in the compiler (order as in the binary):
  "Create Icons for Executable Files"; "Runtime Error Debugger…",
  "Debugger:", "Long Arrays:", "Expand Macros:", "Quoted Macros:"; "Make
  Smallest Code"; "Create Debug Info …". Then the buffers "Object, Libs,
  System, Data, Macro, String Buffer" with the defaults 16384, 16384, 4096,
  4096, 8192, 10240; then "Object Maximums", "Resident". Under "Runtime
  Error Options" there are AUTO RUN, INTERUPT CHECKING, ASSEMBLER CHECKING,
  OVERFLOW CHECKING. This is the same order as `ptr_xtradata` in AB3.
* [Bin] **Structure of the `.xtra`**, derived from hex dumps of
  `bui8/jackexplains/filerequester.bb2.xtra`, `blitz2_pddisk_*/…` and
  `bss3/redmapeditor/…`; the field names are **[Schluss]**:

  | Offset | Length | Contents |
  |---|---|---|
  | `$00` | 1 | `$01` (seen in all files; version byte?) |
  | `$01` | 1 | Create Icons (`$80`/`$00`) |
  | `$02` | 14 × 2 | switches (`$0080` = on) |
  | `$1E` | 6 × 4 | buffers Object, Libs, System, Data, Macro, String (e.g. Macro `$2000` = 8192, String `$2800` = 10240) |
  | `$36` | 8 × 68 | resident names |
  | `$256` | 2 × 192 | path and name of the executable (e.g. `df0:caffeinefree`, `CaffeineFree`) |
  | `$3D6` | 128 | CLI argument (`viewlibs.bb2.xtra`: `commands`) |
  | `$456` | rest | word pairs (number, maximum) of the Object Maximums; their count explains the varying sizes 1054–1250 bytes |

  Noticeable: some resident fields begin with `$00`, followed by
  `litzlibs:…`. Whether this is a disabled field is open.

### 3.5 USEPATH, directives, comments, names

* **USEPATH** [Doku] lines 1784–1822: textual insertion before `\feld`. "UsePath
  is a compiler directive … affects the compiler as it reads through your
  program top to bottom … governed by the closest previous usepath in the
  listing." Reference lines 6490–6505 (spelled `USEPATH` there).
* **Directives in 2.10** [Bin] The syntax help of the system lib in the binary lists:
  NEWTYPE, End, Let, Dim, Goto, Gosub, Return, Statement, Function,
  MouseWait, If, While, **Macro**, Select, Case, Default, AsmExit,
  **XINCLUDE**, **INCLUDE**, (placeholder "Jimi"), SHARED, For, Next, To,
  Step, Data, Read, **CNIF**, **CSIF**, **CELSE**, **CEND**, **CERR**, Then,
  **Else**, Even4, Even8, Dc, Ds, Even, Dcb, GetReg, IncBin, …, DEFTYPE,
  BLITZ, AMIGA, QAMIGA, …, the 68000 mnemonics **only in upper case**,
  then INCDIR, WBStartup, USEPATH, CloseEd, NoCli, Runerrson, Runerrsoff.
  According to this order `Macro` = `$800D`, `XINCLUDE` = `$8012`,
  `INCLUDE` = `$8013`, CNIF…CEND = `$801C`–`$801F`, `Else` = `$8022`, as
  today. **Missing are** `OPTIMIZE`, `SYNTAX`, `USEPATH2`, `USELASTPATH` and
  the lower-case PPC/FPU mnemonics.
* **Comments** [Doku] lines 3618–3623: "Blitz2 uses the semicolon character.
  Any text after a semicolon on a line will be ignored by the Blitz2
  compiler." Likewise lines 3959–3970. [Bin] `getchar` (`$7BB4`–`$7BC4`) reads
  after `;` outside a string up to the end of the line; a check for
  `:@` does not exist.
* **Names** [Doku] lines 3553–3568: "names can be of any length; they must
  start with a letter … or an underscore; must only contain alphanumeric
  chars and underscores; **must not start with the same letters as any
  Blitz2 command**. Also, label and variable names in Blitz2 are always
  treated as **case-sensitive**".
* **Line continuation `@@`, escape sequences** are **not** documented
  in the manual. Strings have no means of escaping; documented is
  only "No Terminating Quote" (line 11636).

### 3.6 The CMake character `~`

* [Doku] lines 3424–3450: "A special character known as the cmake character
  can be used to evaluate constant expressions and insert the literal
  result into your code." Example `NPrint var~`1~` with `!Ivar{2+1}` →
  `var3`.
* [Bin] `getchar` compares with **`~` = `$7E`** (`$7A7A cmpw #126`), not
  with `$7F`. It evaluates a constant expression (`$7ACA bsr $BF6E`),
  expects a closing `~` (`$7ACE`, otherwise syntax error) and writes
  the value in decimal into a buffer. The buffer end is `$01`; when reading
  `$01` it jumps back to the saved position (`$7A94`). `$02` is
  the end of a USEPATH insertion (`$7A82`). A `~` within an
  insertion ends it (flag `$79AC`).
* [Schluss] The check is in `getchar` before any string distinction,
  and string literals are also read via `getchar` (§4.3). A `~` was
  thus presumably active **everywhere**, also in strings and names. This is
  not checked.

---

## 4 Editor TED and file format

### 4.1 Tokenisation

* [Doku] `superted.guide` lines 813–824: "Blitz 2 treats keywords as 'Token's.
  These are special characters which are recognized as separate from
  standard ASCII characters. The Tokenise option allows you to switch off
  tokenisation … when importing a large ASCII file … With this option
  ticked, Blitz commands will look like and are treated the same as
  standard non-Blitz words."
* [Doku] `extras/newteditor/readme.ted` lines 22, 47: NewTED 2.1 can "Save
  entire file as ASCII", has the option "Tokenise – Tokenise into Blitz
  commands" and its own colour for tokens. [Bin] TED 1.1 already has
  "Tokenise", "Token Colour:" and "Token Unavailable".
* [Doku] `internet/frac/read.me` lines 1–45: the author wrote the readme with
  tokenisation switched on: "The capitalisation is a bit strange in this
  file 'cos I forgot to turn off Tokenising". In the text, in mid-sentence, there are
  `On`, `To`, `AND`, `Use`, `Free`, `Point`, `Used`, `Line`, `If`, `For`.
  [Schluss] TED recognised keywords **regardless of
  upper/lower case**, even in plain text, and stored them as
  tokens. They were displayed in the spelling of the token table.
* [Doku] `blitz2_manual` line 6535 and elsewhere: file names "may be optionally quote
  enclosed to avoid tokenisation problems". [Schluss] So a file name without
  quotes could contain tokens.
* [Doku] `blitz2_manual` line 573: "Any file ending with the suffix .bb2 is a
  source file". [Bin] All TED versions contain `.xtra` and `.bak`;
  NewTED and SuperTED filter with `(#?.bb|#?.bb2)`.

### 4.2 Lines

* [Doku] `bss3/superted/extras/autodocs/ted.doc` (`TEDDetoke`,
  `TEDGetLabelList`): "SuperTED's line structure is as follows: *PrevLine,
  *NextLine, **linelength.b**, <actual tokenised text>". So a line is
  at most 255 bytes long in memory (tokenised).
* [Doku] ARexx `GETLINE`/`GETWORD` return "detokenised" text
  (`superted.guide` lines 334–335, 506–507).

### 4.3 Umlauts

* [Doku] `superted.guide` lines 1007–1019 ("Foreign Users"): "any 'umlauts' …
  will not be accepted by SuperTED. This is due to the fact that **all
  characters with ASCII values of 128 or higher are treated as tokens**
  (Blitz commands) … A few of the more common characters are handled".
* [Bin] **Umlaut table in all three TED binaries**: `blitz2/ted` @
  `$5DCE`, `extras/newteditor/ted` @ `$77A8`, `bum10a/ted223/ted` @ `$7E34`,
  each `E4 1A F6 1B FC 1C C4 1D D6 1E DC 1F 00`. That means ä→`$1A`,
  ö→`$1B`, ü→`$1C`, Ä→`$1D`, Ö→`$1E`, Ü→`$1F`. The table ends with `00`:
  **ß (`$19`) and © (`$18`) are missing** in these versions; they are only added in
  `ted2.bb2` (bb2-corpus.md §3.2).
* [Bin] **Reverse direction in compiler 2.10**: `blitz2/blitz2` has at VMA
  `$7BEE` the table `gs` with 26 × `$00`, then **`E4 F6 FC C4 D6 DC`**. That
  means `$1A`→ä, `$1B`→ö, `$1C`→ü, `$1D`→Ä, `$1E`→Ö, `$1F`→Ü – **the same
  order as TED**. It is used only in `get1bytemain`
  (`$7C0E`–`$7C28`): if inside a string and the character is < `$20`,
  it is mapped via the table; `$00`–`$19` become 0.
* [Bin] **String literals go through this mapping in 2.10**: the
  literal routine (`$ADC0`–`$ADF2`) reads each character with `bsr $7C0E` and
  writes it into the code with `bsr $7428`. On 0 it jumps to `$8224`
  (presumably "No Terminating Quote").
* [Schluss] So Blitz 2.10 output the umlaut bytes of old sources as Latin-1
  ä ö ü Ä Ö Ü. Other control characters `$01`–`$19` ended a string
  like the end of the line. AB3 has the table with **swapped** ä/ö and Ä/Ö
 , and `litstring` does not use it at all. Both are later
  changes, not the behaviour of Blitz 2.1.

### 4.4 Unknown tokens

* [Doku] `blitz2_manual` lines 11672–11674: "Token Not Found: 'token number':
  When loading source, **Blitz 2 replaces any unfound tokens with ?????**,
  compiling your code with these unknown tokens present will generate the
  above error." Likewise `bum9a/blitz2/releasenotes`: "Blitz will report the
  error 'Token Not Found' and the command will appear as ????? instead of
  text".
* [Bin] Error texts "Token Not Found : $ffff." and "Library not Found :
  $ffff."

---

## 5 Library system

* [Doku] `blitz2/releasenotes` section 4: in 2.1 the command set is loaded from
  three sources. "acidlibs – a single file containing standard Blitz
  commands; deflibs – commands compiled from third party libs found in
  blitzlibs:; userlibs – all libs found in directories branching off
  blitz2:userlibs". New in 2.1 is "restructured library handling". [Bin]
  The binary contains `blitz2:acidlibs`, `blitz2:deflibs`, `blitz2:userlibs/`.
* [Doku] `extras/libsdev/documentation` lines 43–69 and 366–396 (as of 1994,
  before 2.1): first `Blitz2:Deflibs` is loaded, otherwise all
  subdirectories of `Blitzlibs:` (System, Amigalibs, Basic, Userlibs).
  "Deflibs is simply all the separate Blitzlibs: libraries joined together
  as one continuous file." `Makedeflibs` generates it; "Reload All Libs"
  (menu, manual lines 911–914) reloads.
* **Lib numbers** [Doku] `extras/libsdev/documentation`:
  * "Each library in Blitz 2 has it's own unique identifying number"
    (line 406).
  * Development libs 10…50 (line 411).
  * BUM7 note lines 118–124: "Do NOT use library numbers above 40 … we left
    the odd numbers for our own expansion"; assigned were among others Elmore 99,
    97, 95, RI 63, 58, 51, Neil 50, 49, 48.
  * Amigalibs via `fdconvert`: "first available should be #217" (lines 85–88);
    commands get an appended `_` (lines 92–94).
* **Token number** [Doku] `blitz2_manual` lines 11663–11666: "token number =
  libnumber*128 + token offset" (with `TokeJsr`). Lines 6729–6730: "A full list
  of all token numbers … will be available shortly".
* **Consequences of changes** [Doku]:
  * `blitz2.guide` lines 3263–3265 (CIA tracker): commands "are only included
    because they served a purpose in debugging a long time ago, and **to
    remove them would cause problems with the Blitz tokens**".
  * `extras/libsdev/userlibdocs/xbonesciatrklib.doc` line 445 and elsewhere:
    "ModuleToChip still tokenises, but performs no function".
  * `bum10b/bum10examples1/bum10docs/xbciatrackerlib.doc` lines 102–103:
    "WARNING! Some of the tokens have changed … you MUST save off any source
    that uses this library in ASCII format."
  * `internet/tritonblitz/readme`: "save all your sources as ASCII. The new
    libraries have probably other IDs … sonst werden die Tokens nach der
    Neuinstallation … nicht mehr erkannt."
  * `bum9a/bum9examples2/deflibman/libman.doc`: LibMan shows
    number collisions ("Clashes (i.e. double libnums)") and leaves them out with
    "Auto".
* **Looking ahead** [Doku] `extras/libsdev/documentation` lines 112–116 (Mark
  Sibly): "we are probably going to get rid of tokenising in Blitz3 so that
  1. we don't get all the clashing that has been going on and 2. users can
  use what ever ascii editor they please."
* [Schluss] An automatic conversion of old token numbers (like
  `replacedat` in AB3) is mentioned nowhere for 2.1. The recommended
  way was to save the source as ASCII and tokenise it again. The
  tables in bb2 `tokens/` (not taken over) show the numbers of 2.1,
  e.g. the string commands in lib 185 and `Edit$`/`Inkey$` in lib 186.

---

## 6 Version history and differences to later versions

* **2.1 compared with earlier** [Doku] `blitz2/releasenotes`: "online help,
  restructured library handling, bug free parameter passing, comprehensive
  AGA and 3.0 support". Earlier the Help key only showed the syntax in the
  title bar.
* **BUM9 (13.5.96)** [Doku] `bum9a/blitz2/releasenotes`: new debuggers,
  NewTED 2.1 (from WB 2.0), new third-party libs; the compiler stays the same.
  `MakeDefLibs.bb2` and LibMan generate small deflibs.
* **BUM10 (25.9.96)** [Doku] `bum10b/blitz2/releasenotes`: "Blitz2 compiler
  (unchanged from BUM9)", "acidlibs (unchanged from BUM9)", SuperTED 2.23;
  new libs in `userlibs/beta`. `bum10_readme`: "it will probably be the
  last one ever".
* **SuperTED 2.5** (Red When Excited, 1996): ARexx port `TED_REXX1…9`,
  multiple documents, `.xtra` via temporary files in `T:`.
* **Differences to AmiBlitz 2/3**, as far as recognisable from the CD
  [Bin/Schluss]:
  * There was no `OPTIMIZE`, no `SYNTAX` (new syntax mode), no
    escape sequences `\\…`, no character literals `@"…"`, no `:@`, no
    header comment for macros, no `USEPATH2`/`USELASTPATH` and no
    lower-case mnemonics.
  * The CMake character was `~` (`$7E`), not `$7F`.
  * The umlaut table `gs` was the right way round, and string literals used
    it.
  * The macro buffer was 8192 bytes (AB3: 40 000).
  * The editor tokenised regardless of upper/lower case (AB3:
    case-sensitive).

---

## 7 Where the 2.1 documentation and the 2.10 binary differ from AmiBlitz3

Observations on the CD's documentation [Doku] and the 2.10 compiler [Bin];
details are in the sections above.

| Topic | BB2.1 docs / binary | Note |
|---|---|---|
| Umlaut table `gs` | 2.10 binary: `gs` = `E4 F6 FC C4 D6 DC` (`$1A`=ä, `$1B`=ö …), same as TED 1.1/2.1/2.23 [Bin] | compiler and editor agreed; AB3's table has ä/ö and Ä/Ö swapped |
| String literals | 2.10 reads literals via `get1bytemain` and thus via `gs` (`$ADC0`) [Bin] | Blitz 2.1 output ä ö ü Ä Ö Ü; AB3's `litstring` does not map `$1A`–`$1F` |
| `$00`–`$19` in strings | mapped to 0 in 2.10 [Bin] | control characters ended the string [Schluss] |
| `:@`, escapes | 2.10 `getchar` skips after `;` to the end of the line; the docs know only `;` and "No Terminating Quote" [Bin, Doku line 3618] | later extensions |
| CMake `~…~` | docs lines 3424–3450 use `~`; the binary compares with `$7E` [Doku, Bin] | the character is `~` itself, also in old `.bb2` |
| Names | docs lines 3553–3568: names may not begin with a command name, case-sensitive [Doku] | presumably a consequence of the TED tokenizer [Schluss] |
| Spelling in the docs (`UsePath`, `DefType`, `Nprint`) | TED recognised keywords regardless of spelling (frac/read.me) [Doku-indirect, Schluss] | in BB2 the spelling did not matter because TED normalised; only a case-sensitive ASCII tokenizer breaks such examples |
| USEPATH | textual, "top to bottom", the closest previous USEPATH applies; end marker `$02` [Doku, Bin] | `USEPATH2`/`USELASTPATH` did not exist in 2.10 |
| CNIF | docs: "Constant Comparison Constant" with `< > = <> <= >=`; binary: expression + `D1=5` (≠ 0) | docs simplify; CSIF operators `=<`, `=>`, `><` are not in the docs |
| Multiple CELSE, CELSE without CNIF | docs: "toggle"; binary toggles every time with *nest* = 0 and checks only *nest*, not the stack [Bin] | quirks from 2.10 |
| Redefinition of constants | docs: "Constant already defined: Constants can only be defined once" (line 11612); text also in the binary [Doku] | whether 2.10 actually reported the error is open |
| Fractions in constants | "Illegal Constant Expression: … a decimal place, text or a variable name"; but also "Fractions Not allowed … usually rounded and no error" | the docs contradict themselves |
| Residents | "contains a pre-compiled list of NewTypes, macros and constants"; "must not include the same Macro and Constant definitions" [Doku]; `.xtra` has 8 fields of 68 bytes [Bin] | BB2 had 8 slots; AB3's limit of 7 lies in the text header (`ParseAB3Header`) |
| Macro parameters `` `1 ``…`` `9 ``, `` `a ``…`` `z `` | docs: "1-9, a-z"; binary: `` `z `` → error | docs error already in 2.1; `` `0 `` (two digits, `divu #10`), `` `@ `` (error text, line 11884) and `` `{ausdruck} `` are supported by the binary |
| Nested macro definition | "Can't create Macro inside Macro: Macro definitions must occur in the primary code" [Doku] | an error in 2.1 |
| Macro size and depth | buffer in the options window, default 8192 [Bin]; "Macro too Big", "Macro Buffer Overflow", "Macros Nested too Deep: Eight levels" [Doku] | a depth limit of 8 existed (for what exactly is open) |
| Macro header comment | no handling of `;` in the 2.10 expansion [Bin] | later extension |
| Macro call search | search up to `:`/`;`, restart at the start of the statement (`$3A46`–`$3A90`, `$3C9C`) [Bin] | |
| Line length | SuperTED: `linelength.b` (≤ 255) [Doku] | the editor did not allow longer lines |
| Old token numbers | no automatic conversion; recommendation "save as ASCII" [Doku] | old sources without matching libs show `?????`; tokenisation in TED can be switched off |
| Library numbers | "token number = libnumber*128 + token offset" [Doku]; `$8012` = XINCLUDE: order in the system lib …, XINCLUDE, INCLUDE … [Bin] | |
| `@@`, `@#name`, `[`/`]` counting | not documented | open |

---

## 8 Answers to open points in `ab3c/docs/bb2-converter`

| Open point | Answer from the CD | Kind |
|---|---|---|
| What does Blitz 2 do with `$1A`–`$1F` in string literals? (README, corpus §3.3/§8.1) | Compiler 2.10 maps them to Latin-1 when compiling: `$1A` ä, `$1B` ö, `$1C` ü, `$1D` Ä, `$1E` Ö, `$1F` Ü. `$00`–`$19` become 0, which acts like the end of the line | [Bin]; an Amiberry test is still pending |
| TED table vs. `gs` | In 2.10 both agree (TED order). The swap in `gs` is younger | [Bin] |
| `$19` ß, `$18` © | Missing in TED 1.1, NewTED 2.1, SuperTED 2.23 and in `gs` of 2.10; only in `ted2.bb2`. Compiler 2.10 turns `$18`/`$19` in a string into 0 | [Bin], consequence [Schluss] |
| Bytes ≥ `$80` in strings | SuperTED: "all characters with ASCII values of 128 or higher are treated as tokens"; `getchar` 2.10 always combines ≥ `$80` into a word, also in a string. So Latin-1 in strings is not a Blitz 2.1 form | [Doku], [Bin] |
| Keywords as text (`Macro`, `End NEWTYPE`, `XINCLUDE`) | TED could switch off tokenisation; NewTED saves the "entire file as ASCII". Files with text keywords came about this way or with third-party editors. For INCLUDE the docs require "tokenised form – ascii files may not be INCLUDE'd" | [Doku], [Schluss] |
| `.xtra` layout (README no. 8) | Structure see §3.4 (offsets from hex dumps, matches the order of `ptr_xtradata`). Deviation: BB2 has 14 switch words, AB3 lists 15 | [Bin]/[Schluss] |
| Header `BB22` | Not explained on the CD; no file with this header (hits only by chance in binary data) | open |
| Old token numbers | Docs: number = lib × 128 + index; tokens not found appear as `?????` and result in "Token Not Found". Lib change: save the source as ASCII beforehand. Numbers of 2.1: bb2 `tokens/acidlibs.txt` (not taken over) | [Doku] |
| `~…~` "kommt im Korpus nicht vor" (corpus §2) | The search was for `$7F`; but in Blitz 2 it is `~` (`$7E`). The corpus must be searched for `~` in macro bodies | [Bin] → new search needed |

---

## 9 Open points

1. **Amiberry test with Blitz 2.1:** confirm that `NPrint "a␚␛␜␝␞␟b"`
   outputs the bytes `61 E4 F6 FC C4 D6 DC 62` and that a control character
   `$01`–`$19` in a string results in an error. So far this is only inferred from the
   disassembly.
2. **`~` in strings and names in 2.10:** is `"a~b"` treated as a CMake
   expression? Follows from the code, but is not checked.
3. **"Constant already defined" and "Can't create Macro inside Macro":**
   are these errors actually reported in 2.10? The texts are in the
   binary; the caller has not been looked for.
4. **"Macros Nested too Deep" (8 levels):** what does the limit refer to?
   The expansion works iteratively; presumably to macros in
   include/expansion stacks. Not investigated.
5. **`.xtra`:** meaning of the 14 switch words in detail; the byte
   `$01` at `$00`; resident fields with `$00` at the start; meaning of the
   word pairs from `$456` on (lib number or object type?). Clarify the difference 14 ↔ 15
   words to AB3 before AB3/ab3c reads a BB2 `.xtra`.
6. **TED tokenizer:** did TED also tokenise prefixes (reason for "must not
   start with the same letters as any Blitz2 command") and also in
   comments and strings? frac/read.me points to recognition regardless of
   upper/lower case. Checkable with `blitz2/ted` in
   Amiberry.
7. **`@@`, `@#name`:** present in 2.10? (`cmpw #64` at `$983A`, `$D670`
   not investigated.)
8. **Header `BB22`:** not explained on the CD.
