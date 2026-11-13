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
; @fixed #1002
; @pass file=pal.ilbm|@TESTDATA@/palettelib-crng-flags.ilbm
DEFTYPE .l
Dim m.l(64)
If ReadFile(0,"CORPUS:pal.ilbm")
  ReadMem 0,&m(0),256
  CloseFile 0
EndIf
DecodePalette 0,&m(0)
pn=0 : Gosub cdump
LoadPalette 1,"CORPUS:pal.ilbm"
pn=1 : Gosub cdump
End

cdump:
  o=Addr Palette(pn)
  For k=0 To 2
    NPrint "pal ",pn," cycle ",k," ",Peek.w(o+6+k*8)," ",Peek.w(o+8+k*8)," ",Peek.w(o+10+k*8)
  Next
Return
