; ilbmifflib-plane-word: an ILBM of 16 x 1 pixels, two planes,
; uncompressed: plane 0 A5 5A, plane 1 C3 3C
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 2 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk BODY
b $a5 $5a $c3 $3c
end
end
