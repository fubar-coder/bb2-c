; rifnslib-fnslength-last-prefs (#661) - FNSLength without prefs takes
; the prefs of the last FNSPrint, not those of FNSPrefs, which a
; following FNSPrint without prefs uses: after a bold print (prefs 2) the
; short form counts one pixel more per character although FNSPrefs is 0.
; Correct: the short form gives the width with FNSPrefs' prefs, the same
; as FNSLength(0,a$,0) ("same").
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @fixed #661
; @rwe rifnslib riamosfunclib
DEFTYPE .l
BitMap 0,320,64,1
FNSOutput 0
FNSClip 0,0,319,63
FNSPrefs 0
FNSPrint 0,10,10,"bold",2,1
a=FNSLength(0,"plain")
b=FNSLength(0,"plain",0)
If a=b Then NPrint "same" Else NPrint "different: ",a," ",b
End
