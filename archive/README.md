# Archive: unchanged originals

This holds third-party original material that the build or the tests need
as a reference – **unchanged**, never edited. Anything derived from it is
created in the build directory (e.g. the reference build of TED) or is
stored as generated test data under `tests/data/`.

| Directory / file | Contents | Origin |
|---|---|---|
| `sources/amibl2src.readme` | the readme of the Aminet archive `amibl2src.lha` – evidence of the origin and license of the sources | `amibl2src.lha` (Aminet `dev/blitz/amibl2src`) |
| `sources/blitz2-1.95/` (in git) | the compiler Blitz Basic 2 v1.95 (`version equ 195`): `blitz2.s` with the files it includes (`myoffs`, `wbheader`, `options2.src.new`, `asmmac`, `typereq`, `options.src`, `calcreq`, `obj.info`) and `GenAm.opts` (`nocase`) | `bb2src.zip` (as of 1995-09-14, top-level directory), from `amibl2src.lha` as below; only the files `blitz2.s` needs. `myoffs` and `wbheader` are the same as in `ted-1.1` (each version complete). |
| `amiblitz3-github/Sourcecodes/Data/fns/fonts/PERSONAL.FNS` (in git) | the FNS font "Personal" (1712 bytes) of Reflective Images' FNS library: the font RIFNSLib builds in (`IncBin "FNS:Personal.FNS"` of `rwelibsrc/FNSLib/fns-library11.bb`; the library's binary has it with 7 words changed, which the C port applies itself, `libs/userlibs/rwe/rifnslib.c`) | AmiBlitz3 on GitHub, <https://github.com/AmiBlitz/AmiBlitz3>, commit `b1a9dee150ef4d2793d9745a49a958fdb9178124` (file last changed in `df78b6840e14`), path as there; the repository is under the Apache License 2.0. The same file is on the CD (`BSS2/REDBLITZLIBS/EXAMPLES/FNS/FONTS/PERSONAL.FNS`) and in AmiBlitz3's SVN; AmiBlitz3's `BlitzLibs/Decompiled/thirdpartylibs/Rifnslib.obj` is byte-identical to the CD's BSS2 `RIFNSLIB.OBJ`. Decision of 2026-10-29: in git because AmiBlitz3 publishes it. |
| `sources/ted-1.1/` (in git) | TED 1.1, the editor of Blitz Basic 2: `ted.s` with includes (`myoffs`, `wbheader`, `aboutreq`, `specialreq`, `filereq`) and binary files (`rawed.obj`, fonts, mouse pointers, icon) | `bb2src.zip` → `tedt/` (as of 1995-09-14), from `amibl2src.lha` (Aminet `dev/basic`, uploader Bernd Rösch; sources by Acid Software, under GNU license according to the readme). `tedt/ted.s` is the `ted.s` of the archive with simple include paths instead of the Devpac assignment `e:` – already so in the archive. |

## Downloaded (not in git)

Third-party material without a clear license for redistribution – above
all the contributions of third parties on the CD – is not in the
repository. For the tests, `cmake -P cmake/fetch-originals.cmake` (or the
build target `fetch-originals`) fetches it; only this step needs the
network, the normal build does not. The downloads end up in
`.cache/downloads/` and are checked by their SHA-256.

| Directory | Contents | Download | SHA-256 |
|---|---|---|---|
| `cd/ultimate-blitz-basic-2.1/` | the CD "Ultimate Blitz Basic 2.1" (1997, Acid Software), 4310 files | <https://archive.org/download/ultimate-blitz-basic-v2_1/Ultimate%20Blitz%20Basic%202.1%20%281997%29%28Acid%20Software%29.iso> | `205a16fa83403d61a785733cec2518bcc1fe5a6fae5b7da4da6329bd32c99166` |
| `amiblitz3/svn-r279/` | single files of AmiBlitz3's SVN repository, revision 279 (2016-06-01, the AmiBlitz3 team), paths as there: `BlitzLibs/UserLibs/userlibs/RIAmosFuncLib.obj` – RIAmosFuncLib of the BSS2 release with fixes of the AmiBlitz3 team, the reference of the port of AMOS functions (`docs/rwelibs/README.md` §3.2) | <https://sourceforge.net/p/amiblitz3/code/279/tree/BlitzLibs/UserLibs/userlibs/RIAmosFuncLib.obj?format=raw> | `0863e9766957ee548cd7d9393e85d80c28a896a208d183487dbde5fe3aa45ea2` |
| `amiblitz3/github-b1a9dee/` | five AmiBlitz3 sources with an "Embedded .xtra Header", paths as there: `Sourcecodes/Amiblitz3/Tests/compilersignals.ab3`, `Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/LotanMemLib.ab3`, `Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/ElmoreDosLib.ab3`, `Docs/Blitzlibs/examples/RIAppLib3.ab3`, `Sourcecodes/Amiblitz3/Compiler/CLIModeSupport.ab3` – the test data of `tools/xtra` (ctest `xtra_ab3_examples`, `docs/formats.md` §7); Apache License 2.0, downloaded like the rest to keep test data out of git | AmiBlitz3 on GitHub, commit `b1a9dee150ef4d2793d9745a49a958fdb9178124`: `https://raw.githubusercontent.com/AmiBlitz/AmiBlitz3/<commit>/<path>` | in this order: `294459511eaeab48a9fb853ee383627323e58699b84ae528e29c492b87e74374`, `aea86f40bcc938448b487359807c596c934d9093e0124342c709735d5c635902`, `e431124dbb97010063dfa37c451208cc356f2a54185d466801a0fae4a12a59ce`, `8b2ecd015833b4a5d3e64fed7819d909788cc99183c56e568a6120f6afc3d5d7`, `cd2c926d71c4d6beeec5d6993feb20d8039477d2961ca0d669e3efa8746f8c15` |
| `amiblitz3/github-58a11658/` | single files of AmiBlitz3 v3.14.0 (the release the AB3 mode of `blitz2-ab3` is built against, `docs/ab3/variant.md`), paths as there: the binary BigLibs `BlitzLibs/baseLibs`, `BlitzLibs/utilLibs`, `BlitzLibs/OsLibs`, `BlitzLibs/csLibs`, the resident `BlitzLibs/Residents/all.res`, the compiler `Amiblitz3` (the reference of the AB3 tests), and the test programs `Sourcecodes/Amiblitz3/Tests/` `atoi.ab3`, `compilersignals.ab3`, `data_restore_in_function.ab3`, `exec_data.ab3`, `localization.ab3`, `fabrizio_speedquestion.ab3`, `memoryleak_test.ab3`, `setint.ab3`, `test_newtypes.ab3`, `trim.ab3`, `hex_error.ab3`, `hexdigits.ab3`, `int_problem.ab3`, `int_tests.ab3`, `jsr.ab3`, `macro_test.ab3`, `unknown_function.ab3`, `unknown_function_include.ab3`, `kick13.ab3`, `windowtags.ab3`, and the include `Sourcecodes/Includes/datetime.include.ab3`; Apache License 2.0 (the BigLibs contain third-party libraries under their own, often unstated licenses: only downloaded as AmiBlitz3 publishes them, never in git or in results of this repository) | AmiBlitz3 on GitHub, commit `58a1165840a7be19d408e727d2fe444c29da91b3` (tag v3.14.0): `https://raw.githubusercontent.com/AmiBlitz/AmiBlitz3/<commit>/<path>` | in this order: `23ac17b06b60916076586cb56a4c391b8137ef107a1ee3a9bfca6680a4030e87`, `9b0f6d2c8cc95785b1cbc2e1d590f4bbf52a15d6d71bb0e2581c963268a2c04f`, `e7eab41bebfb456ecd88464948e5273c66b1e79dcd5a9740868120506fa83a4e`, `940ea4fc66656df12f73c0566d68e7f463e5521ba455eeb4ef94806eb63fd037`, `87ce951e1fd25984f110c9f6158084b1685d4eaf1dae6619efcc4b854655dc70`, `4120e8ecae94d2489899175d0758e6f1481e24196ee6c7e922d88e7b2bb51787`, `cb0c4f3df66d6b12ca679c0f90a72d212292f54ddc72ee7b6336c48e0e39d121`, `294459511eaeab48a9fb853ee383627323e58699b84ae528e29c492b87e74374`, `a6c2575ccc664efd904b016e1deb30d0d047aacc831129eb956cf661d299a824`, `e059325a2fe72346f1bc51610acab238950231e5509c5c9bc9ac1d2b33bae802`, `680c91ea170ff0aa3c888cf55fdfddf1c94cb488ecbe71b4cfce19eecfb0eca4`, `7b310c6a88d28f10cc67485b2f038026b6c8428ce2aea6d2dc46dd5fdd1d7f7d`, `55575c695af83140801dc609ecd136a20645efba467893ce05f53a087ecc08a9`, `bf120eaad18b23b4e0b8f450826d460e22dfdac9d1a1ef64bb04b364d8f2205a`, `a40aafb02addd7534e64506329f066f021f65dc88d9d4ab3af444b737ae66873`, `f71b479bca8a0a3ec613d17fa549d71076ca3bd04bf4de5274c96784d1cf3ecd`, `3c08a8db340365e0fc9a0a3b39f0db05d89efb29bb410d567232d5a17d2da508`, `b8800234f7c2f5683d4b8dda352f272067a761e6d437aa4c922b34765c96c223`, `addd87e4454c4ea7ef06e82d47dbedc6763dcb83f2e27efd6b27e03d31324a65`, `6453ac0aa8e6c0518898dad6774cea63d485495ce8f0666b8f8ca61625f62133`, `a7e39e00557fb2d374b2b6c20b9b42f2d8bba3dc4d90286f710bb9c4befae4cb`, `f8aa4c99c6bfd5783ec73bd9312c8bfb35d817166ae5797f1efebaa3e7dd11fa`, `7a12a957665f3168f7da33cda38691668c59f772ffd0325c1f6bfa15c27ba24d`, `e5ab24146a409e01d63625d8fbce805e61a8755d2b76dd05a89ce1b8aef8b9ed`, `4ee0f1d98eaa505679da143b85c5b53211f6abdcdcf8495040009a4052ffdb3a`, `994f756fab869fcdf976ffe9dd18fbb42af857e809de5c99944fe40844dc38cb`, `2c421f2f596275b4298d7c9319fd90373b2af747525dc71d741a838bac720c8b` |
| `aminet/amibl2src/` | `amibl2src.lha`, in it `bb2src.zip` unpacked to `bb2src/` and `bblibsrc.lha` (Blitz sources of the libraries, tokenized, 209 files) to `bblibsrc/`, `RWELibSourc.lha` (Blitz sources of the libraries Red When Excited released in September 1999, tokenized, 287 files) to `rwelibsrc/` | <https://aminet.net/dev/blitz/amibl2src.lha> | `b71e6445d0838b15416f5b55c5be422f07bd4fae5da5c065ce329042067628ff` |

* The CD has only ISO 9660 names (no Rock Ridge, no Joliet), hence upper
  case – they stay that way. The Linux kernel shows them in lower case
  when mounting (`map=normal`); the copy in bb2 therefore has lower-case
  names. The only change: the three names with Latin-1 characters become
  UTF-8 (`BUI6/BLITZBANK/CATALOGS/FRANçAIS`,
  `BUI6/FRONTENDV1.0/CATALOGS/ESPAñOL`,
  `BUI7/FRONTENDV1.2/CATALOGS/ESPAñOL`).
* The script checks that `sources/ted-1.1/` and `sources/blitz2-1.95/`
  correspond exactly to the files in `bb2src.zip` of the Aminet archive.
* `bblibsrc/` is the contents of the directory `bblibsrc/` of
  `bblibsrc.lha` (own marker `bblibsrc.sha256`, so it is also caught up in
  an archive that is already unpacked). The build target `bblibsrc` makes
  it readable as text (`build/<preset>/bblibsrc/NAME`, README, section
  "Readable library sources").
* `rwelibsrc/` is the contents of the directory `Libs/` of
  `RWELibSourc.lha` (marker `rwelibsrc.sha256`, as `bblibsrc/`): the
  library sources Red When Excited released in September 1999
  (`amibl2src.readme`: "RWElibsource this is part what RWE release on
  September 1999"). About 45 libraries, many in several versions; by
  their headers mostly Reflective Images (1994–1996), Leading Edge
  Software, Red When Excited (Registry, LVCallHook) and a few others
  (section "License situation"); not part of blitz2 2.10 and its
  acidlibs. They are to be ported as libraries of their own (userlibs),
  next to the 2.10 state. The token list of blitz2
  1.95 (`tests/data/ted-tokens.txt`) does not know all of their tokens
  (23 of the 123 sources).

## License situation of the originals

As of 2026-10-28, after research:

* **Sources of Blitz Basic 2 and TED** (`bb2src.zip` in
  `amibl2src.lha`): `sources/amibl2src.readme` says "Thanks to Mark Sibly
  and Simon Armstrong and RWE that they give the Source and allow to
  enhance my loved Blitzbasic2" and "The Source is under GNU see Acid
  Homepage at www.acid.co.nz" – without a version number and without a
  license text in the archive. At that time only the GPL 2 existed; if a
  program names no version, GPL 2 § 9 allows choosing any published
  version. The Acid homepage can no longer be reached.
* **AmiBlitz3**, the later further development of the same code line, is
  on SourceForge under the Apache License 2.0
  (<https://sourceforge.net/projects/amiblitz3/>: "Acid Soft was so
  friendly to release the BB2 source as Opensource."). The header of the
  compiler source there (`Sourcecodes/Amiblitz3/Compiler/AmiBlitz3.ab3`)
  says: „Auf Bestreben von Bernd Roesch durfte die Amiga-Version
  disassembliert und weiterentwickelt werden.“ (At Bernd Roesch's
  instigation, the Amiga version was allowed to be disassembled and
  developed further.) When and on what basis the switch to Apache took
  place can no longer be traced. **In the code repository itself** (SVN,
  converted to git, 602 commits from 2009-02-10) there is neither a
  license file nor a commit about a license change; `Docs/Amiblitz3.guide`,
  section "Program Licenses", says unchanged from the first commit to the
  latest state: "The "AmiBlitz3" and "PED" programs are relased under the
  GNU General Public License […] either version 2 of the License, or (at
  your own option) any later version.", "The libraries and additional
  tools are released under their own original or updated licenses." and
  "No-one is claiming any intellectual or any other copyright over
  software developed in AmiBlitz3. You are free to release anything you
  create in AmiBlitz3 under any license you wish (at the time of writing
  there are no GPL licensed libraries)." The Apache statement is thus only
  in the project details on SourceForge. This project builds on the GPL
  state; the last statement corresponds in substance to its runtime
  exception.
* **Library sources of Red When Excited** (`RWELibSourc.lha` in the same
  Aminet archive): released by RWE in September 1999 together with the
  Blitz 2 sources (`amibl2src.readme`: "Red When Excited (RWE) have
  released their Blitz2 Source(incl. Superted) on their Homepage on
  September 1999"); the readme's thanks to RWE and its GNU statement
  cover them as they cover blitz2 and TED. Decision of 2026-10-29: taken
  over on this basis (RWE had taken over Blitz Basic 2 and allowed the
  release). Contained code of others: the Protracker 2.3A play routine in
  `ptlib/` (see below), `RTLib.bb` (the ReqTools interface by Neil
  O'Rourke, 1994).
* **The Protracker 2.3A play routine** (in `ptlib/ptlib*.bb` and
  `ptlib/CIATrackerLib1.bb` of the RWE release, by way of Neil
  O'Rourke's library): by Peter "Crayon" Hanning / Mushroom Studios
  (header of `PT-CIAPlay.s`: "Protracker V2.1A Playroutine, Peter
  "CRAYON" Hanning / Mushroom Studios 1992"). The ReadMe of the
  Protracker 2.3A disk (`PT2.3.ReadMe!`, 1993-01-01; copy in
  https://github.com/cmatsuoka/tracker-history,
  `reference/amiga/protracker/`): "Protracker V2.3 Release A - (C) 1993
  Mushroom Studios/Noxious … Protracker is a Public Domain program, and
  may be copied freely and included in any PD library. You may not
  charge anything for the program other than the cost of the disk and
  the PD-service." The disk carries the play routines `PT-Play.s` and
  `PT-CIAPlay.s` for use in other programs ("You can use this routine
  to play a module"); the condition on charging names "the program".
  The routine was shipped in commercial products of the time (Blitz
  libraries of BUM and the Blitz Support Suite, many games) without
  objection. Decision of 2026-10-30: the libraries with the routine
  (ptlib, CIA tracker) are ported on this basis. No code is taken over
  from later rewrites (ProTracker 2.3F by Olav Sørensen, BSD-3-Clause:
  its changelog serves only as a list of the routine's known bugs).
* **The FNS font "Personal"** (`amiblitz3-github/…/PERSONAL.FNS`): not in
  RWE's 1999 release (the source only `IncBin`s it); part of the FNS
  library's binary and its examples on the CD and published by AmiBlitz3
  in its repository (Apache License 2.0) – in git on that basis (decision
  of 2026-10-29).
* **The font of the LES debug library**
  (`libs/userlibs/rwe/rilesdebuglib-font.bin`, 2048 bytes: 256
  characters of 8 rows; derived data, so not here but next to its user):
  both LES debug sources of RWE's release (`rwelibsrc/Debuglib/` and
  `LESDebuglib/LESDEBUGlib.bb`) only `IncBin "Blitz2:Debugger/Font.bin"`,
  a file that exists nowhere. The bytes are cut from AmiBlitz3's
  `BlitzLibs/Decompiled/thirdpartylibs/RILESDEBUGLib.obj` on GitHub
  (<https://github.com/AmiBlitz/AmiBlitz3>, commit
  `b1a9dee150ef4d2793d9745a49a958fdb9178124`, file last changed in
  `9d854a532a39`; 11632 bytes, the same as `RILESDEBUGLib.obj` of
  AmiBlitz3's SVN revision 279), file offset 4566 – the place of the
  IncBin, between the code of `draw_char` and `_GetHexAdr`. The same
  bytes are the commented `Dc.l`/`Ds.l` lines of AmiBlitz3's disassembly
  `Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/rilesdebuglib.ab3` (same
  commit, file last changed in `3c24a4c3d161`) and are in the CD's
  `BUM9B`/`BUM10A` `BLITZLIBS/RILIBS/RILESDEBUGLIB.OBJ` at offset 3460
  (ctest `rwe_lesfont_cd` compares them). The repository is under the
  Apache License 2.0. Decision of 2026-10-30: only the font is taken
  over, not the library's binary.
* **Library manuals** of the time carry the note "Used with the
  permission of Acid Software" (e.g. AmiBlitz3
  `Docs/AcidLibs/SORTLIB.guide`).
* **Rights holders:** Acid Software and Red When Excited no longer exist;
  no body that could decide bindingly on the license of the originals is
  known. Copyrights do not expire because of this – that is why this
  project builds on the documented GNU release.

Rules:

* Change nothing here. A new state is a new directory with a version
  number (`ted-1.1`, later e.g. `blitz2-1.95`).
* Every directory is listed in this table with its origin.
* Git treats everything here as binary (no line ending conversion);
  assembler sources get diffs.
