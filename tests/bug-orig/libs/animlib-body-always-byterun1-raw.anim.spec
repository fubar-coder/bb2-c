; animlib-body-always-byterun1-raw: an ANIM of 32 x 2 pixels, one plane,
; whose BODY is stored uncompressed (BMHD compression 0): the rows
; $FDAAFDBB and $FCCC0102 (read as ByteRun1 they would be the runs
; "4 x $AA" and "4 x $BB", which stay inside the BODY)
form ANIM
form ILBM
chunk BMHD
w 32 2                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression 0 (none), pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 32 2                  ; page size
end
chunk BODY
b $FD $AA $FD $BB       ; row 0
b $FC $CC $01 $02       ; row 1
end
end
end
