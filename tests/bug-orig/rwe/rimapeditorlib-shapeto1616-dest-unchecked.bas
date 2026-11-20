; rimapeditorlib-shapeto1616-dest-unchecked (#900) - ShapeTo1616 writes
; 16 words per plane of the source into the destination's data and reads
; nothing of the destination but its data pointer: a destination of 16 x
; 16 with one plane takes 32 bytes, a source of depth 2 writes 64.  Made
; safe: the destination's data is replaced by a block of the program's
; own of 128 bytes, whose bytes 32 to 63 hold a marker, and the shape is
; marked as not owning its data ("cL" at +30), so that the end of the
; program does not free the block as shape data when it stops at the
; runtime error (the data pointer and the mark are put back afterwards).
; The destination is the programmer's; the original has no error check
; area.  Here with runtime error checking.
; Correct: the ShapeTo1616 into a destination of depth 2 works; the
; error check refuses the destination of depth 1: "ShapeTo1616 needs a
; 16 x 16 destination as deep as the source", nothing is written.
; @fixed #900
; @rwe rimapeditorlib
; @pass runtime_errors=on
DEFTYPE .l
InitShape 0,16,16,2
InitShape 1,16,16,1
InitShape 2,16,16,2
ShapeTo1616 0,2
NPrint "depth 2 done"
b=AllocMem_(128,$10002)
If b=0 Then NPrint "no memory" : End
For i=32 To 60 Step 4 : Poke.l b+i,$12345678 : Next
s=Addr Shape(1)
d=Peek.l(s+14)
Poke.l s+14,b : Poke.w s+30,$634C
ShapeTo1616 0,1
Poke.l s+14,d : Poke.w s+30,0
k=0
For i=32 To 60 Step 4
  If Peek.l(b+i)=$12345678 Then k+1
Next
FreeMem_ b,128
If k=8 Then NPrint "marker kept" Else NPrint "marker overwritten"
End
