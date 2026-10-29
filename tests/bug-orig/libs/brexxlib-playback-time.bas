; brexxlib-playback-time (#100) - during PlayBack the input handler
; compares the time of the tape's events only with the time of the FIRST
; event of the incoming list, and links the due ones in front of it.  A
; tape event whose time lies between the first and a later incoming event
; of the same call is played only at the next call, behind them.
; The tape (LoadTape, testdata) holds "b" down and up, 2 s after its
; start.  The program runs through inject: its input handler stamps every
; event with its own clock (standing still), and once the window is
; open, one call gets "a" down and up at +1 s and "c" down and up at
; +3 s (keys=).  The program prints the keys its window gets (Blitz's
; events drop the key-ups).
; Correct: the tape's "b" comes between "a" and "c" (20 35 33).  The
; original plays it behind "c" (20 33 35).
; @configs clib
; @pass file=tape|@TESTDATA@/brexxlib-playback-time.tape
; @pass runwrap=@INJECT@|keys=BREXX/20@1,A0@1,33@3,B3@3
; @pass timeout=60
LoadTape 0,"CORPUS:tape"
WbToScreen 0
DefaultIDCMP $400
Window 0,0,20,200,60,$100E,"BREXX",1,2
DefaultOutput
PlayBack 0
n=0
While n<3
  e.l=WaitEvent
  If e=$400
    NPrint "key ",Hex$(EventCode)
    n=n+1
  EndIf
Wend
End
