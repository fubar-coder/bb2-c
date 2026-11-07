; windowslib-closewindow-ereignisse (#322) - CloseWindow (freewindow)
; replies every message at the port all windows share and then drops the
; whole event list: the events of the other windows are lost too.
; Two windows on the Workbench screen; the program puts an event for
; window 1 to the port itself (PutMsg, as in the corpus test t71), then
; closes window 0.
; Correct: Event still gets the event of window 1.
; @fixed #322
; @configs clib
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
Window 0,20,30,200,100,$000E,"Close 0",1,2
Window 1,240,30,200,100,$000E,"Close 1",1,2
DefaultOutput
w1=Peek.l(Addr Window(1))
port=Peek.l(w1+86)
rport=CreateMsgPort("")
Dim mb.l(16)
pm=&mb(0)
Poke.l pm+14,rport : Poke.w pm+18,52
Poke.l pm+20,4 : Poke.w pm+24,7 : Poke.l pm+44,w1
GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
MOVE.l 4,a6 : JSR -366(a6)
CloseWindow 0
Use Window 1 : DefaultOutput
e=Event : c=EventCode : NPrint "event ",Hex$(e)," code ",c
CloseWindow 1
End
