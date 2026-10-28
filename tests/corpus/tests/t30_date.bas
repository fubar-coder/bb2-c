; t30_date - datelib: Date$, NumDays, DateFormat, Days, Months, Years,
; WeekDay with fixed values (edge cases, invalid input), SystemDate/Hours/
; Mins/Secs only as plausibility flags
; NumDays with month 0 reads the tick count of the last DateStamp: before
; SystemDate/Hours it is 0
NPrint "init ",Days," ",Months," ",Years," ",WeekDay
NPrint "m0 ",NumDays("1/0/1990")," ",NumDays("15/0/2000")
Restore dlist
Read n
For i=1 To n
  Read d.l
  a$=Date$(d)
  NPrint d," ",a$," ",Days," ",Months," ",Years," ",WeekDay
Next
DateFormat 1
NPrint "fmt1 ",Date$(0)," ",Date$(4473)," ",Days," ",Months," ",Years
NPrint "fmt1 ",NumDays("12/31/1999")," ",NumDays("2/29/1980")," ",NumDays("13/1/1999")
DateFormat 2
NPrint "fmt2 ",Date$(9000)," ",NumDays("3/4/1990")
DateFormat -1
NPrint "fmt-1 ",Date$(9000)," ",NumDays("3/4/1990")
DateFormat 0
NPrint "fmt0 ",Date$(9000)," ",NumDays("3/4/1990")
Restore slist
Read n
For i=1 To n
  Read s$
  NPrint "[",s$,"] ",NumDays(s$)
Next
; round trip Date$ -> NumDays
e=0
For d.l=-1600 To 3000 Step 7
  If NumDays(Date$(d))<>d Then NPrint "rt ",d," ",Date$(d)," ",NumDays(Date$(d)) : e=e+1
Next
For d.l=65405 To 65700 Step 13
  If NumDays(Date$(d))<>d Then NPrint "rt ",d," ",Date$(d)," ",NumDays(Date$(d)) : e=e+1
Next
NPrint "rt diffs ",e
; weekdays of a whole 3 weeks
For d.l=10000 To 10020 : a$=Date$(d) : Print WeekDay : Next : NPrint ""
; system time: only plausibility
sd.l=SystemDate
NPrint "sysdate ",sd>=0," ",Len(Date$(sd))
h=Hours : mi=Mins : se=Secs
NPrint "time ",h>=0 AND h<24," ",mi>=0 AND mi<60," ",se>=0 AND se<60
NPrint "same ",Mins=mi," ",Secs=se
End

dlist:
Data 34
Data.l 0,1,30,31,58,59,60,364,365,366,729,730,731,790,1095,1096,1097,1460
Data.l 1461,-1,-2,-365,-366,-1569,-1570,63967,65535,65536,100000
Data.l -722465,-722466,-800000,2000000,40000000

slist:
Data 29
Data$ "1/1/1978","01/01/1978","31/12/1977","1/1/1900","28/2/1981","1/3/1981"
Data$ "29/2/1980","1/3/1980","31/12/2099","1/1/9999","1/13/1990","1/14/1990"
Data$ "1/15/1990","1/1/78/","1/1/0/","5/6/7/","1/1/19901","/5/1990"
Data$ "abc","123/1/1990","1x/2/1990","1 /2/1990","1/2/19 0","1/1/19x0"
Data$ "32/1/2000","0/1/2000","99/12/9999","1/1/2155","1/1/2156"
