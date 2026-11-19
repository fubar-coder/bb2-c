; rishapeslib-lecludgeshapes-chunk-lengths: an LE shapes file (FORM LESH)
; whose SHHD chunk is 24 bytes long (20 bytes of header, 4 more), with
; one shape of 16 x 8 pixels, one plane, uncompressed
form LESH
chunk SHHD
l 0                     ; shapes - 1
w 0                     ; not compressed
fill 18 0
end
chunk SDAT
w 16 8 1                ; width, height, depth
l 0                     ; handles
w 16                    ; bytes of data
fill 16 $ff
end
end
