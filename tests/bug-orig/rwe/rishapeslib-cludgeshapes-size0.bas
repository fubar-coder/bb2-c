; rishapeslib-cludgeshapes-size0 (#808) - CludgeShapes computes a shape's
; data size as plane size x depth and copies it with a DBRA counter: a
; size of 0 (depth 0 here) copies 65536 bytes into a block without user
; bytes - a crash (the session ends in a timeout).  The shape block is
; made in the program's memory: 16 x 8 pixels, depth 0, plane size 2, the
; data behind it zeros.
; Correct: a shape without data is not copied; the program goes on.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
b=AllocMem_(70000,$10001)
If b=0 Then NPrint "no memory" : End
Poke.w b,16 : Poke.w b+2,8 : Poke.w b+4,0
Poke.w b+6,2 : Poke.w b+8,8*64+1
Poke.w b+22,2 : Poke.w b+24,18 : Poke.w b+26,0 : Poke.w b+28,0
r=CludgeShapes(0,1,b)
NPrint "after"
FreeMem_ b,70000
End
