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
; @fixed #229
; @configs clib
DEFTYPE .l
InitPalette 1,6
PalRGB 1,1,15,0,0 : PalRGB 1,2,0,15,0
SetCycle 2,0,1,2
o=Addr Palette(1) : q=Addr Palette(2)
NPrint "next object ",q-o
NPrint "end ",Peek.w(o+6)
CyclePalette 1
d=Peek.l(o)
For i=1 To 2
  e=d+4+i*12
  NPrint i," ",Hex$(Peek.b(e) AND 255)," ",Hex$(Peek.b(e+4) AND 255)," ",Hex$(Peek.b(e+8) AND 255)
Next
NPrint "palette 2 var ",Peek.w(q+12)
End
