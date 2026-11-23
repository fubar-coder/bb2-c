; datelib-day366 (#120) - Date$ counts in 4 year cycles of 1461 days
; from a leap year on; the test for "behind the leap year" is CMP #366 /
; BLE instead of BLT, so day 366 of a cycle (1 January of the year after
; a leap year) stays in the leap year as its 32 December.
; Here Date$ of the days around 1/1/1981 (1096) and 1/1/1977 (-365), each
; with Days, Months and Years as one string; the expected row follows its
; day in the Data.
; Correct: 1096 is 01/01/1981, -365 is 01/01/1977.  The original gives
; 32/12/1980 and 32/12/1976.
DEFTYPE .l

.dl
Data.l 1095
Data$ "1095 31/12/1980 31 12 1980"
Data.l 1096
Data$ "1096 01/01/1981 1 1 1981"
Data.l 1097
Data$ "1097 02/01/1981 2 1 1981"
Data.l -366
Data$ "-366 31/12/1976 31 12 1976"
Data.l -365
Data$ "-365 01/01/1977 1 1 1977"
Data.l -364
Data$ "-364 02/01/1977 2 1 1977"

!TestCase{day_366}
  Restore dl
  For i=1 To 6
    Read d
    Read e$
    a$=Date$(d)
    dd=Days : mm=Months : yy=Years
    r$=Str$(d)+" "+a$+" "+Str$(dd)+" "+Str$(mm)+" "+Str$(yy)
    !CheckStrEq{r$,e$,"row"}
  Next
!EndTestCase
