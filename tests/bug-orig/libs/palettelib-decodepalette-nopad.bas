; palettelib-decodepalette-nopad (#225) - DecodePalette walks the chunks
; in memory by their length without the pad byte of an odd length.  The
; ILBM here (generated, read into memory) has a NAME of 3 bytes behind the
; CMAP, then a "(c) " chunk and the CRNG: the search for CRNG goes on one
; byte too early, reads the "(c) " chunk shifted (length $20xxxxxx,
; behind the end of the FORM) and stops - the palette gets no colour
; cycle.  (A NAME before the CMAP would end in the runtime error "Can't
; find CMAP", whose code is not there without the debugger, #232.)
; Correct: the pad byte is skipped, the CRNG is found - cycle 0 of the
; palette object (+6: low, high, speed) is 0 1 1000.
; @configs clib
; @pass file=pal.ilbm|@TESTDATA@/palettelib-decodepalette-nopad.ilbm
DEFTYPE .l
Dim m.l(64)
If ReadFile(0,"CORPUS:pal.ilbm")
  ReadMem 0,&m(0),256
  CloseFile 0
EndIf
DecodePalette 0,&m(0)
o=Addr Palette(0)
p=Peek.l(o)+4+12
NPrint "colour 1 ",Hex$(Peek.b(p) AND 255)," ",Hex$(Peek.b(p+4) AND 255)," ",Hex$(Peek.b(p+8) AND 255)
NPrint "cycle ",Peek.w(o+6)," ",Peek.w(o+8)," ",Peek.w(o+10)
End
