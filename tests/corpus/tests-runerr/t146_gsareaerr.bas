; t146_gsareaerr - shapeslib error check gschk: GetaShape outside the
; bitmap
BitMap 0,32,8,1
Use BitMap 0
NPrint "before"
GetaShape 0,20,0,20,4
NPrint "after"
End
