; compiler-object-used-below (#998) - the forms of compiler-object-used-word
; with a maximum whose last object still lies within $7FFF: the BitMap
; maximum is 512 (bitmaplib = 160, objects of 64 bytes), BitMap 511 is
; 32704 bytes behind BitMap 0; Used keeps the word shift (lsr.w), as in
; the original.
; Correct: after Use BitMap 511, Used BitMap is 511 and b+Used BitMap 512.
; @fixed #998
; @pass jobline=max=160|512
BitMap 511,16,8,1
BitMap 0,16,8,1
Use BitMap 511
NPrint Used BitMap
b.w=1
NPrint b+Used BitMap
End
