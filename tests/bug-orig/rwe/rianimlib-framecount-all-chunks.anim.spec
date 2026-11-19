; rianimlib-framecount-all-chunks: ANIM5, 16 x 16, 1 plane; three
; frames, an ANNO chunk (4 bytes) between the first and the second
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
chunk ANNO
s "text"
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
