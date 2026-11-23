; datelib-century-leap-years (#1001) - Date$ and NumDays count in 4 year
; cycles only: every year divisible by 4 is a leap year, also 1800, 1900,
; 2100, 2200, 2300, which the Gregorian calendar of AmigaDOS (DateStamp,
; file dates) does not have.  The day count is right from 1/3/1900 to
; 28/2/2100; outside, the dates are one day off for each missing century
; rule (29/2/2100 is shown, 1/3/2200 is two days off).
; Here Date$ of the days around 1/3/1900, 1/3/2100 and of 1/3/1800,
; 1/3/2200, 1/3/2300, 29/2/2400, 1/1/9999 with Days, Months, Years and
; the round trip NumDays(Date$(d)), then NumDays of some date strings.
; The rows are the day, the date, Days, Months, Years and the round trip
; as one string; the expected row follows its day in the Data.
; Correct: the Gregorian dates (date -d) and NumDays(Date$(d)) = d.
DEFTYPE .l

.dl
Data.l -64954
Data$ "-64954 01/03/1800 1 3 1800 -64954"
Data.l -28490
Data$ "-28490 31/12/1899 31 12 1899 -28490"
Data.l -28489
Data$ "-28489 01/01/1900 1 1 1900 -28489"
Data.l -28431
Data$ "-28431 28/02/1900 28 2 1900 -28431"
Data.l -28430
Data$ "-28430 01/03/1900 1 3 1900 -28430"
Data.l 44618
Data$ "44618 28/02/2100 28 2 2100 44618"
Data.l 44619
Data$ "44619 01/03/2100 1 3 2100 44619"
Data.l 81143
Data$ "81143 01/03/2200 1 3 2200 81143"
Data.l 117667
Data$ "117667 01/03/2300 1 3 2300 117667"
Data.l 154191
Data$ "154191 29/02/2400 29 2 2400 154191"
Data.l 154192
Data$ "154192 01/03/2400 1 3 2400 154192"
Data.l 2929610
Data$ "2929610 01/01/9999 1 1 9999 2929610"
.sl
Data$ "1/1/1900","-28489","28/2/1900","-28431","1/3/1900","-28430"
Data$ "28/2/2100","44618","1/3/2100","44619","1/3/2400","154192"
Data$ "1/1/9999","2929610"

!TestCase{date_rows}
  Restore dl
  For i=1 To 12
    Read d
    Read e$
    a$=Date$(d)
    dd=Days : mm=Months : yy=Years : n=NumDays(a$)
    r$=Str$(d)+" "+a$+" "+Str$(dd)+" "+Str$(mm)+" "+Str$(yy)+" "+Str$(n)
    !CheckStrEq{r$,e$,"row"}
  Next
!EndTestCase

!TestCase{numdays_rows}
  Restore sl
  For i=1 To 7
    Read s$
    Read e$
    n=NumDays(s$)
    r$="["+s$+"] "+Str$(n)
    !CheckStrEq{r$,"["+s$+"] "+e$,"row"}
  Next
!EndTestCase
