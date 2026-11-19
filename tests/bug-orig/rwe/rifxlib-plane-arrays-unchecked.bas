; rifxlib-plane-arrays-unchecked (#674) - FadeInBitmap stores two
; pointers per plane (source, destination) in each of bit_add and
; dest_add (16 longs each) without a check against the depth: with 9
; planes the pair of plane 8 in bit_add overwrites the first pair of
; dest_add, and the copy pass copies plane 8 instead of plane 0 (plane 0
; of the destination gets only the dithered lines of the mask pass).
; Bitmaps 0 and 1 are 16x8 with 8 planes and get a ninth one (depth 9,
; the pointer at +40 of the object: 256 bytes of chip memory, room for
; the pointers that run on); plane 0 of the source is all set.  The
; manual: "the maximum bitmap depth for these functions is 8"; the
; original has no error check area.  Here with runtime error checking.
; Correct: the FadeInBitmap of depth 8 works; the error check refuses the
; depth of 9: "Illegal BitMap Depth", nothing is copied.
; @fixed #674
; @rwe rifxlib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,16,8,8
BitMap 1,16,8,8
FadeInBitmap 0,1,1
NPrint "depth 8 done"
m=AllocMem_(512,$10002)
If m=0 Then NPrint "no memory" : End
a=Addr BitMap(0) : b=Addr BitMap(1)
p=Peek.l(a+8)
For i=0 To 14 Step 2 : Poke.w p+i,-1 : Next
Poke.b a+5,9 : Poke.l a+40,m
Poke.b b+5,9 : Poke.l b+40,m+256
FadeInBitmap 0,1,1
Poke.b a+5,8 : Poke.l a+40,0
Poke.b b+5,8 : Poke.l b+40,0
q=Peek.l(b+8)
s=0
For i=0 To 14 Step 2
  If Peek.w(q+i)<>-1 Then s+1
Next
If s Then NPrint "plane 0: ",s," rows incomplete" Else NPrint "plane 0 complete"
FreeMem_ m,512
End
