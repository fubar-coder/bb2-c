; palettelib-make4bitrgbs-lowbyte (#224) - make4bitrgbs (CD binary) builds
; the 4 bit colours for LoadRGB4 from the LOW byte of each 32 bit
; component, but DecodePalette sets only the top byte: under Kickstart
; < 3.0 (agamode 0, ShowPalette with LoadRGB4) such colours are black.
; agamode comes from ExecVersion when palettelib starts.  Here the case
; sets the version of exec.library to 37, starts a child
; (palettelib-make4bitrgbs-lowbyte.test/child.bas) and puts the version
; back.  The child opens a screen, decodes a CMAP (bytes i*21+5) and shows
; it; Red/Green/Blue (GetRGB4) of colours 1-3 are 0 in the original.
; Correct: the top 4 bits of each component: 4,5,6 8,9,10 12,13,14.
DEFTYPE .l

!TestCase{agamode_zero}
  e=Peek.l(4) : v=Peek.w(e+20)
  Poke.w e+20,37
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  Poke.w e+20,v
  !CheckEq{Peek.w(e+20),v,"version_back"}
  !CheckStrEq{*r\printed,"exec 37"+Chr$(10)+"cols 4,5,6 8,9,10 12,13,14"+Chr$(10),"printed"}
!EndTestCase
