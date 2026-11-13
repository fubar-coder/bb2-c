; palettelib-crng-flags: an ILBM (palette part only): a CMAP of four
; colours and four CRNG with the flags 0 (disabled), 1 (active), 2
; (disabled, reverse) and 3 (active, reverse)
form ILBM
chunk CMAP
b $00 $00 $00           ; colour 0
b $f0 $80 $10           ; colour 1
b $10 $f0 $80           ; colour 2
b $80 $10 $f0           ; colour 3
end
chunk CRNG
w 0                     ; pad
w 1000                  ; rate
w 0                     ; flags: disabled
b 0 1                   ; low, high
end
chunk CRNG
w 0
w 2000
w 1                     ; flags: active
b 1 2
end
chunk CRNG
w 0
w 3000
w 2                     ; flags: disabled, reverse
b 2 3
end
chunk CRNG
w 0
w 4000
w 3                     ; flags: active, reverse
b 0 3
end
end
