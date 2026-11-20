; rilesdebuglib-searchmem-partial-match (#723) - SearchMem's string search
; starts again at the next byte after a mismatch, not one byte behind the
; start of the partial match: "ab" is not found in "aab".
; Correct: "ab" is found at b+1.
; @fixed #723
; @rwe rilesdebuglib
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"aab"
s=b+32
Poke$ s,"ab"
NPrint SearchMem(4,s,b,b+3)
If SearchMemFound=0 Then NPrint "SearchMemFound: 0" Else NPrint "SearchMemFound: b+",SearchMemFound-b
FreeMem_ b,64
End
