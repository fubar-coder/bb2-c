; rianimlib-chunk-signed-compare: ANIM5, 16 x 16, 1 plane; behind the
; first frame a chunk that claims $40000000 bytes
form ANIM
form ILBM
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
chunk JUNK $40000000
l 0
end
end
