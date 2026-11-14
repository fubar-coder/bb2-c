; animlib-no-delta: an ANIM of 16 x 1 pixels, one plane, without any
; delta: FORM ILBM (BMHD, BODY: the row $AABB), FORM ILBM with an ANHD
; (op 5) and no DLTA
form ANIM
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 1 0 1 0               ; planes, masking, compression (ByteRun1), pad
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
end
end
