; richunkylib-c2p-planes-check (#884) - ChunkyToPlanar writes plane n
; at plane 0 + n * 10240 and does not read the bitmap's other plane
; pointers; the conversion of other sizes takes the plane stride from the
; pointers and needs all planes equally spaced.  Here: bitmap 1 is 320 x
; 256 x 8 (CludgeBitMap, contiguous planes), converted once as it is;
; then its plane 7 pointer is set to a buffer of its own (a Poke into the
; bitmap) and it is converted again.  The original has no error check
; area.  Here with runtime error checking.
; Correct: the first conversion is done, the error check refuses the
; second: "Bitmap planes not equally spaced", the buffer stays as it is.
; @fixed #884
; @rwe richunkylib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem_(81920,$10002)
b=AllocMem_(10240,$10001)
If m=0 OR b=0 Then NPrint "no memory" : End
CludgeBitMap 1,320,256,8,m
ChunkyBitmap 0,320,256
Poke.b ChunkyData(0),$FF
ChunkyToPlanar 0,1
NPrint "contiguous: plane 7 ",Peek.b(m+7*10240)&255
bm=Addr BitMap(1)
op=Peek.l(bm+8+28)
Poke.l bm+8+28,b
ChunkyToPlanar 0,1
Poke.l bm+8+28,op
NPrint "plane 7 elsewhere: buffer ",Peek.b(b)&255
FreeMem_ b,10240
FreeMem_ m,81920
End
