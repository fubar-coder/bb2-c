; rianimlib-init-bmhd-range: ANIM5; the first frame has no BMHD (CMAP
; and BODY only), a BMHD chunk (16 x 16, 1 plane) stands in the ANIM
; behind it, then a delta frame
form ANIM
form ILBM
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 16 rows of 2 bytes, uncompressed
fill 32 0
end
end
chunk BMHD
w 16 16 0 0             ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
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
l 0 0 0 0 0 0 0 0       ; no plane changes
l 0 0 0 0 0 0 0 0
end
end
end
