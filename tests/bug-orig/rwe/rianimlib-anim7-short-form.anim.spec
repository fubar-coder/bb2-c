; rianimlib-anim7-short-form: ANIM7, 32 x 16, 1 plane; the delta of
; frame 2 has short (word) data: column 0 a run of count 1 with the word
; $1234, column 1 no op
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
b 7 0                  ; operation, mask
w 32 16 0 0             ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits: bit 0 clear, short data
fill 16 0
end
chunk DLTA
l 68 0 0 0 0 0 0 0      ; plane 0: its ops at 68
l 64 0 0 0 0 0 0 0      ; plane 0: its data at 64
w $1234 $5678           ; the data list
b 1 0 1                 ; column 0: 1 op, run, count 1
b 0                     ; column 1: no op
end
end
end
