; brexxlib-tapetrap-a3 (#95) - TapeTrap without a tape number clears
; tapeadd and falls through, without an RTS, into TapeTrap Tape#, which
; sets tapeadd to a3, the caller's string work pointer: the redirection
; to a tape never ends, and the next simple events are chained into the
; memory a3 points to instead of being executed.
; Here: with QuietTrap On, AbsMouse 40,30 goes to tape 0; TapeTrap ends
; the redirection; AbsMouse 200,100 must then move the pointer.
; Correct: tape 0 holds one event and the pointer moved.  The original
; does not execute the second AbsMouse (the pointer stays).
; @fixed #95
; @configs clib
DEFTYPE .l
WbToScreen 0
QuietTrap On
TapeTrap 0
AbsMouse 40,30
TapeTrap
VWait 10
x=SMouseX : y=SMouseY
AbsMouse 200,100
VWait 10
c=0 : e=Peek.l(Addr Tape(0))
While e<>0 : c=c+1 : e=Peek.l(e) : Wend
NPrint "tape 0 events ",c
NPrint "pointer moved ",x<>SMouseX OR y<>SMouseY
End
