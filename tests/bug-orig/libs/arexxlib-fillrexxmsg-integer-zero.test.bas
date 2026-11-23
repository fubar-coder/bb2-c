; arexxlib-fillrexxmsg-integer-zero (#50) - FillRexxMsg takes every value
; 0 of the FillStruct as the end of the list, also in a slot whose flag
; bit marks it as an integer: the integer 0 and all arguments behind it do
; not get into the RexxMsg.  ClearRexxMsg stops at the first 0 slot, so an
; argument behind an integer 0 would not be deleted either.
; FillStruct as in the guide's example: "open", the integer 0 (flag bit
; 14), "0123456789", then the end.
; Correct: slot 3 holds "0123456789"; ClearRexxMsg deletes it (the guide:
; ClearRexxMsg works from slot 1 to 16).
DEFTYPE .l

!TestCase{integer_zero_slot}
  Dim m.l(63)
  b.l=&m(0)
  For i=0 To 63 : m(i)=0 : Next
  Poke.s b+128,"open" : Poke.s b+136,"0123456789"
  Poke.w b,%0100000000000000
  Poke.l b+2,b+128 : Poke.l b+6,0 : Poke.l b+10,b+136 : Poke.l b+14,0
  rp.l=CreateMsgPort("bb2fill")
  r.l=CreateRexxMsg(rp,"vc","host")
  FillRexxMsg r,b
  !CheckStrEq{GetRexxCommand(r,1),"open","slot_1"}
  !CheckEq{Peek.l(r+$2C),0,"slot_2_integer"}
  !CheckStrEq{GetRexxCommand(r,3),"0123456789","slot_3"}
  ClearRexxMsg r
  !CheckEq{Peek.l(r+$28),0,"cleared_slot_1"}
  !CheckEq{Peek.l(r+$30),0,"cleared_slot_3"}
  DeleteRexxMsg r
  DeleteMsgPort rp
!EndTestCase
