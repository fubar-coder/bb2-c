; rianimlib-anim7-long-columns-rest: ANIM7 (long data), 56 x 16 (8 bytes,
; 4 words, 2 longs per row), 1 plane; the delta of frame 2 has no op in
; column 0 and a run of one long ($FFFFFFFF) in column 1, the second long
; of the row
form ANIM
form ILBM
chunk BMHD
w 56 16 0 0             ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 16 rows of 8 bytes, uncompressed
fill 128 0
end
end
form ILBM
chunk ANHD
b 7 0                  ; operation, mask
w 56 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 1                     ; bits: bit 0 set, long data
fill 16 0
end
chunk DLTA
l 68 0 0 0 0 0 0 0      ; plane 0: its ops at 68
l 64 0 0 0 0 0 0 0      ; plane 0: its data at 64
l $FFFFFFFF             ; the data list
b 0                     ; column 0: no op
b 1 0 1                 ; column 1: 1 op, run, count 1
end
end
end
