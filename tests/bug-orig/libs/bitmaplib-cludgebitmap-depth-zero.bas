; bitmaplib-cludgebitmap-depth-zero (#393, #74) - CludgeBitMap has no
; error check (BLITZ_SUBS(_cludgebitmap,0,0)); BitMap's _bitmapchk
; refuses a depth of 0 or more than 8 only for BitMap.  With depth 0 the
; plane loop (DBF on depth-1) stores 65536 plane pointers, 256 KB behind
; the 8 of the object, and the bitmap of depth 0 then makes the plane
; loops of the drawing and blitting libraries run 65536 or 256 times.
; Here with runtime error checking: the bitmap maximum is raised to 4200
; (tedstub max=, bitmaplib = 160), so that the 65536 pointers lie in
; objects of this program; they are cleared again before End.
; Correct: an error check refuses the depth (0 or more than 8, as for
; BitMap): "Illegal BitMap Depth", the command is not done.
; @fixed #393
; @configs clib
; @pass runtime_errors=on
; @pass jobline=max=160|4200
DEFTYPE .l
m=AllocMem(256,$10003)
NPrint "before"
CludgeBitMap 0,16,8,0,m
o=Addr BitMap(0)
For i=0 To 4200*64-4 Step 4 : Poke.l o+i,0 : Next
NPrint "after"
FreeMem m,256
End
