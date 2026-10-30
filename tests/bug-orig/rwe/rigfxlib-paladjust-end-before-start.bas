; rigfxlib-paladjust-end-before-start (#684) - PalAdjust computes the
; count end - start + 1 as a word, * 3 (MULU.w) - 1 as the DBRA counter:
; end = start - 1 gives 0 channels and the counter $FFFF, 65536 channels
; (one byte in every long, 256 KB) are written from colour `start` on.
; Made safe: the colour data of palette 1 (source through PaletteInfo
; and target) are temporarily a block of the program's own (300000
; bytes); a marker byte $FF lies on the 1000th channel behind colour 5,
; ratio 0.5 halves what is written there.
; Correct: end < start adjusts nothing, the marker stays $FF.
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
DEFTYPE .l
InitPalette 1,10
b=AllocMem_(300000,$10001)
If b=0 Then NPrint "no memory" : End
Poke.w b,10
Poke.b b+64+4000,$FF
q=Addr Palette(1) : o=Peek.l(q) : Poke.l q,b
PaletteInfo 1
PalAdjust 1,0.5,5,4
v=Peek.b(b+64+4000) AND $FF
Poke.l q,o
FreeMem_ b,300000
If v=$FF Then NPrint "marker kept" Else NPrint "marker overwritten"
End
