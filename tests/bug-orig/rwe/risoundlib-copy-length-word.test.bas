; risoundlib-copy-length-word (#829) - CludgeSound copies the BODY with a
; DBRA loop on length - 1, which counts the low word only, and takes the
; length in words from the low word of the length: of a BODY of 70000
; bytes it copies 4464 bytes and gives the sound 2232 words.  The 8SVX
; file is built in memory: a VHDR of the usual length 20 with
; samplesPerSec 8000, a BODY of 70000 bytes.
; Correct: CludgeSound returns -1, the sound has 35000 words.
DEFTYPE .l

!TestCase{copy_length}
  m=AllocMem_(70100,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,70040 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,20
  Poke.w m+32,8000 : Poke.b m+34,1 : Poke.l m+36,$10000
  Poke.l m+40,$424F4459 : Poke.l m+44,70000
  r=CludgeSound(0,m)
  w=Peek.w(Addr Sound(0)+6) AND $FFFF
  FreeMem_ m,70100
  !CheckEq{r,-1,"cludged"}
  !CheckEq{w,35000,"words"}
!EndTestCase
