; 2dlib-floodfill-border-wide (#986) - FloodFill with a border colour builds
; a mask of the border-coloured pixels (monofetch, in columns of 64 words
; because of the blitter's width limit, and in parts of 1024 lines because
; of its height limit) and inverts it with one blit that uses the pointer
; and BLTSIZE monofetch leaves behind: those of the last column, with the
; height of its last part.  On a bitmap wider than 1024 pixels only the
; last column of the mask is inverted, on one taller than 1024 lines only
; the first height MOD 1024 lines; the rest keeps the border pixels as the
; fillable ones.
; Here a box of colour 1 on a 1280 x 100 bitmap is filled from its inside
; with border colour 1; the pixels inside the box are read at x = 100, 600
; (columns 0 and 1) and 1100 (the last column).  Then a box on a 64 x 1100
; bitmap is filled from y = 500; read at y = 50 (the first 76 lines) and
; y = 500 and 1050.
; The same box on a 320 x 100 bitmap is the control (filled).
; Correct: the whole inside of each box is colour 1.
; @fixed #986
; @configs clib
; @pass timeout=60
DEFTYPE .l
BitMap 0,1280,100,1
Use BitMap 0
Boxf 0,0,1279,99,0
Box 10,10,1200,90,1
FloodFill 600,50,1,1
BitMap 1,320,100,1
Use BitMap 1
Boxf 0,0,319,99,0
Box 10,10,300,90,1
FloodFill 100,50,1,1
NPrint "narrow: ",Point(100,50)
Use BitMap 0
NPrint "x=100: ",Point(100,50)
NPrint "x=600: ",Point(600,50)
NPrint "x=1100: ",Point(1100,50)
BitMap 2,64,1100,1
Use BitMap 2
Boxf 0,0,63,1099,0
Box 10,10,50,1090,1
FloodFill 30,500,1,1
NPrint "y=50: ",Point(30,50)
NPrint "y=500: ",Point(30,500)
NPrint "y=1050: ",Point(30,1050)
End
