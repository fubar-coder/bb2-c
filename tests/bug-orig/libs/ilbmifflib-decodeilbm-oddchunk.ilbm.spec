; ilbmifflib-decodeilbm-oddchunk: an ILBM of 16 x 1 pixels, one plane,
; uncompressed; between BMHD and BODY an ANNO of 5 bytes (+ pad byte) and
; a "(c) " chunk
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 16 1                  ; page size
end
chunk ANNO
s "hello"
end
chunk '(c) '
s "test"
end
chunk BODY
b $a5 $5a
end
end
