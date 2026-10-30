; rianimlib-delta-columns: ANIM5, 12 x 16 (2 bytes per row), 1 plane;
; the delta of frame 2 sets row 0 of both byte columns
form ANIM
form ILBM
chunk BMHD
w 12 16 0 0             ; width, height, x, y
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
chunk DLTA
l 64 0 0 0 0 0 0 0      ; plane 0: its ops at 64
l 0 0 0 0 0 0 0 0
b 1 0 1 $F0             ; column 0: 1 op, run, count 1, value $F0
b 1 0 1 $0F             ; column 1: 1 op, run, count 1, value $0F
end
end
end
