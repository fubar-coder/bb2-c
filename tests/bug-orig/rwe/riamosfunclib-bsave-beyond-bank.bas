; riamosfunclib-bsave-beyond-bank (#489) - BSave(name$,bank,length)
; writes length bytes from the bank's start, whatever the bank's size:
; the memory behind the bank goes into the file.  RIAmosFuncLib's manual:
; "If length > bank length, then the length of the bank is saved
; instead."
; Correct: a bank of 10 bytes saved with the length 20 gives a file of
; 10 bytes.
; @fixed #489
; @rwe riamosfunclib
DEFTYPE .l
r=Reserve(0,10)
r=BSave("RAM:bug-orig-bsave",0,20)
NPrint r
NPrint FileSize("RAM:bug-orig-bsave")
End
