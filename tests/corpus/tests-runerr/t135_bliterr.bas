; t135_bliterr - blitlib error check blitchk: Blit outside the bitmap (the
; check comes before the blit)
BitMap 0,64,32,2
Use BitMap 0
GetaShape 0,0,0,32,16
BitMap 1,64,32,2
Use BitMap 1
NPrint "before"
Blit 0,40,20
NPrint "after"
End
