; windowslib-wsize-limits-check (#320) - WSize waits until the window has
; the new size; Intuition limits the size of a window with a sizing
; gadget to its size limits, and the original then waits forever.  The
; fix of #320 (release code) ends the wait; a size beyond the limits is
; a programming error, and wsizechk of the original checks only the
; screen edge, not the size limits.
; Here with runtime error checking, on the Workbench screen: a window
; without a sizing gadget and the size limits 50,50,250,150 (Intuition
; does not limit it: WSize 300,100 is no error), then one with a sizing
; gadget: WSize 200,120 (inside the limits), WSize 300,100; the original
; waits forever in the last WSize (the session times out).
; Correct: the error check refuses the size beyond the limits: "Can't
; Size Window".
; @fixed #320
; @configs clib
; @pass runtime_errors=on
; @pass timeout=60
DEFTYPE .l
WbToScreen 0
SizeLimits 50,50,250,150
Window 0,0,20,200,100,$000E,"NoSize",1,2
DefaultOutput
WSize 300,100
NPrint "nosize ",WindowWidth," ",WindowHeight
CloseWindow 0
Window 0,0,20,200,100,$000F,"Limits",1,2
DefaultOutput
WSize 200,120
NPrint "size ",WindowWidth," ",WindowHeight
WSize 300,100
NPrint "size ",WindowWidth," ",WindowHeight
CloseWindow 0
End
