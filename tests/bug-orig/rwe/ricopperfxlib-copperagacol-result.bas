; ricopperfxlib-copperagacol-result (#590) - CopperAGACol is declared
; with a long result but returns the constant 1234 without a custom
; copper space and its col# argument with one.  The custom space is the
; program's own memory (CopperResetAdr).
; Correct (decision W6 Q3): -1 when the command worked, 0 when not (no
; custom space).
; @rwe ricopperfxlib
DEFTYPE .l
m=AllocMem_(256,$10000)
If m=0 Then NPrint "no memory" : End
r=CopperAGACol(37,2,4,6)
NPrint "no space: ",r
CopperResetAdr m,0
r=CopperAGACol(37,2,4,6)
NPrint "space: ",r
FreeMem_ m,256
End
