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
| `sources/ab2.42-distri.readme` | the readme of the Aminet archive `ab2.42-distri.lha` (AmiBlitz 2.42) – evidence of the licence of AmiBlitz 2.x and of the update location `www.david-mcminn.co.uk/bernd` | Aminet `dev/blitz/ab2.42-distri.readme` |
| `sources/amiblitz2-2.41/` (in git) | the compiler AmiBlitz 2.41 (`amiblitz2.bb2`, 2004-05-15, `vers "2.4 "`) and the editor PED 2.41 (`ped.bb2`, 2004-06-09) with their `.xtra` files; tokenised Blitz sources (`docs/formats.md` §3), comments readable as text | `amiblitz2.41src.lha` of Bernd Rösch's archive (below), all files, as unpacked |
| `sources/amiblitz2-2.43b12/` (in git) | the compiler of AmiBlitz 2.43 (`amiblitz2.bb2`, 2005-07-23, `vers "2.43b12 "`), PED 2.43b31 (`ped.bb2`, 2005-07-18), `.xtra` files | `ab2.43src.lha` of Bernd Rösch's archive, all files |
| `sources/amiblitz2-2.50beta/` (in git) | the compiler 2.50beta (`Blitz2/Developer/amiblitz2/amiblitz2_V251.bb2`, 2006-10-18, Sven Dröge; the header comment lists the changes of 2.50/2.51), its GUI (`compiler_gui.bb2`, `compiler.wizard`) and the LVO include `library_offsets.bb2` | `ab2.5beta/source/amiblitz2_V251_source.lha` and `ab2.5beta/source/library_offsets.lha` of Bernd Rösch's archive, all files, paths as in the archives |
| `sources/amiblitz2-2.517beta/` (in git) | the compiler 2.517beta (`Blitz2/Quelltexte/amiblitz2/amiblitz2_V2517.bb2`, 2006-11-11); it includes `dos.include.bb2` (in `archive/bernd/ab3/Source/pedsrc_v30030/`) and `library_offsets.bb2` | `ab2.5beta/source/amiblitz2_V2517_source.lha` of Bernd Rösch's archive, all files |
| `sources/amiblitz3-3.00.19a/` (in git) | the compiler AmiBlitz3 3.00.19a (`amiblitz3_V30019a.bb2`, 2007-07-31), the last source before AmiBlitz3's version control (2009) | `ab3/Source/ab3scr_30019.lha` of Bernd Rösch's archive, all files |
| `releases/amiblitz2-2.42/Amiblitz2/` (in git) | Bernd Rösch's changelogs `news2.4.txt` (2003-11-29), `news2.41b1.txt` (2004-02-12), `news2.42.txt` (2004-08-14) and `amiblitzHistory.txt` (2.2–2.27) | `ab2.42-distri.lha` (Aminet, below), single files, paths as there |
| `releases/amiblitz2-2.43b2/amiblitz2.43/news2.43b2.txt` (in git) | changelog 2.41–2.43b2 (2004-10-27) | `ab2.43b2_upd.lha` of Bernd Rösch's archive |
| `releases/amiblitz2-2.43/amiblitz2.43/news2.43.txt` (in git) | changelog 2.41–2.43b40 (2005-07-26) | `ab2.42_upd_to_2.43.lha` of Bernd Rösch's archive |
| `releases/amiblitz2-2.45b2/amiblitz2.45/news2.45.txt` (in git) | changelog 2.44b2–2.45b2 (2006-07-09), followed by the 2.43 one | `ab2.45betadir/ab2.45b2.lha` of Bernd Rösch's archive |
| `releases/amiblitz3-3.00-beta4/AmiBlitz3/Docs/Changelog.guide` (in git) | AmiBlitz3's changelog as of 3.00.20 (Sven Dröge, `$VER` 2007-01-30): compiler 2.50–3.00.20, PED, debugger, the history 2.2–2.3 | `ab3/AmiBlitz3full_beta4.lha` (2007-08-14) of Bernd Rösch's archive |
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
| `amiblitz3/github-58a11658/` | single files of AmiBlitz3 v3.14.0 (the release the AB3 mode of `blitz2-ab3` is built against, `docs/ab3/variant.md`), paths as there: the binary BigLibs `BlitzLibs/baseLibs`, `BlitzLibs/utilLibs`, `BlitzLibs/OsLibs`, `BlitzLibs/csLibs`, the resident `BlitzLibs/Residents/all.res`, the compiler `Amiblitz3` (the reference of the AB3 tests), and the test programs `Sourcecodes/Amiblitz3/Tests/` `atoi.ab3`, `compilersignals.ab3`, `data_restore_in_function.ab3`, `exec_data.ab3`, `localization.ab3`, `fabrizio_speedquestion.ab3`, `memoryleak_test.ab3`, `setint.ab3`, `test_newtypes.ab3`, `trim.ab3`, `hex_error.ab3`, `hexdigits.ab3`, `int_problem.ab3`, `int_tests.ab3`, `jsr.ab3`, `macro_test.ab3`, `unknown_function.ab3`, `unknown_function_include.ab3`, `kick13.ab3`, `windowtags.ab3`, `function_problem.ab3`, `serial_read.ab3`, and the includes `Sourcecodes/Includes/` `datetime.include.ab3`, `longlong.include.ab3`, `error.include.ab3`, `serial_device.include.ab3`; Apache License 2.0 (the BigLibs contain third-party libraries under their own, often unstated licenses: only downloaded as AmiBlitz3 publishes them, never in git or in results of this repository) | AmiBlitz3 on GitHub, commit `58a1165840a7be19d408e727d2fe444c29da91b3` (tag v3.14.0): `https://raw.githubusercontent.com/AmiBlitz/AmiBlitz3/<commit>/<path>` | in this order: `23ac17b06b60916076586cb56a4c391b8137ef107a1ee3a9bfca6680a4030e87`, `9b0f6d2c8cc95785b1cbc2e1d590f4bbf52a15d6d71bb0e2581c963268a2c04f`, `e7eab41bebfb456ecd88464948e5273c66b1e79dcd5a9740868120506fa83a4e`, `940ea4fc66656df12f73c0566d68e7f463e5521ba455eeb4ef94806eb63fd037`, `87ce951e1fd25984f110c9f6158084b1685d4eaf1dae6619efcc4b854655dc70`, `4120e8ecae94d2489899175d0758e6f1481e24196ee6c7e922d88e7b2bb51787`, `cb0c4f3df66d6b12ca679c0f90a72d212292f54ddc72ee7b6336c48e0e39d121`, `294459511eaeab48a9fb853ee383627323e58699b84ae528e29c492b87e74374`, `a6c2575ccc664efd904b016e1deb30d0d047aacc831129eb956cf661d299a824`, `e059325a2fe72346f1bc51610acab238950231e5509c5c9bc9ac1d2b33bae802`, `680c91ea170ff0aa3c888cf55fdfddf1c94cb488ecbe71b4cfce19eecfb0eca4`, `7b310c6a88d28f10cc67485b2f038026b6c8428ce2aea6d2dc46dd5fdd1d7f7d`, `55575c695af83140801dc609ecd136a20645efba467893ce05f53a087ecc08a9`, `bf120eaad18b23b4e0b8f450826d460e22dfdac9d1a1ef64bb04b364d8f2205a`, `a40aafb02addd7534e64506329f066f021f65dc88d9d4ab3af444b737ae66873`, `f71b479bca8a0a3ec613d17fa549d71076ca3bd04bf4de5274c96784d1cf3ecd`, `3c08a8db340365e0fc9a0a3b39f0db05d89efb29bb410d567232d5a17d2da508`, `b8800234f7c2f5683d4b8dda352f272067a761e6d437aa4c922b34765c96c223`, `addd87e4454c4ea7ef06e82d47dbedc6763dcb83f2e27efd6b27e03d31324a65`, `6453ac0aa8e6c0518898dad6774cea63d485495ce8f0666b8f8ca61625f62133`, `a7e39e00557fb2d374b2b6c20b9b42f2d8bba3dc4d90286f710bb9c4befae4cb`, `f8aa4c99c6bfd5783ec73bd9312c8bfb35d817166ae5797f1efebaa3e7dd11fa`, `7a12a957665f3168f7da33cda38691668c59f772ffd0325c1f6bfa15c27ba24d`, `e5ab24146a409e01d63625d8fbce805e61a8755d2b76dd05a89ce1b8aef8b9ed`, `4ee0f1d98eaa505679da143b85c5b53211f6abdcdcf8495040009a4052ffdb3a`, `994f756fab869fcdf976ffe9dd18fbb42af857e809de5c99944fe40844dc38cb`, `0b60e8d257c761a69aff04c4a0c541e9c423a1fbdfec0e813e1c71dea44cb171`, `1a198072c6f6e7fe43be08a5f20653de55aeef06dfa9ecd12b7d112de46b8821`, `2c421f2f596275b4298d7c9319fd90373b2af747525dc71d741a838bac720c8b`, `0e1c7c65ef1921d5766e2de6c306beac6dbba85e05a58ed8ca98963f76046a50`, `d2fab91bc78b37b8c752067f8a36cd5d9054cfcf2365e8ef845401086bfa14b8`, `d4b2005daf2adea40d146f76db759602ac9794ef6b35075c040c80e8ba03df04` |
| `aminet/ab2.42-distri/` | AmiBlitz 2.42, the full distribution (1119 files; uploader "Cj-Stroker", an Amiforce user distribution based on David McMinn's Blitz2000 one): compiler and PED of 2004-08, `AcidLibs` (2004-08-14), `Deflibs`, residents, userlibs, debugger, guides, examples, Thilo Köhler's includes; unpacked | <https://aminet.net/dev/blitz/ab2.42-distri.lha>, readme <https://aminet.net/dev/blitz/ab2.42-distri.readme> | `6f3993d1b6d911b90db9cf73a944f08d62a1576e6f8cf5deabe9abb9b3554dac`, readme `b515e098cd24f2f6b476068ccdf30107245d7fc69d71b9795df9a26a72f37707` |
| `bernd/` | Bernd Rösch's AmiBlitz 2 archive, paths as on the site, every LhA archive unpacked to a directory of its name without `.lha` (`pedsrc.bb2`, an LhA archive, to `pedsrc/`): the updates and releases 2.43b2 (2004-10), 2.42 → 2.43 (2005-07, with `AcidLibs`, `DefLibs`, `all.res` and its source, the single acidlibs as `decompiled/*.obj`, userlibs, `amigalibs/*.library1`), 2.43, 2.45b1–b3 (2006–2007), the 2.5 betas of compiler and PED (2006), AmiBlitz3 3.00.10–3.00.19 and PED 3.00.18–3.00.30 (2007), `ab3/AmiBlitz3full_beta4.lha` (AmiBlitz3 3.00 beta 4, 2007-08-14, the whole distribution), `acidlibs.lha` (AcidLibs 2007-12-30), `deflibs.lha` (2006-07), `all.res.lha` (2006-06), all sources of compiler and PED (2.40 … 3.00.30, with the wizard GUI files and includes) and the library number tables `lib_mapping.txt` | <https://www.david-mcminn.co.uk/bernd/> + path, the 64 files of `cmake/fetch-originals.cmake` (`bernd_files`) | per file in `cmake/fetch-originals.cmake` |
| `aminet/amibl2src/` | `amibl2src.lha`, in it `bb2src.zip` unpacked to `bb2src/` and `bblibsrc.lha` (Blitz sources of the libraries, tokenized, 209 files) to `bblibsrc/`, `RWELibSourc.lha` (Blitz sources of the libraries Red When Excited released in September 1999, tokenized, 287 files) to `rwelibsrc/` | <https://aminet.net/dev/blitz/amibl2src.lha> | `b71e6445d0838b15416f5b55c5be422f07bd4fae5da5c065ce329042067628ff` |

* The CD has only ISO 9660 names (no Rock Ridge, no Joliet), hence upper
  case – they stay that way. The Linux kernel shows them in lower case
  when mounting (`map=normal`); the copy in bb2 therefore has lower-case
  names. The only change: the three names with Latin-1 characters become
  UTF-8 (`BUI6/BLITZBANK/CATALOGS/FRANçAIS`,
  `BUI6/FRONTENDV1.0/CATALOGS/ESPAñOL`,
  `BUI7/FRONTENDV1.2/CATALOGS/ESPAñOL`).
* The script checks that `sources/ted-1.1/` and `sources/blitz2-1.95/`
  correspond exactly to the files in `bb2src.zip` of the Aminet archive,
  and that the AmiBlitz sources and changelogs in git
  (`sources/amiblitz2-*`, `sources/amiblitz3-*`,
  `sources/ab2.42-distri.readme`, `releases/`) are the files of the
  downloaded archives.
* Bernd Rösch's archive (`www.david-mcminn.co.uk/bernd`, newest file
  2008-02-03) is a private site, unchanged since 2008. If it is gone, the
  Wayback Machine has its files (snapshots of 2019-11-20):
  `https://web.archive.org/web/2019id_/http://www.david-mcminn.co.uk/bernd/<path>`
  (the `id_` form returns the unchanged file; the SHA-256 values are the
  same). Not fetched from the site: the manual and its derivatives
  (`blitz2_manual_english.lha` = the CD's `BLITZ2_MANUAL`, `blitzguide.lha`,
  `amiblitz_pdfguide.lha`, the German and Italian translations,
  `guigfx_renderguide.lha`, `NeilsReqtoolslib.GUIDE` – Acid's and RWE's
  texts without a licence), unrelated tools and media (`hd-rec_tools/`,
  `winuae_lame/`, `cpubench_1.2.lha`, `memperf.lha`,
  `game_prototype.lha`, `benchmarks/`, `rawkeytest`, `testsound/`,
  `screenshots_3d/`, `ped_palettes/`), `ab3/Source/ab3src_30014.lha`
  (the same file as `ab3/Exe/AB3_V30014.lha`, a binary, not a source)
  and `ab3/Data/Residents/` (the same files as the residents in
  `AmiBlitz3full_beta4.lha`).
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

As of 2026-11-06, after research:

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
* **AmiBlitz 2.4x–3.00 sources and changelogs** (`sources/amiblitz2-*`,
  `sources/amiblitz3-3.00.19a/`, `releases/`): the further development
  of the same GPL'd code by Bernd Rösch (2.2–2.45, 1999–2006) and Sven
  Dröge (2.50–3.00, 2006–2007). The archives themselves carry no licence
  file and the sources no licence header; the evidence is in the
  distributions of the time:
  * `sources/ab2.42-distri.readme` (Aminet, AmiBlitz 2.42, "Author:
    Acidsoft /Red When Excited /berndroesch1@compuserve.de"): "This is
    version 2.42 of AmiBlitz, BlitzBasic "gpl`ed"." and "*** Important,
    for updates check: http://www.david-mcminn.co.uk/bernd ***" – Bernd
    Rösch's archive, from which the sources come, is the update location
    the release names.
  * The guide of that distribution (`aminet/ab2.42-distri/Amiblitz2/
    Amiblitz2_E.guide`, `$VER: amiblitz2.guide 1.0 (03/09/2004)`, not in
    git), node "Program Licenses": "The "AmiBlitz2" and "PED" programs
    are relased under the GNU General Public License. AmiBlitz2 and PED
    are free software; you can redistribute them and/or modify them under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your own
    option) any later version." and "The libraries and additional tools
    are released under their own original or updated licenses."; node
    "Authors": "Bernd Roesch has updated the compiler and editor and his
    work has convinced Simon and Mark to allow the release of AmiBlitz2
    as freeware." The full GPL 2 text is in the guide.
  * AmiBlitz3 3.00 beta 4 (`bernd/ab3/AmiBlitz3full_beta4/AmiBlitz3/Docs/
    Amiblitz3.guide`, 2007-01-31, not in git), node "Program Licenses":
    the same text for "AmiBlitz3" and "PED" ("either version 2 of the
    License, or (at your own option) any later version") – the text
    AmiBlitz3's repository keeps from its first commit on (above).
  Decision of 2026-11-06: the sources and Bernd's changelogs are in git on
  this basis (GPL 2 or later). The binaries, libraries, residents and
  guides of these releases are only downloaded: they contain the
  libraries and additional tools "under their own original or updated
  licenses" and the Amiforce distribution restricts its own parts
  ("The parts of this distribution from Amiforce (e.g. translations) do
  not used without permission in other distributions.").
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
