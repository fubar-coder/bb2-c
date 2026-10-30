; risnowlib-bit-above-7-lookup (#910) - LDoSnow takes the bit of a new
; flake from the random table as a byte; the bit operations use it
; modulo 8, but a blocked flake looks up its side moves with bit * 8 in
; the tables of 8 rows: bit 8 reads row 0 of the right-first table
; behind the left-first one.  The flake (byte 10 of row 0, bit 8) is
; blocked by a pixel below it (bit 0: x 87, y 1); the first try must be
; the pixel to its left (x 86), the right-first row moves it right to
; x 88.  Higher bits read the code behind the tables.
; Correct: the bit is taken modulo 8, the flake moves left to x 86.
; @rwe risnowlib
DEFTYPE .l
BitMap 0,320,256,3
t=AllocMem_(1030,$10001)
If t=0 Then NPrint "no memory" : End
Poke.b t+2,10 : Poke.b t+3,8
Plot 87,1,1
lt=InitSnowDrop(0,100,0,10240)
LDoSnow 0,t
LDoSnow 0,t
If Point(86,0)=1 Then NPrint "moved left"
If Point(88,0)=1 Then NPrint "moved right"
If Point(87,0)=1 Then NPrint "stayed"
FreeMem_ t,1030
End
