; windowslib-menus-16byte (#310) - Menus On|Off, MenusOn and MenusOff step
; through the window objects with 16 bytes (the objects are 32 bytes) and
; only Maximum steps: with the default Maximum 5 they reach windows 0, 1
; and 2 (and the second halves of objects 0 and 1, which are 0), never
; windows 3 and 4.
; Windows 0 and 4 on the Workbench screen; Menus Off sets RMBTRAP
; ($10000 in the window flags).
; Correct: both windows get RMBTRAP.
; @configs clib
DEFTYPE .l
WbToScreen 0
Window 0,20,30,200,100,$000E,"Menus 0",1,2
Window 4,240,30,200,100,$000E,"Menus 4",1,2
DefaultOutput
w0=Peek.l(Addr Window(0)) : w4=Peek.l(Addr Window(4))
Menus Off
NPrint "rmbtrap ",Hex$(Peek.l(w0+24)&$10000)," ",Hex$(Peek.l(w4+24)&$10000)
CloseWindow 4 : CloseWindow 0
End
