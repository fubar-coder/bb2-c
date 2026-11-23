; datelib-weekday-borrow (#121) - WeekDay takes the day count of the last
; Date$ (days since 1/1/0000) and subtracts 2 with SUBQ.w: when the low
; word of the count is 0 or 1 there is no borrow from the high word, the
; count is 65536 too large, and 65536 mod 7 = 2 makes the weekday 2 too
; large.  The counts $B0000/$B0001 are the days -1569/-1568 (15 and 16
; September 1973), $C0000/$C0001 the days 63967/63968 (2153).
; Here WeekDay (0 = Sunday) of these days and of their neighbours; the
; Data holds the day and its weekday.
; Correct: 15/9/1973 is a Saturday (6), the weekdays follow each other
; (day 0, 1/1/1978, is a Sunday: the weekday is the day mod 7).  The
; original gives 1 and 2 for -1569/-1568 and 3 and 4 for 63967/63968.
DEFTYPE .l

.dl
Data.l -1570,5,-1569,6,-1568,0,-1567,1
Data.l 63966,0,63967,1,63968,2,63969,3

!TestCase{week_days}
  Restore dl
  For i=1 To 8
    Read d,e
    a$=Date$(d)
    w=WeekDay
    !CheckEq{w,e,"weekday"}
  Next
!EndTestCase
