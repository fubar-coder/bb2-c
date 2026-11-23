; windowslib-calcwidth-proportional (#321) - calcwidth (the width of a
; character in the line editor of Edit$ in a window) tests the table
; index of a proportional font with SUB.b/BMI: characters from LoChar+128
; on (with LoChar 32: from $A0, the umlauts) get tf_XSize instead of their
; width, and the cursor is in the wrong place.
; Here: on an own 8 colour screen, a window with helvetica 11 (proportional,
; from the Workbench's FONTS:): the line "aax" (a = a-umlaut, Chr$(228))
; printed, and below it Edit$ with the default "aa", EditFrom 2 and the
; keys x and Return put to the port by the program (as in the corpus test
; t72).  The pixels of both lines are compared (ReadPixel).
; Correct: the x is inserted behind the umlauts, both lines look the same.
DEFTYPE .l

; put a key (ecls/ecode/equal) for window w0 to its port
Statement post{pm,ecls,ecode,equal}
  SHARED rport,port,w0
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal
  Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
End Statement

!TestCase{proportional_edit}
  Screen 0,0,0,640,200,3,0,"calcwidth",1,2
  Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
  GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
  LoadFont 0,"helvetica.font",11
  tf=Peek.l(Addr IntuiFont(0)+8)
  !Require{tf<>0,"font_loaded"}
  !Require{(Peek.b(tf+23)&$20)<>0,"font_proportional"}
  DefaultIDCMP $400|8
  Window 0,10,20,300,120,$000E,"Edit",1,2
  WindowFont 0
  w0=Peek.l(Addr Window(0)) : port=Peek.l(w0+86) : rp=RastPort(0)
  rport=CreateMsgPort("")
  Dim mb.l(16*8)
  u$=Chr$(228)+Chr$(228)
  WLocate 0,10 : Print u$+"x"
  post{&mb(0),$400,$32,0}
  post{&mb(0)+64,$400,$44,0}
  WLocate 0,40 : EditFrom 2 : a$=Edit$(u$,10)
  !CheckStrEq{a$,u$+"x","edit_result"}
  ; sum of pixel*position over an area of 80 x 20 of the line (0: printed,
  ; 1: edited)
  bx=Peek.b(w0+54) : bw=80 : bh=20
  For ln=0 To 1
    by=Peek.b(w0+55)+5+ln*30
    s=0
    For yy=by To by+bh-1
      For xx=bx To bx+bw-1
        GetReg d0,gfx : MOVE.l d0,a6
        GetReg d0,rp : MOVE.l d0,a1
        GetReg d1,yy
        GetReg d0,xx
        JSR -318(a6)
        PutReg d0,v
        s=s+v*((xx-bx)+(yy-by)*3+1)
      Next
    Next
    If ln=0 Then s1=s Else s2=s
  Next
  !Check{s1<>0,"line_drawn"}
  !CheckEq{s1,s2,"same_pixels"}
  CloseWindow 0
  CloseScreen 0
!EndTestCase
