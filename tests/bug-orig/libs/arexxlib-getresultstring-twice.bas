; arexxlib-getresultstring-twice (#53) - GetResultString copies the
; ArgString of rm_Result2 and deletes it, but leaves rm_Result2 pointing
; to the freed block: a second GetResultString (or DeleteArgString of the
; field) works with freed memory and may free it again.
; Correct: rm_Result2 is 0 after GetResultString, a second call gives "".
; @fixed #53
; @configs clib
rp.l=CreateMsgPort("bb2result")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
Poke.l r+36,CreateArgString("two")
s$=GetResultString(r)
NPrint "first [",s$,"] result2 0 ",Peek.l(r+36)=0
t$=GetResultString(r)
NPrint "second [",t$,"]"
DeleteRexxMsg r
DeleteMsgPort rp
End
