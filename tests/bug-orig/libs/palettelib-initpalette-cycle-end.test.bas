; palettelib-initpalette-cycle-end (#229) - InitPalette gives a palette
; new colour data but writes no end mark (low -1) into its cycle table
; (+6, 14 entries of 8 bytes): in a palette object that was never used
; the 14 entries are 0 and have no end, so CyclePalette (and Cycle) run
; behind them into the next palette object and take its data as cycles.
; Here: InitPalette 1,6 on a fresh palette 1 with colours 1 and 2 set;
; palette 2 (the next object, 128 bytes on) gets a cycle of colours 1-2
; and its end mark by SetCycle on the empty palette.  The original's
; CyclePalette 1 runs into palette 2's cycle: it rotates colours 1 and 2
; of palette 1 and changes the cycle's var in palette 2.
; Correct: the cycle table of palette 1 ends at once (low -1 at +6),
; CyclePalette 1 changes nothing.
DEFTYPE .l

; the byte at a as two hex digits
Function$ hb{a.l}
  Function Return Right$(Hex$(Peek.b(a) AND 255),2)
End Function

; the three components of colour i of the palette data d as hex bytes
Function$ rgbhex{d.l,i.l}
  e=d+4+i*12
  Function Return hb{e}+" "+hb{e+4}+" "+hb{e+8}
End Function

!TestCase{cycle_end}
  InitPalette 1,6
  PalRGB 1,1,15,0,0 : PalRGB 1,2,0,15,0
  SetCycle 2,0,1,2
  o=Addr Palette(1) : q=Addr Palette(2)
  !CheckEq{q-o,128,"next_object"}
  !CheckEq{Peek.w(o+6),-1,"end_mark"}
  CyclePalette 1
  d=Peek.l(o)
  !CheckStrEq{rgbhex{d,1},"FF 00 00","colour_1"}
  !CheckStrEq{rgbhex{d,2},"00 FF 00","colour_2"}
  !CheckEq{Peek.w(q+12),0,"palette_2_var"}
!EndTestCase
