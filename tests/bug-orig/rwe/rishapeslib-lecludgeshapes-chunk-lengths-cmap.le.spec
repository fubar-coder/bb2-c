; rishapeslib-lecludgeshapes-chunk-lengths: an LE shapes file (FORM LESH)
; with a CMAP of 3 bytes (one colour) and its IFF pad byte, then one shape
; of 24 x 8 pixels, one plane, uncompressed
form LESH
chunk SHHD
l 0                     ; shapes - 1
w 0                     ; not compressed
fill 14 0
end
chunk CMAP
b $f0 $80 $10
end
chunk SDAT
w 24 8 1                ; width, height, depth
l 0                     ; handles
w 32                    ; bytes of data
fill 32 $ff
end
end
