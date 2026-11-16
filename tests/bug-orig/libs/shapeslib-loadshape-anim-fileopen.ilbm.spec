; shapeslib-loadshape-anim-fileopen: an ILBM of 16 x 1 pixels, one plane,
; uncompressed, with an ANHD of operation 0 (not 5) behind its BODY
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk BODY
b $a5 $5a
end
chunk ANHD
b 0 0                   ; operation 0, mask
fill 22 0               ; the rest of the ANHD
end
end
