; ridatatypeslib-close-not-detached: an ILBM of 16 x 2
; pixels, one plane, two colours, uncompressed
form ILBM
chunk BMHD
w 16 2                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk CMAP
b $00 $00 $00           ; colour 0
b $f0 $f0 $f0           ; colour 1
end
chunk BODY
b $a5 $5a $5a $a5
end
end
