; editlib-edit-default-pad (#129) - Edit(Default,n) pads the default to n
; characters with spaces before the line editor shows it; the padding
; starts one byte before a3 (meant for a string argument, where a3 is
; behind its terminating 0), but Edit's default comes from UStr$, which
; leaves a3 right behind the last character: the first space replaces it.
; Here: a window as the input, a Return put to its port by the program
; (as in the corpus test t71), so the line editor returns the default as
; shown.
; Correct: Edit(9.5,5) returns 9.5, Edit(25,4) 25.
DEFTYPE .l

; put a key (ecls/ecode) for window w0 to its port, from the buffer pm
Statement post{pm,ecls,ecode}
  SHARED rport,port,w0
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,0
  Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
End Statement

!TestCase{default_pad}
  WbToScreen 0
  DefaultIDCMP $400
  Window 0,10,20,300,100,$000E,"Edit",1,2
  w0=Peek.l(Addr Window(0)) : port=Peek.l(w0+86)
  rport=CreateMsgPort("")
  Dim mb.l(16*8)
  post{&mb(0),$400,$44}
  f.f=Edit(9.5,5)
  post{&mb(0)+64,$400,$44}
  h.f=Edit(25,4)
  !CheckFltNear{f,9.5,0.001,"edit_9_5"}
  !CheckFltNear{h,25,0.001,"edit_25"}
  CloseWindow 0
!EndTestCase
