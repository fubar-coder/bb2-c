; rigfxlib-cpucls-depth-word (#679) - CPUCls takes the number of planes
; from the word at +4 of the bitmap (Flags << 8 | Depth) minus 1 as a
; DBRA counter: a depth of 0 clears 65536 "planes", read as plane
; pointers from +8 of the object and the longs behind it.
; Made safe: the bitmap object is redirected to a block of the program's
; own (item 0 of the bitmaps: the pointers in the globals that hold the
; address of bitmap 0 are replaced for the call), with depth 0 and all
; 65536 plane pointers pointing to one 640-byte buffer filled with $AA
; (BytesPerRow 40 x Rows 16) between margins of $AA.  Nothing else is
; touched, so the program goes on and reports whether the buffer was
; cleared.
; Correct: a bitmap of depth 0 has no plane to clear, the buffer stays.
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
DEFTYPE .l
Dim h(15)
BitMap 0,320,16,1
v=0 : a=&v
ob=Addr BitMap(0)+1                 ; +1: not found by the scan below
bs=8+65536*4
buf=AllocMem_(bs,$10001)
sac=AllocMem_(640+128,$10001)
If buf=0 OR sac=0 Then NPrint "no memory" : End
s=sac+64
For i=0 To 191 : Poke.l sac+i*4,$AAAAAAAA : Next
For i=0 To 65535 : Poke.l buf+8+i*4,s : Next
nh=0
For i=-256 To 256
  p=a+i*4
  If Peek.l(p)=ob-1 AND nh<16 Then h(nh)=p : nh=nh+1 : Poke.l p,buf
Next
If Addr BitMap(0)<>buf
  NPrint "object not redirected"
  For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  End
EndIf
Poke.w buf,40 : Poke.w buf+2,16 : Poke.w buf+4,0 : Poke.w buf+6,0
NPrint "before"
CPUCls 0
For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
c=0
For i=0 To 191
  If Peek.l(sac+i*4)<>$AAAAAAAA Then c=c+1
Next
FreeMem_ buf,bs
FreeMem_ sac,640+128
If c=0
  NPrint "buffer untouched"
Else
  NPrint "cleared: ",c," longs"
EndIf
End
