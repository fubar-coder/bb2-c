; rifnslib-height-zero (#663) - FNSPrint counts the rows of a glyph with
; DBRA on height - 1: a font of height 0 draws 65536 rows; every row in
; the clip zone is drawn.  The font (built in memory, installed in slot
; 1) is 16 pixels wide with height 0; the glyph data of "!" are all set.
; The clip zone is rows 0-31 and the print starts at row 0, so the 32
; rows drawn stay inside the bitmap.
; Correct: a font of height 0 draws nothing (pixel 1,5 stays 0).
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @rwe rifnslib riamosfunclib
DEFTYPE .l
f=AllocMem_(1024,$10001)
If f=0 Then NPrint "no memory" : End
Poke.l f,$464E532E
Poke.w f+4,0 : Poke.w f+6,16 : Poke.w f+8,0 : Poke.w f+10,32
Poke.b f+33,8
For i=0 To 127 Step 4 : Poke.l f+288+i,-1 : Next
BitMap 0,320,64,1
r=InstallFNS(1,f)
FNSOutput 0
FNSClip 0,0,319,31
FNSInk 1
FNSPrint 1,0,0,"!"
NPrint Point(1,5)
RemoveFNS 1
FreeMem_ f,1024
End
