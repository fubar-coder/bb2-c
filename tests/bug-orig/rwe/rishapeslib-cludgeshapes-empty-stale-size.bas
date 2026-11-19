; rishapeslib-cludgeshapes-empty-stale-size (#809) - CludgeShapes walks
; the shapes in memory (a 32-byte block, then the data) by 32 + the size
; of each, but clears the size, a long, with MOVE.w #0: an empty shape
; (width 0) keeps the low word of the size of the shape before, and the
; walk skips that much too far.  Three shapes in the program's memory:
; 16 x 8 pixels with 16 bytes of data, an empty one, 24 x 4 pixels with
; 16 bytes of data (zeros).
; Correct: shape 2 is the third shape (width 24); the original reads it
; 16 bytes too far on, from its zero data: an empty shape.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
; @fixed #809
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
b=AllocMem_(1024,$10001)
If b=0 Then NPrint "no memory" : End
Poke.w b,16 : Poke.w b+2,8 : Poke.w b+4,1
Poke.w b+6,2 : Poke.w b+8,8*64+1
Poke.w b+22,16 : Poke.w b+24,32 : Poke.w b+26,16 : Poke.w b+28,32
For i=0 To 15 : Poke.b b+32+i,$ff : Next
c=b+80
Poke.w c,24 : Poke.w c+2,4 : Poke.w c+4,1
Poke.w c+6,4 : Poke.w c+8,4*64+2
Poke.w c+22,16 : Poke.w c+24,24 : Poke.w c+26,16 : Poke.w c+28,24
r=CludgeShapes(0,3,b)
NPrint ShapeWidth(0)," ",ShapeWidth(1)," ",ShapeWidth(2)
FreeMem_ b,1024
End
