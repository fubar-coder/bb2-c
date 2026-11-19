; ripacklib-loadiff-bank-51: an ILBM, 16 x 1, one plane, ByteRun1; the
; row $FF,$00 (one literal run of 2 bytes)
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
b 1 255 0
end
end
