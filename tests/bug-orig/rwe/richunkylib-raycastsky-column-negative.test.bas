; richunkylib-raycastsky-column-negative (#887) - RaycastSky adds the
; column to the texture as a signed word without a look at it: a negative
; column reads before the texture.  Made safe: the texture given is 320
; bytes into an allocation, so the byte before it is still the program's
; (marked $55).  A call with column 0 comes first.  The original has no
; error check area.  Here with runtime error checking.
; Correct: the error check refuses column -1: "RaycastSky column below
; 0", the byte before the texture is not copied.
DEFTYPE .l

!TestCase{column}
  ChunkyBitmap 0,320,256
  c=ChunkyData(0)
  m=AllocMem_(320*21,$10001)
  !Require{c<>0 AND m<>0,"memory"}
  t=m+320
  Poke.b t,$AA : Poke.b t-1,$55
  RaycastSky 0,10,1,0,t
  !CheckEq{Peek.b(c+10)&255,170,"column0"}
  !ExpectError{"RaycastSky column below 0","refused"}
  RaycastSky 0,11,1,-1,t
  !CheckEq{Peek.b(c+11)&255,0,"column_minus1"}
  FreeMem_ m,320*21
!EndTestCase
