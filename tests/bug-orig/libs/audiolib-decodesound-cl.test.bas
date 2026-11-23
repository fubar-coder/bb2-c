; audiolib-decodesound-cl (#68) - DecodeSound of an 8SVX FORM in chip
; memory (below 2 MB) uses the BODY data in place and marks the sound
; object "cL" (+14: not ours, not freed).  dofreesound skips the free of
; such data and clears only the data pointer, the mark stays: the next
; sample of that object (a DecodeSound of fast memory, which copies the
; sample to chip memory, InitSound, LoadSound) is never freed either.
; Here: sound 0 from DecodeSound in CHIP memory, then DecodeSound of the
; same FORM in FAST memory and Free Sound 0; again DecodeSound in CHIP
; memory, then InitSound 0,1000 and Free Sound 0.  The free CHIP memory
; (sum of mh_Free of the CHIP MemHeaders) before the second sample and
; after the Free shows the leak.
; Correct: the mark goes with the freed in-place data (+14 = 0 after the
; DecodeSound of fast memory) and Free Sound returns the copy and the
; InitSound memory (leak 0).
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function
Statement mk8svx{a.l}
  Poke.l a,$464F524D : Poke.l a+4,56 : Poke.l a+8,$38535658
  Poke.l a+12,$56484452 : Poke.l a+16,20
  Poke.l a+20,16 : Poke.l a+24,0 : Poke.l a+28,0 : Poke.w a+32,8000
  Poke.w a+34,$100 : Poke.l a+36,$10000
  Poke.l a+40,$424F4459 : Poke.l a+44,16
  For i.l=0 To 15 : Poke.b a+48+i,i*7 : Next
End Statement

!TestCase{mark_and_leaks}
  c=AllocMem_(64,$10002) : f=AllocMem_(64,$10004)
  !Require{c<>0 AND f<>0,"memory"}
  mk8svx{c} : mk8svx{f}
  DecodeSound 0,c
  m1=Peek.w(Addr Sound(0)+14) AND $FFFF
  f0=chipfree{}
  DecodeSound 0,f
  k=Peek.w(Addr Sound(0)+14) AND $FFFF
  Free Sound 0
  f1=chipfree{}
  leak1=f0-f1
  DecodeSound 0,c
  f0=chipfree{}
  InitSound 0,1000
  Free Sound 0
  f1=chipfree{}
  leak2=f0-f1
  FreeMem_ c,64 : FreeMem_ f,64
  !CheckEq{m1,$634C,"chip_mark"}
  !CheckEq{k,0,"fast_mark"}
  !CheckEq{leak1,0,"leak"}
  !CheckEq{leak2,0,"initsound_leak"}
!EndTestCase
