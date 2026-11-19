; rifnslib-fnslength-font-unmasked (#660) - every rifnslib command takes
; the font number & 15, FNSLength does not (ASL.w #2 of the number): 16
; reads the long behind the 16 slots - the first bytes of the built-in
; font, "FNS." - as the address of a font, and gives -1 or the width in
; whatever lies there.
; Correct: FNSLength(16,...) is FNSLength(0,...), as for the other
; commands (the two numbers are the same).
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @fixed #660
; @rwe rifnslib riamosfunclib
DEFTYPE .l
a=FNSLength(0,"xyz")
b=FNSLength(16,"xyz")
If a=b Then NPrint "same" Else NPrint "different: ",a," ",b
End
