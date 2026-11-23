; 2dlib-poly-one-point (#396) - Poly stores numpoints-1 in a word counter
; and draws before it counts down (SUB #1,pfc:BNE): with 1 point the
; counter goes from 0 to $FFFF and 65535 more lines are drawn from the
; words behind the coordinates.  A count below 2 is a programming error,
; and the original has no error check for it, not even with runtime
; error checking.
; Here with runtime error checking: the coordinates are in a zeroed block
; with room for 65536 more points, all (0,0), so the lines stay inside
; the bitmap.  Counted: bytes set in the bitmap's plane, for 2 points
; (sanity: one short line, no error) and for 1 point.
; Correct: the error check refuses the count: "Poly needs at least 2
; Points".  The original draws the 65536 lines.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l
Function.l nset{}
  pl.l=Peek.l(Addr BitMap(0)+8) : n.l=0
  For i.l=0 To 7999
    If Peek.b(pl+i)<>0 Then n=n+1
  Next
  Function Return n
End Function

!TestCase{one_point}
  BitMap 0,320,200,1
  p=AllocMem(262400,$10000)
  Poke.w p,40 : Poke.w p+2,30 : Poke.w p+4,47 : Poke.w p+6,30
  Poly 2,p,1
  !Check{nset{}<=2,"two_points"}
  Cls
  Poke.w p+4,0
  !ExpectError{"Poly needs at least 2 Points","refused"}
  Poly 1,p,1
  !Check{0,"after"}
!EndTestCase
