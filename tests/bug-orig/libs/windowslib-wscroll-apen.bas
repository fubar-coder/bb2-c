; windowslib-wscroll-apen (#315) - WScroll and WPrintScroll set the A
; pen (rp_FgPen, byte +25 of the RastPort) of the window to 0: they call
; ScrollRaster through the pen setting of the drawing commands with the
; colour 0. ScrollRaster fills with the B pen; the A pen is not theirs to
; change, and a program drawing with RastPort(n) itself sees pen 0.
; WPlot sets the A pen to 3, then each scroll; the A pen after it.
; Correct: the A pen stays 3.
; @fixed #315
; @configs clib
DEFTYPE .l
WbToScreen 0
Window 0,20,30,200,100,$000E,"WScroll",1,2
DefaultOutput
rp=RastPort(0)
WPlot 10,10,3
NPrint "before ",Peek.b(rp+25)
WScroll 0,0,100,50,0,4
NPrint "wscroll ",Peek.b(rp+25)
WPlot 10,10,3
WLocate 0,200
WPrintScroll
NPrint "wprintscroll ",Peek.b(rp+25)
CloseWindow 0
End
