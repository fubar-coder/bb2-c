; animlib-missing-body: an ANIM whose first frame (FORM ILBM, 16 x 1, one
; plane) has a BMHD but no BODY
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
end
end
