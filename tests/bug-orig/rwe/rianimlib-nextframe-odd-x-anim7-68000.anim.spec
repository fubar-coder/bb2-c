; rianimlib-nextframe-odd-x-anim7-68000: 32 x 16 (4 bytes per row), 1
; plane.  Frame 2 (ANIM5): row 0 of column 0 gets $AA.  Frame 3 (ANIM7,
; long data): a run of one long ($11223344) into row 0.
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
l 64 0 0 0 0 0 0 0      ; plane 0: its ops at 64
l 0 0 0 0 0 0 0 0
b 1 0 1 $AA             ; column 0: 1 op, run, count 1, $AA
b 0 0 0                 ; columns 1 to 3: no op
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
l 64 0 0 0 0 0 0 0      ; plane 0: its ops at 64
l 68 0 0 0 0 0 0 0      ; plane 0: its data at 68
b 1 0 1 0               ; column 0: 1 op, run, count 1; a pad byte
l $11223344             ; the data list, at 68
end
end
end
