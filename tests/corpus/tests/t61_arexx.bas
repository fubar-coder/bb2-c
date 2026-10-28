; t61_arexx - arexxlib without a running ARexx host: ArgStrings, RexxMsgs,
; FillRexxMsg/ClearRexxMsg, RexxError texts of rexxsyslib and a command
; round trip through an own public port "REXX" (priority 100, so FindPort
; finds it before the port of a running RexxMast) with RexxEvent,
; GetRexxCommand, ReplyRexxMsg, GetRexxResult, GetResultString
Dim m.l(63)
b.l=&m(0)
For i=0 To 63 : m(i)=0 : Next
NPrint "rexxerror [",RexxError(1),"] [",RexxError(5),"] [",RexxError(48),"]"
e$=RexxError(200)
NPrint "rexxerror unknown [",e$,"] ",Len(e$)
a.l=CreateArgString("hello")
Print "arg [",Peek$(a),"] size ",Hex$(Peek.l(a-8))," len ",Peek.w(a-4)," flags ",Peek.b(a-2) AND 255
NPrint " hash ",Peek.b(a-1) AND 255
e.l=CreateArgString("")
NPrint "empty size ",Hex$(Peek.l(e-8))," len ",Peek.w(e-4)," hash ",Peek.b(e-1) AND 255," end ",Peek.b(e)
DeleteArgString a
DeleteArgString e
DeleteArgString 0
NPrint "zero block isrexx ",IsRexxMsg(b)," result ",GetRexxResult(b,1)," [",GetRexxCommand(b,1),"]"
rp.l=CreateMsgPort("bb2rexxreply")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
Print "msg isrexx ",IsRexxMsg(r)," type ",Peek.b(r+8)," len ",Peek.w(r+18)," name [",Peek$(Peek.l(r+10))
NPrint "] port ",Peek.l(r+14)=rp
NPrint "ext [",Peek$(Peek.l(r+112)),"] host [",Peek$(Peek.l(r+108)),"] ",Hex$(Peek.l(Peek.l(r+108)-8))
; FillStruct at b+64: flags.w ($4000: value 1 is an integer), values, 0
Poke.s b+128,"one" : Poke.s b+136,"second"
Poke.w b+196,7
Poke.w b+64,$4000 : Poke.l b+66,b+128 : Poke.l b+70,b+200 : Poke.l b+74,b+136 : Poke.l b+78,0
FillRexxMsg r,b+64
Print "fill [",GetRexxCommand(r,1),"] [",GetRexxCommand(r,2),"] [",GetRexxCommand(r,3)
NPrint "] ",GetRexxResult(r,4)=b+200," ",GetRexxResult(r,6)
NPrint "fill hash ",Peek.b(GetRexxResult(r,5)-1) AND 255," index 0 ",Hex$(GetRexxResult(r,0))
ClearRexxMsg r
NPrint "cleared ",GetRexxResult(r,3)," ",GetRexxResult(r,4)=b+200," ",GetRexxResult(r,5)
; own "REXX" port
rx.l=CreateMsgPort("REXX")
GetReg d0,rx
MOVE.l d0,a1:MOVE.l 4,a6:JSR -360(a6)
Poke.b rx+9,100
GetReg d0,rx
MOVE.l d0,a1:MOVE.l 4,a6:JSR -354(a6)
NPrint "event empty ",RexxEvent(rx)," ",RexxEvent(0)
SendRexxCommand r,"do something",$01000000
q.l=RexxEvent(rx)
NPrint "got ",q=r," type ",Peek.b(q+8)," action ",Hex$(GetRexxResult(q,0))," [",GetRexxCommand(q,1),"]"
ReplyRexxMsg q,0,0,"the result"
NPrint "after reply ",RexxEvent(rx)
q=RexxEvent(rp)
NPrint "reply ",q=r," type ",Peek.b(q+8)," res ",GetRexxResult(q,1)," ",GetRexxResult(q,2)," arg0 ",GetRexxResult(q,3)
ReplyRexxMsg q,1,2,"again"
NPrint "reply to reply ",RexxEvent(rp)," ",GetRexxResult(q,1)
; RXFUNC: result string (Result1 = 0)
SendRexxCommand r,"func",$02020000
q=RexxEvent(rx)
ReplyRexxMsg q,0,7,"the result"
q=RexxEvent(rp)
s$=GetResultString(q)
NPrint "func result [",s$,"] ",Len(s$)," res1 ",GetRexxResult(q,1)
; RXFUNC with Result1 <> 0: no string
SendRexxCommand r,"func2",$02020000
q=RexxEvent(rx)
ReplyRexxMsg q,5,9,"x"
q=RexxEvent(rp)
NPrint "func error ",GetRexxResult(q,1)," ",GetRexxResult(q,2)
; RXCOMM with RXFF_RESULT: BTST #17 on memory tests RXFUNC, so no string
SendRexxCommand r,"cmd",$01020000
q=RexxEvent(rx)
ReplyRexxMsg q,0,7,"not sent"
q=RexxEvent(rp)
NPrint "comm result ",GetRexxResult(q,1)," ",GetRexxResult(q,2)
; Args[0] already set: SendRexxCommand keeps it
Poke.l r+40,CreateArgString("preset")
SendRexxCommand r,"ignored",$01000000
q=RexxEvent(rx)
NPrint "preset [",GetRexxCommand(q,1),"]"
ReplyRexxMsg q,0,0,""
q=RexxEvent(rp)
NPrint "preset cleared ",GetRexxResult(q,3)
DeleteRexxMsg r
DeleteMsgPort rx
DeleteMsgPort rp
NPrint "done"
End
