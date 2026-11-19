; rigfxlib-cpucls-depth-above-8 (#679) - CPUCls counts the planes with
; DBRA on the depth and takes the plane pointers from +8 of the bitmap
; on: a struct BitMap has 8 of them, so a depth above 8 takes the longs
; behind them (the object's other fields, then the next object) as plane
; addresses.  Such a depth is an invalid argument; the original has no
; error check area.  Here with runtime error checking and depth 9.
; Made safe: the bitmap object is redirected to a block of the program's
; own (item 0 of the bitmaps: the pointers in the globals that hold the
; address of bitmap 0 are replaced for the call), with depth 9 and all 64
; longs from +8 pointing to one 640-byte buffer filled with $AA
; (BytesPerRow 40 x Rows 16) between margins of $AA.  The buffer lies at
; an address whose low word is below $8000, so the word at +62 of the
; block ("planes allocated here" of bitmaplib) is positive and the end of
; the program frees nothing of it even when the pointers are not put
; back.  The program goes on and reports whether the buffer was cleared.
; Correct: the error check refuses the bitmap: "Illegal BitMap Depth".
; @fixed #679
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
; @pass runtime_errors=on
DEFTYPE .l
Dim h(15)
BitMap 0,320,16,1
v=0 : a=&v
ob=Addr BitMap(0)+1                 ; +1: not found by the scan below
bs=8+64*4
ss=65536+1024
buf=AllocMem_(bs,$10001)
sac=AllocMem_(ss,$10001)
If buf=0 OR sac=0 Then NPrint "no memory" : End
s=((sac+64+$FFFF) AND $FFFF0000)+64
For i=0 To 191 : Poke.l s-64+i*4,$AAAAAAAA : Next
For i=0 To 63 : Poke.l buf+8+i*4,s : Next
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
Poke.w buf,40 : Poke.w buf+2,16 : Poke.w buf+4,9 : Poke.w buf+6,0
NPrint "before"
CPUCls 0
For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
c=0
For i=0 To 191
  If Peek.l(s-64+i*4)<>$AAAAAAAA Then c=c+1
Next
FreeMem_ buf,bs
FreeMem_ sac,ss
If c=0
  NPrint "buffer untouched"
Else
  NPrint "cleared: ",c," longs"
EndIf
End
