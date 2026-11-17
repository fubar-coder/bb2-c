; riamosfunclib-nextbank-d0 (#497) - NextBank without an argument returns
; "the first available bank" (RIAmosFuncLib's manual), but both forms
; share one routine that starts the search at d0, and the form without an
; argument does not set d0: it starts at whatever the program left there.
; Here d0 holds 60 from the assignment before the call (above the 50
; banks): the original returns -1 although every bank is free.
; Correct: NextBank returns 0.
; @fixed #497
; @rwe riamosfunclib
DEFTYPE .l
a=60
b=NextBank
NPrint b
End
