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
