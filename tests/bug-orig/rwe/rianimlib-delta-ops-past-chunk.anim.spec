; rianimlib-delta-ops-past-chunk: 32 x 16 (4 bytes per row), 1 plane.
; Frame 2's DLTA ends behind the first op of column 0, which claims 2
; ops: the second op and the counts of columns 1 to 3 are the 5 bytes
; behind the chunk, still inside the frame (copy 1 byte, $AA; no ops)
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
b 2 3                   ; column 0: 2 ops, skip 3 rows; the chunk ends
end
b $81 $AA 0 0 0         ; behind it: copy 1 byte ($AA); columns 1 to 3
end
end
