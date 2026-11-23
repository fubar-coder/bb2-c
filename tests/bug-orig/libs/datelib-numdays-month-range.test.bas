; datelib-numdays-month-range (#124) - NumDays indexes its table of month
; starts with the month without a check: month 0 reads the word before
; the table (the low word of the ticks of the last DateStamp, 0 before
; SystemDate or Hours), month 13 the table's end mark 1000, months 14 and
; 15 the two words behind it (the code at the end of the library), larger
; months whatever memory follows.
; Here NumDays with the months 0, 13, 14 and 15 (no SystemDate or Hours
; before), and with 12 for reference.
; Correct: a month outside 1-12 is invalid input, NumDays returns 0 as for
; its other invalid input.  The original gives a count from the words it
; reads.
DEFTYPE .l

!TestCase{month_range}
  m0=NumDays("1/0/1990")
  m12=NumDays("1/12/1990")
  m13=NumDays("1/13/1990")
  m14=NumDays("1/14/1990")
  m15=NumDays("1/15/1990")
  DateFormat 1
  f1=NumDays("13/1/1999")
  DateFormat 0
  !CheckEq{m0,0,"month_0"}
  !CheckEq{m12,4717,"month_12"}
  !CheckEq{m13,0,"month_13"}
  !CheckEq{m14,0,"month_14"}
  !CheckEq{m15,0,"month_15"}
  !CheckEq{f1,0,"format_1_month_13"}
!EndTestCase
