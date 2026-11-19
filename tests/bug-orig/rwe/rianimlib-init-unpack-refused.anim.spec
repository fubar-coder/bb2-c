; rianimlib-init-unpack-refused: ANIM5, 16 x 16, 2 planes, two frames
form ANIM
form ILBM
chunk BMHD
w 16 16 0 0             ; width, height, x, y
b 2 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255 255 0 0 0 0 255
end
chunk BODY              ; 16 rows of 2 planes of 2 bytes, uncompressed
fill 64 0
end
end
form ILBM
chunk ANHD
b 5 0                  ; operation, mask
w 16 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 0 0 0 0 0 0 0 0       ; no plane changes
l 0 0 0 0 0 0 0 0
end
end
end
