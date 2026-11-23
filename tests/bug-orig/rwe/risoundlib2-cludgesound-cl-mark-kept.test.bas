; risoundlib2-cludgesound-cl-mark-kept (#925) - CludgeSound fills the
; audiolib sound object with a copy of the sample (memlib globalloc) but
; no longer clears +14 (risoundlib's MOVE.w #0,14(a0) became the write of
; the rate at +16, #926): audiolib's "cL" mark of DecodeSound data stays,
; and audiolib never frees the copy (Free Sound skips "cL" data).
; The mark is set with a Poke into the empty sound 0: that is the state
; the original audiolib leaves after DecodeSound of chip data and Free
; Sound (#68), and it keeps the test free of CludgeSound's free of the
; in-place data (#945).  The 8SVX file is built in chip memory: a VHDR of
; the usual length 20 with samplesPerSec 8000, a BODY of 16 bytes.
; CludgeSound also writes the rate at +16, into the high word of sound
; 1's sample pointer (#926): the program clears it again.  The free CHIP
; memory (sum of mh_Free of the CHIP MemHeaders) before CludgeSound and
; after Free Sound 0 shows the leak.
; Correct: +14 = 0 after CludgeSound, Free Sound returns the copy (leak 0).
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function

!TestCase{cl_mark_kept}
  m=AllocMem_(64,$10002)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,56 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,20
  Poke.l m+20,16 : Poke.w m+32,8000 : Poke.b m+34,1 : Poke.l m+36,$10000
  Poke.l m+40,$424F4459 : Poke.l m+44,16
  For i=0 To 15 : Poke.b m+48+i,i*7 : Next
  Poke.w Addr Sound(0)+14,$634C
  f0=chipfree{}
  r=CludgeSound(0,m)
  Poke.w Addr Sound(1),0
  k=Peek.w(Addr Sound(0)+14) AND $FFFF
  Free Sound 0
  f1=chipfree{}
  leak=f0-f1
  FreeMem_ m,64
  !CheckEq{r,-1,"cludged"}
  !CheckEq{k,0,"mark"}
  !CheckEq{leak,0,"leak"}
!EndTestCase
