; rianimlib-anim7-long-columns-wide: ANIM7 (long data), 32768 x 2 (4096
; bytes, 1024 longs per row), 1 plane; the delta of frame 2 has a run of
; one long in column 1023 ($11111111, the last long of row 0) and in
; column 1024 ($22222222), which the row does not have; 64512 columns of
; ops in all (what a count of width >> 5 with ASR.w reads)
form ANIM
form ILBM
chunk BMHD
w 32768 2 0 0           ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 2 rows of 4096 bytes, uncompressed
fill 8192 0
end
end
form ILBM
chunk ANHD
b 7 0                  ; operation, mask
w 32768 2 0 0           ; w, h, x, y
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 1                     ; bits: bit 0 set, long data
fill 16 0
end
chunk DLTA
l 72 0 0 0 0 0 0 0      ; plane 0: its ops at 72
l 64 0 0 0 0 0 0 0      ; plane 0: its data at 64
l $11111111 $22222222   ; the data list
fill 1023 0             ; columns 0 to 1022: no op
b 1 0 1                 ; column 1023: 1 op, run, count 1
b 1 0 1                 ; column 1024: 1 op, run, count 1
fill 63487 0            ; columns 1025 to 64511: no op
end
end
end
