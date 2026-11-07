; windowslib-wmove-wsize-endlos (#320) - WMove and WSize call
; MoveWindow/SizeWindow and then wait, without an end, until the window
; has exactly the new position or size; if Intuition limits the values
; (the size limits of the window, the screen edge) the loop never ends.
; Here, runtime error checking off (no wmovechk/wsizechk): on the
; Workbench screen a window with the size limits 50,50,250,150, WSize to
; a width of 300 (Intuition stops at 250), then WMove to x = 2000
; (Intuition stops at the screen edge).
; Correct: both return, with the values Intuition set.  The original
; hangs in WSize.
; @fixed #320
; @configs clib
; @pass timeout=60
DEFTYPE .l
WbToScreen 0
SizeLimits 50,50,250,150
Window 0,0,20,200,100,$000F,"Limits",1,2
DefaultOutput
WSize 300,100
NPrint "size ",WindowWidth," ",WindowHeight
WMove 2000,20
NPrint "moved ",WindowX>0 AND WindowX<2000," ",WindowY
CloseWindow 0
End
