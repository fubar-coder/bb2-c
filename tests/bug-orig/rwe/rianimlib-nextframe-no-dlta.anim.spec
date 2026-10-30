; rianimlib-nextframe-no-dlta: ANIM5, 16 x 16, 1 plane; frame 2 has a
; DPAN chunk between ANHD and DLTA
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
form ILBM
chunk ANHD
b 5 0                  ; operation, mask
w 16 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DPAN
w 4 2                   ; version, number of frames
l 0
end
chunk DLTA
l 0 0 0 0 0 0 0 0
l 0 0 0 0 0 0 0 0
end
end
end
