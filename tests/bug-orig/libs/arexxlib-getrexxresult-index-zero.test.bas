; arexxlib-getrexxresult-index-zero (#51) - GetRexxResult(rexxmsg,n)
; reads the long word at rm_Result1 + (n-1)*4 without a check of n (the
; guide: 1 or 2); n = 0 reads rm_Action, the field before rm_Result1.
; Here runtime error checking is on.
; Correct: the error check refuses n = 0: "Illegal Result Number".
DEFTYPE .l

!TestCase{index_zero}
  rp.l=CreateMsgPort("bb2rexxindex")
  r.l=CreateRexxMsg(rp,"bb","BB2HOST")
  Poke.l r+28,$01000000
  !ExpectError{"Illegal Result Number","refused"}
  v=GetRexxResult(r,0)
  !Check{0,"not_refused"}
  DeleteRexxMsg r
  DeleteMsgPort rp
!EndTestCase
