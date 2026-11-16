; arexxlib-getrexxresult-index-zero (#51) - GetRexxResult(rexxmsg,n)
; reads the long word at rm_Result1 + (n-1)*4 without a check of n (the
; guide: 1 or 2); n = 0 reads rm_Action, the field before rm_Result1.
; Here runtime error checking is on.
; Correct: the error check refuses n = 0: "Illegal Result Number".
; @fixed #51
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
rp.l=CreateMsgPort("bb2rexxindex")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
Poke.l r+28,$01000000
NPrint "before"
NPrint "action ",Hex$(GetRexxResult(r,0))
DeleteRexxMsg r
DeleteMsgPort rp
End
