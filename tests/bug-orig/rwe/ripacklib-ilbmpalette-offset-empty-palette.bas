; ripacklib-ilbmpalette-offset-empty-palette (#991) - ILBMPalette(source,
; palette#,offset) on a palette that has no colour block yet goes to
; reserve_block without killmem, the only place that resets the offset:
; the block is allocated for the CMAP's colours (12 bytes each + 16), but
; palbig still adds offset * 12 to the destination and writes all the
; colours from there.  (#769 is the other case, a block that is too
; small.)  The ILBM (built in memory) has a CMAP of 4 colours: the block
; is 4 * 12 + 16 = 64 bytes, the colours are written at offset 4 from
; 4 + 48 = 52 to 100: 36 bytes behind the block.  The program runs
; through inject (runwrap=, inject guard: a pattern behind every
; allocation, checked when it is freed and at the end).
; Correct: nothing is written behind an allocation.
; ripacklib needs RIAmosFuncLib (LoadIFF calls its NextBank, BLoad, ...).
; @rwe ripacklib riamosfunclib
; @pass runwrap=@INJECT@|guard
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,32 : Poke.l m+8,$494C424D
Poke.l m+12,$434D4150 : Poke.l m+16,12
For i=0 To 11 : Poke.b m+20+i,i+1 : Next
r=ILBMPalette(m,0,4)
NPrint "result ",r
FreeMem_ m,64
End
