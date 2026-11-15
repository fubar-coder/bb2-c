; animlib-zero-dimensions-load-depth: an ANIM of 16 x 1 pixels with 0
; planes: a BODY of one row and two DLTA chunks (frames 1)
form ANIM
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 0 0 1 0               ; planes 0, masking, compression (ByteRun1), pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 16 1                  ; page size
end
chunk BODY
b 1 $AA $BB             ; literal run of 2 bytes
end
end
form ILBM
chunk ANHD
b 5                     ; operation 5 (ByteVertical)
fill 19 0
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64                    ; plane 0: its ops at offset 64
fill 60 0               ; planes 1-15: none
b 1 $81 $CC             ; column 0: one op, uniq 1 byte $CC
b 1 $81 $DD             ; column 1
end
end
form ILBM
chunk ANHD
b 5
fill 19 0
l 0
fill 16 0
end
chunk DLTA
l 64
fill 60 0
b 1 $81 $11
b 1 $81 $22
end
end
end
