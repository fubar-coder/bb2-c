; riamosfunclib-v36-caches (#1145) - CachesOn and CachesOff call
; CacheControl, an exec V36 function, without checking exec's version;
; on Kickstart 1.3 (exec V34) the offset lies behind exec's jump table:
; the program crashes at CachesOn (the session ends in a timeout).
; Correct: below exec V36 both return 0 and switch nothing; from V36 on
; they call CacheControl, which gives 0 on a 68000 (no cache bits): the
; same output on the a500 and the a500-204.
DEFTYPE .l

!TestCase{v36_caches}
  c1=CachesOn(1)
  !CheckEq{c1,0,"caches_on"}
  c2=CachesOff(1)
  !CheckEq{c2,0,"caches_off"}
!EndTestCase
