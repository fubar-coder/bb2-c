; risnowlib-lists-unbounded (#914) - the drop and kill lists of a field
; have room for numpoints * 4 entries, but LDoSnow appends every moving
; flake and one new flake per call without a bound.  numpoints 10 (40
; entries per list), an empty bitmap 256 rows high, every new flake at
; offset 0: the 41st call writes 41 flakes into the second drop list,
; the last one over the header (the "last" pointer) of the first kill
; list behind it (inside the block).  (The 42nd would overwrite the
; header of the second drop list, which the 43rd reads: a crash.)
; Correct: a full list takes no more entries, the kill list stays.
; @rwe risnowlib
DEFTYPE .l
BitMap 0,320,256,3
t=AllocMem_(1030,$10001)
If t=0 Then NPrint "no memory" : End
lt=InitSnowDrop(0,10,0,10240)
For i=1 To 41 : LDoSnow 0,t : Next
k=Peek.l(lt+8)
If Peek.l(k)=k+4 Then NPrint "kill list intact" Else NPrint "kill list overwritten"
FreeMem_ t,1030
End
