; rifnslib-underline-outside-clip (#1130) - FNSPrint clips the underline
; at xmin and xmax but does not test for an empty result: the underline
; draws its length - 1 pixels, but always at least one, so an underline
; wholly left of xmin sets one pixel at xmin, and one that starts at or
; behind xmax one pixel at its start.  The font (built in memory, slot
; 1) is 16 pixels wide, 8 rows high, underline offset 6; "a" is 8 pixels
; wide, its glyph empty.  The clip zone is 0,0,319,63 (xmax 304; xmin
; 0, where the original clips the underline too): "aa" underlined at
; x = -40 ends at -24, left of xmin; "aa" underlined at x = 310 starts
; behind xmax.
; Correct: no underline pixel at all ("0: 0 310: 0").
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @fixed #1130
; @rwe rifnslib riamosfunclib
DEFTYPE .l
f=AllocMem_(4096,$10001)
If f=0 Then NPrint "no memory" : End
Poke.l f,$464E532E
Poke.w f+4,8 : Poke.w f+6,16 : Poke.w f+8,6 : Poke.w f+10,16
Poke.b f+97,8
BitMap 0,320,64,1
r=InstallFNS(1,f)
FNSOutput 0
FNSClip 0,0,319,63
FNSPrint 1,-40,10,"aa",4,1
FNSPrint 1,310,20,"aa",4,1
NPrint "0: ",Point(0,16)," 310: ",Point(310,26)
RemoveFNS 1
FreeMem_ f,4096
End
