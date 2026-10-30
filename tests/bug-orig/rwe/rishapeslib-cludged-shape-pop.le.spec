; rishapeslib-cludged-shape-pop: an LE shapes file (FORM LESH)
; with one shape of 16 x 8
; pixels, one plane, uncompressed
form LESH
chunk SHHD
l 1                     ; shapes
w 0                     ; not compressed
fill 14 0
end
chunk SDAT
w 16 8 1                ; width, height, depth
l 0                     ; handles
w 16                    ; bytes of data
fill 16 $ff
end
end
