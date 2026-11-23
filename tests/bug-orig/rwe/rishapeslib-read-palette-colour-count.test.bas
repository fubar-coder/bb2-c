; rishapeslib-read-palette-colour-count (#820) - read_palette takes the
; number of colours as CMAP size / 3 and decodes them with a DBRA
; counter: a CMAP of less than 3 bytes gives 0 colours, an allocation of 8
; bytes and 65536 colours decoded into it (768 KB written).  The LE file
; (generated) has a CMAP of 2 bytes; it is loaded with a palette.  The
; program runs through inject (runwrap=, inject guard: a pattern behind
; every allocation, checked when it is freed; the 768 KB land in free
; fast memory here, without a crash).
; Correct: no colours, nothing decoded - nothing written behind an
; allocation.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{colour_count}
  LELoadShapes 0,0,"CORPUS:s.le",0
  !Check{1,"after"}
!EndTestCase
