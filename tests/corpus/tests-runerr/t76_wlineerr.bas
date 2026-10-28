; t76_wlineerr - windowslib error check wlinechk: WLine with one point
; (the window is closed first: the count is checked before the window)
WbToScreen 0
Window 0,20,30,200,80,$000E,"Line",1,2
DefaultOutput
CloseWindow 0
NPrint "before"
WLine 10,10,1
NPrint "after"
End
