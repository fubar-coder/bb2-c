; animlib-body-always-byterun1-run: an ANIM of 16 x 2 pixels, one plane,
; ByteRun1 (compression 1) whose first run is longer than the row: row 0
; is "4 x $11" (the row has 2 bytes), row 1 "2 x $22"
form ANIM
form ILBM
chunk BMHD
w 16 2                  ; width, height
w 0 0                   ; position
b 1 0 1 0               ; planes, masking, compression (ByteRun1), pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 16 2                  ; page size
end
chunk BODY
b $FD $11               ; row 0: repeat $11 four times
b $FF $22               ; row 1: repeat $22 twice
end
end
end
