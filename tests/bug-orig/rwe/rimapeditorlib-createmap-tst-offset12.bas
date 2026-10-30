; rimapeditorlib-createmap-tst-offset12 (#894) - CreateMap frees the map
; the object holds only if the long at offset 12 is not 0 - the low word
; of the data pointer and viewx; the data pointer is at 10.  A map whose
; data lie at $xxxx0000 (viewx 0) is not freed.  The program places the
; data there: it allocates a region of 256 KB and, under Forbid, every
; free block below it in its memory list (exec's list, taken with
; AllocAbs), frees the region and takes back its part below the first
; address $xxxx0000 in it (AllocAbs), so the data of the first CreateMap
; start there.  The second CreateMap's data take the lowest free place:
; the same address if the old map was freed.
; Correct: the second CreateMap frees the first map's data.
; @rwe rimapeditorlib
DEFTYPE .l
Dim f.l(4000),s.l(4000)
r=AllocMem_(262144,0)
If r=0 Then NPrint "no memory" : End
x=(r+65535) AND $FFFF0000
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
FreeMem_ r,262144
h=0
If x>r Then h=AllocAbs_(x-r,r)
CreateMap 0,100,100,16,16,"a"
d=MapDataAdr
CreateMap 0,100,100,16,16,"b"
e=MapDataAdr
If h Then FreeMem_ h,x-r
For i=0 To n-1 : If f(i) Then FreeMem_ f(i),s(i)
Next
Permit_
If d<>x Then NPrint "data not at $xxxx0000" : End
If e=d Then NPrint "old map freed" Else NPrint "old map not freed"
End
