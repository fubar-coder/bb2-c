; t128_decodeerr - palettelib: DecodePalette of memory without FORM
; (runtime error "Can't Open IFF" from the normal code)
Dim mm.l(10)
NPrint "before"
DecodePalette 0,&mm(0)
NPrint "after"
End
