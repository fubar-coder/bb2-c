; animlib-zero-dimensions: an ANIM of 16 x 0 pixels (height 0), one
; plane.  The BODY holds 65536 ByteRun1 rows of 2 bytes: the first
; repeats $11, all others $FF (the pairs $FF $FF: repeat $FF twice), so
; however many rows a decoder runs (up to 65536), it reads only the BODY.
form ANIM
form ILBM
chunk BMHD
w 16 0                  ; width, height 0
w 0 0                   ; position
b 1 0 1 0               ; planes, masking, compression (ByteRun1), pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 16 1                  ; page size
end
chunk BODY
b $FF $11               ; row 1: repeat $11 twice
fill 131070 $FF         ; rows 2..65536: repeat $FF twice
end
end
end
