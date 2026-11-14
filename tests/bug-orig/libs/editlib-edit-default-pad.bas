; editlib-edit-default-pad (#129) - Edit(Default,n) pads the default to n
; characters with spaces before the line editor shows it; the padding
; starts one byte before a3 (meant for a string argument, where a3 is
; behind its terminating 0), but Edit's default comes from UStr$, which
; leaves a3 right behind the last character: the first space replaces it.
; A window as the input, a Return put to its port by the program (as in
; the corpus test t71), so the line editor returns the default as shown.
; Correct: Edit(9.5,5) returns 9.5, Edit(25,4) 25.
; @fixed #129
; @configs clib
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400
Window 0,10,20,300,100,$000E,"Edit",1,2
w0=Peek.l(Addr Window(0)) : port=Peek.l(w0+86)
rport=CreateMsgPort("")
Dim mb.l(16*8) : mi=0
ecls=$400 : equal=0 : ecode=$44 : Gosub post
f.f=Edit(9.5,5)
ecode=$44 : Gosub post
h.f=Edit(25,4)
DefaultOutput
NPrint "edit ",f," ",h
CloseWindow 0
End

; put a key (ecls/ecode/equal) for window 0 to the port
post:
  pm=&mb(0)+mi*64 : mi=mi+1
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal
  Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
Return
