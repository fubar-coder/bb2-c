; animlib-errors-check-area: an ANIM of 16 x 1 pixels, one plane, whose
; ANHD has operation 3 (not supported by animlib): FORM ILBM (BMHD,
; BODY), FORM ILBM (ANHD op 3)
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
b 3                     ; operation 3 (not supported)
fill 19 0
l 0                     ; bits
fill 16 0
end
end
end
