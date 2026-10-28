; windowslib-wcls-y1 (#314) - WCls fills the window with a rectangle from
; (0,1) to (width-1,height-1): the top pixel line stays (visible in a
; window without border).
; A borderless window on an own 8 colour screen (ReadPixel on a true
; colour Workbench gives no pens, as in the corpus test t70); WCls 2,
; then the pens of the pixels (5,0) and (5,1) of its RastPort.
; Correct: both have pen 2.
; @configs clib
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"wcls",1,2
Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
Window 0,20,30,100,50,$0800,"",0,1
DefaultOutput
rp=RastPort(0)
x=5 : y=0 : Gosub pix : a=v : y=1 : Gosub pix : b=v
NPrint "before ",a," ",b
WCls 2
x=5 : y=0 : Gosub pix : a=v : y=1 : Gosub pix : b=v
NPrint "after ",a," ",b
CloseWindow 0
CloseScreen 0
End

; pixel at x,y of rp -> v
pix:
  GetReg d0,gfx : MOVE.l d0,a6
  GetReg d0,rp : MOVE.l d0,a1
  GetReg d1,y
  GetReg d0,x
  JSR -318(a6)
  PutReg d0,v
Return
