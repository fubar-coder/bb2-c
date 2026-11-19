; ricopperfxlib-redocolsplit-numlines (#589) - RedoColSplit does not
; check numlines either: the DBRA counter is numlines - 1 as a word, so
; -1 rewrites the colour words of 65536 lines (20 bytes each) of the
; custom copper space; 0 divides by zero.  The custom space is the
; program's own memory (CopperResetAdr), 1.4 MB and cleared, with a
; marker long at 1000008 over the colour word of line 50000 (+10).  The
; original has no error check area.  Here with runtime error checking.
; Correct: the error check refuses numlines -1: "Colour split needs at
; least one line", nothing is written.
; @rwe ricopperfxlib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
Dim c.l(5)
For i=0 To 5 : c(i)=i*40 : Next
s=1400000
m=AllocMem_(s,$10000)
If m=0 Then NPrint "no memory" : End
Poke.l m+1000008,$12345678
CopperResetAdr m,0
RedoColSplit &c(0),-1,0
If Peek.l(m+1000008)=$12345678 Then NPrint "marker kept" Else NPrint "marker overwritten"
RedoColSplit &c(0),0,0
NPrint "after"
FreeMem_ m,s
End
