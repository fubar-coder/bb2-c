; windowslib-calcwidth-proportional (#321) - calcwidth (the width of a
; character in the line editor of Edit$ in a window) tests the table
; index of a proportional font with SUB.b/BMI: characters from LoChar+128
; on (with LoChar 32: from $A0, the umlauts) get tf_XSize instead of their
; width, and the cursor is in the wrong place.
; On an own 8 colour screen, a window with helvetica 11 (proportional,
; from the Workbench's FONTS:): the line "aax" (a = a-umlaut, Chr$(228))
; printed, and below it Edit$ with the default "aa", EditFrom 2 and the
; keys x and Return put to the port by the program (as in the corpus test
; t72).  The pixels of both lines are compared (ReadPixel).
; Correct: the x is inserted behind the umlauts, both lines look the same.
; @configs clib
DEFTYPE .l
Screen 0,0,0,640,200,3,0,"calcwidth",1,2
Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
LoadFont 0,"helvetica.font",11
tf=Peek.l(Addr IntuiFont(0)+8)
DefaultOutput
NPrint "font ",tf<>0," proportional ",(Peek.b(tf+23)&$20)<>0
If tf=0 Then End
DefaultIDCMP $400|8
Window 0,10,20,300,120,$000E,"Edit",1,2
WindowFont 0
w0=Peek.l(Addr Window(0)) : port=Peek.l(w0+86) : rp=RastPort(0)
rport=CreateMsgPort("")
Dim mb.l(16*8) : mi=0
u$=Chr$(228)+Chr$(228)
WLocate 0,10 : Print u$+"x"
ecls=$400 : equal=0 : ecode=$32 : Gosub post : ecode=$44 : Gosub post
WLocate 0,40 : EditFrom 2 : a$=Edit$(u$,10)
DefaultOutput
NPrint "edit ",a$=u$+"x"
bx=Peek.b(w0+54) : bw=80 : bh=20
by=Peek.b(w0+55)+5 : Gosub area : s1=s
by=by+30 : Gosub area : s2=s
NPrint "same pixels ",s1=s2
CloseWindow 0
CloseScreen 0
End

; put a key (ecls/ecode/equal) for window 0 to the port
post:
  pm=&mb(0)+mi*64 : mi=mi+1
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal
  Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
Return

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
