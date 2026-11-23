; rizonejoylib-circle-distance-word (#1153) - Zone and ZoneTest take a
; point's distance from a circle's centre as word differences (x - x1,
; y - y1) before they square them.  A difference beyond 32767 wraps: the
; point (-32768,0) is 65535 from the centre (32767,0) of the circle of
; radius 10, the word difference is 1, its square 1, and the point lies
; in the circle.  Both CPU modes (a500: 68000 mode, a1200: 68020 mode).
; Correct: the point lies in no zone (it is 65535 from the centre).
; Not fixed (wontfix): long differences would cost an instruction per zone
; in the loop that programs run every frame; the test stays REPRODUCED.
DEFTYPE .l

!TestCase{wrapped_distance}
  ZoneInit
  s=SetZone(1,32767,0,10)
  z=Zone(-32768,0)
  !CheckEq{s,-1,"set"}
  !CheckEq{z,-1,"outside"}
!EndTestCase
