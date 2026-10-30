; richunkylib-raycastsky-height0 (#887) - RaycastSky copies height bytes
; of a texture column, 320 bytes apart, with height - 1 as a DBRA
; counter: a height of 0 copies 65536 rows, 20 MB of addresses from the
; chunky data and the texture on.  Made safe: the chunky bitmap is
; 32000 x 656 (21 MB) and the texture an allocation of the same size, so
; the 65536 rows stay inside both; the texture has a marker byte in row
; 1000, the program prints whether it arrived in the chunky data.
; Correct: a height of 0 copies nothing.
; @rwe richunkylib
DEFTYPE .l
s=32000*656
ChunkyBitmap 0,32000,656
c=ChunkyData(0)
t=AllocMem_(s,$10000)
If c=0 OR t=0 Then NPrint "no memory" : End
Poke.b t+320*1000,$AA
RaycastSky 0,0,0,0,t
If Peek.b(c+320*1000)=0 Then NPrint "row 1000 kept" Else NPrint "row 1000 written"
FreeMem_ t,s
End
