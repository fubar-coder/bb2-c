; rifxlib-zoomyzoom-small (#676) - ZoomyZoom ignores the bitmap's size:
; per plane it doubles 32 lines of 80 bytes in place (the left 40 bytes
; of each line into its 80), whatever BytesPerRow and Rows say, so it
; fits a bitmap of 640 x 32 only.  Bitmap 1 is 320 x 64 with one plane
; (40 bytes per row: the same 2560 bytes), the bytes of every row its
; number: each 80-byte "line" is two rows, and the second row of each
; pair gets the doubled bytes of the first (on a bitmap of fewer bytes
; per plane ZoomyZoom writes behind it).  The original has no error
; check area.  Here with runtime error checking.
; Correct: the 640 x 32 bitmap is zoomed; the error check refuses the
; 320 x 64 one: "ZoomyZoom needs a bitmap of at least 640 x 32", nothing
; is written.
; @rwe rifxlib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,640,32,1
ZoomyZoom 0
NPrint "640x32 done"
BitMap 1,320,64,1
p=Peek.l(Addr BitMap(1)+8)
For i=0 To 2559 : Poke.b p+i,i/40 : Next
ZoomyZoom 1
NPrint "row 1: ",Hex$(Peek.l(p+40))
End
