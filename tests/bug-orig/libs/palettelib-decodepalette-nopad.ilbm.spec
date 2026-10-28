; palettelib-decodepalette-nopad: an ILBM (palette part only): a CMAP of
; two colours, a NAME of 3 bytes (+ pad byte), a "(c) " chunk, a CRNG
; (colours 0-1)
form ILBM
chunk CMAP
b $00 $00 $00           ; colour 0
b $f0 $80 $10           ; colour 1
end
chunk NAME
s "abc"
end
chunk '(c) '
s "test"
end
chunk CRNG
w 0                     ; pad
w 1000                  ; rate
w 1                     ; flags: active
b 0 1                   ; low, high
end
end
