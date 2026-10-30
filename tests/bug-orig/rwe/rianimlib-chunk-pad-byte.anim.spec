; rianimlib-chunk-pad-byte: the first frame starts with an ANNO chunk
; of 5 bytes (and its pad byte), then a chunk "XYZ " and the BMHD
; (depth 1)
form ANIM
form ILBM
chunk ANNO
s "hello"
end
chunk 'XYZ '
l 0
end
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
end
end
