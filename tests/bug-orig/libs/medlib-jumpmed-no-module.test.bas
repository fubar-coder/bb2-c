; medlib-jumpmed-no-module (#210) - JumpMed writes the position (n-1) to
; mmd_pseqnum and 0 to mmd_pline of the playing module without a check
; that one plays: before the first StartMedModule (and after StopMed or an
; FFE stop) the module pointer is 0, and JumpMed writes the long at
; address 44, exception vector 11 (Line-F).  Here runtime error checking
; is on and no module was started; the case saves and restores the
; vector around the call.
; Correct: the error check refuses JumpMed without a playing module: "No
; Module Playing" (announced with !ExpectError; the original raises no
; error and writes the vector, see the configuration).
DEFTYPE .l

!TestCase{no_module}
  v=Peek.l(44)
  !ExpectError{"No Module Playing","refused"}
  JumpMed 3
  w=Peek.l(44)
  Poke.l 44,v
  changed=0
  If w<>v Then changed=1
  !CheckEq{changed,0,"vector_changed"}
!EndTestCase
