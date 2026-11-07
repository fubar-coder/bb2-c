; datelib-day366 (#120) - Date$ counts in 4 year cycles of 1461 days
; from a leap year on; the test for "behind the leap year" is CMP #366 /
; BLE instead of BLT, so day 366 of a cycle (1 January of the year after
; a leap year) stays in the leap year as its 32 December.
; Here Date$ of the days around 1/1/1981 (1096) and 1/1/1977 (-365), each
; with Days, Months and Years.
; Correct: 1096 is 01/01/1981, -365 is 01/01/1977.  The original gives
; 32/12/1980 and 32/12/1976.
; @fixed #120
; @configs clib
DEFTYPE .l
Restore dl
For i=1 To 6
  Read d
  a$=Date$(d)
  NPrint d," ",a$," ",Days," ",Months," ",Years
Next
End

dl:
Data.l 1095,1096,1097,-366,-365,-364
