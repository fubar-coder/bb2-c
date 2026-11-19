; rifxlib-clearbitmap-last-word (#668) - ClearBitmap clears the area word
; by word from its end backwards, but starts at the end minus 4 (the
; source's comment says 2): every pass is one word too low, so the last
; word of the area stays and the last pass clears the word in front of
; the area (in front of the bitplane for offset 0).  Bitmap 0 is 32 x 4
; with one plane (two words per row), all bits set; the area is rows 1
; and 2 (height 2, offset 1), so the word in front of it is the last word
; of row 0.
; Correct: rows 1 and 2 are cleared, rows 0 and 3 stay.
; @fixed #668
; @rwe rifxlib
DEFTYPE .l
BitMap 0,32,4,1
p=Peek.l(Addr BitMap(0)+8)
For i=0 To 3 : Poke.l p+i*4,-1 : Next
ClearBitmap 0,1,2,1
For i=0 To 3 : NPrint "row ",i,": ",Hex$(Peek.l(p+i*4)) : Next
End
