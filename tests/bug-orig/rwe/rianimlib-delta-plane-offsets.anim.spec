; rianimlib-delta-plane-offsets: 32 x 16 (4 bytes per row), 1 plane.
; Frame 2 (ANIM5): the op list offset of plane 0 points behind its DLTA,
; at the ops of frame 4.  Frame 3 (ANIM7, long data): the data list
; offset of plane 0 points behind its DLTA, at the ops of frame 4.
; Frame 4 (ANIM5): its own ops, inside: row 3 of column 0 gets $AA.
form ANIM
form ILBM
chunk BMHD
w 32 16 0 0             ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 16 rows of 4 bytes, uncompressed
fill 64 0
end
end
form ILBM
chunk ANHD
b 5 0                  ; operation, mask
w 32 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
body2:
l @ops4-@body2 0 0 0 0 0 0 0    ; corrupt: plane 0's ops behind the DLTA
l 0 0 0 0 0 0 0 0
b 0 0 0 0               ; 4 columns without ops
end
end
form ILBM
chunk ANHD
b 7 0                  ; operation, mask
w 32 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 1                     ; bits: long data
fill 16 0
end
chunk DLTA
body3:
l 64 0 0 0 0 0 0 0      ; plane 0: its ops at 64
l @ops4-@body3 0 0 0 0 0 0 0    ; corrupt: plane 0's data behind the DLTA
b 1 0 1                 ; column 0: 1 op, run, count 1 (one long of data)
end
end
form ILBM
chunk ANHD
b 5 0                  ; operation, mask
w 32 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
body4:
l @ops4-@body4 0 0 0 0 0 0 0    ; plane 0: its ops at 64
l 0 0 0 0 0 0 0 0
ops4:
b 2 3 0 1 $AA           ; column 0: 2 ops, skip 3 rows, run, count 1, $AA
b 0 0 0                 ; columns 1 to 3: no op
end
end
end
