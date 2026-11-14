; polylib-dither-plane-step (#1035) - the dither form of BitMapPoly blits
; all planes of a line at once; B (the dither bitmap) gets the modulo
; mod + negmod with negmod = target plane distance - dither plane
; distance, so per plane B advances by 2 x target distance - dither
; distance instead of the dither bitmap's own plane distance.  With a
; dither bitmap whose planes are smaller than the target's, planes 1 and
; up read behind the dither bitmap.
; Here a target of 64 x 16 pixels, 2 planes (plane distance 128), and a
; dither bitmap of 64 x 8, 2 planes (distance 64), made with CludgeBitMap
; in a block of the program: its plane 0 is $AA bytes, its plane 1 $55,
; the rest of the block behind it $F0 (the original reads plane 1 from
; offset 192 of the block).  BitMapPoly fills the whole target with
; colour 0 (dither lines 0 and 1); then a word of line 4 of both target
; planes.
; Correct: each target plane gets the dither bitmap's plane: plane 0
; $AAAA, plane 1 $5555.
; @fixed #1035
; @configs clib
DEFTYPE .l
m=AllocMem(512,$10003)
For i=0 To 63 : Poke.b m+i,$AA : Next
For i=64 To 127 : Poke.b m+i,$55 : Next
For i=128 To 511 : Poke.b m+i,$F0 : Next
CludgeBitMap 1,64,8,2,m
BitMap 0,64,16,2
Use BitMap 0
Dim p.w(8)
Restore pts
For i=0 To 7 : Read v : p(i)=v : Next
VWait
BLITZ
BitMapPoly 4,&p(0),0,1
VWait 2
AMIGA
o=Addr BitMap(0)
NPrint "plane 0 ",Hex$(Peek.w(Peek.l(o+8)+34) AND $FFFF)
NPrint "plane 1 ",Hex$(Peek.w(Peek.l(o+12)+34) AND $FFFF)
Free BitMap 1
FreeMem m,512
End
pts:
Data.l 0,0,63,0,63,15,0,15
