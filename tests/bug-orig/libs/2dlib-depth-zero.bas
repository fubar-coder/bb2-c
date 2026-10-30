; 2dlib-depth-zero (#393) - the plane loops of 2dlib count up and compare
; with the depth afterwards: with depth 0 the CPU plot of Circle (cplot,
; CMP.b 5(a3)) runs 256 times and takes plane pointers far behind the 8
; of the bitmap object (+8..+1027).
; BitMap n,w,h,0 itself already overruns (bitmaplib's plane loop, DBF
; depth-1), so the depth of a bitmap with 1 plane is set to 0 here ("a
; bitmap set up by other means").  The bitmap maximum is raised to 20
; (tedstub max=, bitmaplib = 160), so that the 256 pointers lie in
; objects of this program; plane pointers 1-7, the unused fields +40/+44
; and the objects 1-19 point at a block of $FF bytes, the colour field
; (+60, read as the high word of a pointer) at the ROM (writes ignored);
; origin (+48) and +52 are 0 (pointer 0: bit 7 of address 0).  Circle
; 0,0,0,1 plots the point 0,0 (offset 0): plane 0 is set, the other
; "planes" are cleared (BCLR #7).  Everything is restored before End.
; Correct: a bitmap of depth 0 has no planes beyond its depth to write;
; the block is untouched ($FF).
; @configs clib
; @pass jobline=max=160|20
DEFTYPE .l
BitMap 0,320,200,1
o=Addr BitMap(0)
s=AllocMem(64,$10000)
For i=0 To 60 Step 4 : Poke.l s+i,-1 : Next
For i=12 To 44 Step 4 : Poke.l o+i,s : Next
For i=64 To 1279 Step 4 : Poke.l o+i,s : Next
cl=Peek.w(o+60) : Poke.w o+60,$00F8
Poke.b o+5,0
Circle 0,0,0,1
Poke.b o+5,1
Poke.w o+60,cl
For i=12 To 44 Step 4 : Poke.l o+i,0 : Next
For i=64 To 1279 Step 4 : Poke.l o+i,0 : Next
NPrint "block ",Hex$(Peek.b(s) AND 255)
FreeMem s,64
End
