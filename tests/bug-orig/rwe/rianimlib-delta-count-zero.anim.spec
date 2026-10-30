; rianimlib-delta-count-zero: ANIM5, 16 x 16, 1 plane; the delta of
; frame 2 has one op in column 0: a run (0) of count 0, value $FF
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
chunk DLTA
l 64 0 0 0 0 0 0 0      ; plane 0: its ops at 64
l 0 0 0 0 0 0 0 0
b 1 0 0 $FF             ; column 0: 1 op, run, count 0, value $FF
b 0                     ; column 1: no op
end
end
end
