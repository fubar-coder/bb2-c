; risnowlib-initsnowdrop-size-aslw (#912) - InitSnowDrop computes the
; size of its block as numpoints * 16 (ASL.l #4) times 4 with ASL.w #2,
; which keeps only the low word: for numpoints 1024 it allocates 40
; bytes (plus memlib's header) but places the lists 65576 bytes wide.
; Made safe: the program allocates a region of 128 KB and, under Forbid,
; every free block below it in its memory list (found in exec's list,
; taken with AllocAbs), frees the region and calls InitSnowDrop: its
; block is the start of the region, the lists written behind it stay
; inside the region.  The size of the block is what the region lost.
; Correct: the block holds the four lists (at least 65576 bytes).
; @rwe risnowlib
DEFTYPE .l
Dim f.l(4000),s.l(4000)
BitMap 0,320,256,3
r=AllocMem_(131072,0)
If r=0 Then NPrint "no memory" : End
Forbid_
h=Peek.l(Peek.l(4)+322)
n=0
While Peek.l(h)<>0
  If r>=Peek.l(h+20) AND r<Peek.l(h+24)
    c=Peek.l(h+16)
    While c<>0 AND c<r AND n<4000
      f(n)=c : s(n)=Peek.l(c+4) : n+1 : c=Peek.l(c)
    Wend
  EndIf
  h=Peek.l(h)
Wend
For i=0 To n-1 : f(i)=AllocAbs_(s(i),f(i)) : Next
FreeMem_ r,131072
a=AvailMem_(0)
lt=InitSnowDrop(0,1024,0,10240)
u=a-AvailMem_(0)
p=Peek.l(lt)
For i=0 To n-1 : If f(i) Then FreeMem_ f(i),s(i)
Next
Permit_
If p<r OR p>=r+131072 Then NPrint "block outside the region" : End
If u>=65576 Then NPrint "block large enough" Else NPrint "block too small"
End
