; palettelib-crng-flags (#1002) - the flags word of an ILBM CRNG chunk
; has bit 0 RNG_ACTIVE (the range cycles) and bit 1 RNG_REVERSE (it
; cycles backwards); DecodePalette and LoadPalette only compare the word
; with 1: every other value enters the range as an active one with the
; rate negated, so a disabled range (flags 0 or 2) cycles backwards.
; The ILBM here (generated) has four CRNG: flags 0 (colours 0-1, rate
; 1000), 1 (1-2, 2000), 2 (2-3, 3000) and 3 (0-3, 4000); DecodePalette
; of it in memory and LoadPalette of the file, then the cycle entries of
; the palette object (+6, 8 bytes each: low, high, speed).
; Correct: only the active ranges are entered, 1-2 forward (2000) and
; 0-3 backwards (-4000), then the end of the list (low -1).
DEFTYPE .l

; the first two cycle entries (low high speed) and the low of the third
Function$ cycles{o.l}
  s$=""
  For k=0 To 1
    s$=s$+Str$(Peek.w(o+6+k*8))+" "+Str$(Peek.w(o+8+k*8))+" "+Str$(Peek.w(o+10+k*8))+", "
  Next
  Function Return s$+Str$(Peek.w(o+22))
End Function

!TestCase{cycle_flags}
  Dim m.l(64)
  ok=ReadFile(0,"CORPUS:pal.ilbm")
  !Require{ok,"file_read"}
  ReadMem 0,&m(0),256
  CloseFile 0
  DecodePalette 0,&m(0)
  !CheckStrEq{cycles{Addr Palette(0)},"1 2 2000, 0 3 -4000, -1","decodepalette"}
  LoadPalette 1,"CORPUS:pal.ilbm"
  !CheckStrEq{cycles{Addr Palette(1)},"1 2 2000, 0 3 -4000, -1","loadpalette"}
!EndTestCase
