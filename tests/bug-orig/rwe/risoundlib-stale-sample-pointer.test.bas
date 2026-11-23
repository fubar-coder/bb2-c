; risoundlib-stale-sample-pointer (#831) - CludgeSound frees the old
; sample of the sound object before it checks the file, and keeps the
; pointer when the check fails: the object points to freed memory, and the
; next CludgeSound frees it again - memlib does not find the block and
; raises its runtime error "Unable to Free Memory" (runtime error checking
; on: the debugger's requester).  Two files in memory: an 8SVX file (a
; VHDR of the usual length 20 with samplesPerSec 8000, a BODY of 4 bytes)
; and 64 zero bytes (no 8SVX file).
; Correct: the second call returns 0 and leaves sound 0 as it was, the
; third returns -1; no runtime error.
DEFTYPE .l

!TestCase{stale_pointer}
  m=AllocMem_(64,$10001)
  b=AllocMem_(64,$10001)
  !Require{m<>0 AND b<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,44 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,20
  Poke.w m+32,8000 : Poke.b m+34,1 : Poke.l m+36,$10000
  Poke.l m+40,$424F4459 : Poke.l m+44,4 : Poke.l m+48,$01020304
  r1=CludgeSound(0,m)
  r2=CludgeSound(0,b)
  w=Peek.w(Addr Sound(0)+6) AND $FFFF
  !CheckEq{r1,-1,"first"}
  !CheckEq{r2,0,"second"}
  !CheckEq{w,2,"words"}
  r3=CludgeSound(0,m)
  !CheckEq{r3,-1,"third"}
  FreeMem_ b,64
  FreeMem_ m,64
!EndTestCase
