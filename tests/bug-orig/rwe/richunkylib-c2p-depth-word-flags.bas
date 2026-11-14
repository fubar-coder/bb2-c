; richunkylib-c2p-depth-word-flags (#883) - ChunkyToPlanar tests the
; depth with CMPI.w #8,4(a1) / BLT: the word at BitMap+4 is Flags * 256
; + Depth, compared signed.  Flags 1-127 let any depth pass (8 planes
; written into a bitmap of fewer), a Flags byte with bit 7 set refuses
; even a bitmap of depth 8.
; Here: the chunky data (320 x 256, zeroed) converted into bitmap 0 whose
; first plane pointer is set to a buffer of 8 planes of 10240 bytes
; filled with $AA (the planes the routine writes); the Flags and Depth
; bytes of bitmap 0 are set for the call and put back after it.  Counted:
; the longs changed in planes 0-4 and in planes 5-7.  First Flags 1 with
; Depth 5, then Flags $80 with Depth 8.
; Correct: depth 5 is refused whatever the Flags (nothing changed),
; depth 8 is converted (all 8 planes cleared).  The original converts
; the depth-5 bitmap (planes 5-7 written too) and refuses the depth-8
; one.
; @fixed #883
; @rwe richunkylib
; @pass timeout=60
DEFTYPE .l
BitMap 0,320,16,1
ChunkyBitmap 0,320,256
bs=8*10240
buf=AllocMem_(bs,$10001)
If buf=0 Then NPrint "no memory" : End
bm=Addr BitMap(0)
op=Peek.l(bm+8) : of=Peek.b(bm+4) : od=Peek.b(bm+5)
For k=0 To 1
  For i=0 To bs-4 Step 4 : Poke.l buf+i,$AAAAAAAA : Next
  f=1 : d=5
  If k=1 Then f=$80 : d=8
  Poke.l bm+8,buf : Poke.b bm+4,f : Poke.b bm+5,d
  ChunkyToPlanar 0,0
  Poke.l bm+8,op : Poke.b bm+4,of : Poke.b bm+5,od
  c0=0 : c1=0
  For i=0 To 5*10240-4 Step 4
    If Peek.l(buf+i)<>$AAAAAAAA Then c0=c0+1
  Next
  For i=5*10240 To bs-4 Step 4
    If Peek.l(buf+i)<>$AAAAAAAA Then c1=c1+1
  Next
  NPrint "flags ",f," depth ",d,": planes 0-4 ",c0,", planes 5-7 ",c1
Next
FreeMem_ buf,bs
End
