; 2dlib-floodfill-border-wide (#986) - FloodFill with a border colour builds
; a mask of the border-coloured pixels (monofetch, in columns of 64 words
; because of the blitter's width limit) and inverts it with one blit that
; uses the pointer and BLTSIZE monofetch leaves behind: those of the last
; column.  On a bitmap wider than 1024 pixels only the last column of the
; mask is inverted; the rest keeps the border pixels as the fillable ones.
; Here a box of colour 1 on a 1280 x 100 bitmap is filled from its inside
; with border colour 1; the pixels inside the box are read at x = 100, 600
; (columns 0 and 1) and 1100 (the last column).
; The same box on a 320 pixel wide bitmap is the control (filled).
; Correct: the whole inside of the box is colour 1.
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
End
