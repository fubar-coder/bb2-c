; risoundlib2-cludgesound-rate-behind-object (#926) - CludgeSound writes
; the sample rate at +16 of the sound object, for PlaySoundAHI; audiolib's
; sound object is 16 bytes long, so +16 is the high word of the next
; object's sample pointer.  The 8SVX file is built in memory: a VHDR of
; the usual length 20 with samplesPerSec 8363 ($20AB), a BODY of 4 bytes.
; The program clears sound 1's pointer again, so that audiolib does not
; free a bad pointer at the end.
; Correct: CludgeSound returns -1, sound 1's sample pointer stays 0.
DEFTYPE .l

!TestCase{rate_behind_object}
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,44 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,20
  Poke.w m+32,8363 : Poke.b m+34,1 : Poke.l m+36,$10000
  Poke.l m+40,$424F4459 : Poke.l m+44,4 : Poke.l m+48,$01020304
  r=CludgeSound(0,m)
  s=Peek.l(Addr Sound(1))
  Poke.l Addr Sound(1),0
  FreeMem_ m,64
  !CheckEq{r,-1,"cludged"}
  !CheckEq{s,0,"sound_1_pointer"}
!EndTestCase
