; rishapeslib-savecmap-zero-colours (#823) - LESaveShapes allocates
; colours x 3 + 8 bytes for the CMAP chunk and copies the colours with a
; DBRA counter: a palette of 0 colours copies 65536 colours (192 KB) into
; the 8-byte buffer - a crash (the session ends in a timeout).  The
; palette is made with 2 colours and its count then set to 0; the shape
; is 16 x 8 pixels; the file goes to RAM:.
; Correct: no colours, nothing copied; the program goes on.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
InitPalette 0,2
Poke.w Peek.l(Addr Palette(0)),0
BitMap 0,32,16,1
Boxf 0,0,15,7,1
GetaShape 0,0,0,16,8
LESaveShapes 0,0,"RAM:bug-orig.le",0
NPrint "after"
End
