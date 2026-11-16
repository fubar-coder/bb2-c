; ilbmifflib-loadbitmap-nobody-fileopen: an ILBM of 16 x 1 pixels, one
; plane, with a BMHD but no BODY
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
end
