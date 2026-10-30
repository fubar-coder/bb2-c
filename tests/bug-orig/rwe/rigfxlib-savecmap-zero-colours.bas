; rigfxlib-savecmap-zero-colours (#688) - SaveCMAP allocates 3 * colours
; + 48 bytes (memlib's globalloc: + 8) and copies the colours with
; MOVE.w (a0),d0 : SUBQ.w #1,d0 ... DBRA: with 0 colours the counter is
; $FFFF, 65536 colours (196608 bytes) are written behind the 48 bytes
; (and 786432 bytes behind the palette data are read).  Made safe: the
; palette data (0 colours) are a block of the program's own (800000
; bytes); the program allocates a region of 256 KB and, under Forbid,
; every free block below it in its memory list (exec's list, taken with
; AllocAbs), frees the region and takes all of it back but its first 56
; bytes (AllocAbs): the block of SaveCMAP is there, the rest of the
; region is the program's; a marker 100000 bytes behind the block shows
; the overwrite.
; Correct: 0 colours are not copied, the marker stays.
; @rwe rigfxlib riamosfunclib
DEFTYPE .l
Dim f.l(4000),s.l(4000)
InitPalette 1,4
b=AllocMem_(800000,$10001)
If b=0 Then NPrint "no memory" : End
q=Addr Palette(1) : o=Peek.l(q) : Poke.l q,b
r=AllocMem_(262144,0)
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
FreeMem_ r,262144
t=AllocAbs_(262144-56,r+56)
If t
  Poke.l r+56+100000,$12345678
  x=SaveCMAP(1,"RAM:bug-orig-cmap")
  v=Peek.l(r+56+100000)
  FreeMem_ r+56,262144-56
EndIf
For i=0 To n-1 : If f(i) Then FreeMem_ f(i),s(i)
Next
Permit_
Poke.l q,o
FreeMem_ b,800000
If t=0 Then NPrint "no AllocAbs" : End
If v=$12345678 Then NPrint "marker kept" Else NPrint "marker overwritten"
End
