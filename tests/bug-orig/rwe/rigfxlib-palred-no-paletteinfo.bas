; rigfxlib-palred-no-paletteinfo (#686) - PaletteInfo stores the address
; of the palette's colours; PalRed ... AGAPalBlue and PalAdjust use it
; without a check.  Before the first PaletteInfo it is 0: PalRed(1) reads
; the byte at address 12 (the exception vectors) and returns a
; meaningless value.  A command before its setup command is a
; programming error; the original has no error check area.  Here with
; runtime error checking, PalRed(1) as the first command of the library.
; Correct: the error check refuses it: "No palette selected with
; PaletteInfo".
; @fixed #686
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
r=PalRed(1)
NPrint "after"
End
