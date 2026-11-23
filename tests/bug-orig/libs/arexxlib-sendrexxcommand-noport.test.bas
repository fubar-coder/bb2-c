; arexxlib-sendrexxcommand-noport (#55) - SendRexxCommand calls exec
; Disable, looks for the public port "REXX" and, when there is none,
; shows its alert and returns without the Enable: the program goes on
; with interrupts disabled (exec's IDNestCnt stays 0 instead of -1).
; The program runs through inject, which answers the alert at once
; (alerts); a "REXX" port of a running RexxMast is renamed for the call.
; The program reads IDNestCnt (ExecBase+$126) right after the call and
; enables the interrupts itself if they are still disabled.
; Correct: after the alert the interrupts are enabled (IDNestCnt -1, as
; a byte 255).
DEFTYPE .l

!TestCase{interrupts_after_alert}
  Dim nm.l(3) : Dim hd.l(3)
  Poke.s &nm(0),"REXX" : Poke.s &hd(0),"XEXX"
  GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -390(a6) : PutReg d0,p.l
  o.l=0
  If p<>0 Then o=Peek.l(p+10) : Poke.l p+10,&hd(0)
  rp.l=CreateMsgPort("bb2rexxnoport")
  r.l=CreateRexxMsg(rp,"bb","BB2HOST")
  SendRexxCommand r,"cmd",$01000000
  c.l=Peek.b(Peek.l(4)+$126) AND 255
  If c<>255
    MOVE.l 4,a6 : JSR -126(a6)
  EndIf
  If p<>0 Then Poke.l p+10,o
  !CheckEq{c,255,"idnestcnt"}
  ClearRexxMsg r
  DeleteRexxMsg r
  DeleteMsgPort rp
!EndTestCase
