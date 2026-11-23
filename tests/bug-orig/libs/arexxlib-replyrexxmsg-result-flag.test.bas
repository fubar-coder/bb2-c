; arexxlib-replyrexxmsg-result-flag (#48) - ReplyRexxMsg makes the result
; ArgString (Result1 = 0) when `BTST #17,28(a0)` is set: a BTST on memory
; tests bit 17 mod 8 of the first byte of rm_Action, bit 25 = RXFUNC,
; not RXFF_RESULT (bit 17).  A command with RXFF_RESULT gets no result
; string, a function call without it gets one.
; The program has its own public port "REXX" (priority 100, so FindPort
; finds it before the port of a running RexxMast) and answers its own
; messages, as t61_arexx does.
; Correct: RXCOMM|RXFF_RESULT gets the result string "sent"; RXFUNC
; without RXFF_RESULT keeps Result2 = 7.
DEFTYPE .l

!TestCase{result_flag}
  rp.l=CreateMsgPort("bb2rexxflag")
  r.l=CreateRexxMsg(rp,"bb","BB2HOST")
  rx.l=CreateMsgPort("REXX")
  GetReg d0,rx
  MOVE.l d0,a1:MOVE.l 4,a6:JSR -360(a6)
  Poke.b rx+9,100
  GetReg d0,rx
  MOVE.l d0,a1:MOVE.l 4,a6:JSR -354(a6)
  SendRexxCommand r,"cmd",$01020000
  q.l=RexxEvent(rx)
  ReplyRexxMsg q,0,7,"sent"
  q=RexxEvent(rp)
  !CheckStrEq{GetResultString(q),"sent","command_result_string"}
  SendRexxCommand r,"func",$02000000
  q=RexxEvent(rx)
  ReplyRexxMsg q,0,7,"not sent"
  q=RexxEvent(rp)
  !CheckEq{GetRexxResult(q,2),7,"function_result2"}
  ClearRexxMsg r
  DeleteRexxMsg r
  DeleteMsgPort rx
  DeleteMsgPort rp
!EndTestCase
