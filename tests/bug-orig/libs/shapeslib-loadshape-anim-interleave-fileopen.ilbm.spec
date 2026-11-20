; shapeslib-loadshape-anim-interleave-fileopen: an ILBM of 16 x 1 pixels,
; one plane, uncompressed, with an ANHD of operation 5 and interleave 0
; (not 1) behind its BODY
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
b 5 0                   ; operation 5, mask
fill 16 0               ; size, position, abstime, reltime
b 0 0                   ; interleave 0, pad
fill 4 0                ; bits
end
end
