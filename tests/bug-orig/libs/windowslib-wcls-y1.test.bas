; windowslib-wcls-y1 (#314) - WCls fills the window with a rectangle from
; (0,1) to (width-1,height-1): the top pixel line stays (visible in a
; window without border).
; Here: a borderless window on an own 8 colour screen (ReadPixel on a true
; colour Workbench gives no pens, as in the corpus test t70); WCls 2,
; then the pens of the pixels (5,0) and (5,1) of its RastPort.
; Correct: both have pen 2.
DEFTYPE .l

; pen of the pixel x,y of the rastport rp (graphics.library ReadPixel) -> v
Statement pix{x,y}
  SHARED gfx,rp,v
  GetReg d0,gfx : MOVE.l d0,a6
  GetReg d0,rp : MOVE.l d0,a1
  GetReg d1,y
  GetReg d0,x
  JSR -318(a6)
  PutReg d0,v
End Statement

!TestCase{top_line}
  Screen 0,0,0,320,200,3,0,"wcls",1,2
  Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
  GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
  Window 0,20,30,100,50,$0800,"",0,1
  rp=RastPort(0)
  pix{5,0} : !CheckEq{v,0,"before_y0"}
  pix{5,1} : !CheckEq{v,0,"before_y1"}
  WCls 2
  pix{5,0} : !CheckEq{v,2,"after_y0"}
  pix{5,1} : !CheckEq{v,2,"after_y1"}
  CloseWindow 0
  CloseScreen 0
!EndTestCase
