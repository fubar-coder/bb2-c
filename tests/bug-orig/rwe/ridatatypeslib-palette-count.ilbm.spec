; ridatatypeslib-palette-count: an ILBM of 16 x 2 pixels, two planes,
; four colours, uncompressed
form ILBM
chunk BMHD
w 16 2                  ; width, height
w 0 0                   ; position
b 2 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk CMAP
b $00 $00 $00           ; colour 0
b $f0 $00 $00           ; colour 1
b $00 $f0 $00           ; colour 2
b $00 $00 $f0           ; colour 3
end
chunk BODY
b $a5 $5a $5a $a5       ; row 0, planes 0 and 1
b $ff $00 $00 $ff       ; row 1
end
end
