; riamosfunclib-bload-read-error (#488) - after Read, BLoad tests only
; for 0 and returns -1 (success) otherwise; Read returns -1 on an error.
; The program saves 100 bytes with BSave and loads them again; inject
; makes the first Read of the file fail (readfail=).
; Correct: BLoad reports the failed read with 0.
; @rwe riamosfunclib
; @pass runwrap=@INJECT@|readfail=bug-orig-bload/1
DEFTYPE .l
a=AllocMem_(100,$10001)
If a=0 Then NPrint "no memory" : End
r=BSave("RAM:bug-orig-bload",a,100)
r=BLoad("RAM:bug-orig-bload",a,100)
NPrint r
FreeMem_ a,100
End
