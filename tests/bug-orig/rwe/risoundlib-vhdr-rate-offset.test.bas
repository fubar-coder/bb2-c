; risoundlib-vhdr-rate-offset (#832) - CludgeSound takes the sample rate
; from the word at VHDR + the VHDR's length, which is samplesPerSec (at
; offset 12 of the VHDR data) only for the usual length 20.  The 8SVX
; file is built in memory: a VHDR of 28 bytes with samplesPerSec 8363 and
; the word 16000 at VHDR + 28 (in the 8 bytes behind the usual fields), a
; BODY of 4 bytes.
; Correct: the period is 3579545 / 8363 = 428 (the original's: 223 =
; 3579545 / 16000).
DEFTYPE .l

!TestCase{vhdr_rate_offset}
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,52 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,28
  Poke.w m+32,8363 : Poke.b m+34,1 : Poke.l m+36,$10000 : Poke.w m+40,16000
  Poke.l m+48,$424F4459 : Poke.l m+52,4 : Poke.l m+56,$01020304
  r=CludgeSound(0,m)
  p=Peek.w(Addr Sound(0)+4) AND $FFFF
  FreeMem_ m,64
  !CheckEq{r,-1,"cludged"}
  !CheckEq{p,428,"period"}
!EndTestCase
