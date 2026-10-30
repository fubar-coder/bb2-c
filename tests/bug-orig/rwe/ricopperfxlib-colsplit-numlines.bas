; ricopperfxlib-colsplit-numlines (#589) - DoColSplit does not check
; numlines: the DBRA counter is numlines - 1 as a word, so -1 writes
; 65536 lines (20 bytes each, 24 with the line 256 wait: 1.3 MB) into the
; custom copper space; 0 divides by zero (DIVS: the divide-by-zero
; exception, the session ends in a timeout).  The custom space is the
; program's own memory (CopperResetAdr), 1.4 MB, with a marker long at
; 1000000 (line 50000 or so) that shows the run-away loop safely; then
; the program calls DoColSplit with 0 lines.
; Correct: numlines -1 and 0 are refused (nothing written, no exception).
; @rwe ricopperfxlib
DEFTYPE .l
Dim c.l(5)
For i=0 To 5 : c(i)=i*40 : Next
s=1400000
m=AllocMem_(s,$10000)
If m=0 Then NPrint "no memory" : End
Poke.l m+1000000,$12345678
CopperResetAdr m,0
DoColSplit &c(0),-1,1
If Peek.l(m+1000000)=$12345678 Then NPrint "marker kept" Else NPrint "marker overwritten"
CopperResetAdr m,0
DoColSplit &c(0),0,1
NPrint "after"
FreeMem_ m,s
End
