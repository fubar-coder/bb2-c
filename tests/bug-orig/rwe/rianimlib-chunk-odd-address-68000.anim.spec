; rianimlib-chunk-odd-address-68000: the first frame starts with an ANNO
; chunk of 5 bytes written without its pad byte, so the BMHD behind it
; starts at an odd address (depth 1)
form ANIM
form ILBM
s "ANNO"                ; no pad byte behind its 5 bytes
l 5
s "hello"
chunk BMHD
w 16 16 0 0             ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 16 rows of 2 bytes, uncompressed
fill 32 0
end
end
end
