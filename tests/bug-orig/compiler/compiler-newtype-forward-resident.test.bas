; compiler-newtype-forward-resident (#974) - a resident with the types of
; compiler-newtype-forward-transitive (.tb contains the forward .tc by
; value, .ta contains .tb): the resident is written after .tb and .ta are laid
; out again.
; Correct: the program that loads the resident sees .tb with 16 bytes (z at
; 14) and .ta with 20 (q at 16).
DEFTYPE .ta v

!TestCase{forward_resident}
  v\m\x\t=7
  v\q=9
  !CheckEq{SizeOf .tb,16,"size_tb"}
  !CheckEq{SizeOf .tb\z,14,"offset_z"}
  !CheckEq{SizeOf .ta,20,"size_ta"}
  !CheckEq{SizeOf .ta\q,16,"offset_q"}
  !CheckEq{v\m\x\t,7,"x_t"}
  !CheckEq{v\q,9,"q"}
!EndTestCase
