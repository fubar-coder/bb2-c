; richunkylib-c2p-depth-word-flags (#883) - ChunkyToPlanar tests the
; depth with CMPI.w #8,4(a1) / BLT: the word at BitMap+4 is Flags * 256
; + Depth, compared signed.  Flags 1-127 let any depth pass (8 planes
; written into a bitmap of fewer), a Flags byte with bit 7 set refuses
; even a bitmap of depth 8.
; Here: the chunky data (320 x 256, zeroed) converted into bitmap 0 whose
; plane pointers are set to a buffer of 8 planes of 10240 bytes filled
; with $AA (the planes the routine writes) and its Rows to 256, so that
; it is a 320 x 256 bitmap with contiguous planes; the Flags and Depth
; bytes of bitmap 0 are set for the call and put back after it, with the
; pointers and Rows.  Counted:
; the longs changed in planes 0-4 and in planes 5-7.  First Flags 1 with
; Depth 5, then Flags $80 with Depth 8.
; Correct: depth 5 is refused whatever the Flags (nothing changed),
; depth 8 is converted (all 8 planes cleared).  The original converts
; the depth-5 bitmap (planes 5-7 written too) and refuses the depth-8
; one.
DEFTYPE .l

; the number of longs changed by one conversion in planes 0-4 (area 0) or
; in planes 5-7 (area 1); -1 without memory
Function.l c2pcount{f,d,area}
  bs=8*10240
  buf=AllocMem_(bs,$10001)
  sv=AllocMem_(32,$10001)
  If buf=0 OR sv=0 Then Function Return -1
  bm=Addr BitMap(0)
  For p=0 To 7 : Poke.l sv+p*4,Peek.l(bm+8+p*4) : Next
  orw=Peek.w(bm+2) : of=Peek.b(bm+4) : od=Peek.b(bm+5)
  For i=0 To bs-4 Step 4 : Poke.l buf+i,$AAAAAAAA : Next
  For p=0 To 7 : Poke.l bm+8+p*4,buf+p*10240 : Next
  Poke.w bm+2,256 : Poke.b bm+4,f : Poke.b bm+5,d
  ChunkyToPlanar 0,0
  For p=0 To 7 : Poke.l bm+8+p*4,Peek.l(sv+p*4) : Next
  Poke.w bm+2,orw : Poke.b bm+4,of : Poke.b bm+5,od
  c=0
  If area=0
    For i=0 To 5*10240-4 Step 4
      If Peek.l(buf+i)<>$AAAAAAAA Then c=c+1
    Next
  Else
    For i=5*10240 To bs-4 Step 4
      If Peek.l(buf+i)<>$AAAAAAAA Then c=c+1
    Next
  EndIf
  FreeMem_ sv,32
  FreeMem_ buf,bs
  Function Return c
End Function

!TestBatchInit
  BitMap 0,320,16,1
  ChunkyBitmap 0,320,256
!EndTestBatchInit

!TestCase{flags1_depth5}
  a=c2pcount{1,5,0}
  b=c2pcount{1,5,1}
  !CheckEq{a,0,"planes_0_4"}
  !CheckEq{b,0,"planes_5_7"}
!EndTestCase

!TestCase{flags80_depth8}
  a=c2pcount{$80,8,0}
  b=c2pcount{$80,8,1}
  !CheckEq{a,12800,"planes_0_4"}
  !CheckEq{b,7680,"planes_5_7"}
!EndTestCase
