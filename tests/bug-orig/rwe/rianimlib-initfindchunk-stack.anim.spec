; rianimlib-initfindchunk-stack: an IFF LIST of type ANIM (no FORM
; ANIM) with one FORM ILBM holding a BMHD (depth 1)
chunk LIST
s "ANIM"
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
end
