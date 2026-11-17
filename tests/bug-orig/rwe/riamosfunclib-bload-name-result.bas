; riamosfunclib-bload-name-result (#486) - BLoad(name$) loads a file into
; a new bank and returns "the number of the bank ... or -1 for failure"
; (RIAmosFuncLib's manual).  When Reserve fails it returns 0, the number
; of the first bank; when the load fails it ignores that and returns the
; bank, which stays reserved with undefined contents.  The program saves
; 4321 bytes of the ROM; inject makes the first AllocMem of 4321 bytes
; fail (the first BLoad's Reserve) and the first Read of the file fail
; (the second BLoad's load).
; Correct: both BLoads return -1, bank 0 stays free.
; @fixed #486
; @rwe riamosfunclib
; @pass runwrap=@INJECT@|allocfail=4321-4321/1 readfail=bug-orig-bload4/1
DEFTYPE .l
r=BSave("RAM:bug-orig-bload4",$F80000,4321)
b=BLoad("RAM:bug-orig-bload4")
NPrint b
b=BLoad("RAM:bug-orig-bload4")
NPrint b
If Start(0)=0 Then NPrint "bank 0 free" Else NPrint "bank 0 used"
End
