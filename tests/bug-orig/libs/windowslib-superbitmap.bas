; windowslib-superbitmap (#319) - Window with flag $80 (SuperBitMap)
; enters bmapbase as the bitmap: the bitmap the last Window with BitMap#
; left there, also for a Window without BitMap#.
; On an own screen: window 0 with $80 and BitMap 0 (gadget list 0 is only
; there for the form with BitMap#), closed; then window 1 with $80 and
; no bitmap.  The SuperBitMap of a window is in its layer (RastPort+0,
; Layer+32).
; Correct: window 1 does not get BitMap 0 (a bitmap of another window).
; @configs clib
DEFTYPE .l
Screen 0,0,0,320,200,3,0,"sbm",1,2
BitMap 0,320,200,3
TextGadget 0,10,10,0,1,"G"
Window 0,10,20,100,50,$008E,"With BitMap",1,2,0,0
DefaultOutput
w0=Peek.l(Addr Window(0))
NPrint "window 0 ",Peek.l(Peek.l(Peek.l(w0+50))+32)=Addr BitMap(0)
CloseWindow 0
Window 1,120,20,100,50,$008E,"Without",1,2
DefaultOutput
w1=Peek.l(Addr Window(1))
NPrint "window 1 ",Peek.l(Peek.l(Peek.l(w1+50))+32)=Addr BitMap(0)
CloseWindow 1
CloseScreen 0
End
