; compiler-object-used-word (#998) - Used computes the number of the used object
; as (used pointer - table base) >> shift; the original shifts with lsr.w:
; from n << shift = $10000 on the high word of the difference is lost.  The
; BitMap maximum is raised to 1100 (bitmaplib = 160, objects of 64
; bytes); BitMap 1025 is 65600 bytes behind BitMap 0, the original's Used
; BitMap gives 65600 AND $FFFF >> 6 = 1 (there Use BitMap 1025 also
; addresses BitMap 1 already, #939).  The long shift is also checked in a
; second data register (b+Used BitMap) and for a small number (BitMap 1).
; Correct: after Use BitMap 1025, Used BitMap is 1025 and b+Used BitMap
; 1026; after Use BitMap 1, Used BitMap is 1.
; @fixed #998
; @pass jobline=max=160|1100
BitMap 1025,16,8,1
BitMap 1,16,8,1
BitMap 0,16,8,1
Use BitMap 1025
NPrint Used BitMap
b.w=1
NPrint b+Used BitMap
Use BitMap 1
NPrint Used BitMap
End
