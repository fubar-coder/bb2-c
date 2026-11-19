; rifxlib-derez-zoomxy (#671) - Derez with a value of 3 or more builds its
; mask table myline with 20 words (21 for some values) into the 18 words
; the source reserves; behind them lie myflag, its pad byte and the code
; of ZoomXY, so the 20th word replaces ZoomXY's first instruction
; (TST.w d0).  For 5 that word is $4210, CLR.b (a0): it clears a byte
; wherever a0 points and sets Z, and the BEQ behind it leaves ZoomXY at
; once - nothing is zoomed any more.  The port cannot overwrite code: its
; table has room for 21 words, ZoomXY stays intact (the difference the
; port could not avoid; W6 decision Q6).  A ZoomXY 2,2 of 8 lines into
; bitmap 1 before and after a Derez ...,5,...; the bytes of bitmap 1
; after each are summed (bitmap 1 cleared before each).
; Correct: ZoomXY gives the same picture after the Derez as before it.
; @fixed #671
; @rwe rifxlib
; @pass timeout=30
DEFTYPE .l
BitMap 0,320,16,1
BitMap 1,320,32,1
p=Peek.l(Addr BitMap(0)+8)
q=Peek.l(Addr BitMap(1)+8)
For i=0 To 639 : Poke.b p+i,i*7 : Next
InitZoomXY 0,1,0,0
For i=0 To 1279 : Poke.b q+i,0 : Next
ZoomXY 2,2,8
s=0 : For i=0 To 1279 : s=s+Peek.b(q+i) : Next
NPrint "before Derez: ",s
Derez 0,1,0,0,5,8
For i=0 To 1279 : Poke.b q+i,0 : Next
ZoomXY 2,2,8
t=0 : For i=0 To 1279 : t=t+Peek.b(q+i) : Next
If t=s Then NPrint "after Derez: the same" Else NPrint "after Derez: ",t
End
