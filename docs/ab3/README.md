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
| [variant.md](variant.md) | plan: compiling AmiBlitz3 sources with AmiBlitz3's libraries (not decided) |

## Sources

* AmiBlitz3's git history, 2009-02-10 to 2026-10-25
  (`AmiBlitz/AmiBlitz3@ea8c25c7` … `AmiBlitz/AmiBlitz3@b1a9dee1`). Its
  first 602 commits (to `AmiBlitz/AmiBlitz3@c95cce17`, 2016-06-01) are the
  SVN history; the SVN revision numbers come from a git-svn import of the
  same history (identical trees).
* The history starts with AmiBlitz 2.x (Bernd Roesch, 1999–2008): the
  first commit already carries all changes made before 2009. Those are
  known only from the difference to BB2 2.10 and from
  `Docs/Changelog.guide`.

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
