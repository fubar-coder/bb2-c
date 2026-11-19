; rifnslib-clip-ymax-inclusive (#1128) - the manual: "if your bitmap is
; 320x256 then you would use the following to set the clipping rectangle
; to the full bitmap: FNSClip 0,0,320,256" (x2, y2 the corner outside the
; zone); the library takes x2 as the first column outside but draws a row
; while y <= y2, so with the manual's example a text that reaches the
; bottom draws row 256, behind such a bitmap.  Made safe: the bitmap has
; 264 rows, the clip zone is the manual's 0,0,320,256.  The font (built in
; memory, slot 1) is 16 pixels wide and 8 rows high, every row of "!" set;
; the print at row 250 covers rows 250 to 257.
; Correct: row 255 is drawn, row 256 is not ("255: 1 256: 0").
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @fixed #1128
; @rwe rifnslib riamosfunclib
DEFTYPE .l
f=AllocMem_(1024,$10001)
If f=0 Then NPrint "no memory" : End
Poke.l f,$464E532E
Poke.w f+4,8 : Poke.w f+6,16 : Poke.w f+8,0 : Poke.w f+10,16
Poke.b f+33,8
For i=0 To 15 Step 2 : Poke.w f+272+i,-1 : Next
BitMap 0,320,264,1
r=InstallFNS(1,f)
FNSOutput 0
FNSClip 0,0,320,256
FNSInk 1
FNSPrint 1,0,250,"!"
NPrint "255: ",Point(1,255)," 256: ",Point(1,256)
RemoveFNS 1
FreeMem_ f,1024
End
