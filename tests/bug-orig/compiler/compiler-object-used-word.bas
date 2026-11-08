; compiler-object-used-word (#998) - Used computes the number of the used object
; as (used pointer - table base) >> shift with a word shift (lsr.w): from
; n << shift = $10000 on the high word of the difference is lost.  The
; BitMap maximum is raised to 1100 (bitmaplib = 160, objects of 64
; bytes); BitMap 1025 is 65600 bytes behind BitMap 0, Used BitMap gives
; 65600 AND $FFFF >> 6 = 1 (in the original Use BitMap 1025 also addresses
; BitMap 1 already, #939).
; Correct: after Use BitMap 1025, Used BitMap is 1025.
; @pass jobline=max=160|1100
BitMap 1025,16,8,1
BitMap 0,16,8,1
Use BitMap 1025
NPrint Used BitMap
End
