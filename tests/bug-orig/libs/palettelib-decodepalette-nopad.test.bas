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
DEFTYPE .l

; the byte at a as two hex digits
Function$ hb{a.l}
  Function Return Right$(Hex$(Peek.b(a) AND 255),2)
End Function

!TestCase{odd_name}
  Dim m.l(64)
  ok=ReadFile(0,"CORPUS:pal.ilbm")
  !Require{ok,"file_read"}
  ReadMem 0,&m(0),256
  CloseFile 0
  DecodePalette 0,&m(0)
  o=Addr Palette(0)
  p=Peek.l(o)+4+12
  !CheckStrEq{hb{p}+" "+hb{p+4}+" "+hb{p+8},"F0 80 10","colour_1"}
  !CheckStrEq{Str$(Peek.w(o+6))+" "+Str$(Peek.w(o+8))+" "+Str$(Peek.w(o+10)),"0 1 1000","cycle_0"}
!EndTestCase
