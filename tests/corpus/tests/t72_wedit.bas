; t72_wedit - windowslib text: Print/NPrint into a window (cursor,
; WLocate, WColour, WJam, WPrintScroll, WindowFont-less default font),
; Edit$/Edit in a window with keys put to the port (line editor: insert,
; cursor keys, backspace, delete, Amiga-d/y, EditFrom, Editat, EditExit,
; Cursor), pixels read back.  Every key sequence ends with Return
; (rawkeylib: console.device keymap, $44 -> 13), so Edit$ does not wait;
; a watchdog (timer.device, 3 s) would put a Return there otherwise.
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"t72",1,2
Dim nm.l(8) : Poke.s &nm(0),"graphics.library"
GetReg d0,&nm(0) : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -408(a6) : PutReg d0,gfx
DefaultIDCMP $400|8
Window 0,10,30,300,90,$000E,"Text",1,2
w0=Peek.l(Addr Window(0)) : port=Peek.l(w0+86) : rp=RastPort(0)
rport=CreateMsgPort("")
Dim mb.l(16*60) : mi=0
Gosub hsetup
; print into the window
Print "Hello"
x1=WCursX : y1=WCursY
NPrint " world"
x2=WCursX : y2=WCursY
WLocate 40,20 : WColour 3,1 : WJam 1 : Print "AB"
x3=WCursX : y3=WCursY
WColour 1,0 : WJam 0
NPrint "" : NPrint "line" : NPrint "line" : NPrint "line" : NPrint "line" : NPrint "line"
y4=WCursY : WPrintScroll : y5=WCursY
DefaultOutput
NPrint "timer ",herr
NPrint "cursor ",x1," ",y1," ",x2," ",y2," ",x3," ",y3," ",y4," ",y5
bx=0 : by=0 : bw=120 : bh=40 : Gosub area : NPrint "text ",s
; Edit$ with keys
WindowOutput 0 : WLocate 0,60
ks$="20,35,33,4F,4F,25,4E,41,46,44" : Gosub keys
Gosub wdstart : a$=Edit$(20) : Gosub wdstop
DefaultOutput
If wdfired Then NPrint "TIMEOUT"
NPrint "edit [",a$,"] ",Len(a$)," at ",Editat," exit ",EditExit," ",WCursX," ",WCursY
bx=0 : by=56 : bw=120 : bh=12 : Gosub area : NPrint "editarea ",s
; Amiga-y / Amiga-d, shift cursor
WindowOutput 0 : WLocate 0,70
ks$="20,35,33,22,4F,4F" : Gosub keys
eq=$80 : ks$="15" : Gosub keys : eq=0
ks$="21,4E" : Gosub keys
eq=1 : ks$="4F" : Gosub keys : eq=0
ks$="36" : Gosub keys
eq=1 : ks$="4E" : Gosub keys : eq=0
ks$="37,44" : Gosub keys
Gosub wdstart : a$=Edit$(10) : Gosub wdstop
DefaultOutput
If wdfired Then NPrint "TIMEOUT"
NPrint "edit2 [",a$,"] at ",Editat," exit ",EditExit
WindowOutput 0 : WLocate 0,40
ks$="20,35" : Gosub keys
eq=$80 : ks$="22" : Gosub keys : eq=0
ks$="33,44" : Gosub keys
Cursor 2
Gosub wdstart : a$=Edit$(8) : Gosub wdstop
DefaultOutput
If wdfired Then NPrint "TIMEOUT"
NPrint "edit3 [",a$,"] at ",Editat
bx=0 : by=36 : bw=80 : bh=12 : Gosub area : NPrint "cursorarea ",s
; EditFrom: start in the middle, end on any other character (ctrl-a)
WindowOutput 0 : WLocate 0,10 : EditFrom 3 : Cursor -1
ks$="20,35,33,34" : Gosub keys
eq=8 : ks$="20" : Gosub keys : eq=0
ks$="44" : Gosub keys
Gosub wdstart : a$=Edit$("default",12) : Gosub wdstop
DefaultOutput
If wdfired Then NPrint "TIMEOUT"
NPrint "editfrom [",a$,"] at ",Editat," exit ",EditExit
EditFrom
WindowOutput 0 : WLocate 0,10
ks$="5F,44" : Gosub keys
Gosub wdstart : a$=Edit$(6) : Gosub wdstop
DefaultOutput
If wdfired Then NPrint "TIMEOUT"
NPrint "funny [",a$,"] exit ",EditExit
Gosub hclose
CloseWindow 0
CloseScreen 0
End

keys:
  ecls=$400 : equal=eq
  While ks$<>""
    k=Instr(ks$,",")
    If k=0 Then h$=ks$ : ks$="" Else h$=Left$(ks$,k-1) : ks$=Mid$(ks$,k+1)
    ecode=Val("$"+h$) : Gosub post
  Wend
Return

post:
  pm=&mb(0)+mi*64 : mi=mi+1
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal
  Poke.l pm+44,w0
  GetReg d0,pm : MOVE.l d0,a1 : GetReg d0,port : MOVE.l d0,a0
  MOVE.l 4,a6 : JSR -366(a6)
Return

pix:
  GetReg d0,gfx : MOVE.l d0,a6
  GetReg d0,rp : MOVE.l d0,a1
  GetReg d1,y
  GetReg d0,x
  JSR -318(a6)
  PutReg d0,v
Return

area:
  s=0
  For yy=by To by+bh-1
    For xx=bx To bx+bw-1
      x=xx : y=yy : Gosub pix
      s=s+v*((xx-bx)+(yy-by)*3+1)
    Next
  Next
Return

; ---------------------------------------------------------------------
; watchdog: a timer.device request replies to a port with a software
; interrupt (PA_SOFTINT); its code (poked below) GetMsg's the request and
; puts the message stored behind it (+40 port, +44 message) to that port.
; Before Edit$ the watchdog is started with a Return key message for the
; window; afterwards it is aborted (its message then goes to a dummy
; port).  wdfired<>0: the Return had to be sent (= Edit$ would hang).
hsetup:
  Dim hm.l(100) : hb=&hm(0)
  hx$="2F09,2049,2C78,0004,4EAE,FE8C,225F,4A80,6714,2F09,2240,2069,0028,2269,002C,4EAE,FE92,225F,60DA,4E75"
  ha=hb
  While hx$<>""
    k=Instr(hx$,",")
    If k=0 Then h$=hx$ : hx$="" Else h$=Left$(hx$,k-1) : hx$=Mid$(hx$,k+1)
    Poke.w ha,Val("$"+h$) : ha=ha+2
  Wend
  MOVE.l 4,a6 : JSR -636(a6)
  tp=hb+64 : ti=hb+100 : t1=hb+176
  Poke.b tp+8,4 : Poke.b tp+14,1 : Poke.l tp+16,ti
  Poke.l tp+20,tp+24 : Poke.l tp+24,0 : Poke.l tp+28,tp+20
  Poke.b ti+8,2 : Poke.l ti+14,tp : Poke.l ti+18,hb
  Poke.s hb+224,"timer.device"
  Poke.l t1+14,tp : Poke.w t1+18,40
  GetReg d0,hb+224 : MOVE.l d0,a0 : GetReg d0,t1 : MOVE.l d0,a1
  MOVEQ #1,d0 : MOVEQ #0,d1 : MOVE.l 4,a6 : JSR -444(a6) : PutReg d0,herr
  dport=CreateMsgPort("")
Return

wdstart:
  ecls=$400 : ecode=$44 : equal=0 : Gosub mkmsg
  Poke.l t1+40,port : Poke.l t1+44,pm
  Poke.w t1+28,9 : Poke.l t1+32,3 : Poke.l t1+36,0
  GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -462(a6)
Return

wdstop:
  Poke.l t1+40,dport
  GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -480(a6)
  kk=0 : While Peek.b(t1+8)<>7 AND kk<500 : VWait : kk=kk+1 : Wend
  wdfired=0 : If Peek.b(t1+31)=0 Then wdfired=1
Return

hclose:
  GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -450(a6)
Return

mkmsg:
  pm=&mb(0)+mi*64 : mi=mi+1
  For kk=0 To 15 : Poke.l pm+kk*4,0 : Next
  Poke.l pm+14,rport : Poke.w pm+18,52
  Poke.l pm+20,ecls : Poke.w pm+24,ecode : Poke.w pm+26,equal
  Poke.l pm+44,w0
Return
