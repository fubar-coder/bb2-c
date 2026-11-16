; arexxlib-getrexxcommand-index-zero (#51) - GetRexxCommand(rexxmsg,n)
; reads the slot rm_Args[n-1] without a check of n (the guide: 1 to 16);
; n = 0 reads rm_Result2 and takes it for an ArgString.
; Here runtime error checking is on and rm_Result2 holds an ArgString.
; Correct: the error check refuses n = 0: "Illegal Argument Number".
; @fixed #51
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
rp.l=CreateMsgPort("bb2rexxindex")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
a.l=CreateArgString("result")
Poke.l r+36,a
NPrint "before"
NPrint "arg [",GetRexxCommand(r,0),"]"
DeleteArgString a
DeleteRexxMsg r
DeleteMsgPort rp
End
