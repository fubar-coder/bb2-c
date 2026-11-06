# AmiBlitz3: what it changed relative to Blitz Basic 2.10

AmiBlitz3 (GitHub `AmiBlitz/AmiBlitz3`, before 2016 SourceForge SVN
`amiblitz3`) is the later development of the same code line. These notes
record what it changed in the parts this repository ports, as the basis
for deciding which fixes of original bugs the port takes over. TED and
AmiBlitz3's editors (PED, IDE) are out of scope.

| File | Part |
|---|---|
| [compiler.md](compiler.md) | the compiler (`blitz2`) |
| [deflibs.md](deflibs.md) | the AmigaOS call libraries (`deflibs`) |
| [acidlibs-a-l.md](acidlibs-a-l.md), [acidlibs-m-z.md](acidlibs-m-z.md) | the command libraries (`acidlibs`) |
| [userlibs.md](userlibs.md) | the RWE and third-party userlibs (`libs/userlibs/rwe`) |
| [usage.md](usage.md) | `blitz2-ab3` in use: the command line, the modes, `ab3cli`, the tests |
| [variant.md](variant.md) | plan: compiling AmiBlitz3 sources with AmiBlitz3's libraries (not decided) |

## Sources

* AmiBlitz3's git history, 2009-02-10 to 2026-10-25
  (`AmiBlitz/AmiBlitz3@ea8c25c7` … `AmiBlitz/AmiBlitz3@b1a9dee1`). Its
  first 602 commits (to `AmiBlitz/AmiBlitz3@c95cce17`, 2016-06-01) are the
  SVN history; the SVN revision numbers come from a git-svn import of the
  same history (identical trees).
* The history starts with AmiBlitz 2.x (Bernd Roesch, 1999–2008): the
  first commit already carries all changes made before 2009. They are
  dated and explained by AmiBlitz's own changelogs and releases before
  git (`archive/README.md`):
  * `Docs/Changelog.guide` of AmiBlitz3 has the history 2.2–2.3 and the
    entries from 2.50 on ("following changes were made to the last
    official AmiBlitz2 Version 2.44"); the same text for 2.2–2.27 is
    `archive/releases/amiblitz2-2.42/Amiblitz2/amiblitzHistory.txt`,
    and the beta 4 copy of 2007
    (`archive/releases/amiblitz3-3.00-beta4/AmiBlitz3/Docs/Changelog.guide`)
    has 3.00.09–3.00.20.
  * The gap 2.4–2.45 (2003–2006) is in Bernd's news files:
    `archive/releases/amiblitz2-2.42/Amiblitz2/news2.4.txt`,
    `news2.41b1.txt`, `news2.42.txt` (2.4–2.42),
    `archive/releases/amiblitz2-2.43/amiblitz2.43/news2.43.txt`
    (2.41–2.43b40, 2004–2005) and
    `archive/releases/amiblitz2-2.45b2/amiblitz2.45/news2.45.txt`
    (2.44b2–2.45b2, 2006).
  * The compiler sources 2.41 (2004), 2.43b12 (2005), 2.50beta,
    2.517beta (2006) and 3.00.19a (2007) are in `archive/sources/`
    (tokenised); the 2.50/2.51 reasons are in their header comments.
  * The AcidLibs and DefLibs of the releases 2.42 (2004-08), 2.43b2
    (2004-10), 2.43 (2005-07), 3.00 beta 4 (2007-05) and Bernd's last
    AcidLibs (2007-12) are fetched to `archive/aminet/ab2.42-distri/`
    and `archive/bernd/`; comparing their libraries with those of the
    first commit gives the release by which a library had its 2009
    state ([acidlibs-a-l.md](acidlibs-a-l.md#dating-the-pre-git-changes)).

## Classes

Every change is one of: **fix** (of an original bug; our issue if one
matches), **extension**, **incompatible** (changed declarations, token or
library numbers, behaviour, or needs a newer OS/CPU/FPU or AmiBlitz3's
own compiler), **cosmetic** (only counted). Uncertain mappings are marked
"(?)". References to AmiBlitz3 commits and issues are in code spans
(`AmiBlitz/AmiBlitz3@hash`, `AmiBlitz/AmiBlitz3#N`).

## Result (2026-10-31)

AmiBlitz3 fixed few of the original bugs this repository has issues for;
most of its work are extensions and changes that break the token and
library numbers of BB2 2.10, so they are not candidates for the default
variant. Fixes that match our issues:

* compiler: #943 (several `HUNK_RELOC32` blocks), #38 partly (the lock of
  `IncBin`), #958 (A4/A5 saved around OS calls that take parameters there);
* deflibs: #44 (library init without the endless loop),
  #959 (exec `CachePreDMA_`/`CachePostDMA_` take `flags` in d0); #45 is
  not fixed in AmiBlitz3;
* acidlibs: #148 (fontlib), #120 (datelib), #42 partly (environlib),
  #174 (gtlib), windowslib #317, #320, #322, #329 (and #318 partly);
  uncertain: #59, #168, #169, #173, #175, #224, #250, #251, #327;
* userlibs: none beyond the fixes already in the reference (RIAmosFuncLib
  of SVN r279).

## Taken over (2026-10-31)

Fixed in this repository's default variant, BB2-compatibly (kind F in
`docs/libs/deviations.md`, each with a bug-orig test that comes out
FIXED): compiler #38, #42, #943, #958; deflibs #44, #45 (not fixed in
AmiBlitz3), #959; acidlibs #59, #120, #148, #168, #169, #173, #174,
#224, #250, #317, #318, #320, #322, #327. Left open: #175 (which gadget
of an Id is right is unclear). Found on the way: #960, #961, #962.
