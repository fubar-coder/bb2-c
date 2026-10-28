; t137_bufferr - bblitlib error check bblitchk: buffer too small (the
; check comes before the save blit)
BitMap 0,64,32,2
Use BitMap 0
GetaShape 0,0,0,32,16
Buffer 0,100
NPrint "before"
BBlit 0,0,0,0
NPrint "after"
End
