; rianimlib-nextframe-skip-chunk: ANIM5, 16 x 16 (2 bytes per row), 1
; plane; frame 2 sets row 0 of column 0 ($F0), then an ANNO chunk of the
; ANIM, then frame 3 sets row 1 of column 0 ($0F)
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
chunk BODY              ; 16 rows of 2 bytes, uncompressed
fill 32 0
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
b 0 0                   ; column 1: no op; a byte to an even length
end
end
chunk ANNO              ; a text chunk between the frames
b $54 $45 $53 $54       ; "TEST"
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
b 2 1 0 1 $0F           ; column 0: 2 ops, skip 1 row, run, count 1, value $0F
b 0                     ; column 1: no op
end
end
end
