; ribbmaplib-getpalette-count-high (#548) - BBGetPalette on a palette
; with fewer colours than the COLR chunk's high colour + 1 allocates a
; block of high + 1 colours but sets the count (in the palette object
; and in the block) to high: the last colour lies outside the palette,
; and every further BBGetPalette allocates a new block again.  The
; scenario (built in memory) has a COLR chunk of the colours 0 to 31;
; palette 0 has 16 colours.
; Correct: the count is 32 in the object and in the block, and a second
; BBGetPalette keeps the block.
; @fixed #548
; @rwe ribbmaplib
DEFTYPE .l
m=AllocMem_(128,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,110 : Poke.l m+8,$5343454E
Poke.l m+12,$434F4C52 : Poke.l m+16,98 : Poke.b m+20,0 : Poke.b m+21,31
For i=0 To 95 : Poke.b m+22+i,i : Next
InitPalette 0,16
NPrint SetBBScenarioLoc(m)
BBGetPalette 0
o=Addr Palette(0) : d=Peek.l(o)
NPrint "count ",Peek.w(o+4)," block ",Peek.w(d)
BBGetPalette 0
If Peek.l(o)=d Then NPrint "same block" Else NPrint "new block"
NPrint "count ",Peek.w(o+4)
FreeMem_ m,128
End
