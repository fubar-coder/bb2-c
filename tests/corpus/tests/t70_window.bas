; t70_window - windowslib: Window/WindowTags/CloseWindow, the window
; functions (position, size, borders, cursor), WMove/WSize, Menus,
; drawing (pixels read back with ReadPixel), WTitle, IDCMP settings.
; On an own 8 colour screen (ReadPixel on a true colour Workbench gives
; no pens).
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"t70",1,2
Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
NPrint "gfx ",gfx<>0
DefaultIDCMP $400|$100|8|$40|$20
SizeLimits 40,30,600,400
Window 0,20,30,240,120,$000E,"Window 0",1,2
DefaultOutput : DefaultInput
w=Peek.l(Addr Window(0))
NPrint "open ",w<>0," num ",Peek.w(w+120)," idcmp ",Hex$(Peek.l(w+82))
NPrint "flags ",Hex$(Peek.l(w+24))," min ",Peek.w(w+16)," ",Peek.w(w+18)," max ",Peek.w(w+20)," ",Peek.w(w+22)
NPrint "port ",Peek.l(w+86)<>0," pens ",Peek.b(w+98)," ",Peek.b(w+99)
NPrint "pos ",WindowX," ",WindowY," size ",WindowWidth," ",WindowHeight
NPrint "inner ",InnerWidth," ",InnerHeight," off ",WTopOff," ",WLeftOff
NPrint "curs ",WCursX," ",WCursY
WLocate 10,5 : NPrint "locate ",WCursX," ",WCursY
NPrint "title [",Peek$(Peek.l(w+32)),"]"
NPrint "rastport ",RastPort(0)=Peek.l(w+50)
WTitle "New title","Screen title"
NPrint "wtitle [",Peek$(Peek.l(w+32)),"] [",Peek$(Peek.l(w+104)),"]"
WMove 50,60 : NPrint "moved ",WindowX," ",WindowY
WSize 200,100 : NPrint "sized ",WindowWidth," ",WindowHeight," ",InnerWidth," ",InnerHeight
; drawing
rp=RastPort(0)
WCls 0
WPlot 10,20,1 : WPlot 11,20,2 : WPlot 12,21,3
x=10 : y=20 : Gosub pix : a=v : x=11 : Gosub pix : b=v : x=12 : y=21 : Gosub pix
NPrint "plot ",a," ",b," ",v
WBox 60,40,30,30,2 : bx=30 : by=30 : bw=40 : bh=20 : Gosub area : NPrint "box ",s
WCircle 100,50,10,1 : bx=88 : by=38 : bw=25 : bh=25 : Gosub area : NPrint "circle ",s
WEllipse 100,50,20,8,3 : bx=78 : by=40 : bw=45 : bh=21 : Gosub area : NPrint "ellipse ",s
WLine 20,60,40,60,40,80,20,80,2 : bx=15 : by=55 : bw=30 : bh=30 : Gosub area : NPrint "line ",s
WScroll 30,30,70,50,5,2 : bx=25 : by=25 : bw=50 : bh=30 : Gosub area : NPrint "scroll ",s
InnerCls 3 : bx=0 : by=0 : bw=200 : bh=100 : Gosub area : NPrint "innercls ",s
WCls : Gosub area : NPrint "wcls ",s
WColour 2,1 : WJam 1
; menus on/off: RMBTRAP and MENUVERIFY
MenusOff : NPrint "menusoff ",Hex$(Peek.l(w+24))," ",Hex$(Peek.l(w+82))
MenusOn : NPrint "menuson ",Hex$(Peek.l(w+24))," ",Hex$(Peek.l(w+82))
Menus Off : NPrint "menus off ",Hex$(Peek.l(w+24)&$10000)
Menus On : NPrint "menus on ",Hex$(Peek.l(w+24)&$10000)
; a second window: GIMMEZEROZERO, IDCMP added/removed
AddIDCMP $40000|$10 : SubIDCMP $100
SizeLimits 0
Window 1,150,90,160,90,$040E,"GZZ",2,1
DefaultOutput : DefaultInput
w1=Peek.l(Addr Window(1))
NPrint "w1 ",Peek.w(w1+120)," idcmp ",Hex$(Peek.l(w1+82))," flags ",Hex$(Peek.l(w1+24))
NPrint "w1 min ",Peek.w(w1+16)," ",Peek.w(w1+18)," max ",Peek.w(w1+20)," ",Peek.w(w1+22)
NPrint "w1 pos ",WindowX," ",WindowY," inner ",InnerWidth," ",InnerHeight," curs ",WCursX," ",WCursY
NPrint "w1 origin ",Peek.w(w1+122)
Use Window 0
NPrint "used 0 ",WindowX," ",WindowWidth
DefaultIDCMP $400|$100|8|$40|$20
; WindowTags
WindowTags 2,$000E,"Tags",$80000064,100,$80000065,100,$80000066,120,$80000067,60
DefaultOutput : DefaultInput
w2=Peek.l(Addr Window(2))
NPrint "tags ",WindowX," ",WindowY," ",WindowWidth," ",WindowHeight," [",Peek$(Peek.l(w2+32)),"] ",Peek.w(w2+120)
Dim tl.l(10) : tl(0)=$80000064 : tl(1)=10 : tl(2)=$80000065 : tl(3)=30 : tl(4)=$80000066 : tl(5)=90
tl(6)=$80000067 : tl(7)=40 : tl(8)=0
WindowTags 3,$000E,"Taglist",&tl(0)
DefaultOutput : DefaultInput
NPrint "taglist ",WindowX," ",WindowY," ",WindowWidth," ",WindowHeight
CloseWindow 3 : CloseWindow 2
NPrint "closed ",Peek.l(Addr Window(2))," ",Peek.l(Addr Window(3))
CloseWindow 1 : CloseWindow 1
CloseWindow 0
NPrint "all closed ",Peek.l(Addr Window(0))
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

; sum of pixel*position over an area -> s
area:
  s=0
  For yy=by To by+bh-1
    For xx=bx To bx+bw-1
      x=xx : y=yy : Gosub pix
      s=s+v*((xx-bx)+(yy-by)*3+1)
    Next
  Next
Return
