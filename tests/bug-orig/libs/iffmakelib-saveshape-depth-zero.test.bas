; iffmakelib-saveshape-depth-zero (#326) - SaveShape enters the plane
; pointers of the shape into tempbm with MOVE 4(a0),d0:SUBQ #1,d0 ... DBF
; without looking at the depth: depth 0 writes 65536 pointers (256 KB)
; from tempbm on, over the library's variables and code: a crash.
; A shape of depth 0 comes from InitShape only without runtime error
; checking (#243), or from LoadShape, which takes the depth of the BMHD
; as it is; here the program makes one by poking the depth word of a
; shape (+4) to 0. Runtime error checking off: SetErr catches a runtime
; error.
; Correct: SaveShape refuses the depth with a runtime error.
; The handler of SetErr holds the check and ends the program, so the case
; has no end entry; the check behind the SaveShape is reached only if no
; error is raised.
DEFTYPE .l

!TestCase{depth_zero}
  SetErr
    !Check{1,"refused"}
    End
  End SetErr
  InitShape 0,16,1,1
  Poke.w Addr Shape(0)+4,0
  SaveShape 0,"CORPUS:bo326z.iff"
  !Check{0,"not_refused"}
  KillFile "CORPUS:bo326z.iff"
!EndTestCase
