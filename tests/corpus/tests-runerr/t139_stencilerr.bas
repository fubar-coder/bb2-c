; t139_stencilerr - stencillib error check sblitchk: SBlit with an
; uninitialized stencil
BitMap 0,64,32,2
Use BitMap 0
GetaShape 0,0,0,32,16
NPrint "before"
SBlit 0,0,0,0
NPrint "after"
End
