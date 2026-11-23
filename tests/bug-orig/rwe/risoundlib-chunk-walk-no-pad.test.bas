; risoundlib-chunk-walk-no-pad (#828) - CludgeSound steps from chunk to
; chunk by length + 8 and does not skip the pad byte IFF puts behind a
; chunk of odd length: behind it the walk reads the id and the length one
; byte off and misses BODY.  The 8SVX file is built in memory: a VHDR of
; the usual length 20 with samplesPerSec 8000, an ANNO of 5 bytes with its
; pad byte, a chunk "AB" of 1024 zero bytes (on it the original's walk
; ends: it lands in the zeros and gives up after seven chunks), a BODY of
; 4 bytes.
; Correct: CludgeSound returns -1, the sound has 2 words.
DEFTYPE .l

!TestCase{chunk_walk}
  m=AllocMem_(1100,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,1090 : Poke.l m+8,$38535658
  Poke.l m+12,$56484452 : Poke.l m+16,20
  Poke.w m+32,8000 : Poke.b m+34,1 : Poke.l m+36,$10000
  Poke.l m+40,$414E4E4F : Poke.l m+44,5 : Poke.l m+48,$61626364 : Poke.b m+52,$65
  Poke.l m+54,$41420000 : Poke.l m+58,1024
  Poke.l m+1086,$424F4459 : Poke.l m+1090,4 : Poke.l m+1094,$01020304
  r=CludgeSound(0,m)
  w=Peek.w(Addr Sound(0)+6) AND $FFFF
  FreeMem_ m,1100
  !CheckEq{r,-1,"cludged"}
  !CheckEq{w,2,"words"}
!EndTestCase
