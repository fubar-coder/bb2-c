; shapeslib-loadshape-shortbody-fileopen: an ILBM of 16 x 4 pixels, one
; plane, uncompressed, whose BODY (and file) ends after 2 of its 8 bytes
form ILBM
chunk BMHD
w 16 4                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk BODY
b $a5 $5a
end
end
