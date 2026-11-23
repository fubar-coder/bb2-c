; bitmaplib-mulu-size (#76) - BitMap computes the memory of its planes as
; MOVE d2,d0:MULU d3,d0:MULU d1,d0: the second MULU takes only the low
; word of height * depth.  BitMap 0,16,8192,8 needs 8192 * 8 * 2 = 131072
; bytes, but 65536 AND $FFFF = 0 gives 0 bytes, and the 8 planes of 16384
; bytes each point behind the block into foreign memory.
; Here the free CHIP memory (sum of mh_Free of the CHIP MemHeaders) is
; measured before and after the BitMap; nothing is written into the
; planes (the original would overwrite foreign memory).
; Correct: the bitmap takes at least 131072 bytes of CHIP memory.
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function

!TestCase{planes_size}
  f0.l=chipfree{}
  BitMap 0,16,8192,8
  f1.l=chipfree{}
  b.l=Addr BitMap(0)
  !CheckEq{Peek.l(b+36)-Peek.l(b+8),7*16384,"plane7_offset"}
  !Check{(f0-f1)>=131072,"allocated"}
!EndTestCase
