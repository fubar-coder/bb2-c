; datelib-century-leap-years (#1001) - Date$ and NumDays count in 4 year
; cycles only: every year divisible by 4 is a leap year, also 1800, 1900,
; 2100, 2200, 2300, which the Gregorian calendar of AmigaDOS (DateStamp,
; file dates) does not have.  The day count is right from 1/3/1900 to
; 28/2/2100; outside, the dates are one day off for each missing century
; rule (29/2/2100 is shown, 1/3/2200 is two days off).
; Here Date$ of the days around 1/3/1900, 1/3/2100 and of 1/3/1800,
; 1/3/2200, 1/3/2300, 29/2/2400, 1/1/9999 with Days, Months, Years and
; the round trip NumDays(Date$(d)), then NumDays of some date strings.
; Correct: the Gregorian dates (date -d) and NumDays(Date$(d)) = d.
; @fixed #1001
DEFTYPE .l
Restore dl
For i=1 To 12
  Read d
  a$=Date$(d)
  NPrint d," ",a$," ",Days," ",Months," ",Years," ",NumDays(a$)
Next
Restore sl
For i=1 To 7
  Read s$
  NPrint "[",s$,"] ",NumDays(s$)
Next
End

dl:
Data.l -64954,-28490,-28489,-28431,-28430,44618,44619,81143,117667
Data.l 154191,154192,2929610
sl:
Data$ "1/1/1900","28/2/1900","1/3/1900","28/2/2100","1/3/2100","1/3/2400"
Data$ "1/1/9999"
