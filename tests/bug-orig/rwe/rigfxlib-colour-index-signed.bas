; rigfxlib-colour-index-signed (#677) - the colour functions (PalRed ...
; AGAPalBlue) and PalAdjust's start compute the offset of a colour with
; MULU.w #12 and use the low word of the product, sign-extended: from
; colour 2731 on (12 * 2731 = 32772) the offset is negative and the
; colour is read (by PalAdjust written) before the palette; no routine
; checks the colour number against the palette's number of colours.  A
; colour number is an argument the programmer controls; the original has
; no error check area.  Here with runtime error checking: AGAPalRed(2731)
; on PaletteInfo's palette of 32 colours (it reads 32764 bytes before
; colour 0; the value is not printed, it depends on the memory there).
; Correct: the error check refuses the colour number: "Colour number
; outside the PaletteInfo palette".
; @fixed #677
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
; @pass runtime_errors=on
DEFTYPE .l
InitPalette 0,32
PaletteInfo 0
NPrint "before"
r=AGAPalRed(2731)
NPrint "after"
End
