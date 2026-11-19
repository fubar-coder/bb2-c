; ribbmaplib-getpalette-low-gt-high (#549) - the store loop of
; BBGetPalette tests its end only after a colour, so it always writes the
; colour at low; whether a new block is needed is decided from high
; alone.  The scenario (built in memory) has a COLR chunk with low 20 and
; high 10; palette 0 has 16 colours: colour 20 is written behind the
; colour block.  The program runs through inject (runwrap=, inject
; guard: a pattern behind every allocation, checked when it is freed and
; at the end).
; Correct: nothing is stored (the range is empty), nothing is written
; behind an allocation, the palette keeps its 16 colours.
; @fixed #549
; @rwe ribbmaplib
; @pass runwrap=@INJECT@|guard
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,18 : Poke.l m+8,$5343454E
Poke.l m+12,$434F4C52 : Poke.l m+16,6 : Poke.b m+20,20 : Poke.b m+21,10
Poke.b m+22,$11 : Poke.b m+23,$22 : Poke.b m+24,$33
InitPalette 0,16
NPrint SetBBScenarioLoc(m)
BBGetPalette 0
NPrint "count ",Peek.w(Addr Palette(0)+4)
FreeMem_ m,64
End
