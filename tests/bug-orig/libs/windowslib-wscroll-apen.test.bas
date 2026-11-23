; windowslib-wscroll-apen (#315) - WScroll and WPrintScroll set the A
; pen (rp_FgPen, byte +25 of the RastPort) of the window to 0: they call
; ScrollRaster through the pen setting of the drawing commands with the
; colour 0. ScrollRaster fills with the B pen; the A pen is not theirs to
; change, and a program drawing with RastPort(n) itself sees pen 0.
; Here: WPlot sets the A pen to 3, then each scroll; the A pen after it.
; Correct: the A pen stays 3.
DEFTYPE .l

!TestCase{a_pen}
  WbToScreen 0
  Window 0,20,30,200,100,$000E,"WScroll",1,2
  rp=RastPort(0)
  WPlot 10,10,3
  !CheckEq{Peek.b(rp+25),3,"pen_before"}
  WScroll 0,0,100,50,0,4
  !CheckEq{Peek.b(rp+25),3,"pen_after_wscroll"}
  WPlot 10,10,3
  WLocate 0,200
  WPrintScroll
  !CheckEq{Peek.b(rp+25),3,"pen_after_wprintscroll"}
  CloseWindow 0
!EndTestCase
