; intlib-vwait-zero (#1201) - VWait n waits for n vertical blanks: the
; compiler passes n-1 (subq #1,d0) and intlib's $C104 runs a DBF loop
; over it, so a count of 0 arrives as $FFFF and the loop runs 65536 times
; (about 22 minutes on a PAL machine: the program looks frozen).  The
; original has no check for it, not even with runtime error checking.
; Here with runtime error checking: a SetInt 5 routine counts the frames
; of VWait, VWait 1 and VWait 5 (no error; every count starts right after
; a VWait, at the start of a frame); then VWait 0 and VWait n with n.w=0,
; in Amiga mode and in Blitz mode.
; Correct: the error check refuses the count 0: "Can't VWait 0 Frames!";
; the other counts wait as many frames as they say.  The original hangs
; in VWait 0 (the session ends by its time limit).
DEFTYPE .l

!TestCase{counts}
  c=0
  SetInt 5
    c+1
  End SetInt
  VWait
  VWait : a=c : VWait : b=c
  !CheckEq{b-a,1,"none"}
  VWait : a=c : VWait 1 : b=c
  !CheckEq{b-a,1,"one"}
  VWait : a=c : VWait 5 : b=c
  !CheckEq{b-a,5,"five"}
  ClrInt 5
!EndTestCase

!TestCase{zero}
  !ExpectError{"Can't VWait 0 Frames!","refused"}
  VWait 0
  !Check{0,"not_refused"}
!EndTestCase

!TestCase{var_zero}
  n.w=0
  !ExpectError{"Can't VWait 0 Frames!","refused"}
  VWait n
  !Check{0,"not_refused"}
!EndTestCase

!TestCase{blitz_zero}
  BLITZ
  !ExpectError{"Can't VWait 0 Frames!","refused"}
  VWait 0
  AMIGA
  !Check{0,"not_refused"}
!EndTestCase
